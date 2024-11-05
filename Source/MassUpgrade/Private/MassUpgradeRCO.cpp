#include "MassUpgradeRCO.h"

#include "FGPlayerController.h"
#include "Logic/MassUpgradeLogic.h"
#include "Logic/ProductionInfoAccessor.h"
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

void UMassUpgradeRCO::CollectPowerPoleProductionInfo_Implementation
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includeWires,
	bool includePowerPoles,
	bool includePowerPoleWalls,
	bool includePowerPoleWallDoubles,
	bool includePowerTowers,
	const TArray<TSubclassOf<UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	UProductionInfoAccessor::CollectPowerPoleProductionInfo_Server(
		massUpgradeEquipment,
		targetBuildable,
		includeWires,
		includePowerPoles,
		includePowerPoleWalls,
		includePowerPoleWallDoubles,
		includePowerTowers,
		TSet<TSubclassOf<class UFGBuildDescriptor>>(selectedTypes),
		crossAttachmentsAndStorages,
		collectProductionInfoIntent
		);
}

bool UMassUpgradeRCO::CollectPowerPoleProductionInfo_Validate
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includeWires,
	bool includePowerPoles,
	bool includePowerPoleWalls,
	bool includePowerPoleWallDoubles,
	bool includePowerTowers,
	const TArray<TSubclassOf<UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	return true;
}

void UMassUpgradeRCO::CollectPipelineProductionInfo_Implementation
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includePipelines,
	bool includePumps,
	const TArray<TSubclassOf<UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	UProductionInfoAccessor::CollectPipelineProductionInfo_Server(
		massUpgradeEquipment,
		targetBuildable,
		includePipelines,
		includePumps,
		TSet<TSubclassOf<UFGBuildDescriptor>>(selectedTypes),
		crossAttachmentsAndStorages,
		collectProductionInfoIntent
		);
}

bool UMassUpgradeRCO::CollectPipelineProductionInfo_Validate
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includePipelines,
	bool includePumps,
	const TArray<TSubclassOf<UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	return true;
}

void UMassUpgradeRCO::CollectConveyorProductionInfo_Implementation
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includeBelts,
	bool includeLifts,
	bool includeStorages,
	const TArray<TSubclassOf<UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	UProductionInfoAccessor::CollectConveyorProductionInfo_Server(
		massUpgradeEquipment,
		targetBuildable,
		includeBelts,
		includeLifts,
		includeStorages,
		TSet<TSubclassOf<UFGBuildDescriptor>>(selectedTypes),
		crossAttachmentsAndStorages,
		collectProductionInfoIntent
		);
}

bool UMassUpgradeRCO::CollectConveyorProductionInfo_Validate
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includeBelts,
	bool includeLifts,
	bool includeStorages,
	const TArray<TSubclassOf<UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	return true;
}

void UMassUpgradeRCO::UpgradePowerPoles_Implementation
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newWireTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleWallTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerPoleWallDoubleTypeRecipe,
	TSubclassOf<UFGRecipe> newPowerTowerTypeRecipe,
	const TArray<FProductionInfoWithArray>& infos
)
{
	UMassUpgradeLogic::UpgradePowerPoles_Server(
		player,
		newWireTypeRecipe,
		newPowerPoleTypeRecipe,
		newPowerPoleWallTypeRecipe,
		newPowerPoleWallDoubleTypeRecipe,
		newPowerTowerTypeRecipe,
		UProductionInfoAccessor::ToProductionInfo(infos)
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
	const TArray<FProductionInfoWithArray>& infos
)
{
	return true;
}

void UMassUpgradeRCO::UpgradePipelines_Implementation
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newPipelineTypeRecipe,
	TSubclassOf<UFGRecipe> newPumpTypeRecipe,
	const TArray<FProductionInfoWithArray>& infos
)
{
	UMassUpgradeLogic::UpgradePipelines_Server(
		player,
		newPipelineTypeRecipe,
		newPumpTypeRecipe,
		UProductionInfoAccessor::ToProductionInfo(infos)
		);
}

bool UMassUpgradeRCO::UpgradePipelines_Validate
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newPipelineTypeRecipe,
	TSubclassOf<UFGRecipe> newPumpTypeRecipe,
	const TArray<FProductionInfoWithArray>& infos
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
	const TArray<FProductionInfoWithArray>& infos
)
{
	UMassUpgradeLogic::UpgradeConveyors_Server(
		player,
		newBeltTypeRecipe,
		newLiftTypeRecipe,
		newStorageTypeRecipe,
		UProductionInfoAccessor::ToProductionInfo(infos)
		);
}

bool UMassUpgradeRCO::UpgradeConveyors_Validate
(
	AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newBeltTypeRecipe,
	TSubclassOf<UFGRecipe> newLiftTypeRecipe,
	TSubclassOf<UFGRecipe> newStorageTypeRecipe,
	const TArray<FProductionInfoWithArray>& infos
)
{
	return true;
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
