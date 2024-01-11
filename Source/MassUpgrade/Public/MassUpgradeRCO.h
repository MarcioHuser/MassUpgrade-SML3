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
		TSubclassOf<UFGRecipe> newBeltTypeRecipe,
		TSubclassOf<UFGRecipe> newLiftTypeRecipe,
		TSubclassOf<UFGRecipe> newStorageTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable, Category="MassUpgrade")
	void UpgradePipelines
	(
		class AFGCharacterPlayer* player,
		TSubclassOf<UFGRecipe> newPipelineTypeRecipe,
		TSubclassOf<UFGRecipe> newPumpTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable, Category="MassUpgrade")
	void UpgradePowerPoles
	(
		class AFGCharacterPlayer* player,
		TSubclassOf<UFGRecipe> newWireTypeRecipe,
		TSubclassOf<UFGRecipe> newPowerPoleTypeRecipe,
		TSubclassOf<UFGRecipe> newPowerPoleWallTypeRecipe,
		TSubclassOf<UFGRecipe> newPowerPoleWallDoubleTypeRecipe,
		TSubclassOf<UFGRecipe> newPowerTowerTypeRecipe,
		const TArray<struct FProductionInfo>& infos
	);

	UPROPERTY(Replicated)
	bool dummy = true;
};
