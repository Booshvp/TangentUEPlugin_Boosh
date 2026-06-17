// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision$

using UnrealBuildTool;

public class Tangent : ModuleRules
{
	public Tangent(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
				"Networking",
				"Sockets",
				"Projects",
				"UnrealEd",
				"LevelEditor",
				"CinematicCamera",
				"EditorStyle",
				"Sequencer",
				"LevelSequence",
				"ColorCorrectRegions",
				"TangentInput",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
