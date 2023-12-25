#include "MassUpgradeRCO.h"

#include "FGPlayerController.h"
#include "Logic/MassUpgradeLogic.h"
#include "Net/UnrealNetwork.h"
#include "Util/MUOptimize.h"
#include "Util/MULogging.h"

#ifndef OPTIMIZE
#pragma optimize("", off )
#endif

UMassUpgradeRCO* UMassUpgradeRCO::getRCO(UWorld* world)
{
	auto rco = Cast<UMassUpgradeRCO>(
		Cast<AFGPlayerController>(world->GetFirstPlayerController())->GetRemoteCallObjectOfClass(UMassUpgradeRCO::StaticClass())
		);

	return rco;
}

void UMassUpgradeRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMassUpgradeRCO, dummy);
}

void UMassUpgradeRCO::UpgradeConveyors_Implementation
(
	AFGCharacterPlayer* player,
	 TSubclassOf<UFGRecipe> newBeltTypeRecipe,
	 TSubclassOf<UFGRecipe>newLiftTypeRecipe,
	const TArray<FConveyorProductionInfo>& infos
)
{
	UMassUpgradeLogic::UpgradeConveyors_Server(
		player,
		newBeltTypeRecipe,
		newLiftTypeRecipe,
		infos
		);
}

bool UMassUpgradeRCO::UpgradeConveyors_Validate
(
	AFGCharacterPlayer* player,
	 TSubclassOf<UFGRecipe> newBeltTypeRecipe,
	 TSubclassOf<UFGRecipe> newLiftTypeRecipe,
	const TArray<FConveyorProductionInfo>& infos
)
{
	return true;
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
