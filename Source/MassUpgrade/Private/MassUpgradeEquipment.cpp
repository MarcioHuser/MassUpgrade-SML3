#include "MassUpgradeEquipment.h"

#include "FGPlayerController.h"
#include "Buildables/FGBuildableConveyorBase.h"
#include "Logic/ConveyorProductionInfoAccessor.h"
#include "Util/MULogging.h"
#include "Util/MarcioCommonLibsUtils.h"
#include "Util/MassUpgradeConfiguration.h"

#ifndef OPTIMIZE
#pragma optimize("", off )
#endif

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

	auto outline = GetInstigatorCharacter()->GetOutline();

	auto hitActor = UMarcioCommonLibsUtils::GetHitActor(hit);

	if (hitActor && playerController->WasInputKeyJustPressed(EKeys::NumPadZero))
	{
		UMarcioCommonLibsUtils::DumpUnknownClass(hitActor);
		// ARecipeCopierLogic::DumpUnknownClass(hitActor);
	}

	auto tempTargetConveyor = Cast<AFGBuildableConveyorBase>(hitActor);

	if (tempTargetConveyor != targetConveyor)
	{
		if (tempTargetConveyor)
		{
			MU_LOG_Display_Condition(
				*getTagName(),
				TEXT("Targeted new building = "),
				*GetPathNameSafe(tempTargetConveyor),
				TEXT(" / "),
				*getAuthorityAndPlayer(this)
				);

			setBuildDescriptor(tempTargetConveyor->GetBuiltWithDescriptor());
		}
	}

	targetConveyor = tempTargetConveyor;

	if (!targetConveyor)
	{
		outline->HideOutline();

		pointLight->SetIntensity(0);

		return;
	}

	outline->ShowOutline(targetConveyor, EOutlineColor::OC_USABLE);

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
		*GetPathNameSafe(targetConveyor),
		TEXT(" / "),
		*getAuthorityAndPlayer(this)
		);

	if (!targetConveyor)
	{
		return;
	}

	// infos.Empty();
	//
	// UConveyorProductionInfoAccessor::CollectConveyorProductionInfo(
	// 	targetConveyor,
	// 	crossAttachmentsAndStorages,
	// 	infos
	// 	);

	ShowPopupWidget();
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
