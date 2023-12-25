#pragma once
#include "Buildables/FGBuildableConveyorBase.h"
#include "Components/HorizontalBox.h"
#include "Model/ConveyorProductionInfo.h"

#include "ConveyorProductionInfoAccessor.generated.h"

UCLASS(Blueprintable)
class MASSUPGRADE_API UConveyorProductionInfoAccessor : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="MassUpgrade")
	static void GetRefundableItems
	(
		UObject* worldContext,
		TSubclassOf<UFGRecipe> newBeltTypeRecipe,
		TSubclassOf<UFGRecipe> newLiftTypeRecipe,
		UPARAM(DisplayName = "Conveyor Production Info") const FConveyorProductionInfo& conveyorProductionInfo,
		UPARAM(DisplayName = "Items To Refund") TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToRefund,
		UPARAM(Ref, DisplayName = "Upgrade Cost") TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
		UPARAM(DisplayName = "Conveyor Length") float& length
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade")
	static bool CanAffordUpgrade
	(
		class AFGCharacterPlayer* player,
		const TSubclassOf<class UFGRecipe>& targetBeltRecipe,
		const TSubclassOf<class UFGRecipe>& targetLiftRecipe,
		UPARAM(DisplayName = "Upgrade Cost") const TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
		const TArray<FConveyorProductionInfo>& infos
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade")
	static void CollectConveyorProductionInfo
	(
		class AFGBuildableConveyorBase* targetConveyor,
		bool includeBelts,
		bool includeLifts,
		bool crossAttachmentsAndStorages,
		UPARAM(Ref) TArray<FConveyorProductionInfo>& infos
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgrade")
	static void AddInfosToShoppingList
	(
		class AFGCharacterPlayer* player,
		const TArray<FConveyorProductionInfo>& infos,
		const TSubclassOf<class UFGRecipe>& targetBeltRecipe,
		const TSubclassOf<class UFGRecipe>& targetLiftRecipe
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
};
