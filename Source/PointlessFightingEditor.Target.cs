// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class PointlessFightingEditorTarget : TargetRules
{
	public PointlessFightingEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "GeneratedWorldGenerators" } );
		ExtraModuleNames.AddRange( new string[] { "PointlessFighting" } );
	}
}
