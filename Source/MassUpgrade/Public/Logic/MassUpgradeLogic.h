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
		const TArray<struct FConveyorProductionInfo>& infos
	);
	static void UpgradeConveyors_Server
	(
		AFGCharacterPlayer* player,
		const TSubclassOf<UFGRecipe>& newBeltTypeRecipe,
		const TSubclassOf<UFGRecipe>& newLiftTypeRecipe,
		const TArray<struct FConveyorProductionInfo>& infos
	);

	static int32 UpgradeBelt
	(
		class AFGCharacterPlayer* player,
		const TArray<class AFGBuildableConveyorBelt*>& belts,
		TSubclassOf<UFGRecipe> newBeltTypeRecipe,
		TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
	);

	static int32 UpgradeLifts
	(
		class AFGCharacterPlayer* player,
		const TArray<class AFGBuildableConveyorLift*>& lifts,
		TSubclassOf<UFGRecipe> newBeltTypeRecipe,
		TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
	);
};
