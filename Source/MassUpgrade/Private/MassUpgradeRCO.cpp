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

void UMassUpgradeRCO::UpgradePowerPoles_Implementation
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newWireTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleWallTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleWallDoubleTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerTowerTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	UMassUpgradeLogic::UpgradePowerPoles_Server(
		player,
		newWireTypeRecipe,
		newPowerPoleTypeRecipe,
		newPowerPoleWallTypeRecipe,
		newPowerPoleWallDoubleTypeRecipe,
		newPowerTowerTypeRecipe,
		infos
		);
}

bool UMassUpgradeRCO::UpgradePowerPoles_Validate
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newWireTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleWallTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleWallDoubleTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerTowerTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	return true;
}

void UMassUpgradeRCO::UpgradePipelines_Implementation
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newPipelineTypeRecipe,
	TSubclassOf<UFGRecipe> newPumpTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	UMassUpgradeLogic::UpgradePipelines_Server(
		player,
		newPipelineTypeRecipe,
		newPumpTypeRecipe,
		infos
		);
}

bool UMassUpgradeRCO::UpgradePipelines_Validate
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newPipelineTypeRecipe,
	TSubclassOf<UFGRecipe> newPumpTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	return true;
}

void UMassUpgradeRCO::UpgradeConveyors_Implementation
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newBeltTypeRecipe,
	TSubclassOf<UFGRecipe> newLiftTypeRecipe,
	TSubclassOf<UFGRecipe> newStorageTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	UMassUpgradeLogic::UpgradeConveyors_Server(
		player,
		newBeltTypeRecipe,
		newLiftTypeRecipe,
		newStorageTypeRecipe,
		infos
		);
}

bool UMassUpgradeRCO::UpgradeConveyors_Validate
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newBeltTypeRecipe,
	TSubclassOf<UFGRecipe> newLiftTypeRecipe,
	TSubclassOf<UFGRecipe> newStorageTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	return true;
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
