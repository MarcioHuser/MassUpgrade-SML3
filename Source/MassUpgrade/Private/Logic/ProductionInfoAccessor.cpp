#include "Logic/ProductionInfoAccessor.h"

#include "FGCharacterPlayer.h"
#include "FGFactoryConnectionComponent.h"
#include "FGRecipeManager.h"
#include "Buildables/FGBuildableConveyorAttachment.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildableConveyorLift.h"
#include "Buildables/FGBuildableStorage.h"
#include "Resources/FGBuildDescriptor.h"
#include "FGGameState.h"
#include "FGPlayerController.h"
#include "Engine/Engine.h"
#include "Reflection/ReflectionHelper.h"
#include "ShoppingList/FGShoppingListObject.h"
#include "Subsystems/CommonInfoSubsystem.h"

#include <map>

#include "FGCircuitSubsystem.h"
#include "FGPipeConnectionComponent.h"
#include "FGPowerConnectionComponent.h"
#include "FGRailroadSubsystem.h"
#include "FGTrainStationIdentifier.h"
#include "Buildables/FGBuildableCircuitBridge.h"
#include "Buildables/FGBuildableDockingStation.h"
#include "Buildables/FGBuildablePipeBase.h"
#include "Buildables/FGBuildablePipeline.h"
#include "Buildables/FGBuildablePipelinePump.h"
#include "Buildables/FGBuildablePipeReservoir.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Buildables/FGBuildableRailroadStation.h"
#include "Buildables/FGBuildableTrainPlatformCargo.h"

#ifndef OPTIMIZE
#pragma optimize("", off)
#endif

void UProductionInfoAccessor::GetRefundableItems
(
	UObject* worldContext,
	TSubclassOf<UFGRecipe> newTypeRecipe,
	const FProductionInfo& productionInfo,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToRefund,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
	float& amount
)
{
	auto newType = newTypeRecipe ? AFGBuildable::GetBuildableClassFromRecipe(newTypeRecipe) : nullptr;
	auto newCost = newTypeRecipe ? UFGRecipe::GetIngredients(newTypeRecipe) : TArray<FItemAmount>();

	auto gameState = Cast<AFGGameState>(GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull)->GetGameState());

	for (const auto buildable : productionInfo.buildables)
	{
		int32 multiplier = 1;

		if (auto conveyor = Cast<AFGBuildableConveyorBase>(buildable))
		{
			amount += conveyor->GetLength() / 100;
			multiplier = buildable->GetDismantleRefundReturnsMultiplier();
		}
		else if (auto pipe = Cast<AFGBuildablePipeBase>(buildable))
		{
			amount += pipe->GetLength() / 100;
			multiplier = pipe->GetDismantleRefundReturnsMultiplier();
		}
		else
		{
			amount += 1;
		}

		auto buildableClass = buildable->GetClass();

		if (!gameState->GetCheatNoCost() && buildableClass != newType)
		{
			TArray<FInventoryStack> refunds;
			buildable->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToRefund.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}

			if (newType)
			{
				for (const auto& item : newCost)
				{
					upgradeCost.FindOrAdd(item.ItemClass) += multiplier * item.Amount;
				}
			}
		}
	}

	auto sortItemByName = [](const auto& x, const auto& y)
	{
		auto nameX = UFGItemDescriptor::GetItemName(x);
		auto nameY = UFGItemDescriptor::GetItemName(y);

		return nameX.CompareTo(nameY) < 0;
	};

	itemsToRefund.KeySort(sortItemByName);

	upgradeCost.KeySort(sortItemByName);
}

bool UProductionInfoAccessor::CanAffordUpgrade
(
	AFGCharacterPlayer* player,
	const TArray<TSubclassOf<UFGRecipe>>& targetRecipes,
	const TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
	const TArray<FProductionInfo>& infos
)
{
	if (!player)
	{
		return false;
	}

	auto playerInventory = player->GetInventory();

	auto gameState = Cast<AFGGameState>(player->GetWorld()->GetGameState());

	TSet<TSubclassOf<AActor>> newTypes;

	for (const auto& targetRecipe : targetRecipes)
	{
		if (!targetRecipe)
		{
			continue;
		}

		newTypes.Add(AFGBuildable::GetBuildableClassFromRecipe(targetRecipe));
	}

	if (!gameState->GetCheatNoCost())
	{
		// Must afford all to be true
		for (const auto& entry : upgradeCost)
		{
			if (playerInventory->GetNumItems(entry.Key) < entry.Value)
			{
				return false;
			}
		}
	}

	// Must refund something to be true
	for (const auto& info : infos)
	{
		auto buildClass = UFGBuildDescriptor::GetBuildClass(info.buildableType);

		if (newTypes.Contains(buildClass))
		{
			// Already of the target type. No upgrade will happen
			continue;
		}

		return true;
	}

	return false;
}

void UProductionInfoAccessor::CollectConveyorProductionInfo
(
	class AFGBuildable* targetBuildable,
	bool includeBelts,
	bool includeLifts,
	bool includeStorages,
	const TSet<TSubclassOf<class UFGBuildDescriptor>> selectedTypes,
	bool crossAttachmentsAndStorages,
	TArray<FProductionInfo>& infos
)
{
	auto commonInfoSubsystem = ACommonInfoSubsystem::Get();

	if (!GetValid(targetBuildable) || !targetBuildable->IsA(AFGBuildableConveyorBase::StaticClass()) && !commonInfoSubsystem->IsStorageContainer(targetBuildable))
	{
		return;
	}

	std::map<TSubclassOf<AFGBuildable>, FProductionInfo> infosMap;

	TSet<AFGBuildable*> seenBuildable;
	TSet<AFGBuildable*> pendingBuildable;

	seenBuildable.Add(targetBuildable);
	pendingBuildable.Add(targetBuildable);

	while (!pendingBuildable.IsEmpty())
	{
		auto buildable = *pendingBuildable.begin();
		pendingBuildable.Remove(buildable);

		auto& info = infosMap[buildable->GetClass()];
		info.buildableType = buildable->GetBuiltWithDescriptor<UFGBuildDescriptor>();

		if (info.buildables.Contains(buildable))
		{
			// Already processed. Skip to next buildable
			continue;
		}

		if (includeBelts && buildable->IsA(AFGBuildableConveyorBelt::StaticClass()) ||
			includeLifts && buildable->IsA(AFGBuildableConveyorLift::StaticClass()) ||
			includeStorages && commonInfoSubsystem->IsStorageContainer(buildable))
		{
			info.buildables.Add(buildable);
		}

		TSet<UFGFactoryConnectionComponent*> components;

		if (commonInfoSubsystem->IsStorageTeleporter(buildable))
		{
			handleStorageTeleporter(buildable, components);
		}
		else if (commonInfoSubsystem->IsModularLoadBalancer(buildable))
		{
			if (auto modularLoadBalancer = FReflectionHelper::GetObjectPropertyValue<AFGBuildableFactory>(buildable, TEXT("GroupLeader")))
			{
				handleModularLoadBalancerComponents(modularLoadBalancer, components);
			}
		}
		else if (commonInfoSubsystem->IsUndergroundSplitter(buildable))
		{
			if (auto undergroundBelt = Cast<AFGBuildableStorage>(buildable))
			{
				handleUndergroundBeltsComponents(undergroundBelt, components);
			}
		}
		else if (auto trainPlatformCargo = Cast<AFGBuildableTrainPlatformCargo>(buildable))
		{
			handleTrainPlatformCargoConveyor(trainPlatformCargo, components);
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
						otherBuildable->IsA(AFGBuildableTrainPlatformCargo::StaticClass()) ||
						otherBuildable->IsA(AFGBuildableStorage::StaticClass()) ||
						commonInfoSubsystem->IsStorageTeleporter(otherBuildable) ||
						commonInfoSubsystem->IsModularLoadBalancer(otherBuildable) ||
						commonInfoSubsystem->IsUndergroundSplitter(otherBuildable)
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
		if (entry.second.buildables.IsEmpty() || selectedTypes.Contains(entry.second.buildableType))
		{
			continue;
		}

		infos.Add(entry.second);
	}

	enum BuildableType
	{
		Unknown,
		Belt,
		Lift,
		Storage,
	};

	auto getBuildableType = [commonInfoSubsystem](const TSubclassOf<AActor>& buildableType)
	{
		if (buildableType->IsChildOf(AFGBuildableConveyorBelt::StaticClass()))
		{
			return BuildableType::Belt;
		}

		if (buildableType->IsChildOf(AFGBuildableConveyorLift::StaticClass()))
		{
			return BuildableType::Lift;
		}

		if (commonInfoSubsystem->IsStorageContainer(nullptr, buildableType))
		{
			return BuildableType::Storage;
		}

		return BuildableType::Unknown;
	};

	infos.Sort(
		[&getBuildableType](const FProductionInfo& buildable1, const FProductionInfo& buildable2)
		{
			auto buildClass1 = UFGBuildDescriptor::GetBuildClass(buildable1.buildableType);
			auto buildClass2 = UFGBuildDescriptor::GetBuildClass(buildable2.buildableType);

			auto type1 = getBuildableType(buildClass1);
			auto type2 = getBuildableType(buildClass2);

			float order = type1 - type2;

			if (order == 0)
			{
				if ((type1 == BuildableType::Belt || type1 == BuildableType::Lift) && (type2 == BuildableType::Belt || type2 == BuildableType::Lift))
				{
					order = buildClass1->GetDefaultObject<AFGBuildableConveyorBase>()->GetSpeed() - buildClass2->GetDefaultObject<AFGBuildableConveyorBase>()->GetSpeed();
				}

				if ((type1 == BuildableType::Storage) && (type2 == BuildableType::Storage))
				{
					auto storage1 = buildClass1->GetDefaultObject<AFGBuildableStorage>();
					auto storage2 = buildClass2->GetDefaultObject<AFGBuildableStorage>();

					order = storage1->mInventorySizeX * storage1->mInventorySizeY -
						storage2->mInventorySizeX * storage2->mInventorySizeY;
				}
			}

			return order < 0;
		}
		);
}

void UProductionInfoAccessor::CollectPipelineProductionInfo
(
	AFGBuildable* targetBuildable,
	bool includePipelines,
	bool includePumps,
	const TSet<TSubclassOf<class UFGBuildDescriptor>> selectedTypes,
	bool crossAttachmentsAndStorages,
	TArray<FProductionInfo>& infos
)
{
	if (!GetValid(targetBuildable) || !targetBuildable->IsA(AFGBuildablePipeline::StaticClass()) && !targetBuildable->IsA(AFGBuildablePipelinePump::StaticClass()))
	{
		return;
	}

	std::map<TSubclassOf<AFGBuildable>, FProductionInfo> infosMap;

	TSet<AFGBuildable*> seenBuildable;
	TSet<AFGBuildable*> pendingBuildable;

	seenBuildable.Add(targetBuildable);
	pendingBuildable.Add(targetBuildable);

	while (!pendingBuildable.IsEmpty())
	{
		auto buildable = *pendingBuildable.begin();
		pendingBuildable.Remove(buildable);

		auto& info = infosMap[buildable->GetClass()];
		info.buildableType = buildable->GetBuiltWithDescriptor<UFGBuildDescriptor>();

		if (info.buildables.Contains(buildable))
		{
			// Already processed. Skip to next buildable
			continue;
		}

		if (includePipelines)
		{
			if (auto pipeline = Cast<AFGBuildablePipeline>(buildable))
			{
				info.buildables.Add(buildable);
			}
		}
		if (includePumps)
		{
			if (auto pump = Cast<AFGBuildablePipelinePump>(buildable))
			{
				if (pump->GetDesignHeadLift() > 0)
				{
					info.buildables.Add(pump);
				}
			}
		}

		TSet<UFGPipeConnectionComponent*> components;

		if (auto trainPlatformCargo = Cast<AFGBuildableTrainPlatformCargo>(buildable))
		{
			handleTrainPlatformCargoPipeline(trainPlatformCargo, components);
		}
		else
		{
			TArray<UFGPipeConnectionComponent*> tempComponents;
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

			if (!otherBuildable->IsA(AFGBuildablePipeline::StaticClass()) &&
				!otherBuildable->IsA(AFGBuildablePassthrough::StaticClass()) &&
				!(crossAttachmentsAndStorages && (
						otherBuildable->IsA(AFGBuildableTrainPlatformCargo::StaticClass()) ||
						otherBuildable->IsA(AFGBuildablePipelinePump::StaticClass()) ||
						otherBuildable->IsA(AFGBuildablePipelineAttachment::StaticClass()) ||
						otherBuildable->IsA(AFGBuildablePipeReservoir::StaticClass())
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
		if (entry.second.buildables.IsEmpty() || selectedTypes.Contains(entry.second.buildableType))
		{
			continue;
		}

		infos.Add(entry.second);
	}

	enum BuildableType
	{
		Unknown,
		Pipeline,
		Pump,
	};

	auto getBuildableType = [](const TSubclassOf<AActor>& buildableType)
	{
		if (buildableType->IsChildOf(AFGBuildablePipeline::StaticClass()))
		{
			return BuildableType::Pipeline;
		}

		if (buildableType->IsChildOf(AFGBuildablePipelinePump::StaticClass()))
		{
			return BuildableType::Pump;
		}

		return BuildableType::Unknown;
	};

	infos.Sort(
		[&getBuildableType](const FProductionInfo& buildable1, const FProductionInfo& buildable2)
		{
			auto buildClass1 = UFGBuildDescriptor::GetBuildClass(buildable1.buildableType);
			auto buildClass2 = UFGBuildDescriptor::GetBuildClass(buildable2.buildableType);

			auto type1 = getBuildableType(buildClass1);
			auto type2 = getBuildableType(buildClass2);

			float order = type1 - type2;

			if (order == 0)
			{
				if (type1 == BuildableType::Pipeline && type2 == BuildableType::Pipeline)
				{
					auto pipeline1 = buildClass1->GetDefaultObject<AFGBuildablePipeline>();
					auto pipeline2 = buildClass2->GetDefaultObject<AFGBuildablePipeline>();

					order = pipeline1->GetFlowLimit() - pipeline2->GetFlowLimit();
				}

				if (type1 == BuildableType::Pump && type2 == BuildableType::Pump)
				{
					auto storage1 = buildClass1->GetDefaultObject<AFGBuildablePipelinePump>();
					auto storage2 = buildClass2->GetDefaultObject<AFGBuildablePipelinePump>();

					order = storage1->GetMaxHeadLift() - storage2->GetMaxHeadLift();
				}
			}

			return order < 0;
		}
		);
}

void UProductionInfoAccessor::CollectPowerPoleProductionInfo
(
	AFGBuildable* targetBuildable,
	bool includePowerPoles,
	bool includePowerPoleWalls,
	bool includePowerPoleWallDoubles,
	bool includePowerTowers,
	const TSet<TSubclassOf<UFGBuildDescriptor>> selectedTypes,
	bool crossAttachmentsAndStorages,
	TArray<FProductionInfo>& infos
)
{
	auto commonInfoSubsystem = ACommonInfoSubsystem::Get();

	if (!GetValid(targetBuildable) ||
		!commonInfoSubsystem->IsPowerPole(targetBuildable) &&
		!commonInfoSubsystem->IsPowerPoleWall(targetBuildable) &&
		!commonInfoSubsystem->IsPowerPoleWallDouble(targetBuildable) &&
		!commonInfoSubsystem->IsPowerTower(targetBuildable))
	{
		return;
	}

	std::map<TSubclassOf<AFGBuildable>, FProductionInfo> infosMap;

	TSet<AFGBuildable*> seenBuildable;
	TSet<AFGBuildable*> pendingBuildable;

	seenBuildable.Add(targetBuildable);
	pendingBuildable.Add(targetBuildable);

	while (!pendingBuildable.IsEmpty())
	{
		auto buildable = *pendingBuildable.begin();
		pendingBuildable.Remove(buildable);

		auto& info = infosMap[buildable->GetClass()];
		info.buildableType = buildable->GetBuiltWithDescriptor<UFGBuildDescriptor>();

		if (info.buildables.Contains(buildable))
		{
			// Already processed. Skip to next buildable
			continue;
		}

		if (includePowerPoles)
		{
			if (commonInfoSubsystem->IsPowerPole(buildable))
			{
				info.buildables.Add(buildable);
			}
		}
		if (includePowerPoleWalls)
		{
			if (commonInfoSubsystem->IsPowerPoleWall(buildable))
			{
				info.buildables.Add(buildable);
			}
		}
		if (includePowerPoleWallDoubles)
		{
			if (commonInfoSubsystem->IsPowerPoleWallDouble(buildable))
			{
				info.buildables.Add(buildable);
			}
		}
		if (includePowerTowers)
		{
			if (commonInfoSubsystem->IsPowerTower(buildable))
			{
				info.buildables.Add(buildable);
			}
		}

		TSet<UFGPowerConnectionComponent*> components;

		if (auto station = Cast<AFGBuildableRailroadStation>(buildable))
		{
			handleTrainPlatformCargoPowerPole(station, components);
		}
		else
		{
			TArray<UFGPowerConnectionComponent*> tempComponents;
			buildable->GetComponents(tempComponents);
			components.Append(tempComponents);
		}

		for (const auto& component : components)
		{
			TArray<UFGCircuitConnectionComponent*> connections;
			component->GetConnections(connections);

			for (const auto& connection : connections)
			{
				auto otherBuildable = Cast<AFGBuildable>(connection->GetOwner());
				if (!otherBuildable)
				{
					continue;
				}

				if (!crossAttachmentsAndStorages && (
					otherBuildable->IsA(AFGBuildableCircuitBridge::StaticClass()) ||
					otherBuildable->IsA(AFGBuildableRailroadStation::StaticClass())
				))
				{
					// Bridge crossing is not enabled
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
	}

	for (const auto& entry : infosMap)
	{
		if (entry.second.buildables.IsEmpty() || selectedTypes.Contains(entry.second.buildableType))
		{
			continue;
		}

		infos.Add(entry.second);
	}

	enum BuildableType
	{
		Unknown,
		PowerPole,
		PowerPoleWall,
		PowerPoleWallDouble,
		PowerTower,
	};

	auto getBuildableType = [commonInfoSubsystem](const TSubclassOf<AActor>& buildableType)
	{
		if (commonInfoSubsystem->IsPowerPole(nullptr, buildableType))
		{
			return BuildableType::PowerPole;
		}
		if (commonInfoSubsystem->IsPowerPoleWall(nullptr, buildableType))
		{
			return BuildableType::PowerPoleWall;
		}
		if (commonInfoSubsystem->IsPowerPoleWallDouble(nullptr, buildableType))
		{
			return BuildableType::PowerPoleWallDouble;
		}
		if (commonInfoSubsystem->IsPowerTower(nullptr, buildableType))
		{
			return BuildableType::PowerTower;
		}

		return BuildableType::Unknown;
	};

	infos.Sort(
		[&getBuildableType](const FProductionInfo& info1, const FProductionInfo& info2)
		{
			auto pole1 = Cast<AFGBuildablePowerPole>(*info1.buildables.begin());
			auto pole2 = Cast<AFGBuildablePowerPole>(*info2.buildables.begin());

			auto type1 = getBuildableType(pole1->GetClass());
			auto type2 = getBuildableType(pole2->GetClass());

			float order = type1 - type2;

			if (order == 0)
			{
				order = pole1->GetPowerConnection(0)->GetMaxNumConnections() - pole2->GetPowerConnection(0)->GetMaxNumConnections();
			}

			return order < 0;
		}
		);
}

void UProductionInfoAccessor::FilterInfos(TSubclassOf<AFGBuildable> baseType, TArray<FProductionInfo>& infos, TArray<FProductionInfo>& filteredInfos)
{
	for (const auto& info : infos)
	{
		auto buildClass = UFGBuildDescriptor::GetBuildClass(info.buildableType);

		if (!buildClass->IsChildOf(baseType))
		{
			continue;
		}

		filteredInfos.Add(info);
	}
}


void UProductionInfoAccessor::handleStorageTeleporter
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

void UProductionInfoAccessor::handleUndergroundBeltsComponents
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

	if (commonInfoSubsystem->IsUndergroundSplitterInput(undergroundBelt))
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
	else if (commonInfoSubsystem->IsUndergroundSplitterOutput(undergroundBelt))
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


void UProductionInfoAccessor::handleModularLoadBalancerComponents
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

void UProductionInfoAccessor::handleTrainPlatformCargoConveyor(AFGBuildableTrainPlatformCargo* trainPlatformCargo, TSet<UFGFactoryConnectionComponent*>& components)
{
	auto railroadSubsystem = AFGRailroadSubsystem::Get(trainPlatformCargo->GetWorld());

	auto trackId = trainPlatformCargo->GetTrackGraphID();

	TArray<AFGTrainStationIdentifier*> stations;
	railroadSubsystem->GetTrainStations(trackId, stations);

	for (auto stationIdentifier : stations)
	{
		auto station = stationIdentifier->GetStation();

		for (auto i = 0; i <= 1; i++)
		{
			TSet<AFGBuildableTrainPlatform*> seenPlatforms;

			for (auto connectedPlatform = station->GetConnectedPlatformInDirectionOf(i);
			     connectedPlatform;
			     connectedPlatform = connectedPlatform->GetConnectedPlatformInDirectionOf(i))
			{
				if (seenPlatforms.Contains(connectedPlatform))
				{
					// Loop detected
					break;
				}

				TArray<UFGFactoryConnectionComponent*> tempComponents;
				connectedPlatform->GetComponents(tempComponents);

				components.Append(tempComponents);
			}
		}
	}
}

void UProductionInfoAccessor::handleTrainPlatformCargoPipeline(AFGBuildableTrainPlatformCargo* trainPlatformCargo, TSet<UFGPipeConnectionComponent*>& components)
{
	auto railroadSubsystem = AFGRailroadSubsystem::Get(trainPlatformCargo->GetWorld());

	auto trackId = trainPlatformCargo->GetTrackGraphID();

	TArray<AFGTrainStationIdentifier*> stations;
	railroadSubsystem->GetTrainStations(trackId, stations);

	for (auto stationIdentifier : stations)
	{
		auto station = stationIdentifier->GetStation();

		for (auto i = 0; i <= 1; i++)
		{
			TSet<AFGBuildableTrainPlatform*> seenPlatforms;

			for (auto connectedPlatform = station->GetConnectedPlatformInDirectionOf(i);
			     connectedPlatform;
			     connectedPlatform = connectedPlatform->GetConnectedPlatformInDirectionOf(i))
			{
				if (seenPlatforms.Contains(connectedPlatform))
				{
					// Loop detected
					break;
				}

				TArray<UFGPipeConnectionComponent*> tempComponents;
				connectedPlatform->GetComponents(tempComponents);

				components.Append(tempComponents);
			}
		}
	}
}

void UProductionInfoAccessor::handleTrainPlatformCargoPowerPole(AFGBuildableRailroadStation* station, TSet<UFGPowerConnectionComponent*>& components)
{
	auto railroadSubsystem = AFGRailroadSubsystem::Get(station->GetWorld());

	auto trackId = station->GetTrackGraphID();

	TArray<AFGTrainStationIdentifier*> stations;
	railroadSubsystem->GetTrainStations(trackId, stations);

	for (auto stationIdentifier : stations)
	{
		station = stationIdentifier->GetStation();

		TArray<UFGPowerConnectionComponent*> tempComponents;
		station->GetComponents(tempComponents);

		components.Append(tempComponents);
	}
}

void UProductionInfoAccessor::AddInfosToShoppingList(AFGCharacterPlayer* player, const TArray<FProductionInfo>& infos, const TSubclassOf<UFGRecipe>& targetRecipe)
{
	if (!player || !targetRecipe)
	{
		return;
	}

	auto shoppingListComponent = UFGShoppingListComponent::GetShoppingListComponent(player->GetFGPlayerController());

	auto newType = targetRecipe ? AFGBuildable::GetBuildableClassFromRecipe(targetRecipe) : nullptr;

	for (const auto& info : infos)
	{
		if (info.buildables.IsEmpty())
		{
			continue;
		}

		auto buildClass = UFGBuildDescriptor::GetBuildClass(info.buildableType);

		if (newType == buildClass)
		{
			// Already of the target type or no item in list. No upgrade will happen
			continue;
		}

		bool out_result;
		auto shoppingListObject_Class = Cast<UFGShoppingListObject_Class>(shoppingListComponent->GetShoppingListObjectFromClass(targetRecipe, out_result));

		if (!out_result)
		{
			shoppingListObject_Class = NewObject<UFGShoppingListObject_Class>(shoppingListComponent);
			shoppingListObject_Class->SetSubClass(targetRecipe);
		}

		// FShoppingListRecipeEntry recipeEntry(recipe, 0);

		for (auto conveyor : info.buildables)
		{
			shoppingListObject_Class->IncreaseAmount(conveyor->GetDismantleRefundReturnsMultiplier());
		}
	}
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
