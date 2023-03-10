// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

using UnrealBuildTool;

public class VirtualAnimationToolsEditor : ModuleRules
{
	public VirtualAnimationToolsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Public"),
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                 System.IO.Path.Combine(ModuleDirectory,"Private"),
                 System.IO.Path.GetFullPath(Target.RelativeEnginePath) + "Source/Editor/Blutility/Private",
            }
            );

        PrivateIncludePathModuleNames.Add("AssetTools");

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "Engine",
                "EditorSubsystem",
				"MainFrame",
                "VirtualAnimationTools",
                //"AnimationBlueprintLibrary",// UE5
				//"EditorFramework", // UE5
				//"AnimationDataController", // UE5
            }
            );
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
                "EditorStyle",
				"LevelEditor",
				"PropertyEditor",
                "ContentBrowser",
                "Blutility",
                "UMG",
                "UMGEditor",
                "BlueprintGraph",
                "AnimGraph",
                "AnimationEditor",
                "SkeletonEditor",
                "AssetRegistry",
                "AnimationModifiers",
                "AnimationCore",
                "Kismet",
                "Projects",
				"Persona",
                "SequenceRecorder",
                "SkeletalMeshUtilitiesCommon",
                "VirtualAnimationTools",
                //"InteractiveToolsFramework", // UE5
                //"EditorInteractiveToolsFramework" // UE5
            }
            );
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
                "AssetTools",
            }
            );
	}
}
