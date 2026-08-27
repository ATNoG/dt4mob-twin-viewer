// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DT4MOB : ModuleRules
{
	public DT4MOB(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "Core", "CoreUObject", "Engine", "InputCore", "UMG", "HTTP", "Json", "JsonUtilities", "CesiumRuntime", "WebSockets", "Slate", "SlateCore", "ImageWrapper", "RenderCore", "RHI", "glTFRuntime", "DeveloperSettings", "ProceduralMeshComponent" });

		// Prevent Windows.h from defining min/max macros that conflict with std::numeric_limits in Cesium/MovieScene headers
		PublicDefinitions.Add("NOMINMAX");

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// CredentialStoreService persists login credentials at rest via a platform-specific
		// secure-storage backend: Windows DPAPI or macOS Keychain Services.
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("Crypt32.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicFrameworks.Add("Security");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
