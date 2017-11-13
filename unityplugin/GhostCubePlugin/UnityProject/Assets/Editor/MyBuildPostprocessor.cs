using UnityEngine;
using UnityEditor;
using UnityEditor.Callbacks;
using System.IO;

public class MyBuildPostprocessor
{
	// Build postprocessor. Currently only needed on:
	// - iOS: no dynamic libraries, so plugin source files have to be copied into Xcode project
	[PostProcessBuild]
	public static void OnPostprocessBuild(BuildTarget target, string pathToBuiltProject)
	{
		if (target == BuildTarget.iOS)
			OnPostprocessBuildIOS(pathToBuiltProject);
	}

	private static void OnPostprocessBuildIOS(string pathToBuiltProject)
	{
		// We use UnityEditor.iOS.Xcode API which only exists in iOS editor module
		#if UNITY_IOS

		string projPath = pathToBuiltProject + "/Unity-iPhone.xcodeproj/project.pbxproj";

		UnityEditor.iOS.Xcode.PBXProject proj = new UnityEditor.iOS.Xcode.PBXProject();
		proj.ReadFromString(File.ReadAllText(projPath));
		string target = proj.TargetGuidByName("Unity-iPhone");

		Directory.CreateDirectory(Path.Combine(pathToBuiltProject, "Libraries/Unity"));

		string[] filesToCopy = new string[]
		{
			"PlatformBase.h",
			"RenderAPI_Metal.mm",
			"RenderAPI_OpenGLCoreES.cpp",
			"RenderAPI.cpp",
			"RenderAPI.h",
			"RenderingPlugin.cpp",
		};

		for(int i = 0 ; i < filesToCopy.Length ; ++i)
		{
			var srcPath = Path.Combine("../PluginSource/source", filesToCopy[i]);
			var dstLocalPath = "Libraries/" + filesToCopy[i];
			var dstPath = Path.Combine(pathToBuiltProject, dstLocalPath);
			File.Copy(srcPath, dstPath, true);
			proj.AddFileToBuild(target, proj.AddFile(dstLocalPath, dstLocalPath));
		}

		File.WriteAllText(projPath, proj.WriteToString());
		#endif // #if UNITY_IOS
	}
}
