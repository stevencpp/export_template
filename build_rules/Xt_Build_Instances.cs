using System;
using System.IO;
using System.Collections;
using Microsoft.Build.Utilities;
using Microsoft.Build.Tasks;
using Microsoft.Build.Framework;

using System.Collections.Generic;
using Microsoft.Build.Evaluation;
using Microsoft.Build.Exceptions;

using Microsoft.Build.Construction;

public class Xt_Build_Instances : Task
{
	[Required] public String ProjectListFile { get; set; }
	[Required] public String LinkToolCommand { get; set; }
	[Required] public ITaskItem[] CurrentProject {get; set; }
	[Required] public String SolutionFilePath { get; set; }
	
	class Node {
		public Project project = null;
		// the project's references are linked into the project's target
		public List<Node> references = new List<Node>();
		public List<Node> referenced_by = new List<Node>();
		// dependencies are not linked into but may be used by the project
		// e.g DLLs loaded at run-time
		public List<Node> dependencies = new List<Node>();
		public List<Node> dependency_of = new List<Node>();
		public bool visited = false;
		public bool is_changed = false;
		public bool is_referenced = false;
	};
	
	Dictionary<string, Node> guid_to_node = new Dictionary<string, Node>();
	Queue<Node> node_queue = new Queue<Node>();
	ProjectCollection project_collection = null;
	Node root = null;
	
	Exec link_tool = null;
	MSBuild build = null;
	
	SolutionFile solution_file = null;
	
	bool EnqueuePath(String path, Node parent, bool is_reference)
	{
		try {
			Project p = project_collection.LoadProject(path);
			String guid = p.GetPropertyValue("ProjectGuid");
			Node node = null;
			if(!guid_to_node.TryGetValue(guid, out node)) {
				node = new Node {
					project = p
				};
				guid_to_node[guid] = node;
				node_queue.Enqueue(node);
			}
			if(parent != null) {
				if(is_reference) {
					node.referenced_by.Add(parent);
					parent.references.Add(node);
					node.is_referenced |= parent.is_referenced;
				} else {
					node.dependency_of.Add(parent);
					parent.dependencies.Add(node);
				}
			} else {
				root = node;
				node.is_referenced = true;
			}
			return true;
		} catch( InvalidProjectFileException ex ) {
			Log.LogMessage(MessageImportance.High, "failed to load project " + path + " because " + ex.Message);
		}
		return false;
	}
	
	private void WalkProjectDependencies()
	{
		solution_file = SolutionFile.Parse(SolutionFilePath); // throws
		
		// by default the projects are loaded without reference to a particular solution
		// but if e.g an <Import .. /> references something relative to $(SolutionDir)
		// then the project will fail to load without explicitly setting that property here
		project_collection = new ProjectCollection( new Dictionary<String, String> {
			{ "SolutionDir", Path.GetDirectoryName(SolutionFilePath) }
		});
	
		string my_path = CurrentProject[0].GetMetadata("FullPath");
		
		EnqueuePath(path: my_path, parent: null, is_reference: true);
		
		while(node_queue.Count > 0) {
			Node node = node_queue.Dequeue();
			Project project = node.project;
			Log.LogMessage(MessageImportance.High, "processing project " + project.FullPath);

			foreach(ProjectItem reference in project.GetItems("ProjectReference")) {
				string path = reference.GetMetadataValue("FullPath");
				EnqueuePath(path: path, parent: node, is_reference: true);
			}
			
			// find project dependencies that are not necessarily referenced by the current project
			// e.g if the project depends on a dynamically loaded DLL
			ProjectInSolution project_in_sol = null;
			if(!solution_file.ProjectsByGuid.TryGetValue(project.GetPropertyValue("ProjectGuid"), out project_in_sol)) {
				Log.LogMessage(MessageImportance.High, "could not find project " + project.FullPath + " in solution");
				continue;
			}
			
			foreach(string dependency_guid in project_in_sol.Dependencies) {
				ProjectInSolution dependency = null;
				if(!solution_file.ProjectsByGuid.TryGetValue(dependency_guid, out dependency)) {
					Log.LogMessage(MessageImportance.High, "could not find dependency " + dependency_guid + " for project " + project.FullPath + " in solution");
					continue;
				}
				EnqueuePath(path: dependency.AbsolutePath, parent: node, is_reference: false);
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
	
	void Init()
	{
		WalkProjectDependencies();
		
		link_tool = new Exec {
			BuildEngine = this.BuildEngine,
			Command = LinkToolCommand
		};
		
		build = new MSBuild {
			BuildEngine = this.BuildEngine
		};
	}
	
	ITaskItem[] GetItemsToBuild()
	{
		// to allow projects to inject template instantiations into
		// dependencies that are not linked to it (e.g DLLs)
		// we need to find the minimum set of projects to build that
		// recursively covers the whole reference/dependency tree
		
		// build the root project and any dependencies that are not also referenced by the root
		// todo: in principle there could be two such non-root-referenced dependencies A and B
		// such that A references B and then of the two we would only need to build A
		
		List<Node> nodes_to_build = new List<Node>();
		nodes_to_build.Add(root);
		
		List<Node> nodes = new List<Node>();
		nodes.Add(root);
		root.visited = true;
		
		for(int i = 0; i < nodes.Count; ++i) {
			var parent_node = nodes[i];
			foreach(Node child_node in parent_node.references) {
				if(child_node.visited) continue;
				child_node.visited = true;
				nodes.Add(child_node);
			}
			foreach(Node child_node in parent_node.dependencies) {
				if(child_node.visited) continue;
				child_node.visited = true;
				if(!child_node.is_referenced)
					nodes_to_build.Add(child_node);
			}
		}
		
		List<ITaskItem> items_to_build = new List<ITaskItem>();
		foreach(Node node in nodes_to_build) {
			Log.LogMessage(MessageImportance.High, "need to build " + node.project.FullPath);
			items_to_build.Add(new TaskItem(node.project.FullPath));
		}
		
		return items_to_build.ToArray();
	}
	
	bool Build(ITaskItem[] items_to_build)
	{
		Log.LogMessage(MessageImportance.High, "building project " + root.project.FullPath);
		build.Projects = items_to_build;
		return build.Execute();
	}
	
	public override bool Execute()
	{
		Init();
		
		ITaskItem[] items_to_build = GetItemsToBuild();
		
		// the following properties need to be set in order to have the build tasks
		// automatically build the project references recursively 
		Stack<String> properties = new Stack<String>();
		properties.Push("BuildingInsideVisualStudio=false");
		properties.Push("BuildProjectReferences=true");
		properties.Push(""); // to be replaced later
		
		int max_iter = 10;
		for(int iter = 1; iter <= max_iter; ++iter) {
			// find unresolved symbols in the build log
			// and add the necessary explicit instantiations to the xti files
			Log.LogMessage(MessageImportance.High, "running link tool");
			if(!link_tool.Execute())
				return false;
			
			// in order to be able to build the same projects more than once
			// they need to be created with unique dummy properties
			properties.Pop();
			properties.Push("Xt_Iteration=" + iter);
			build.Properties = properties.ToArray();
			
			if(Build(items_to_build))
				return true;
		}
		
		return true;
	}
}