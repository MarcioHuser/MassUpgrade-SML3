﻿#include "MassUpgradeEquipment.h"

#include "FGPlayerController.h"
#include "Buildables/FGBuildableConveyorBase.h"
#include "Buildables/FGBuildablePipeBase.h"
#include "Buildables/FGBuildablePipeline.h"
#include "Buildables/FGBuildablePipelinePump.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Buildables/FGBuildableStorage.h"
#include "Components/PointLightComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logic/ProductionInfoAccessor.h"
#include "Subsystems/CommonInfoSubsystem.h"
#include "Util/MULogging.h"
#include "Util/MarcioCommonLibsUtils.h"
#include "Util/MassUpgradeConfiguration.h"

#ifndef OPTIMIZE
#pragma optimize("", off )
#endif

void AMassUpgradeEquipment::SetProductionInfos_Implementation(const TArray<FProductionInfoWithArray>& infos, CollectProductionInfoIntent collectProductionInfoIntent)
{
	OnSetProductionInfos.Broadcast(infos, collectProductionInfoIntent);
}

FString AMassUpgradeEquipment::getAuthorityAndPlayer(const AActor* actor)
{
	return FString(TEXT("Has Authority = ")) +
		(actor->HasAuthority() ? TEXT("true") : TEXT("false")) +
		TEXT(" / Character = ") +
		(actor->GetInstigator() ? *actor->GetInstigator()->GetHumanReadableName() : TEXT("None"));
}

AMassUpgradeEquipment::AMassUpgradeEquipment()
{
}

void AMassUpgradeEquipment::BeginPlay()
{
	Super::BeginPlay();
}

void AMassUpgradeEquipment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	auto playerController = Cast<AFGPlayerController>(GetInstigatorController());
	if (playerController == nullptr)
	{
		return;
	}

	TArray<AActor*> actorsToIgnore;
	actorsToIgnore.Add(playerController->GetPawn());

	FVector start = playerController->PlayerCameraManager->GetCameraLocation();
	FVector end = start + playerController->PlayerCameraManager->GetActorForwardVector() * 5000;

	FHitResult hit;
	UKismetSystemLibrary::LineTraceSingle(
		this,
		start,
		end,
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		TArray<AActor*>(),
		EDrawDebugTrace::None,
		hit,
		true
		);

	auto hitActor = Cast<AFGBuildable>(UMarcioCommonLibsUtils::GetHitActor(hit));
	AFGBuildable* tempTargetBuildable = nullptr;

	if (hitActor)
	{
		if (playerController->WasInputKeyJustPressed(EKeys::NumPadZero))
		{
			UMarcioCommonLibsUtils::DumpUnknownClass(hitActor);
		}

		auto commonInfoSubsystem = ACommonInfoSubsystem::Get(GetWorld());

		if (hitActor->IsA(AFGBuildableConveyorBase::StaticClass()) ||
			commonInfoSubsystem->IsStorageContainer(hitActor) ||
			hitActor->IsA(AFGBuildablePipeline::StaticClass()) ||
			commonInfoSubsystem->IsPowerPole(hitActor) ||
			commonInfoSubsystem->IsPowerPoleWall(hitActor) ||
			commonInfoSubsystem->IsPowerPoleWallDouble(hitActor) ||
			commonInfoSubsystem->IsPowerTower(hitActor))
		{
			tempTargetBuildable = hitActor;
		}
		if (!tempTargetBuildable)
		{
			auto pump = Cast<AFGBuildablePipelinePump>(hitActor);

			if (pump && pump->GetDesignHeadLift() > 0)
			{
				tempTargetBuildable = pump;
			}
		}

		if (tempTargetBuildable && tempTargetBuildable != targetBuildable)
		{
			MU_LOG_Display_Condition(
				*getTagName(),
				TEXT("Targeted new building = "),
				*GetPathNameSafe(tempTargetBuildable),
				TEXT(" / "),
				*getAuthorityAndPlayer(this)
				);

			setBuildDescriptor(tempTargetBuildable->GetBuiltWithDescriptor());
		}
	}
	else if (hit.GetActor())
	{
		if (playerController->WasInputKeyJustPressed(EKeys::NumPadZero))
		{
			UMarcioCommonLibsUtils::DumpUnknownClass(hit.GetActor());
		}
	}

	targetBuildable = tempTargetBuildable;

	if (!targetBuildable)
	{
		pointLight->SetIntensity(0);

		return;
	}

	ShowOutline(GetInstigatorCharacter(), targetBuildable);

	pointLight->SetIntensity(50);
	pointLight->SetLightColor(FColor(0x0, 0xff, 0x0, 0xff));
}

void AMassUpgradeEquipment::PrimaryFirePressed()
{
	MU_LOG_Display_Condition(
		*getTagName(),
		TEXT("PrimaryFirePressed = "),
		*GetPathName(),
		TEXT(" / Target = "),
		*GetPathNameSafe(targetBuildable),
		TEXT(" / "),
		*getAuthorityAndPlayer(this)
		);

	if (!targetBuildable)
	{
		return;
	}

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(GetWorld());

	if (targetBuildable->IsA(AFGBuildableConveyorBase::StaticClass()) || commonInfoSubsystem->IsStorageContainer(targetBuildable))
	{
		ShowConveyorPopupWidget();
	}
	else if (targetBuildable->IsA(AFGBuildablePipeline::StaticClass()) || targetBuildable->IsA(AFGBuildablePipelinePump::StaticClass()))
	{
		ShowPipelinePopupWidget();
	}
	else if (commonInfoSubsystem->IsPowerPole(targetBuildable) ||
		commonInfoSubsystem->IsPowerPoleWall(targetBuildable) ||
		commonInfoSubsystem->IsPowerPoleWallDouble(targetBuildable) ||
		commonInfoSubsystem->IsPowerTower(targetBuildable))
	{
		ShowPowerPolePopupWidget();
	}
}

void AMassUpgradeEquipment::Equip(AFGCharacterPlayer* character)
{
	Super::Equip(character);

	HideOutline(character);
}

void AMassUpgradeEquipment::UnEquip()
{
	HideOutline(GetInstigatorCharacter());

	Super::UnEquip();
}

void AMassUpgradeEquipment::ShowOutline(AFGCharacterPlayer* character, AActor* actor)
{
	HideOutline(character);

	if (character && actor)
	{
		character->GetOutline()->ShowOutline(actor, EOutlineColor::OC_USABLE);
		outlinedActor = actor;
	}
}

void AMassUpgradeEquipment::HideOutline(AFGCharacterPlayer* character)
{
	if (character && outlinedActor)
	{
		character->GetOutline()->HideOutline(outlinedActor);
		outlinedActor = nullptr;
	}
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
