// ReSharper disable CppUPropertyMacroCallHasNoEffect
#pragma once

#include "FGRemoteCallObject.h"

#include "MassUpgradeRCO.generated.h"

enum class EAutoUpdateType : uint8;

UCLASS()
class MASSUPGRADE_API UMassUpgradeRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()
public:
	static UMassUpgradeRCO* getRCO(UWorld* world);

	UFUNCTION(BlueprintCallable, Category="MassUpgradeRCO", DisplayName="Get Efficiency Checker RCO")
	static UMassUpgradeRCO*
	getRCO(AActor* actor)
	{
		return getRCO(actor->GetWorld());
	}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable, Category="MassUpgrade")
	void UpgradeConveyors
	(
		class AFGCharacterPlayer* player,
		TSubclassOf<class UFGRecipe> newBeltTypeRecipe,
		TSubclassOf<class UFGRecipe> newLiftTypeRecipe,
		TSubclassOf<class UFGRecipe> newStorageTypeRecipe,
		const TArray<struct FProductionInfoWithArray>& infos
	);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable, Category="MassUpgrade")
	void UpgradePipelines
	(
		class AFGCharacterPlayer* player,
		TSubclassOf<class UFGRecipe> newPipelineTypeRecipe,
		TSubclassOf<class UFGRecipe> newPumpTypeRecipe,
		const TArray<struct FProductionInfoWithArray>& infos
	);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable, Category="MassUpgrade")
	void UpgradePowerPoles
	(
		class AFGCharacterPlayer* player,
		TSubclassOf<class UFGRecipe> newWireTypeRecipe,
		TSubclassOf<class UFGRecipe> newPowerPoleTypeRecipe,
		TSubclassOf<class UFGRecipe> newPowerPoleWallTypeRecipe,
		TSubclassOf<class UFGRecipe> newPowerPoleWallDoubleTypeRecipe,
		TSubclassOf<class UFGRecipe> newPowerTowerTypeRecipe,
		const TArray<struct FProductionInfoWithArray>& infos
	);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable, Category="MassUpgrade")
	void CollectConveyorProductionInfo
	(
		class AMassUpgradeEquipment* massUpgradeEquipment,
		class AFGBuildable* targetBuildable,
		bool includeBelts,
		bool includeLifts,
		bool includeStorages,
		const TArray<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		enum CollectProductionInfoIntent collectProductionInfoIntent
	);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable, Category="MassUpgrade")
	void CollectPipelineProductionInfo
	(
		AMassUpgradeEquipment* massUpgradeEquipment,
		AFGBuildable* targetBuildable,
		bool includePipelines,
		bool includePumps,
		const TArray<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		CollectProductionInfoIntent collectProductionInfoIntent
	);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable, Category="MassUpgrade")
	void CollectPowerPoleProductionInfo
	(
		class AMassUpgradeEquipment* massUpgradeEquipment,
		class AFGBuildable* targetBuildable,
		bool includeWires,
		bool includePowerPoles,
		bool includePowerPoleWalls,
		bool includePowerPoleWallDoubles,
		bool includePowerTowers,
		const TArray<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
		bool crossAttachmentsAndStorages,
		CollectProductionInfoIntent collectProductionInfoIntent
	);

	UPROPERTY(Replicated)
	bool dummy = true;
};
