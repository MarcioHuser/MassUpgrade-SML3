// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassUpgradeModule.h"

#include "Buildables/FGBuildableConveyorLift.h"
#include "Equipment/FGBuildGun.h"
#include "Hologram/FGConveyorLiftHologram.h"
#include "Patching/NativeHookManager.h"
#include "Util/MULogging.h"
#include "Util/MUOptimize.h"
#include "Util/MassUpgradeConfiguration.h"

#ifndef OPTIMIZE
#pragma optimize("", off)
#endif

void FMassUpgradeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

#if UE_BUILD_SHIPPING
	// static FString indent;

	static auto hooked = false;

	// static auto insideBuildGunPrimaryFire = true;
	
	if (!hooked)
	{
		hooked = true;

#define AUTO_INDENT_VOID(Method, ...) \
			MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(": Before "), Method); \
			auto prevIndent = indent; \
			indent += TEXT("    "); \
			scope(##__VA_ARGS__); \
			indent = prevIndent; \
			MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(": After "), Method)

#define AUTO_INDENT_RET(Method, ...) \
			MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(": Before "), Method); \
			auto prevIndent = indent; \
			indent += TEXT("    "); \
			auto ret = scope(##__VA_ARGS__); \
			indent = prevIndent; \
			MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(": After "), Method)

		{
			// auto ObjectInstance = GetMutableDefault<AFGConveyorLiftHologram>();

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::BeginPlay,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::BeginPlay"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::IsValidHitResult,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self,const FHitResult& hitResult)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_RET(TEXT("AFGConveyorLiftHologram::IsValidHitResult"), self, hitResult);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::SetHologramLocationAndRotation,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self,const FHitResult& hitResult)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::IsValidHitResult"), self, hitResult);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::TrySnapToActor,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self,const FHitResult& hitResult)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::TrySnapToActor"), self, hitResult);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::DoMultiStepPlacement,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self,bool isInputFromARelease)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::DoMultiStepPlacement"), self, isInputFromARelease);
			// 	MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(":     isInputFromARelease: "), isInputFromARelease ? TEXT("True") : TEXT("False"));
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::TryUpgrade,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self,const FHitResult& hitResult)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::TryUpgrade"), self, hitResult);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::GetUpgradedActor,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_RET(TEXT("AFGConveyorLiftHologram::GetUpgradedActor"), self);
			// 	MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(":     returns: "), *GetNameSafe(ret));
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::GetBaseCostMultiplier,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::GetBaseCostMultiplier"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::GetHologramHoverHeight,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::GetHologramHoverHeight"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::GetIgnoredClearanceActors,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self, TArray< AActor* >& ignoredActors)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::GetIgnoredClearanceActors"), self, ignoredActors);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::GetSupportedBuildModes_Implementation,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self, TArray< TSubclassOf< UFGBuildGunModeDescriptor > >& out_buildmodes)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::GetSupportedBuildModes_Implementation"), self, out_buildmodes);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::PostHologramPlacement,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self, const FHitResult& hitResult)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::PostHologramPlacement"), self, hitResult);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::CheckClearance,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self, const FVector& locationOffset)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::CheckClearance"), self, locationOffset);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::CheckBlueprintCommingling,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::CheckBlueprintCommingling"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::IsHologramIdenticalToActor,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self,AActor* actor, const FVector& hologramLocationOffset)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_RET(TEXT("AFGConveyorLiftHologram::IsHologramIdenticalToActor"), self, actor, hologramLocationOffset);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::ReplaceHologram,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self, AFGHologram* hologram, bool snapTransform)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::ReplaceHologram"), self, hologram, snapTransform);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::CanNudgeHologram,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::CanNudgeHologram"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::CheckValidFloor,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::CheckValidFloor"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::ConfigureActor,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self,class AFGBuildable* inBuildable)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::ConfigureActor"), self, inBuildable);
			// 	MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(":     inBuildable: "), *GetNameSafe(inBuildable));
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::ConfigureComponents,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self,class AFGBuildable* inBuildable)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::ConfigureComponents"), self, inBuildable);
			// 	MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(":     inBuildable: "), *GetNameSafe(inBuildable));
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::GetRotationStep,
			// 	ObjectInstance,
			// 	[](auto& scope, const AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_RET(TEXT("AFGConveyorLiftHologram::GetRotationStep"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD(
			// 	AFGConveyorLiftHologram::UpdateConnectionDirections,
			// 	[](auto& scope, AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::UpdateConnectionDirections"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGConveyorLiftHologram::CheckValidPlacement,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGConveyorLiftHologram* self)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::CheckValidPlacement"), self);
			// 	}
			// 	}
			// 	);

			// SUBSCRIBE_METHOD(
			// 	AFGConveyorLiftHologram::UpdateTopTransform,
			// 	[](auto& scope, AFGConveyorLiftHologram* self, const FHitResult& hitResult, FRotator rotation)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGConveyorLiftHologram::UpdateTopTransform"), self, hitResult, rotation);
			// 	}
			// 	}
			// 	);
		}

		{
			// auto ObjectInstance = GetMutableDefault<AFGBuildableHologram>();

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGBuildableHologram::Construct,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGBuildableHologram* self, TArray< AActor* >& out_children, FNetConstructionID netConstructionID)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_RET(TEXT("AFGBuildableHologram::Construct"), self, out_children, netConstructionID);
			// 	MU_LOG_Display_Condition(GetNameSafe(self), indent, TEXT(":     returns: "), *GetNameSafe(ret));
			// 	}
			// 	}
			// 	);
		}

		{
			// auto ObjectInstance = GetMutableDefault<AFGBuildGun>();

			// SUBSCRIBE_METHOD(
			// 	AFGBuildGun::OnPrimaryFirePressed,
			// 	[](auto& scope, AFGBuildGun* self)
			// 	{
			// 	//insideBuildGunPrimaryFire = true;
			// 	AUTO_INDENT_VOID(TEXT("AFGBuildGun::OnPrimaryFirePressed"), self);
			// 	//insideBuildGunPrimaryFire = false;
			// 	}
			// 	);
		}

		// {
		// 	auto ObjectInstance = GetMutableDefault<AFGBuildable>();
		//
		// 	SUBSCRIBE_METHOD_VIRTUAL(
		// 		AFGBuildable::PreUpgrade_Implementation,
		// 		ObjectInstance,
		// 		[](auto& scope, AFGBuildable* self)
		// 		{
		// 		if(insideBuildGunPrimaryFire)
		// 		{
		// 		AUTO_INDENT_VOID(TEXT("AFGBuildable::PreUpgrade_Implementation"), self);
		// 		}
		// 		}
		// 		);
		// }

		{
			// auto ObjectInstance = GetMutableDefault<AFGBuildableConveyorLift>();

			// SUBSCRIBE_METHOD_VIRTUAL(
			// 	AFGBuildableConveyorLift::Upgrade_Implementation,
			// 	ObjectInstance,
			// 	[](auto& scope, AFGBuildableConveyorLift* self, AActor* newActor)
			// 	{
			// 	if(insideBuildGunPrimaryFire)
			// 	{
			// 	AUTO_INDENT_VOID(TEXT("AFGBuildableConveyorLift::Upgrade_Implementation"), self, newActor);
			// 	}
			// 	}
			// 	);
		}
	}
#endif
}

void FMassUpgradeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

IMPLEMENT_MODULE(FMassUpgradeModule, MassUpgrade)

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
