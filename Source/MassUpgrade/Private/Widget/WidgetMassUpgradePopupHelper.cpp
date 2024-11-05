#include <Widget/WidgetMassUpgradePopupHelper.h>
#include <Util/MassUpgradeConfiguration.h>
#include "FGCharacterPlayer.h"
#include "FGRecipe.h"
#include "FGRecipeManager.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildableConveyorLift.h"
#include "Resources/FGBuildDescriptor.h"
#include "Subsystems/CommonInfoSubsystem.h"
#include "Util/MUOptimize.h"
#include "Util/MULogging.h"
#include "Buildables/FGBuildableStorage.h"
#include "Components/Border.h"
#include "Components/GridSlot.h"
#include "Components/TextBlock.h"
#include "Logic/ProductionInfoAccessor.h"
#include "Model/ProductionInfo.h"
#include "FGPlayerController.h"
#include "Buildables/FGBuildablePipeline.h"
#include "Buildables/FGBuildablePipelinePump.h"
#include "Components/Button.h"
#include "Components/WrapBoxSlot.h"
#include "Util/MarcioCommonLibsUtils.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Model/ComboBoxItem.h"

#include "FGPowerConnectionComponent.h"
#include "MassUpgradeEquipment.h"
#include "Buildables/FGBuildableWall.h"
#include "Buildables/FGBuildableWire.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Hologram/FGWireHologram.h"

#ifndef OPTIMIZE
#pragma optimize("", off)
#endif

std::map<TSubclassOf<UFGBuildDescriptor>, bool> UWidgetMassUpgradePopupHelper::checkedRecipes;

void UWidgetMassUpgradePopupHelper::GetConveyorsBySpeed
(
	UObject* WorldContextObject,
	TArray<struct FComboBoxItem>& beltBySpeed,
	TArray<struct FComboBoxItem>& liftBySpeed,
	TArray<struct FComboBoxItem>& storageByCapacity
)
{
	auto world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	auto recipeManager = AFGRecipeManager::Get(world);

	TArray<TSubclassOf<UFGRecipe>> recipes;
	recipeManager->GetAllAvailableRecipes(recipes);

	beltBySpeed.Empty();
	liftBySpeed.Empty();
	storageByCapacity.Empty();

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(world);

	for (const auto& recipe : recipes)
	{
		auto products = UFGRecipe::GetProducts(recipe);
		if (products.IsEmpty())
		{
			continue;
		}

		TSubclassOf<AFGBuildable> buildClass = *UFGBuildDescriptor::GetBuildClass(*products[0].ItemClass);

		if (!buildClass)
		{
			continue;
		}
		else if (buildClass->IsChildOf(AFGBuildableConveyorBelt::StaticClass()))
		{
			auto buildDefaultObject = buildClass->GetDefaultObject<AFGBuildableConveyorBase>();
			auto speed = buildDefaultObject->GetSpeed() / 2;

			FComboBoxItem cbItem(recipe, speed);

			beltBySpeed.Add(cbItem);
		}
		else if (buildClass->IsChildOf(AFGBuildableConveyorLift::StaticClass()))
		{
			auto buildDefaultObject = buildClass->GetDefaultObject<AFGBuildableConveyorBase>();
			auto speed = buildDefaultObject->GetSpeed() / 2;

			liftBySpeed.Add(FComboBoxItem(recipe, speed));
		}
		else if (commonInfoSubsystem->IsStorageContainer(nullptr, buildClass))
		{
			auto buildDefaultObject = buildClass->GetDefaultObject<AFGBuildableStorage>();
			auto capacity = buildDefaultObject->mInventorySizeX * buildDefaultObject->mInventorySizeY;

			storageByCapacity.Add(FComboBoxItem(recipe, capacity));
		}
	}

	auto sortComboBoxItem = [](const FComboBoxItem& x, const FComboBoxItem& y)
	{
		return x.amount - y.amount < 0;
	};

	beltBySpeed.Sort(sortComboBoxItem);
	liftBySpeed.Sort(sortComboBoxItem);
	storageByCapacity.Sort(sortComboBoxItem);
}

void UWidgetMassUpgradePopupHelper::GetPipesBySpeed
(
	UObject* WorldContextObject,
	TArray<struct FComboBoxItem>& pipeByFlowLimit,
	TArray<struct FComboBoxItem>& pumpByHeadLift
)
{
	auto world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	auto recipeManager = AFGRecipeManager::Get(world);

	TArray<TSubclassOf<UFGRecipe>> recipes;
	recipeManager->GetAllAvailableRecipes(recipes);

	pipeByFlowLimit.Empty();
	pumpByHeadLift.Empty();

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(world);

	for (const auto& recipe : recipes)
	{
		auto products = UFGRecipe::GetProducts(recipe);
		if (products.IsEmpty())
		{
			continue;
		}

		TSubclassOf<AFGBuildable> buildClass = *UFGBuildDescriptor::GetBuildClass(*products[0].ItemClass);

		if (!buildClass)
		{
			continue;
		}
		else if (buildClass->IsChildOf(AFGBuildablePipeline::StaticClass()))
		{
			auto buildDefaultObject = buildClass->GetDefaultObject<AFGBuildablePipeline>();
			auto flowLimit = buildDefaultObject->GetFlowLimit() * 60;

			pipeByFlowLimit.Add(FComboBoxItem(recipe, flowLimit));
		}
		else if (buildClass->IsChildOf(AFGBuildablePipelinePump::StaticClass()))
		{
			auto buildDefaultObject = buildClass->GetDefaultObject<AFGBuildablePipelinePump>();
			auto headLift = buildDefaultObject->GetDesignHeadLift();

			if (headLift > 0)
			{
				pumpByHeadLift.Add(FComboBoxItem(recipe, headLift));
			}
		}
	}

	auto sortComboBoxItem = [](const FComboBoxItem& x, const FComboBoxItem& y)
	{
		return x.amount - y.amount < 0;
	};

	pipeByFlowLimit.Sort(sortComboBoxItem);
	pumpByHeadLift.Sort(sortComboBoxItem);
}

void UWidgetMassUpgradePopupHelper::GetPowerPolesByConnections
(
	UObject* WorldContextObject,
	TArray<FComboBoxItem>& wireTypes,
	TArray<FComboBoxItem>& powerPoleByConnection,
	TArray<FComboBoxItem>& powerPoleWallByConnection,
	TArray<FComboBoxItem>& powerPoleWallDoubleByConnection,
	TArray<FComboBoxItem>& powerTowerByConnection
)
{
	auto world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	auto recipeManager = AFGRecipeManager::Get(world);

	TArray<TSubclassOf<UFGRecipe>> recipes;
	recipeManager->GetAllAvailableRecipes(recipes);

	wireTypes.Empty();
	powerPoleByConnection.Empty();
	powerPoleWallByConnection.Empty();
	powerPoleWallDoubleByConnection.Empty();
	powerTowerByConnection.Empty();

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(world);

	for (const auto& recipe : recipes)
	{
		auto products = UFGRecipe::GetProducts(recipe);
		if (products.IsEmpty())
		{
			continue;
		}

		TSubclassOf<AFGBuildable> buildClass = *UFGBuildDescriptor::GetBuildClass(*products[0].ItemClass);

		if (!buildClass)
		{
			continue;
		}
		else if (buildClass->IsChildOf(AFGBuildableWire::StaticClass()))
		{
			wireTypes.Add(FComboBoxItem(recipe, 0));
		}
		else if (commonInfoSubsystem->IsPowerPole(nullptr, buildClass))
		{
			auto pole = world->SpawnActor<AFGBuildablePowerPole>(buildClass);
			pole->SetHidden(true);
			auto maxConnections = pole->GetPowerConnection(0)->GetMaxNumConnections();
			pole->Destroy();

			powerPoleByConnection.Add(FComboBoxItem(recipe, maxConnections));
		}
		else if (commonInfoSubsystem->IsPowerPoleWall(nullptr, buildClass))
		{
			auto pole = world->SpawnActor<AFGBuildablePowerPole>(buildClass);
			pole->SetHidden(true);
			auto maxConnections = pole->GetPowerConnection(0)->GetMaxNumConnections();
			pole->Destroy();

			powerPoleWallByConnection.Add(FComboBoxItem(recipe, maxConnections));
		}
		else if (commonInfoSubsystem->IsPowerPoleWallDouble(nullptr, buildClass))
		{
			auto pole = world->SpawnActor<AFGBuildablePowerPole>(buildClass);
			pole->SetHidden(true);
			auto maxConnections = pole->GetPowerConnection(0)->GetMaxNumConnections();
			pole->Destroy();

			powerPoleWallDoubleByConnection.Add(FComboBoxItem(recipe, maxConnections));
		}
		else if (commonInfoSubsystem->IsPowerTower(nullptr, buildClass))
		{
			auto pole = world->SpawnActor<AFGBuildablePowerPole>(buildClass);
			pole->SetHidden(true);
			auto maxConnections = pole->GetPowerConnection(0)->GetMaxNumConnections();
			pole->Destroy();

			powerTowerByConnection.Add(FComboBoxItem(recipe, maxConnections));
		}
	}

	auto sortComboBoxItem = [](const FComboBoxItem& x, const FComboBoxItem& y)
	{
		auto recipeX = AFGBuildable::GetBuildableClassFromRecipe(x.recipe);
		auto recipeY = AFGBuildable::GetBuildableClassFromRecipe(y.recipe);

		auto order = 0;

		if (recipeX->IsChildOf(AFGBuildableWire::StaticClass()) && recipeY->IsChildOf(AFGBuildableWire::StaticClass()))
		{
			order = recipeX->GetName().Compare(recipeY->GetName());
		}

		if (!order)
		{
			order = x.amount - y.amount;
		}

		return order < 0;
	};

	wireTypes.Sort(sortComboBoxItem);
	powerPoleByConnection.Sort(sortComboBoxItem);
	powerPoleWallByConnection.Sort(sortComboBoxItem);
	powerPoleWallDoubleByConnection.Sort(sortComboBoxItem);
	powerTowerByConnection.Sort(sortComboBoxItem);
}

void UWidgetMassUpgradePopupHelper::RemoveExtraItems(UPanelWidget* panelWidget, int maxItems)
{
	MU_LOG_Display_Condition(
		*getTagName(),
		TEXT("RemoveExtraItems = "),
		*GetPathNameSafe(panelWidget)
		);

	if (!panelWidget)
	{
		return;
	}

	while (panelWidget->GetChildrenCount() > maxItems)
	{
		panelWidget->RemoveChildAt(maxItems);
	}
}

void UWidgetMassUpgradePopupHelper::InitializeComboBox
(
	UComboBoxKey* comboBoxKey,
	const TArray<FComboBoxItem>& cbItems,
	const float& targetAmount,
	AFGBuildable* targetBuildable
)
{
	comboBoxKey->ClearOptions();

	FName lastValid;

	int index = 0;

	auto targetDescriptor = targetBuildable ? targetBuildable->GetBuiltWithDescriptor<UFGBuildDescriptor>() : nullptr;

	for (const auto& cbItem : cbItems)
	{
		FName option = *LexToString(index);

		auto products = UFGRecipe::GetProducts(cbItem.recipe);

		if ((targetAmount <= 0 || targetAmount == cbItem.amount) &&
			(!targetBuildable || (!products.IsEmpty() && targetDescriptor == products[0].ItemClass)))
		{
			lastValid = option;
		}

		comboBoxKey->AddOption(option);

		index++;
	}

	if (lastValid != TEXT(""))
	{
		comboBoxKey->SetSelectedOption(lastValid);
	}
}

float UWidgetMassUpgradePopupHelper::GetConveyorSpeed(AFGBuildableConveyorBase* conveyor)
{
	return conveyor ? conveyor->GetSpeed() / 2 : 0;
}

void UWidgetMassUpgradePopupHelper::HandleConstructConveyorPopup
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	UGridPanel* gridBuildables,
	UWrapBox* boxItemsCost,
	UButton* btnUpgradeBuildables,
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
	bool crossAttachmentsAndStorages
)
{
	MU_LOG_Display_Condition(
		*getTagName(),
		TEXT("HandleConstructConveyorPopup begin")
		);

	TArray<float> keys;

	auto storage = Cast<AFGBuildableStorage>(targetBuildable);
	auto conveyor = Cast<AFGBuildableConveyorBase>(targetBuildable);

	TSet<TSubclassOf<class UFGBuildDescriptor>> selectedTypes;

	if (cmbBeltMk)
	{
		InitializeComboBox(cmbBeltMk, beltBySpeed, conveyor ? conveyor->GetSpeed() / 2 : 0, Cast<AFGBuildableConveyorBelt>(conveyor));

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbBeltMk->GetSelectedOption(), beltBySpeed));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	if (cmbLiftMk)
	{
		InitializeComboBox(cmbLiftMk, liftBySpeed, conveyor ? conveyor->GetSpeed() / 2 : 0, Cast<AFGBuildableConveyorLift>(conveyor));

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbLiftMk->GetSelectedOption(), liftBySpeed));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	if (cmbStorageMk)
	{
		InitializeComboBox(cmbStorageMk, storageByCapacity, storage ? storage->mInventorySizeX * storage->mInventorySizeY : 0, storage);

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbStorageMk->GetSelectedOption(), storageByCapacity));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	UProductionInfoAccessor::CollectConveyorProductionInfo(
		massUpgradeEquipment,
		targetBuildable,
		includeBelts,
		includeLifts,
		includeStorages,
		selectedTypes,
		crossAttachmentsAndStorages,
		CollectProductionInfoIntent::UpdateWidget
		);

	// AddConveyors(
	// 	gridBuildables,
	// 	boxItemsCost,
	// 	btnUpgradeBuildables,
	// 	gridHeaderCount,
	// 	iconSize,
	// 	cmbBeltMk ? cmbBeltMk->GetSelectedOption() : FName(),
	// 	beltBySpeed,
	// 	cmbLiftMk ? cmbLiftMk->GetSelectedOption() : FName(),
	// 	liftBySpeed,
	// 	cmbStorageMk ? cmbStorageMk->GetSelectedOption() : FName(),
	// 	storageByCapacity,
	// 	infos,
	// 	createWidgetItemIconWithLabel,
	// 	checkboxStateChanged
	// 	);

	MU_LOG_Display_Condition(
		*getTagName(),
		TEXT("HandleConstructConveyorPopup done")
		);
}

void UWidgetMassUpgradePopupHelper::HandleConstructPipelinePopup
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	UGridPanel* gridBuildables,
	UWrapBox* boxItemsCost,
	UButton* btnUpgradeBuildables,
	int gridHeaderCount,
	const float& iconSize,
	AFGBuildable* targetBuildable,
	bool includePipelines,
	UComboBoxKey* cmbPipelineMk,
	const TArray<struct FComboBoxItem>& pipelineByFlowLimit,
	bool includePumps,
	UComboBoxKey* cmbPumpMk,
	const TArray<struct FComboBoxItem>& pumpByHeadLift,
	bool crossAttachmentsAndStorages
)
{
	MU_LOG_Display_Condition(
		*getTagName(),
		TEXT("HandleConstructPipelinePopup begin")
		);

	TArray<float> keys;

	auto pipeline = Cast<AFGBuildablePipeline>(targetBuildable);
	auto pump = Cast<AFGBuildablePipelinePump>(targetBuildable);

	TSet<TSubclassOf<class UFGBuildDescriptor>> selectedTypes;

	if (cmbPipelineMk)
	{
		InitializeComboBox(cmbPipelineMk, pipelineByFlowLimit, pipeline ? pipeline->GetFlowLimit() * 60 : 0, pipeline);

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbPipelineMk->GetSelectedOption(), pipelineByFlowLimit));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	if (cmbPumpMk)
	{
		InitializeComboBox(cmbPumpMk, pumpByHeadLift, pump ? pump->GetDesignHeadLift() : 0, pump);

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbPumpMk->GetSelectedOption(), pumpByHeadLift));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	UProductionInfoAccessor::CollectPipelineProductionInfo(
		massUpgradeEquipment,
		targetBuildable,
		includePipelines,
		includePumps,
		selectedTypes,
		crossAttachmentsAndStorages,
		CollectProductionInfoIntent::UpdateWidget
		);

	// AddPipelines(
	// 	gridBuildables,
	// 	boxItemsCost,
	// 	btnUpgradeBuildables,
	// 	gridHeaderCount,
	// 	iconSize,
	// 	cmbPipelineMk ? cmbPipelineMk->GetSelectedOption() : FName(),
	// 	pipelineByFlowLimit,
	// 	cmbPumpMk ? cmbPumpMk->GetSelectedOption() : FName(),
	// 	pumpByHeadLift,
	// 	infos,
	// 	createWidgetItemIconWithLabel,
	// 	checkboxStateChanged
	// 	);

	MU_LOG_Display_Condition(
		*getTagName(),
		TEXT("HandleConstructPipelinePopup done")
		);
}

void UWidgetMassUpgradePopupHelper::HandleConstructPowerPolePopup
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	UGridPanel* gridBuildables,
	UWrapBox* boxItemsCost,
	UButton* btnUpgradeBuildables,
	int gridHeaderCount,
	const float& iconSize,
	AFGBuildable* targetBuildable,
	bool includeWires,
	UComboBoxKey* cmbWireMk,
	const TArray<FComboBoxItem>& wireTypes,
	bool includePowerPoles,
	UComboBoxKey* cmbPowerPoleMk,
	const TArray<FComboBoxItem>& powerPoleByConnection,
	bool includePowerPoleWalls,
	UComboBoxKey* cmbPowerPoleWallMk,
	const TArray<FComboBoxItem>& powerPoleWallByConnection,
	bool includePowerPoleWallDoubles,
	UComboBoxKey* cmbPowerPoleWallDoubleMk,
	const TArray<FComboBoxItem>& powerPoleWallDoubleByConnection,
	bool includePowerTowers,
	UComboBoxKey* cmbPowerTowerMk,
	const TArray<FComboBoxItem>& powerTowerByConnection,
	bool crossAttachmentsAndStorages
)
{
	MU_LOG_Display_Condition(
		*getTagName(),
		TEXT("HandleConstructPowerPolePopup begin")
		);

	TArray<float> keys;

	auto powerPole = Cast<AFGBuildablePowerPole>(targetBuildable);
	auto wire = Cast<AFGBuildableWire>(targetBuildable);

	TSet<TSubclassOf<class UFGBuildDescriptor>> selectedTypes;

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(targetBuildable->GetWorld());

	if (cmbWireMk)
	{
		InitializeComboBox(
			cmbWireMk,
			wireTypes,
			0,
			wire
			);

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbWireMk->GetSelectedOption(), wireTypes));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	if (cmbPowerPoleMk)
	{
		InitializeComboBox(
			cmbPowerPoleMk,
			powerPoleByConnection,
			commonInfoSubsystem->IsPowerPole(powerPole) ? powerPole->GetPowerConnection(0)->GetMaxNumConnections() : 0,
			commonInfoSubsystem->IsPowerPole(powerPole) ? powerPole : nullptr
			);

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbPowerPoleMk->GetSelectedOption(), powerPoleByConnection));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	if (cmbPowerPoleWallMk)
	{
		InitializeComboBox(
			cmbPowerPoleWallMk,
			powerPoleWallByConnection,
			commonInfoSubsystem->IsPowerPoleWall(powerPole) ? powerPole->GetPowerConnection(0)->GetMaxNumConnections() : 0,
			commonInfoSubsystem->IsPowerPoleWall(powerPole) ? powerPole : nullptr
			);

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbPowerPoleWallMk->GetSelectedOption(), powerPoleWallByConnection));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	if (cmbPowerPoleWallDoubleMk)
	{
		InitializeComboBox(
			cmbPowerPoleWallDoubleMk,
			powerPoleWallDoubleByConnection,
			commonInfoSubsystem->IsPowerPoleWallDouble(powerPole) ? powerPole->GetPowerConnection(0)->GetMaxNumConnections() : 0,
			commonInfoSubsystem->IsPowerPoleWallDouble(powerPole) ? powerPole : nullptr
			);

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbPowerPoleWallDoubleMk->GetSelectedOption(), powerPoleWallDoubleByConnection));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	if (cmbPowerTowerMk)
	{
		InitializeComboBox(
			cmbPowerTowerMk,
			powerTowerByConnection,
			commonInfoSubsystem->IsPowerTower(powerPole) ? powerPole->GetPowerConnection(0)->GetMaxNumConnections() : 0,
			commonInfoSubsystem->IsPowerTower(powerPole) ? powerPole : nullptr
			);

		auto products = UFGRecipe::GetProducts(GetSelectedRecipe(cmbPowerTowerMk->GetSelectedOption(), powerTowerByConnection));
		if (!products.IsEmpty())
		{
			selectedTypes.Add(*products[0].ItemClass);
		}
	}

	UProductionInfoAccessor::CollectPowerPoleProductionInfo(
		massUpgradeEquipment,
		targetBuildable,
		includeWires,
		includePowerPoles,
		includePowerPoleWalls,
		includePowerPoleWallDoubles,
		includePowerTowers,
		selectedTypes,
		crossAttachmentsAndStorages,
		CollectProductionInfoIntent::UpdateWidget
		);

	// AddPowerPoles(
	// 	gridBuildables,
	// 	boxItemsCost,
	// 	btnUpgradeBuildables,
	// 	gridHeaderCount,
	// 	iconSize,
	// 	cmbWireMk ? cmbWireMk->GetSelectedOption() : FName(),
	// 	wireTypes,
	// 	cmbPowerPoleMk ? cmbPowerPoleMk->GetSelectedOption() : FName(),
	// 	powerPoleByConnection,
	// 	cmbPowerPoleWallMk ? cmbPowerPoleWallMk->GetSelectedOption() : FName(),
	// 	powerPoleWallByConnection,
	// 	cmbPowerPoleWallDoubleMk ? cmbPowerPoleWallDoubleMk->GetSelectedOption() : FName(),
	// 	powerPoleWallDoubleByConnection,
	// 	cmbPowerTowerMk ? cmbPowerTowerMk->GetSelectedOption() : FName(),
	// 	powerTowerByConnection,
	// 	infos,
	// 	createWidgetItemIconWithLabel,
	// 	checkboxStateChanged
	// 	);

	MU_LOG_Display_Condition(
		*getTagName(),
		TEXT("HandleConstructPowerPolePopup done")
		);
}

void UWidgetMassUpgradePopupHelper::AddConveyors
(
	UGridPanel* gridBuildables,
	UWrapBox* boxItemsCost,
	UButton* btnUpgradeBuildables,
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
)
{
	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(gridBuildables->GetWorld());

	AddBuildablesAndCost_Implementation(
		gridBuildables,
		boxItemsCost,
		btnUpgradeBuildables,
		gridHeaderCount,
		iconSize,
		infos,
		[=](const FProductionInfo& productionInfo)
		{
			auto buildClass = UFGBuildDescriptor::GetBuildClass(productionInfo.buildableType);

			if (!buildClass)
			{
				return TSubclassOf<UFGRecipe>();
			}
			else if (buildClass->IsChildOf(AFGBuildableConveyorBelt::StaticClass()))
			{
				return GetSelectedRecipe(beltMkType, beltBySpeed);
			}
			else if (buildClass->IsChildOf(AFGBuildableConveyorLift::StaticClass()))
			{
				return GetSelectedRecipe(liftMkType, liftBySpeed);
			}
			else if (commonInfoSubsystem->IsStorageContainer(nullptr, buildClass))
			{
				return GetSelectedRecipe(storageMkType, storageByCapacity);
			}
			else
			{
				return TSubclassOf<UFGRecipe>();
			}
		},
		createWidgetItemIconWithLabel,
		checkboxStateChanged
		);
}

void UWidgetMassUpgradePopupHelper::AddPipelines
(
	UGridPanel* gridBuildables,
	UWrapBox* boxItemsCost,
	UButton* btnUpgradeBuildables,
	int gridHeaderCount,
	const float& iconSize,
	const FName& pipelineMkType,
	const TArray<struct FComboBoxItem>& pipelineByFlowLimit,
	const FName& pumpMkType,
	const TArray<struct FComboBoxItem>& pumpByHeadLift,
	const TArray<FProductionInfo>& infos,
	FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
	FCheckboxStateChanged checkboxStateChanged
)
{
	AddBuildablesAndCost_Implementation(
		gridBuildables,
		boxItemsCost,
		btnUpgradeBuildables,
		gridHeaderCount,
		iconSize,
		infos,
		[=](const FProductionInfo& productionInfo)
		{
			auto buildClass = UFGBuildDescriptor::GetBuildClass(productionInfo.buildableType);

			if (!buildClass)
			{
				return TSubclassOf<UFGRecipe>();
			}
			else if (buildClass->IsChildOf(AFGBuildablePipeline::StaticClass()))
			{
				return GetSelectedRecipe(pipelineMkType, pipelineByFlowLimit);
			}
			else if (buildClass->IsChildOf(AFGBuildablePipelinePump::StaticClass()))
			{
				return GetSelectedRecipe(pumpMkType, pumpByHeadLift);
			}
			else
			{
				return TSubclassOf<UFGRecipe>();
			}
		},
		createWidgetItemIconWithLabel,
		checkboxStateChanged
		);
}

void UWidgetMassUpgradePopupHelper::AddPowerPoles
(
	UGridPanel* gridBuildables,
	UWrapBox* boxItemsCost,
	UButton* btnUpgradeBuildables,
	int gridHeaderCount,
	const float& iconSize,
	const FName& wireMkType,
	const TArray<FComboBoxItem>& wireTypes,
	const FName& powerPoleMkType,
	const TArray<FComboBoxItem>& powerPoleByConnection,
	const FName& powerPoleWallMkType,
	const TArray<FComboBoxItem>& powerPoleWallByConnection,
	const FName& powerPoleWallDoubleMkType,
	const TArray<FComboBoxItem>& powerPoleWallDoubleByConnection,
	const FName& powerTowerMkType,
	const TArray<FComboBoxItem>& powerTowerByConnection,
	const TArray<FProductionInfo>& infos,
	FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
	FCheckboxStateChanged checkboxStateChanged
)
{
	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(gridBuildables->GetWorld());

	AddBuildablesAndCost_Implementation(
		gridBuildables,
		boxItemsCost,
		btnUpgradeBuildables,
		gridHeaderCount,
		iconSize,
		infos,
		[=](const FProductionInfo& productionInfo)
		{
			auto buildClass = UFGBuildDescriptor::GetBuildClass(productionInfo.buildableType);

			if (!buildClass)
			{
				return TSubclassOf<UFGRecipe>();
			}
			else if (buildClass->IsChildOf(AFGBuildableWire::StaticClass()))
			{
				return GetSelectedRecipe(wireMkType, wireTypes);
			}
			else if (commonInfoSubsystem->IsPowerPole(nullptr, buildClass))
			{
				return GetSelectedRecipe(powerPoleMkType, powerPoleByConnection);
			}
			else if (commonInfoSubsystem->IsPowerPoleWall(nullptr, buildClass))
			{
				return GetSelectedRecipe(powerPoleWallMkType, powerPoleWallByConnection);
			}
			else if (commonInfoSubsystem->IsPowerPoleWallDouble(nullptr, buildClass))
			{
				return GetSelectedRecipe(powerPoleWallDoubleMkType, powerPoleWallDoubleByConnection);
			}
			else if (commonInfoSubsystem->IsPowerTower(nullptr, buildClass))
			{
				return GetSelectedRecipe(powerTowerMkType, powerTowerByConnection);
			}
			else
			{
				return TSubclassOf<UFGRecipe>();
			}
		},
		createWidgetItemIconWithLabel,
		checkboxStateChanged
		);
}

void UWidgetMassUpgradePopupHelper::AddBuildablesAndCost
(
	UGridPanel* gridBuildables,
	UWrapBox* boxItemsCost,
	UButton* btnUpgradeBuildables,
	int gridHeaderCount,
	const float& iconSize,
	const TArray<FProductionInfo>& infos,
	FGetRecipe getRecipe,
	FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
	FCheckboxStateChanged checkboxStateChanged
)
{
	AddBuildablesAndCost_Implementation(
		gridBuildables,
		boxItemsCost,
		btnUpgradeBuildables,
		gridHeaderCount,
		iconSize,
		infos,
		[=](const FProductionInfo& productionInfo)
		{
			return getRecipe.IsBound() ? getRecipe.Execute(productionInfo) : nullptr;
		},
		createWidgetItemIconWithLabel,
		checkboxStateChanged
		);
}

void UWidgetMassUpgradePopupHelper::AddBuildablesAndCost_Implementation
(
	UGridPanel* gridBuildables,
	UWrapBox* boxItemsCost,
	UButton* btnUpgradeBuildables,
	int gridHeaderCount,
	const float& iconSize,
	const TArray<FProductionInfo>& infos,
	const std::function<TSubclassOf<UFGRecipe>(const struct FProductionInfo&)>& getRecipe,
	FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
	FCheckboxStateChanged checkboxStateChanged
)
{
	auto player = UMarcioCommonLibsUtils::GetFGPlayer(gridBuildables);

	auto widgetTree = Cast<UWidgetTree>(gridBuildables->GetOuter());

	TMap<TSubclassOf<UFGItemDescriptor>, int32> upgradeCost;
	TSet<TSubclassOf<UFGRecipe>> targetRecipes;
	std::map<TSubclassOf<UFGBuildDescriptor>, TMap<TSubclassOf<UFGItemDescriptor>, int32>> itemsToRefundMap;
	std::map<TSubclassOf<UFGBuildDescriptor>, float> amountMap;

	auto& localCheckedRecipes = checkedRecipes;

	bool hasChecked = false;

	for (const auto& info : infos)
	{
		TSubclassOf<UFGRecipe> recipe = getRecipe(info);

		if (!recipe)
		{
			continue;
		}

		auto checkbox = widgetTree->FindWidget<UCheckBox>(GetCheckboxName(info));
		if (checkbox)
		{
			localCheckedRecipes[info.buildableType] = checkbox->IsChecked();
		}
		else if (localCheckedRecipes.find(info.buildableType) == localCheckedRecipes.end())
		{
			localCheckedRecipes[info.buildableType] = true;
		}

		hasChecked |= localCheckedRecipes[info.buildableType];

		if (localCheckedRecipes[info.buildableType])
		{
			targetRecipes.Add(recipe);

			UProductionInfoAccessor::GetRefundableItems(
				player,
				recipe,
				info,
				itemsToRefundMap[info.buildableType],
				upgradeCost,
				amountMap[info.buildableType]
				);
		}
	}

	RemoveExtraItems(gridBuildables, gridHeaderCount);
	boxItemsCost->ClearChildren();

	FNumberFormattingOptions options;
	options.MinimumFractionalDigits = 0;
	options.MaximumFractionalDigits = 1;
	options.UseGrouping = true;

	auto rowNumber = 0;

	for (const auto& info : infos)
	{
		// if (amountMap.find(info.buildableType) == amountMap.end())
		// {
		// 	continue;
		// }

		auto isChecked = localCheckedRecipes.find(info.buildableType) == localCheckedRecipes.end()
			                 ? true
			                 : localCheckedRecipes[info.buildableType];

		rowNumber++;

		auto buildClass = UFGBuildDescriptor::GetBuildClass(info.buildableType);

		auto amount = amountMap[info.buildableType];
		auto& itemsToRefund = itemsToRefundMap[info.buildableType];

		// Add Buildable Icon Cell
		{
			// auto horizontalBoxWidget = widgetTree->ConstructWidget<UHorizontalBox>();

			auto checkbox = widgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), GetCheckboxName(info));

			checkbox->SetIsChecked(isChecked);

			if (checkboxStateChanged.IsBound())
			{
				checkbox->OnCheckStateChanged.Add(checkboxStateChanged);
			}

			// horizontalBoxWidget->AddChildToHorizontalBox(checkbox);

			UWidget* widget;
			if (createWidgetItemIconWithLabel.IsBound())
			{
				widget = createWidgetItemIconWithLabel.Execute(
					true,
					UFGItemDescriptor::GetBigIcon(info.buildableType),
					iconSize,
					UFGItemDescriptor::GetItemName(info.buildableType)
					);
			}
			else
			{
				widget = widgetTree->ConstructWidget<UBorder>();
			}

			checkbox->AddChild(widget);

			auto gridSlot = gridBuildables->AddChildToGrid(checkbox, rowNumber, 0);
			gridSlot->SetPadding(FMargin(0, 10, 45, 0));
		}

		// Add Pipeline Length
		{
			auto beltLengthTextBoxWidget = widgetTree->ConstructWidget<UTextBlock>();

			beltLengthTextBoxWidget->SetJustification(ETextJustify::Center);
			auto font = beltLengthTextBoxWidget->GetFont();
			font.Size = 16;
			beltLengthTextBoxWidget->SetFont(font);

			if (isChecked)
			{
				if (buildClass->IsChildOf(AFGBuildableConveyorBase::StaticClass()) ||
					buildClass->IsChildOf(AFGBuildablePipeline::StaticClass()))
				{
					beltLengthTextBoxWidget->SetText(FText::Format(FTextFormat::FromString(TEXT("{0} m")), FText::AsNumber(amount, &options)));
				}
				else
				{
					beltLengthTextBoxWidget->SetText(FText::AsNumber(amount, &options));
				}
			}
			else
			{
				beltLengthTextBoxWidget->SetText(FText::FromString(TEXT("--")));
			}

			auto gridSlot = gridBuildables->AddChildToGrid(beltLengthTextBoxWidget, rowNumber, 1);
			gridSlot->SetPadding(FMargin(0, 10, 45, 0));
		}

		// Add Refund
		{
			auto wrapBoxWidget = widgetTree->ConstructWidget<UWrapBox>();
			wrapBoxWidget->SetInnerSlotPadding(FVector2D(5, 5));
			wrapBoxWidget->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);

			auto gridSlot = gridBuildables->AddChildToGrid(wrapBoxWidget, rowNumber, 2);
			gridSlot->SetPadding(FMargin(0, 10, 0, 0));

			auto hasRefund = false;

			if (isChecked)
			{
				for (auto& entry : itemsToRefund)
				{
					const auto& key = entry.Key;

					if (upgradeCost.Contains(key))
					{
						auto commonAmount = FMath::Min(entry.Value, upgradeCost[key]);

						entry.Value -= commonAmount;
						upgradeCost[key] -= commonAmount;

						if (!upgradeCost[key])
						{
							upgradeCost.Remove(key);
						}

						if (!entry.Value)
						{
							continue;
						}
					}

					if (!createWidgetItemIconWithLabel.IsBound())
					{
						continue;
					}

					hasRefund = true;

					auto widget = createWidgetItemIconWithLabel.Execute(
						true,
						UFGItemDescriptor::GetBigIcon(entry.Key),
						iconSize,
						FText::Format(
							FTextFormat::FromString(TEXT("{0} {1}")),
							UFGItemDescriptor::GetItemName(entry.Key),
							entry.Value
							)
						);

					auto wrapSlot = wrapBoxWidget->AddChildToWrapBox(widget);
					if (wrapBoxWidget->GetChildrenCount() > 1)
					{
						wrapSlot->SetPadding(FMargin(15, 0, 0, 0));
					}
				}
			}

			if (!hasRefund)
			{
				auto widget = widgetTree->ConstructWidget<UTextBlock>();

				widget->SetJustification(ETextJustify::Center);
				auto font = widget->GetFont();
				font.Size = 16;
				widget->SetFont(font);
				widget->SetText(FText::FromString(TEXT("--")));

				wrapBoxWidget->AddChildToWrapBox(widget);
			}
		}
	}

	// Add build cost
	for (const auto& entry : upgradeCost)
	{
		if (!createWidgetItemIconWithLabel.IsBound())
		{
			continue;
		}

		auto widget = createWidgetItemIconWithLabel.Execute(
			true,
			UFGItemDescriptor::GetBigIcon(entry.Key),
			iconSize,
			FText::Format(
				FTextFormat::FromString(TEXT("{0} {1}")),
				entry.Value,
				UFGItemDescriptor::GetItemName(entry.Key)
				)
			);

		auto slot = boxItemsCost->AddChildToWrapBox(widget);
		slot->SetPadding(FMargin(0, 0, 15, 0));
	}

	auto canUpgrade = hasChecked && UProductionInfoAccessor::CanAffordUpgrade(
		player,
		targetRecipes.Array(),
		upgradeCost,
		infos
		);

	btnUpgradeBuildables->SetIsEnabled(canUpgrade);
}

UWidget* UWidgetMassUpgradePopupHelper::CreateMkComboBoxItemWidget
(
	UWidget* widgetContext,
	const TArray<FComboBoxItem>& comboBoxItems,
	const FName& indexName,
	const FString& suffix,
	TSubclassOf<UFGItemDescriptor> defaultBuildDescriptor,
	FCreateWidgetItemIconWithLabel createWidgetItemIconWithLabel,
	bool showItemName
)
{
	auto cbItem = GetSelectedComboBoxItem(indexName, comboBoxItems);

	MU_LOG_Display(
		TEXT("Key "),
		*indexName.ToString(),
		TEXT(" maps to recipe "),
		*UKismetSystemLibrary::GetClassDisplayName(cbItem.recipe)
		);

	UWidget* widget;

	if (cbItem.recipe)
	{
		defaultBuildDescriptor = UFGRecipe::GetProducts(cbItem.recipe)[0].ItemClass;
	}

	if (defaultBuildDescriptor && createWidgetItemIconWithLabel.IsBound())
	{
		auto buildClass = UFGBuildDescriptor::GetBuildClass(*defaultBuildDescriptor);

		widget = createWidgetItemIconWithLabel.Execute(
			showItemName,
			UFGItemDescriptor::GetBigIcon(defaultBuildDescriptor),
			48,
			buildClass && buildClass->IsChildOf(AFGBuildableWire::StaticClass())
				? UFGItemDescriptor::GetItemName(defaultBuildDescriptor)
				: FText::Format(
					FTextFormat::FromString(TEXT("{0} {1}")),
					cbItem.amount,
					FText::FromString(suffix)
					)
			);
	}
	else
	{
		auto widgetTree = Cast<UWidgetTree>(widgetContext->GetOuter());

		widget = widgetTree->ConstructWidget<UBorder>();
	}

	return widget;
}

TSubclassOf<UFGBuildDescriptor> UWidgetMassUpgradePopupHelper::GetSelectedBuildDescriptor(const FName& indexName, const TArray<FComboBoxItem>& comboBoxItems)
{
	auto products = UFGRecipe::GetProducts(GetSelectedRecipe(indexName, comboBoxItems));
	if (!products.IsEmpty())
	{
		return *products[0].ItemClass;
	}

	return nullptr;
}

TSubclassOf<UFGBuildDescriptor> UWidgetMassUpgradePopupHelper::GetSelectedBuildDescriptorFromComboboxKey(UComboBoxKey* comboBox, const TArray<FComboBoxItem>& comboBoxItems)
{
	return GetSelectedBuildDescriptor(comboBox ? comboBox->GetSelectedOption() : FName(), comboBoxItems);
}

TSubclassOf<UFGRecipe> UWidgetMassUpgradePopupHelper::GetSelectedRecipe(const FName& indexName, const TArray<struct FComboBoxItem>& comboBoxItems)
{
	return GetSelectedComboBoxItem(indexName, comboBoxItems).recipe;
}

TSubclassOf<UFGRecipe> UWidgetMassUpgradePopupHelper::GetSelectedRecipeFromComboboxKey(UComboBoxKey* comboBox, const TArray<FComboBoxItem>& comboBoxItems)
{
	return GetSelectedRecipe(comboBox ? comboBox->GetSelectedOption() : FName(), comboBoxItems);
}

FComboBoxItem UWidgetMassUpgradePopupHelper::GetSelectedComboBoxItem(const FName& indexName, const TArray<FComboBoxItem>& comboBoxItems)
{
	int index = -1;
	if (indexName.ToString().IsNumeric())
	{
		LexFromString(index, *indexName.ToString());
	}

	if (comboBoxItems.IsValidIndex(index))
	{
		return comboBoxItems[index];
	}

	return FComboBoxItem();
}

FComboBoxItem UWidgetMassUpgradePopupHelper::GetSelectedComboBoxItemFromComboboxKey(UComboBoxKey* comboBox, const TArray<FComboBoxItem>& comboBoxItems)
{
	return GetSelectedComboBoxItem(comboBox ? comboBox->GetSelectedOption() : FName(), comboBoxItems);
}

FName UWidgetMassUpgradePopupHelper::GetCheckboxName(const FProductionInfo& info)
{
	return *(TEXT("check") + info.buildableType->GetName());
}

UCheckBox* UWidgetMassUpgradePopupHelper::GetCheckbox(UWidget* container, const FProductionInfo& info)
{
	auto widgetTree = Cast<UWidgetTree>(container->GetOuter());

	return widgetTree->FindWidget<UCheckBox>(GetCheckboxName(info));
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
