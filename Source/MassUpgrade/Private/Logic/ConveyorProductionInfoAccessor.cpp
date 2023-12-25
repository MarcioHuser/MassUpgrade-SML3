#include "Logic/ConveyorProductionInfoAccessor.h"

#include "FGCharacterPlayer.h"
#include "FGFactoryConnectionComponent.h"
#include "FGRecipeManager.h"
#include "Buildables/FGBuildableConveyorAttachment.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildableConveyorLift.h"
#include "Buildables/FGBuildableStorage.h"
#include "Model/ConveyorBySpeedInfo.h"
#include "Resources/FGBuildDescriptor.h"

#include <map>

#include "FGGameState.h"
#include "FGPlayerController.h"
#include "Engine/Engine.h"
#include "Reflection/ReflectionHelper.h"
#include "ShoppingList/FGShoppingListObject.h"
#include "Subsystems/CommonInfoSubsystem.h"

#ifndef OPTIMIZE
#pragma optimize("", off)
#endif

void UConveyorProductionInfoAccessor::GetRefundableItems
(
	UObject* worldContext,
	TSubclassOf<UFGRecipe> newBeltTypeRecipe,
	TSubclassOf<UFGRecipe> newLiftTypeRecipe,
	const FConveyorProductionInfo& conveyorProductionInfo,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToRefund,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
	float& length
)
{
	itemsToRefund.Empty();
	length = 0;

	auto newBeltType = newBeltTypeRecipe ? AFGBuildable::GetBuildableClassFromRecipe(newBeltTypeRecipe) : nullptr;
	auto newBeltCost = newBeltTypeRecipe ? UFGRecipe::GetIngredients(newBeltTypeRecipe) : TArray<FItemAmount>();

	auto newLiftType = newLiftTypeRecipe ? AFGBuildable::GetBuildableClassFromRecipe(newLiftTypeRecipe) : nullptr;
	auto newLiftCost = newLiftTypeRecipe ? UFGRecipe::GetIngredients(newLiftTypeRecipe) : TArray<FItemAmount>();

	auto gameState = Cast<AFGGameState>(GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull)->GetGameState());

	for (const auto conveyor : conveyorProductionInfo.conveyors)
	{
		length += conveyor->GetLength();

		auto conveyorClass = conveyor->GetClass();
		auto belt = Cast<AFGBuildableConveyorBelt>(conveyor);
		auto lift = Cast<AFGBuildableConveyorLift>(conveyor);

		if (!gameState->GetCheatNoCost() && ((belt && conveyorClass != newBeltType) || (lift && conveyorClass != newLiftType)))
		{
			int32 multiplier = 1;
			if (belt)
			{
				multiplier = conveyor->GetDismantleRefundReturnsMultiplier();
			}
			else if (lift)
			{
				multiplier = conveyor->GetDismantleRefundReturnsMultiplier();
			}

			TArray<FInventoryStack> refunds;
			conveyor->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToRefund.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}

			if (newBeltType && belt)
			{
				for (const auto& item : newBeltCost)
				{
					upgradeCost.FindOrAdd(item.ItemClass) += multiplier * item.Amount;
				}
			}
			else if (newLiftType && lift)
			{
				for (const auto& item : newLiftCost)
				{
					upgradeCost.FindOrAdd(item.ItemClass) += multiplier * item.Amount;
				}
			}
		}
	}

	itemsToRefund.KeySort(
		[](const auto& x, const auto& y)
		{
			auto nameX = UFGItemDescriptor::GetItemName(x);
			auto nameY = UFGItemDescriptor::GetItemName(y);

			return nameX.CompareTo(nameY) < 0;
		}
		);

	upgradeCost.KeySort(
		[](const auto& x, const auto& y)
		{
			auto nameX = UFGItemDescriptor::GetItemName(x);
			auto nameY = UFGItemDescriptor::GetItemName(y);

			return nameX.CompareTo(nameY) < 0;
		}
		);
}

bool UConveyorProductionInfoAccessor::CanAffordUpgrade
(
	class AFGCharacterPlayer* player,
	const TSubclassOf<class UFGRecipe>& targetBeltRecipe,
	const TSubclassOf<class UFGRecipe>& targetLiftRecipe,
	const TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
	const TArray<FConveyorProductionInfo>& infos
)
{
	if (!player)
	{
		return false;
	}

	auto playerInventory = player->GetInventory();

	auto gameState = Cast<AFGGameState>(player->GetWorld()->GetGameState());

	auto newBeltType = targetBeltRecipe ? AFGBuildable::GetBuildableClassFromRecipe(targetBeltRecipe) : nullptr;
	auto newLiftType = targetLiftRecipe ? AFGBuildable::GetBuildableClassFromRecipe(targetLiftRecipe) : nullptr;

	if (!gameState->GetCheatNoCost())
	{
		auto hasSomeCost = false;

		// Must afford all to be true
		for (const auto& entry : upgradeCost)
		{
			if (playerInventory->GetNumItems(entry.Key) < entry.Value)
			{
				return false;
			}

			hasSomeCost |= entry.Value > 0;
		}

		return hasSomeCost;
	}

	// Must refund something to be true
	for (const auto& info : infos)
	{
		auto buildClass = UFGBuildDescriptor::GetBuildClass(info.conveyorType);

		if (newBeltType == buildClass || newLiftType == buildClass)
		{
			// Already of the target type. No upgrade will happen
			continue;
		}

		return true;
	}

	// If we got here, we have nothing to refund or upgrade
	return false;
}

void UConveyorProductionInfoAccessor::CollectConveyorProductionInfo
(
	AFGBuildableConveyorBase* targetConveyor,
	bool includeBelts,
	bool includeLifts,
	bool crossAttachmentsAndStorages,
	TArray<FConveyorProductionInfo>& infos
)
{
	if (!targetConveyor)
	{
		return;
	}

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get();

	std::map<TSubclassOf<AFGBuildableConveyorBase>, FConveyorProductionInfo> infosMap;

	TSet<AFGBuildable*> seenBuildable;
	TSet<AFGBuildable*> pendingBuildable;

	seenBuildable.Add(targetConveyor);
	pendingBuildable.Add(targetConveyor);

	while (!pendingBuildable.IsEmpty())
	{
		auto buildable = *pendingBuildable.begin();
		pendingBuildable.Remove(buildable);

		if (auto conveyor = Cast<AFGBuildableConveyorBase>(buildable))
		{
			auto& info = infosMap[conveyor->GetClass()];
			info.conveyorType = conveyor->GetBuiltWithDescriptor<UFGBuildDescriptor>();

			if (info.conveyors.Contains(conveyor))
			{
				// Already processed. Skip to next buildable
				continue;
			}

			if (includeBelts && conveyor->IsA(AFGBuildableConveyorBelt::StaticClass()) ||
				includeLifts && conveyor->IsA(AFGBuildableConveyorLift::StaticClass()))
			{
				info.conveyors.Add(conveyor);
			}
		}

		TSet<UFGFactoryConnectionComponent*> components;

		if (commonInfoSubsystem->baseStorageTeleporterClass && buildable->IsA(commonInfoSubsystem->baseStorageTeleporterClass))
		{
			handleStorageTeleporter(buildable, components);
		}
		else if (commonInfoSubsystem->baseModularLoadBalancerClass && buildable->IsA(commonInfoSubsystem->baseModularLoadBalancerClass))
		{
			if (auto modularLoadBalancer = FReflectionHelper::GetObjectPropertyValue<AFGBuildableFactory>(buildable, TEXT("GroupLeader")))
			{
				handleModularLoadBalancerComponents(modularLoadBalancer, components);
			}
		}
		else if (commonInfoSubsystem->baseUndergroundSplitterInputClass && buildable->IsA(commonInfoSubsystem->baseUndergroundSplitterInputClass) ||
			commonInfoSubsystem->baseUndergroundSplitterOutputClass && buildable->IsA(commonInfoSubsystem->baseUndergroundSplitterOutputClass))
		{
			if (auto undergroundBelt = Cast<AFGBuildableStorage>(buildable))
			{
				handleUndergroundBeltsComponents(undergroundBelt, components);
			}
		}
		else
		{
			TArray<UFGFactoryConnectionComponent*> tempComponents;
			buildable->GetComponents(tempComponents);
			components.Append(tempComponents);
		}

		for (const auto& component : components)
		{
			if (!component->IsConnected())
			{
				continue;
			}

			auto otherBuildable = Cast<AFGBuildable>(component->GetConnection()->GetOwner());
			if (!otherBuildable)
			{
				continue;
			}

			if (!otherBuildable->IsA(AFGBuildableConveyorBase::StaticClass()) &&
				!(crossAttachmentsAndStorages && (
						otherBuildable->IsA(AFGBuildableConveyorAttachment::StaticClass()) ||
						otherBuildable->IsA(AFGBuildableStorage::StaticClass()) ||
						commonInfoSubsystem->baseStorageTeleporterClass && otherBuildable->IsA(commonInfoSubsystem->baseStorageTeleporterClass) ||
						commonInfoSubsystem->baseModularLoadBalancerClass && otherBuildable->IsA(commonInfoSubsystem->baseModularLoadBalancerClass) ||
						commonInfoSubsystem->baseUndergroundSplitterInputClass && otherBuildable->IsA(commonInfoSubsystem->baseUndergroundSplitterInputClass) ||
						commonInfoSubsystem->baseUndergroundSplitterOutputClass && otherBuildable->IsA(commonInfoSubsystem->baseUndergroundSplitterOutputClass)
					)
				))
			{
				// Not a valid buildable to add to check
				continue;
			}

			if (seenBuildable.Contains(otherBuildable))
			{
				continue;
			}

			seenBuildable.Add(otherBuildable);
			pendingBuildable.Add(otherBuildable);
		}
	}

	for (const auto& entry : infosMap)
	{
		if (entry.second.conveyors.IsEmpty())
		{
			continue;
		}

		infos.Add(entry.second);
	}

	infos.Sort(
		[](const FConveyorProductionInfo& conveyor1, const FConveyorProductionInfo& conveyor2)
		{
			auto buildClass1 = UFGBuildDescriptor::GetBuildClass(conveyor1.conveyorType);
			auto buildClass2 = UFGBuildDescriptor::GetBuildClass(conveyor2.conveyorType);

			auto type1 = buildClass1->IsChildOf(AFGBuildableConveyorBelt::StaticClass()) ? 0 : 1;
			auto type2 = buildClass2->IsChildOf(AFGBuildableConveyorBelt::StaticClass()) ? 0 : 1;

			float order = type1 - type2;

			if (order == 0)
			{
				order = buildClass1->GetDefaultObject<AFGBuildableConveyorBase>()->GetSpeed() - buildClass2->GetDefaultObject<AFGBuildableConveyorBase>()->GetSpeed();
			}

			return order < 0;
		}
		);
}

void UConveyorProductionInfoAccessor::handleStorageTeleporter
(
	AFGBuildable* storageTeleporter,
	TSet<UFGFactoryConnectionComponent*>& components
)
{
	// Find all others of the same type
	auto currentStorageID = FReflectionHelper::GetPropertyValue<FStrProperty>(storageTeleporter, TEXT("StorageID"));

	TArray<UFGFactoryConnectionComponent*> tempComponents;
	storageTeleporter->GetComponents(tempComponents);
	components.Append(tempComponents);

	auto CommonInfoSubsystem = ACommonInfoSubsystem::Get();

	FScopeLock ScopeLock(&ACommonInfoSubsystem::mclCritical);

	for (auto testTeleporter : CommonInfoSubsystem->allTeleporters)
	{
		if (!IsValid(testTeleporter) || testTeleporter == storageTeleporter)
		{
			continue;
		}

		auto storageID = FReflectionHelper::GetPropertyValue<FStrProperty>(testTeleporter, TEXT("StorageID"));
		if (storageID != currentStorageID)
		{
			continue;
		}

		testTeleporter->GetComponents(tempComponents);
		components.Append(tempComponents);
	}
}

void UConveyorProductionInfoAccessor::handleUndergroundBeltsComponents
(
	AFGBuildableStorage* undergroundBelt,
	TSet<UFGFactoryConnectionComponent*>& components
)
{
	TArray<UFGFactoryConnectionComponent*> tempComponents;
	undergroundBelt->GetComponents(tempComponents);
	components.Append(tempComponents);

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get();

	FScopeLock ScopeLock(&ACommonInfoSubsystem::mclCritical);

	if (commonInfoSubsystem->baseUndergroundSplitterInputClass &&
		undergroundBelt->IsA(commonInfoSubsystem->baseUndergroundSplitterInputClass))
	{
		auto outputsProperty = CastField<FArrayProperty>(undergroundBelt->GetClass()->FindPropertyByName("Outputs"));
		if (!outputsProperty)
		{
			return;
		}

		FScriptArrayHelper arrayHelper(outputsProperty, outputsProperty->ContainerPtrToValuePtr<void>(undergroundBelt));
		auto arrayObjectProperty = CastField<FObjectProperty>(outputsProperty->Inner);

		for (auto x = 0; x < arrayHelper.Num(); x++)
		{
			void* ObjectContainer = arrayHelper.GetRawPtr(x);
			auto outputUndergroundBelt = Cast<AFGBuildableFactory>(arrayObjectProperty->GetObjectPropertyValue(ObjectContainer));

			if (outputUndergroundBelt)
			{
				outputUndergroundBelt->GetComponents(tempComponents);
				components.Append(tempComponents);
			}
		}
	}
	else if (commonInfoSubsystem->baseUndergroundSplitterOutputClass &&
		undergroundBelt->IsA(commonInfoSubsystem->baseUndergroundSplitterOutputClass))
	{
		for (auto inputUndergroundBelt : commonInfoSubsystem->allUndergroundInputBelts)
		{
			auto outputsProperty = CastField<FArrayProperty>(inputUndergroundBelt->GetClass()->FindPropertyByName("Outputs"));
			if (!outputsProperty)
			{
				continue;
			}

			FScriptArrayHelper arrayHelper(outputsProperty, outputsProperty->ContainerPtrToValuePtr<void>(inputUndergroundBelt));
			auto arrayObjectProperty = CastField<FObjectProperty>(outputsProperty->Inner);

			auto found = false;

			for (auto x = 0; !found && x < arrayHelper.Num(); x++)
			{
				void* ObjectContainer = arrayHelper.GetRawPtr(x);
				found = undergroundBelt == arrayObjectProperty->GetObjectPropertyValue(ObjectContainer);
			}

			if (!found)
			{
				continue;
			}

			inputUndergroundBelt->GetComponents(tempComponents);
			components.Append(tempComponents);
		}
	}
}


void UConveyorProductionInfoAccessor::handleModularLoadBalancerComponents
(
	AFGBuildableFactory* modularLoadBalancerGroupLeader,
	TSet<UFGFactoryConnectionComponent*>& components
)
{
	auto groupModulesArrayProperty = CastField<FArrayProperty>(modularLoadBalancerGroupLeader->GetClass()->FindPropertyByName(TEXT("mGroupModules")));
	if (!groupModulesArrayProperty)
	{
		return;
	}

	if (groupModulesArrayProperty)
	{
		FScriptArrayHelper arrayHelper(groupModulesArrayProperty, groupModulesArrayProperty->ContainerPtrToValuePtr<void>(modularLoadBalancerGroupLeader));

		auto arrayWeakObjectProperty = CastField<FWeakObjectProperty>(groupModulesArrayProperty->Inner);

		for (auto x = 0; x < arrayHelper.Num(); x++)
		{
			void* modularLoadBalancerObjectContainer = arrayHelper.GetRawPtr(x);
			auto modularLoadBalancer = Cast<AFGBuildableFactory>(arrayWeakObjectProperty->GetObjectPropertyValue(modularLoadBalancerObjectContainer));
			if (!modularLoadBalancer)
			{
				continue;
			}

			TArray<UFGFactoryConnectionComponent*> tempComponents;
			modularLoadBalancer->GetComponents(tempComponents);
			components.Append(tempComponents);
		}
	}
}

void UConveyorProductionInfoAccessor::AddInfosToShoppingList
(
	AFGCharacterPlayer* player,
	const TArray<FConveyorProductionInfo>& infos,
	const TSubclassOf<class UFGRecipe>& targetBeltRecipe,
	const TSubclassOf<class UFGRecipe>& targetLiftRecipe
)
{
	if (!player)
	{
		return;
	}

	auto shoppingListComponent = UFGShoppingListComponent::GetShoppingListComponent(player->GetFGPlayerController());

	auto newBeltType = targetBeltRecipe ? AFGBuildable::GetBuildableClassFromRecipe(targetBeltRecipe) : nullptr;
	auto newLiftType = targetLiftRecipe ? AFGBuildable::GetBuildableClassFromRecipe(targetLiftRecipe) : nullptr;

	for (const auto& info : infos)
	{
		if (info.conveyors.IsEmpty())
		{
			continue;
		}

		auto buildClass = UFGBuildDescriptor::GetBuildClass(info.conveyorType);

		if (newBeltType == buildClass || newLiftType == buildClass)
		{
			// Already of the target type or no item in list. No upgrade will happen
			continue;
		}

		auto recipe = buildClass->IsChildOf(AFGBuildableConveyorBelt::StaticClass()) ? targetBeltRecipe : targetLiftRecipe;
		if (!recipe)
		{
			continue;
		}

		bool out_result;
		auto shoppingListObject_Class = Cast<UFGShoppingListObject_Class>(shoppingListComponent->GetShoppingListObjectFromClass(recipe, out_result));

		if (!out_result)
		{
			shoppingListObject_Class = NewObject<UFGShoppingListObject_Class>(shoppingListComponent);
			shoppingListObject_Class->SetSubClass(recipe);
		}

		// FShoppingListRecipeEntry recipeEntry(recipe, 0);

		for (auto conveyor : info.conveyors)
		{
			shoppingListObject_Class->IncreaseAmount(conveyor->GetDismantleRefundReturnsMultiplier());
		}
	}
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
