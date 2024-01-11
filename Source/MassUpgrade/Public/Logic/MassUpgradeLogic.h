#pragma once
#include "FGRecipe.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildableConveyorLift.h"

#include "MassUpgradeLogic.generated.h"

UCLASS(Blueprintable)
class MASSUPGRADE_API UMassUpgradeLogic : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="MassUpgrade")
	static void UpgradeConveyors
	(
		AFGCharacterPlayer* player,
		const TSubclassOf<UFGRecipe>& newBeltTypeRecipe,
		const TSubclassOf<UFGRecipe>& newLiftTypeRecipe,
		const TSubclassOf<UFGRecipe>& newStorageTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);
	static void UpgradeConveyors_Server
	(
		AFGCharacterPlayer* player,
		const TSubclassOf<UFGRecipe>& newBeltTypeRecipe,
		const TSubclassOf<UFGRecipe>& newLiftTypeRecipe,
		const TSubclassOf<UFGRecipe>& newStorageTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade")
	static void UpgradePipelines
	(
		AFGCharacterPlayer* player,
		const TSubclassOf<UFGRecipe>& newPipelineTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPumpTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);
	static void UpgradePipelines_Server
	(
		AFGCharacterPlayer* player,
		const TSubclassOf<UFGRecipe>& newPipelineTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPumpTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade")
	static void UpgradePowerPoles
	(
		AFGCharacterPlayer* player,
		const TSubclassOf<UFGRecipe>& newWireTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPowerPoleTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPowerPoleWallTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPowerPoleWallDoubleTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPowerTowerTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);
	static void UpgradePowerPoles_Server
	(
		AFGCharacterPlayer* player,
		const TSubclassOf<UFGRecipe>& newWireTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPowerPoleTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPowerPoleWallTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPowerPoleWallDoubleTypeRecipe,
		const TSubclassOf<UFGRecipe>& newPowerTowerTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);

	static int32 UpgradeConveyor
	(
		class AFGCharacterPlayer* player,
		const TArray<class AFGBuildableConveyorBase*>& conveyors,
		TSubclassOf<UFGRecipe> newConveyorTypeRecipe,
		TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
	);

	static int32 UpgradeStorage
	(
		class AFGCharacterPlayer* player,
		const TArray<class AFGBuildableStorage*>& storages,
		TSubclassOf<UFGRecipe> newStorageTypeRecipe,
		TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
	);

	static int32 UpgradePipeline
	(
		class AFGCharacterPlayer* player,
		const TArray<class AFGBuildablePipeline*>& pipelines,
		TSubclassOf<UFGRecipe> newPipelineTypeRecipe,
		TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
	);

	static int32 UpgradePump
	(
		class AFGCharacterPlayer* player,
		const TArray<class AFGBuildablePipelinePump*>& pumps,
		TSubclassOf<UFGRecipe> newPumpTypeRecipe,
		TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
	);

	static int32 UpgradeWire
	(
		class AFGCharacterPlayer* player,
		const TArray<class AFGBuildableWire*>& wires,
		TSubclassOf<UFGRecipe> newWireTypeRecipe,
		TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
	);

	static int32 UpgradePowerPole
	(
		class AFGCharacterPlayer* player,
		const TArray<class AFGBuildablePowerPole*>& powerPoles,
		TSubclassOf<UFGRecipe> newPowerPoleTypeRecipe,
		TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
	);
};
