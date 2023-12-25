#include <Widget/WidgetMassUpgradePopupHelper.h>
#include <Util/MassUpgradeConfiguration.h>
#include "FGCharacterPlayer.h"
#include "FGRecipe.h"
#include "FGRecipeManager.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildableConveyorLift.h"
#include "Model/ConveyorBySpeedInfo.h"
#include "Resources/FGBuildDescriptor.h"
#include "Subsystems/CommonInfoSubsystem.h"
#include "Util/MUOptimize.h"
#include "Util/MULogging.h"

#include <map>

#ifndef OPTIMIZE
#pragma optimize("", off)
#endif

void UWidgetMassUpgradePopupHelper::GetConveyorsBySpeed(TMap<float, TSubclassOf<UFGRecipe>>& beltBySpeedMap, TMap<float, TSubclassOf<UFGRecipe>>& liftBySpeedMap)
{
	auto recipeManager = AFGRecipeManager::Get(ACommonInfoSubsystem::Get());

	TArray<TSubclassOf<UFGRecipe>> recipes;
	recipeManager->GetAllAvailableRecipes(recipes);

	beltBySpeedMap.Empty();
	liftBySpeedMap.Empty();

	for (const auto& recipe : recipes)
	{
		auto products = UFGRecipe::GetProducts(recipe);

		if (products.IsEmpty())
		{
			continue;
		}

		TSubclassOf<AFGBuildableConveyorBase> buildClass = *UFGBuildDescriptor::GetBuildClass(*products[0].ItemClass);

		if (!buildClass)
		{
			continue;
		}

		auto buildDefaultObject = buildClass->GetDefaultObject<AFGBuildableConveyorBase>();
		auto speed = buildDefaultObject->GetSpeed() / 2;

		if (buildClass->IsChildOf(AFGBuildableConveyorBelt::StaticClass()))
		{
			beltBySpeedMap.FindOrAdd(speed) = recipe;
		}
		if (buildClass->IsChildOf(AFGBuildableConveyorLift::StaticClass()))
		{
			liftBySpeedMap.FindOrAdd(speed) = recipe;
		}
	}

	beltBySpeedMap.KeySort(
		[](const auto& x, const auto& y)
		{
			return x - y < 0;
		}
		);

	liftBySpeedMap.KeySort(
		[](const auto& x, const auto& y)
		{
			return x - y < 0;
		}
		);
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

void UWidgetMassUpgradePopupHelper::InitializeComboBox(UComboBoxKey* comboBoxKey, const TArray<float>& speeds, const float& targetSpeed)
{
	comboBoxKey->ClearOptions();

	FNumberFormattingOptions options;
	options.MinimumFractionalDigits = 0;
	options.MaximumFractionalDigits = 2;
	options.UseGrouping = false;

	FName lastValid;

	for (const auto& speed : speeds)
	{
		FName option = *FText::AsNumber(speed, &options).ToString();

		if (targetSpeed <= 0 || targetSpeed == speed)
		{
			lastValid = option;
		}

		comboBoxKey->AddOption(option);
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

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
