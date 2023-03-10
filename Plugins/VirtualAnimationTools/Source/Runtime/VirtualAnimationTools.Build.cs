// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

using UnrealBuildTool;

public class VirtualAnimationTools : ModuleRules
{
	public VirtualAnimationTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
				System.IO.Path.Combine(ModuleDirectory, "Public"),
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				System.IO.Path.Combine(ModuleDirectory, "Private"),
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AnimationCore",
				"AnimGraphRuntime",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
            {
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

        if (Target.bBuildEditor == true)
        {
            PrivateIncludePaths.AddRange(
            new string[] {
            System.IO.Path.GetFullPath(Target.RelativeEnginePath) + "Source/Editor/Blutility/Private",
            }
            );

            PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                    "Blutility",
            }
            );

            PublicDependencyModuleNames.AddRange(new string[]
            {
                    "EditorScriptingUtilities",
                    "AnimationModifiers", // UE4
					//"AnimationBlueprintLibrary", // UE5
            });
        }
    }
}
