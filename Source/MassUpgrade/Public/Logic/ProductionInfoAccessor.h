﻿#pragma once
#include "Buildables/FGBuildableConveyorBase.h"
#include "Components/HorizontalBox.h"
#include "Model/ProductionInfo.h"
#include "ProductionInfoAccessor.generated.h"

UENUM(BlueprintType)
enum class CollectProductionInfoIntent : uint8
{
	UpdateWidget,
	Upgrade,
};

UCLASS(Blueprintable)
class MASSUPGRADE_API UProductionInfoAccessor : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="MassUpgrade|ProductionInfoAccessor")
	static void GetRefundableItems
	(
		class AFGCharacterPlayer* player,
		TSubclassOf<UFGRecipe> newTypeRecipe,
		UPARAM(DisplayName = "Production Info") const FProductionInfo& productionInfo,
		UPARAM(Ref, DisplayName = "Items To Refund") TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToRefund,
		UPARAM(Ref, DisplayName = "Upgrade Cost") TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
		UPARAM(Ref, DisplayName = "Length") float& amount
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade|ProductionInfoAccessor")
	static bool CanAffordUpgrade
	(
		class AFGCharacterPlayer* player,
		const TArray<TSubclassOf<class UFGRecipe>>& targetRecipes,
		UPARAM(DisplayName = "Upgrade Cost") const TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
		const TArray<FProductionInfo>& infos
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade|ProductionInfoAccessor")
	static void CollectConveyorProductionInfo
	(
		class AMassUpgradeEquipment* massUpgradeEquipment,
		class AFGBuildable* targetBuildable,
		bool includeBelts,
		bool includeLifts,
		bool includeStorages,
		const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		CollectProductionInfoIntent collectProductionInfoIntent
	);

	static void CollectConveyorProductionInfo_Server
	(
		class AMassUpgradeEquipment* massUpgradeEquipment,
		class AFGBuildable* targetBuildable,
		bool includeBelts,
		bool includeLifts,
		bool includeStorages,
		const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		CollectProductionInfoIntent collectProductionInfoIntent
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade|ProductionInfoAccessor")
	static void CollectPipelineProductionInfo
	(
		class AMassUpgradeEquipment* massUpgradeEquipment,
		class AFGBuildable* targetBuildable,
		bool includePipelines,
		bool includePumps,
		const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		CollectProductionInfoIntent collectProductionInfoIntent
	);

	static void CollectPipelineProductionInfo_Server
	(
		class AMassUpgradeEquipment* massUpgradeEquipment,
		class AFGBuildable* targetBuildable,
		bool includePipelines,
		bool includePumps,
		const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		CollectProductionInfoIntent collectProductionInfoIntent
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade|ProductionInfoAccessor")
	static void CollectPowerPoleProductionInfo
	(
		class AMassUpgradeEquipment* massUpgradeEquipment,
		class AFGBuildable* targetBuildable,
		bool includeWires,
		bool includePowerPoles,
		bool includePowerPoleWalls,
		bool includePowerPoleWallDoubles,
		bool includePowerTowers,
		const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		CollectProductionInfoIntent collectProductionInfoIntent
	);

	static void CollectPowerPoleProductionInfo_Server
	(
		class AMassUpgradeEquipment* massUpgradeEquipment,
		class AFGBuildable* targetBuildable,
		bool includeWires,
		bool includePowerPoles,
		bool includePowerPoleWalls,
		bool includePowerPoleWallDoubles,
		bool includePowerTowers,
		const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		CollectProductionInfoIntent collectProductionInfoIntent
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade|ProductionInfoAccessor")
	static void FilterInfos(TSubclassOf<AFGBuildable> baseType, UPARAM(Ref) TArray<FProductionInfo>& infos, TArray<FProductionInfo>& filteredInfos);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade|ProductionInfoAccessor", BlueprintPure)
	static void FilterSelectedInfos
	(
		class UWidget* container,
		UPARAM(Ref) TArray<FProductionInfo>& infos,
		TArray<FProductionInfo>& filteredInfos
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade|ProductionInfoAccessor")
	static void AddInfosToShoppingList
	(
		class AFGCharacterPlayer* player,
		const TArray<FProductionInfo>& infos,
		const TSubclassOf<class UFGRecipe>& targetRecipe
	);

	static void handleStorageTeleporter
	(
		AFGBuildable* storageTeleporter,
		TSet<UFGFactoryConnectionComponent*>& components
	);

	static void handleUndergroundBeltsComponents
	(
		class AFGBuildableStorage* undergroundBelt,
		TSet<UFGFactoryConnectionComponent*>& components
	);

	static void handleModularLoadBalancerComponents
	(
		class AFGBuildableFactory* modularLoadBalancerGroupLeader,
		TSet<UFGFactoryConnectionComponent*>& components
	);

	static void handleTrainPlatformCargoConveyor
	(
		class AFGBuildableTrainPlatformCargo* trainPlatformCargo,
		TSet<class UFGFactoryConnectionComponent*>& components
	);

	static void handleTrainPlatformCargoPipeline
	(
		class AFGBuildableTrainPlatformCargo* trainPlatformCargo,
		TSet<class UFGPipeConnectionComponent*>& components
	);

	static void handleTrainStationPowerPole
	(
		class AFGBuildableRailroadStation* station,
		TSet<class UFGPowerConnectionComponent*>& components
	);

	UFUNCTION(BlueprintPure, Category="MassUpgrade|ProductionInfoAccessor")
	static TArray<FProductionInfoWithArray> ToProductionInfoWithArray(const TArray<FProductionInfo>& infos);
	
	UFUNCTION(BlueprintPure, Category="MassUpgrade|ProductionInfoAccessor")
	static TArray<FProductionInfo> ToProductionInfo(const TArray<FProductionInfoWithArray>& infos);
};
