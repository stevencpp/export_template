using System;
using Microsoft.Build.Utilities;
using Microsoft.Build.Tasks;
using Microsoft.Build.Framework;

using System.Collections.Generic;
using Microsoft.Build.Evaluation;
using Microsoft.Build.Exceptions;

public class Xt_Build_Instances : Task
{
	[Required] public String ProjectListFile { get; set; }
	[Required] public String LinkToolCommand { get; set; }
	[Required] public ITaskItem[] CurrentProject {get; set; }
	[Required] public String SolutionDir { get; set; }
	
	class Node {
		public Project project = null;
		public List<Node> references = null;
		public List<Node> referenced_by = null;
		public bool visited = false;
		public bool is_changed = false;
	};
	
	Dictionary<string, Node> guid_to_node = new Dictionary<string, Node>();
	Queue<Node> node_queue = new Queue<Node>();
	ProjectCollection project_collection = null;
	Node root = null;
	
	Exec link_tool = null;
	MSBuild build = null;	
	MSBuild build_compile = null;
	
	bool EnqueuePath(String path, Node parent = null)
	{
		try {
			Project p = project_collection.LoadProject(path);
			String guid = p.GetPropertyValue("ProjectGuid");
			Node node = null;
			if(!guid_to_node.TryGetValue(guid, out node)) {
				node = new Node {
					project = p,
					references = new List<Node>(),
					referenced_by = new List<Node>()
				};
				guid_to_node[guid] = node;
				node_queue.Enqueue(node);
			}
			if(parent != null) {
				node.referenced_by.Add(parent);
				parent.references.Add(node);
			} else {
				root = node;
			}
			return true;
		} catch( InvalidProjectFileException ex ) {
			Log.LogMessage(MessageImportance.High, "failed to load project " + path + " because " + ex.Message);
		}
		return false;
	}
	
	private void WalkProjectDependencies()
	{
		// by default the projects are loaded without reference to a particular solution
		// but if e.g an <Import .. /> references something relative to $(SolutionDir)
		// then the project will fail to load without explicitly setting that property here
		project_collection = new ProjectCollection( new Dictionary<String, String> {
			{ "SolutionDir", SolutionDir }
		});
	
		string my_path = CurrentProject[0].GetMetadata("FullPath");
		
		EnqueuePath(my_path);
		
		while(node_queue.Count > 0) {
			Node node = node_queue.Dequeue();
			Project project = node.project;
			//Log.LogMessage(MessageImportance.High, "found project " + path);

			//var references = project.Items.Where(item => item.ItemType == "ProjectReference");
			foreach(ProjectItem reference in project.GetItems("ProjectReference")) {
				//if(reference.ItemType != "ProjectReference") continue;
				string path = reference.GetMetadataValue("FullPath");
				Log.LogMessage(MessageImportance.High, "found referenced project " + path);
				EnqueuePath(path, node);
			}
		}
	}
	
	List<Node> GetChangedNodes()
	{
		// the link tool writes to a file which contains
		// the guids of projects that contain .xti-s that it added instantiations to
		// look up nodes corresponding to changed projects in the dependency graph
		var nodes = new List<Node>();
		string guid;
		System.IO.StreamReader file =  new System.IO.StreamReader(ProjectListFile);
		while((guid = file.ReadLine()) != null) {
			Node node = null;
			if(!guid_to_node.TryGetValue(guid, out node)) {
				Log.LogMessage(MessageImportance.High, "could not find project for " + guid);
				return null;
			}
			nodes.Add(node);
			node.is_changed = true;
			//Project project = node.project;
			//Log.LogMessage(MessageImportance.High, "Found project " + project.FullPath + " for " + guid);
		}
		return nodes;
	}
	
	List<Node> GetNodesToBuild(List<Node> nodes)
	{
		// every project that depends on the changed projects needs to be built
		// mark nodes that need to be built as visited
		for(int i = 0; i < nodes.Count; ++i) {
			nodes[i].visited = true;
		}
		for(int i = 0; i < nodes.Count; ++i) {
			var child_node = nodes[i];
			foreach(Node parent_node in child_node.referenced_by) {
				if(parent_node.visited) continue;
				nodes.Add(parent_node);
				parent_node.visited = true;
			}
		}
		
		// all of a projects's dependencies need to be built before the project
		// do a topological sort of the nodes and clean up the visited states
		nodes.Clear();
		nodes.Add(root);
		for(int i = 0; i < nodes.Count; ++i) {
			var parent_node = nodes[i];
			foreach(Node child_node in parent_node.references) {
				if(!child_node.visited) continue;
				nodes.Add(child_node);
				child_node.visited = false;
			}
		}
		nodes.Reverse();
		return nodes;
	}
	
	void Init()
	{
		WalkProjectDependencies();
		//return false;
		
		link_tool = new Exec {
			BuildEngine = this.BuildEngine,
			Command = LinkToolCommand
		};
		
		build = new MSBuild {
			BuildEngine = this.BuildEngine
		};
		
		build_compile = new MSBuild {
			BuildEngine = this.BuildEngine,
			Targets = new String[] { "BuildCompile" }
		};
	}
	
	bool BuildCompile()
	{
		// note: if there was a way to get the MSBuild task to recursively build
		// dependencies then we might not need to do it manually
		
		List<Node> changed_nodes = GetChangedNodes();
		if(changed_nodes == null || changed_nodes.Count == 0) {
			Log.LogMessage(MessageImportance.High, "no nodes changed, exiting xt loop");
			return false;
		}
		
		// todo: build leaves in parallel
		List<Node> nodes_to_build = GetNodesToBuild(changed_nodes);
		foreach(Node node in nodes_to_build) {
			Project project = node.project;
			if(node == root) {
				// for the root project compile and link need to be done separately
				// because a compile error is fatal while a link error is not
				if(root.is_changed) {
					Log.LogMessage(MessageImportance.High, "compiling project " + project.FullPath);
					build_compile.Projects = CurrentProject;
					if(!build_compile.Execute()) {
						Log.LogMessage(MessageImportance.High, "compile failed, exiting xt loop");
						return false;
					}
				}
			} else {
				Log.LogMessage(MessageImportance.High, "building project " + project.FullPath);
				build.Projects = new ITaskItem[] { new TaskItem(project.FullPath) };
				if(!build.Execute()) {
					Log.LogMessage(MessageImportance.High, "build failed, exiting xt loop");
					return false;
				}
			}
			node.is_changed = false; // cleanup for next iteration
		}
		return true;
	}
	
	bool BuildLink()
	{
		Log.LogMessage(MessageImportance.High, "building project " + root.project.FullPath);
		build.Projects = CurrentProject;
		return build.Execute();
	}

	public override bool Execute()
	{
		Init();
		
		int max_iter = 10;
		for(int iter = 1; iter <= max_iter; ++iter) {
			// find unresolved symbols in the build log
			// and add the necessary explicit instantiations to the xti files
			Log.LogMessage(MessageImportance.High, "running link tool");
			if(!link_tool.Execute())
				return false;
			
			// in order to be able to build the same projects more than once
			// they need to be created with unique dummy properties
			build_compile.Properties = build.Properties = new String [] { "Xt_Iteration=" + iter };

			if(!BuildCompile())
				return false;
			
			if(BuildLink())
				return true;
		}
		
		return true;
	}
}