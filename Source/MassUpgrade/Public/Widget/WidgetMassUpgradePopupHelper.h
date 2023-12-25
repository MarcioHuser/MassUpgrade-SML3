#pragma once
#include "FGRecipe.h"
#include "Components/ComboBoxKey.h"
#include "Components/PanelWidget.h"

#include <WidgetMassUpgradePopupHelper.generated.h>

UCLASS()
class MASSUPGRADE_API UWidgetMassUpgradePopupHelper : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void GetConveyorsBySpeed
	(
		UPARAM(Ref) TMap<float, TSubclassOf<UFGRecipe>>& beltBySpeedMap,
		UPARAM(Ref) TMap<float, TSubclassOf<UFGRecipe>>& liftBySpeedMap
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void RemoveExtraItems(UPanelWidget* panelWidget, int maxItems);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void InitializeComboBox(UComboBoxKey* comboBoxKey, const TArray<float>& speeds, const float& targetSpeed);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure)
	static float GetConveyorSpeed(class AFGBuildableConveyorBase* conveyor);

	static inline FString
	getTagName()
	{
		return TEXT("WidgetMassUpgradePopupHelper: ");
	}
};
