// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using EpicGames.Core;

public class MassUpgrade : ModuleRules
{
	public MassUpgrade(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add other private include paths required here ...
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				// "OnlineSubsystem",
				// "OnlineSubsystemNull",
				// "OnlineSubsystemEOS",
				// "OnlineSubsystemUtils",
				// "SignificanceManager",
				// "ApexDestruction",
				// "AkAudio",
				"AssetRegistry",
				"NavigationSystem",
				// "ReplicationGraph",
				"AIModule",
				"GameplayTasks",
				"SlateCore", "Slate", "UMG",
				"Json",
				// ... add other public dependencies that you statically link with here ...
			}
		);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"FactoryGame",
			"SML",
			"MarcioCommonLibs",
			"AbstractInstance",
			"DummyHeaders",
		});


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"RenderCore",
				// ... add private dependencies that you statically link with here ...	
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);

		// var factoryGamePchPath = new DirectoryReference(Path.Combine(Target.ProjectFile.Directory.ToString(), "Source",
		// 	"FactoryGame", "Public"));
		// PrivatePCHHeaderFile = factoryGamePchPath.MakeRelativeTo(new DirectoryReference(ModuleDirectory));
	}
}