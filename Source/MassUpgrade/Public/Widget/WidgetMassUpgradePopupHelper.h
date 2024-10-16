#pragma once
#include "FGRecipe.h"
#include "Components/ComboBox.h"
#include "Components/ComboBoxKey.h"
#include "Components/GridPanel.h"
#include "Components/PanelWidget.h"
#include "Components/WrapBox.h"

#include <functional>
#include <map>

#include <WidgetMassUpgradePopupHelper.generated.h>

DECLARE_DYNAMIC_DELEGATE_TwoParams(FAddBuildableIconCell, int32, row, TSubclassOf<class UFGItemDescriptor>, buildableDescriptor);

DECLARE_DYNAMIC_DELEGATE_RetVal_FourParams(UWidget*, FCreateWidgetItemIconWithLabel, bool, showItemName, UTexture2D*, iconImage, float, iconSize, const FText&, itemName);

DECLARE_DYNAMIC_DELEGATE_OneParam(FCheckboxStateChanged, bool, checked);

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(TSubclassOf<UFGRecipe>, FGetRecipe, const struct FProductionInfo&, info);

UCLASS()
class MASSUPGRADE_API UWidgetMassUpgradePopupHelper : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(
		BlueprintCallable,
		Category="WidgetMassUpgradePopupHelper",
		meta = (WorldContext= "WorldContextObject", HidePin= "WorldContextObject", DefaultToSelf = "WorldContextObject")
	)
	static void GetConveyorsBySpeed
	(
		class UObject* WorldContextObject,
		UPARAM(Ref) TArray<struct FComboBoxItem>& beltBySpeed,
		UPARAM(Ref) TArray<struct FComboBoxItem>& liftBySpeed,
		UPARAM(Ref) TArray<struct FComboBoxItem>& storageByCapacity
	);

	UFUNCTION(
		BlueprintCallable,
		Category="WidgetMassUpgradePopupHelper",
		meta = (WorldContext= "WorldContextObject", HidePin= "WorldContextObject", DefaultToSelf = "WorldContextObject")
	)
	static void GetPipesBySpeed
	(
		class UObject* WorldContextObject,
		UPARAM(Ref) TArray<struct FComboBoxItem>& pipeByFlowLimit,
		UPARAM(Ref) TArray<struct FComboBoxItem>& pumpByHeadLift
	);

	UFUNCTION(
		BlueprintCallable,
		Category="WidgetMassUpgradePopupHelper",
		meta = (WorldContext= "WorldContextObject", HidePin= "WorldContextObject", DefaultToSelf = "WorldContextObject")
	)
	static void GetPowerPolesByConnections
	(
		class UObject* WorldContextObject,
		UPARAM(Ref) TArray<struct FComboBoxItem>& wireTypes,
		UPARAM(Ref) TArray<struct FComboBoxItem>& powerPoleByConnection,
		UPARAM(Ref) TArray<struct FComboBoxItem>& powerPoleWallByConnection,
		UPARAM(Ref) TArray<struct FComboBoxItem>& powerPoleWallDoubleByConnection,
		UPARAM(Ref) TArray<struct FComboBoxItem>& powerTowerByConnection
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void RemoveExtraItems(UPanelWidget* panelWidget, int maxItems);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void InitializeComboBox
	(
		UComboBoxKey* comboBoxKey,
		const TArray<FComboBoxItem>& cbItems,
		const float& targetAmount,
		AFGBuildable* targetBuildable
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure)
	static float GetConveyorSpeed(class AFGBuildableConveyorBase* conveyor);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void HandleConstructConveyorPopup
	(
		class UGridPanel* gridBuildables,
		class UWrapBox* boxItemsCost,
		class UButton* btnUpgradeBuildables,
		int gridHeaderCount,
		const float& iconSize,
		AFGBuildable* targetBuildable,
		bool includeBelts,
		UComboBoxKey* cmbBeltMk,
		const TArray<struct FComboBoxItem>& beltBySpeed,
		bool includeLifts,
		UComboBoxKey* cmbLiftMk,
		const TArray<struct FComboBoxItem>& liftBySpeed,
		bool includeStorages,
		UComboBoxKey* cmbStorageMk,
		const TArray<struct FComboBoxItem>& storageByCapacity,
		bool crossAttachmentsAndStorages,
		UPARAM(Ref) TArray<struct FProductionInfo>& infos,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		FCheckboxStateChanged checkboxStateChanged
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void HandleConstructPipelinePopup
	(
		class UGridPanel* gridBuildables,
		class UWrapBox* boxItemsCost,
		class UButton* btnUpgradeBuildables,
		int gridHeaderCount,
		const float& iconSize,
		AFGBuildable* targetBuildable,
		bool includePipelines,
		UComboBoxKey* cmbPipelineMk,
		const TArray<struct FComboBoxItem>& pipelineByFlowLimit,
		bool includePumps,
		UComboBoxKey* cmbPumpMk,
		const TArray<struct FComboBoxItem>& pumpByHeadLift,
		bool crossAttachmentsAndStorages,
		UPARAM(Ref) TArray<struct FProductionInfo>& infos,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		FCheckboxStateChanged checkboxStateChanged
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void HandleConstructPowerPolePopup
	(
		class UGridPanel* gridBuildables,
		class UWrapBox* boxItemsCost,
		class UButton* btnUpgradeBuildables,
		int gridHeaderCount,
		const float& iconSize,
		AFGBuildable* targetBuildable,
		bool includeWires,
		UComboBoxKey* cmbWireMk,
		const TArray<FComboBoxItem>& wireTypes,
		bool includePowerPoles,
		UComboBoxKey* cmbPowerPoleMk,
		const TArray<struct FComboBoxItem>& powerPoleByConnection,
		bool includePowerPoleWalls,
		UComboBoxKey* cmbPowerPoleWallMk,
		const TArray<struct FComboBoxItem>& powerPoleWallByConnection,
		bool includePowerPoleWallDoubles,
		UComboBoxKey* cmbPowerPoleWallDoubleMk,
		const TArray<struct FComboBoxItem>& powerPoleWallDoubleByConnection,
		bool includePowerTowers,
		UComboBoxKey* cmbPowerTowerMk,
		const TArray<struct FComboBoxItem>& powerTowerByConnection,
		bool crossAttachmentsAndStorages,
		UPARAM(Ref) TArray<struct FProductionInfo>& infos,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		FCheckboxStateChanged checkboxStateChanged
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void AddConveyors
	(
		class UGridPanel* gridBuildables,
		class UWrapBox* boxItemsCost,
		class UButton* btnUpgradeBuildables,
		int gridHeaderCount,
		const float& iconSize,
		const FName& beltMkType,
		const TArray<struct FComboBoxItem>& beltBySpeed,
		const FName& liftMkType,
		const TArray<struct FComboBoxItem>& liftBySpeed,
		const FName& storageMkType,
		const TArray<struct FComboBoxItem>& storageByCapacity,
		const TArray<struct FProductionInfo>& infos,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		FCheckboxStateChanged checkboxStateChanged
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void AddPipelines
	(
		class UGridPanel* gridBuildables,
		class UWrapBox* boxItemsCost,
		class UButton* btnUpgradeBuildables,
		int gridHeaderCount,
		const float& iconSize,
		const FName& pipelineMkType,
		const TArray<struct FComboBoxItem>& pipelineByFlowLimit,
		const FName& pumpMkType,
		const TArray<struct FComboBoxItem>& pumpByHeadLift,
		const TArray<struct FProductionInfo>& infos,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		FCheckboxStateChanged checkboxStateChanged
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void AddPowerPoles
	(
		class UGridPanel* gridBuildables,
		class UWrapBox* boxItemsCost,
		class UButton* btnUpgradeBuildables,
		int gridHeaderCount,
		const float& iconSize,
		const FName& wireMkType,
		const TArray<FComboBoxItem>& wireTypes,
		const FName& powerPoleMkType,
		const TArray<struct FComboBoxItem>& powerPoleByConnection,
		const FName& powerPoleWallMkType,
		const TArray<struct FComboBoxItem>& powerPoleWallByConnection,
		const FName& powerPoleWallDoubleMkType,
		const TArray<struct FComboBoxItem>& powerPoleWallDoubleByConnection,
		const FName& powerTowerMkType,
		const TArray<struct FComboBoxItem>& powerTowerByConnection,
		const TArray<struct FProductionInfo>& infos,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		FCheckboxStateChanged checkboxStateChanged
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static void AddBuildablesAndCost
	(
		class UGridPanel* gridBuildables,
		class UWrapBox* boxItemsCost,
		class UButton* btnUpgradeBuildables,
		int gridHeaderCount,
		const float& iconSize,
		const TArray<struct FProductionInfo>& infos,
		FGetRecipe getRecipe,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		FCheckboxStateChanged checkboxStateChanged
	);

	static void AddBuildablesAndCost_Implementation
	(
		class UGridPanel* gridBuildables,
		class UWrapBox* boxItemsCost,
		class UButton* btnUpgradeBuildables,
		int gridHeaderCount,
		const float& iconSize,
		const TArray<struct FProductionInfo>& infos,
		const std::function<TSubclassOf<UFGRecipe>(const struct FProductionInfo&)>& getRecipe,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		FCheckboxStateChanged checkboxStateChanged
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper")
	static UWidget* CreateMkComboBoxItemWidget
	(
		UWidget* widgetContext,
		const TArray<struct FComboBoxItem>& comboBoxItems,
		const FName& indexName,
		const FString& suffix,
		TSubclassOf<UFGItemDescriptor> defaultBuildDescriptor,
		FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
		bool showItemName = true
	);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure)
	static TSubclassOf<class UFGBuildDescriptor> GetSelectedBuildDescriptor(const FName& indexName, const TArray<struct FComboBoxItem>& comboBoxItems);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure, DisplayName="GetSelectedBuildDescriptor (from ComboBoxKey)")
	static TSubclassOf<class UFGBuildDescriptor> GetSelectedBuildDescriptorFromComboboxKey(class UComboBoxKey* comboBox, const TArray<struct FComboBoxItem>& comboBoxItems);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure)
	static TSubclassOf<UFGRecipe> GetSelectedRecipe(const FName& indexName, const TArray<struct FComboBoxItem>& comboBoxItems);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure, DisplayName="GetSelectedRecipe (from ComboBoxKey)")
	static TSubclassOf<UFGRecipe> GetSelectedRecipeFromComboboxKey(class UComboBoxKey* comboBox, const TArray<struct FComboBoxItem>& comboBoxItems);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure)
	static struct FComboBoxItem GetSelectedComboBoxItem(const FName& indexName, const TArray<struct FComboBoxItem>& comboBoxItems);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure, DisplayName="GetSelectedComboBoxItem (from ComboBoxKey)")
	static struct FComboBoxItem GetSelectedComboBoxItemFromComboboxKey(class UComboBoxKey* comboBox, const TArray<struct FComboBoxItem>& comboBoxItems);

	static inline FString
	getTagName()
	{
		return TEXT("WidgetMassUpgradePopupHelper: ");
	}

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure, DisplayName="GetCheckboxName", BlueprintPure)
	static FName GetCheckboxName(const struct FProductionInfo& info);

	UFUNCTION(BlueprintCallable, Category="WidgetMassUpgradePopupHelper", BlueprintPure, DisplayName="GetCheckbox", BlueprintPure)
	static class UCheckBox* GetCheckbox(class UWidget* container, const struct FProductionInfo& info);

	static std::map<TSubclassOf<UFGBuildDescriptor>, bool> checkedRecipes;
};
