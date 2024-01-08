#pragma once

#include "Equipment/FGEquipment.h"
#include "Resources/FGBuildDescriptor.h"

#include "MassUpgradeEquipment.generated.h"


UCLASS(BlueprintType)
class MASSUPGRADE_API AMassUpgradeEquipment : public AFGEquipment
{
	GENERATED_BODY()
public:
	AMassUpgradeEquipment();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable)
	virtual void PrimaryFirePressed();

	static FString getAuthorityAndPlayer(const AActor* actor);

	UFUNCTION(BlueprintImplementableEvent, Category = "MassUpgrade")
	void ShowConveyorPopupWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "MassUpgrade")
	void ShowPipelinePopupWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "MassUpgrade")
	void ShowPowerPolePopupWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = "MassUpgrade")
	void setBuildDescriptor(TSubclassOf<UFGItemDescriptor> buildDescriptor);

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

public:
	//FORCEINLINE ~AEfficiencyCheckerEquipment() = default;
};
