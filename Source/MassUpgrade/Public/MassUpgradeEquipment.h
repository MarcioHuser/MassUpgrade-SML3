#pragma once

#include "Equipment/FGEquipment.h"
#include "Resources/FGBuildDescriptor.h"

#include "MassUpgradeEquipment.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSetProductionInfos,
	const TArray<struct FProductionInfoWithArray>&,
	infos,
	enum CollectProductionInfoIntent,
	collectProductionInfoIntent
	);

UCLASS(BlueprintType)
class MASSUPGRADE_API AMassUpgradeEquipment : public AFGEquipment
{
	GENERATED_BODY()
public:
	AMassUpgradeEquipment();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void Equip(AFGCharacterPlayer* character) override;
	virtual void UnEquip() override;

	UFUNCTION(BlueprintCallable)
	virtual void PrimaryFirePressed();

	UPROPERTY(BlueprintAssignable, Category = "MassUpgrade")
	FSetProductionInfos OnSetProductionInfos;

	UFUNCTION(Category = "EfficiencyChecker", NetMulticast, Reliable)
	virtual void SetProductionInfos
	(
		const TArray<struct FProductionInfoWithArray>& infos,
		enum CollectProductionInfoIntent collectProductionInfoIntent
	);

	static FString getAuthorityAndPlayer(const AActor* actor);

	UFUNCTION(BlueprintImplementableEvent, Category = "MassUpgrade")
	void ShowConveyorPopupWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "MassUpgrade")
	void ShowPipelinePopupWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "MassUpgrade")
	void ShowPowerPolePopupWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "MassUpgrade")
	void setBuildDescriptor(TSubclassOf<UFGItemDescriptor> buildDescriptor);

	virtual void ShowOutline(class AFGCharacterPlayer* character, class AActor* actor);
	virtual void HideOutline(class AFGCharacterPlayer* character);

	FString _TAG_NAME = TEXT("MassUpgradeEquipment: ");

	inline FString
	getTagName() const
	{
		return /*getTimeStamp() + TEXT(" ") +*/ _TAG_NAME;
	}

	UPROPERTY(BlueprintReadWrite, Category = "MassUpgrade")
	class AFGBuildable* targetBuildable = nullptr;

	UPROPERTY(BlueprintReadWrite)
	class UPointLightComponent* pointLight = nullptr;

	UPROPERTY()
	class AActor* outlinedActor = nullptr;

public:
	//FORCEINLINE ~AEfficiencyCheckerEquipment() = default;
};
