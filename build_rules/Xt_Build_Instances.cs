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

using System.Linq;

public class Xt_Build_Instances : Task
{
	[Required] public String ProjectListFile { get; set; }
	[Required] public String LinkToolCommand { get; set; }
	[Required] public ITaskItem[] CurrentProject {get; set; }
	[Required] public String SolutionFilePath { get; set; }
	[Required] public String Configuration { get; set; }
	[Required] public String Platform { get; set; }
	
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
			Log.LogMessage(MessageImportance.High, "failed to load project {0} because {1}", path, ex.Message);
		}
		return false;
	}
	
	private bool WalkProjectDependencies()
	{
		solution_file = SolutionFile.Parse(SolutionFilePath); // throws
		
		// By default the projects are loaded without reference to a particular solution
		// but if e.g an <Import .. /> references something relative to $(SolutionDir)
		// then the project will fail to load without explicitly setting that property here.
		// The properties read from the projects may also depend on Configuration and Platform.
		project_collection = new ProjectCollection( new Dictionary<String, String> {
			{ "SolutionDir", Path.GetDirectoryName(SolutionFilePath) + "\\" },
			{ "Configuration", Configuration }, { "Platform", Platform },
			{ "SolutionPath", SolutionFilePath } // just for completeness
		});
	
		string my_path = CurrentProject[0].GetMetadata("FullPath");
		
		EnqueuePath(path: my_path, parent: null, is_reference: true);
		
		while(node_queue.Count > 0) {
			Node node = node_queue.Dequeue();
			Project project = node.project;
			Log.LogMessage(MessageImportance.High, "processing project {0}", project.FullPath);

			foreach(ProjectItem reference in project.GetItems("ProjectReference")) {
				string path = reference.GetMetadataValue("FullPath");
				EnqueuePath(path: path, parent: node, is_reference: true);
			}
			
			// find project dependencies that are not necessarily referenced by the current project
			// e.g if the project depends on a dynamically loaded DLL
			ProjectInSolution project_in_sol = null;
			if(!solution_file.ProjectsByGuid.TryGetValue(project.GetPropertyValue("ProjectGuid"), out project_in_sol)) {
				Log.LogMessage(MessageImportance.High, "could not find project {0} in solution", project.FullPath);
				continue;
			}
			
			foreach(string dependency_guid in project_in_sol.Dependencies) {
				ProjectInSolution dependency = null;
				if(!solution_file.ProjectsByGuid.TryGetValue(dependency_guid, out dependency)) {
					Log.LogMessage(MessageImportance.High, "could not find dependency {0} for project {1} in solution", dependency_guid, project.FullPath);
					continue;
				}
				EnqueuePath(path: dependency.AbsolutePath, parent: node, is_reference: false);
			}
		}
		return true;
	}
	
	// note: this is currently unused
	List<Node> GetChangedNodes()
	{
		// the link tool writes to a file which contains
		// the guids of projects that contain .xti-s that it added instantiations to
		// look up nodes corresponding to changed projects in the dependency graph
		var nodes = new List<Node>();
		string guid;
		StreamReader file =  new StreamReader(ProjectListFile);
		while((guid = file.ReadLine()) != null) {
			Node node = null;
			if(!guid_to_node.TryGetValue(guid, out node)) {
				Log.LogMessage(MessageImportance.High, "could not find project for {0}", guid);
				return null;
			}
			nodes.Add(node);
			node.is_changed = true;
			//Project project = node.project;
			//Log.LogMessage(MessageImportance.High, "Found project {0} for {1}", project.FullPath, guid);
		}
		file.Dispose();
		return nodes;
	}
	
	List<string> FindInstFiles() {
		try {
			bool files_missing = false;
			List<string> inst_files = new List<string>();
			// by construction guid_to_node should only contain projects that
			// the root project references or depends on transitively
			foreach(Node node in guid_to_node.Values) {
				string[] props = {"Xt_InstFilePath_FromHeader", "Xt_InstFilePath", "Xt_HeaderCachePath", "Xt_InstSuffix"};
				string[] values = props.Select(prop => node.project.GetPropertyValue(prop)).ToArray();
				if(values.Any(value => value == "")) continue;
				//for(int i = 0; i < props.Length; i++)
					//Log.LogMessage(MessageImportance.High, "{0} = {1}", props[i], values[i]);
				bool xti_from_header = Convert.ToBoolean(values[0]);
				string inst_base_path = values[1], header_cache_path = values[2], inst_suffix = values[3];
				string header_path;
				StreamReader file =  new StreamReader(header_cache_path);
				while((header_path = file.ReadLine()) != null) {
					string xti_path = !xti_from_header ?
						(inst_base_path + Path.GetFileNameWithoutExtension(header_path) + inst_suffix)
						: Path.ChangeExtension(header_path, inst_suffix);
					//Log.LogMessage(MessageImportance.High, "xti_path = {0}", xti_path);
					if(!File.Exists(xti_path)) {
						Log.LogMessage(MessageImportance.High, "error: xti file does not exist: {0}", xti_path);
						files_missing = true;
					}
					inst_files.Add(xti_path);
				}
				// Xt_Headers_Read may touch this file right after we're done
				// so make sure the file gets closed first
				file.Dispose();
			}
			return files_missing ? null : inst_files;
		} catch (Exception e) {
			Log.LogMessage(MessageImportance.High, "FindInstFiles failed because: {0}", e.ToString());
			return null;
		}
	}
	
	bool Init()
	{
		if(!WalkProjectDependencies()) {
			Log.LogMessage(MessageImportance.High, "failed to walk dependencies");
			return false;
		}
		
		List<string> inst_files = FindInstFiles();
		if(inst_files == null) {
			Log.LogMessage(MessageImportance.High, "no inst files found");
			return false;
		}
		
		string command = LinkToolCommand + " \"" + String.Join(";", inst_files) + "\"";
		Log.LogMessage(MessageImportance.High, "xt_inst_gen link: {0}", command);
		
		link_tool = new Exec {
			BuildEngine = this.BuildEngine,
			Command = command
		};
		
		build = new MSBuild {
			BuildEngine = this.BuildEngine
		};
		return true;
	}
	
	void ClearVisited()
	{
		foreach(Node node in guid_to_node.Values) {
			node.visited = false;
		}
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
			Log.LogMessage(MessageImportance.High, "need to build {0}", node.project.FullPath);
			items_to_build.Add(new TaskItem(node.project.FullPath));
		}
		
		ClearVisited();
		
		return items_to_build.ToArray();
	}
	
	bool Build(ITaskItem[] items_to_build)
	{
		Log.LogMessage(MessageImportance.High, "export_template: building projects");
		build.Projects = items_to_build;
		return build.Execute();
	}
	
	public override bool Execute()
	{
		if(!Init())
			return false;
		
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