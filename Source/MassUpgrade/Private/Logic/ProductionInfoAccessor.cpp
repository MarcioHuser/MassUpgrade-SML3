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

#include "FGCentralStorageSubsystem.h"
#include "FGCheatManager.h"
#include "FGCircuitSubsystem.h"
#include "FGPipeConnectionComponent.h"
#include "FGPowerConnectionComponent.h"
#include "FGRailroadSubsystem.h"
#include "FGRailroadTimeTable.h"
#include "FGTrain.h"
#include "FGTrainPlatformConnection.h"
#include "FGTrainStationIdentifier.h"
#include "MassUpgradeEquipment.h"
#include "MassUpgradeRCO.h"
#include "Buildables/FGBuildableCircuitBridge.h"
#include "Buildables/FGBuildableDockingStation.h"
#include "Buildables/FGBuildablePipeBase.h"
#include "Buildables/FGBuildablePipeline.h"
#include "Buildables/FGBuildablePipelinePump.h"
#include "Buildables/FGBuildablePipeReservoir.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Buildables/FGBuildableRailroadStation.h"
#include "Buildables/FGBuildableTrainPlatformCargo.h"
#include "Buildables/FGBuildableWire.h"
#include "Components/CheckBox.h"
#include "Util/MarcioCommonLibsUtils.h"
#include "Util/MULogging.h"
#include "Widget/WidgetMassUpgradePopupHelper.h"

#include "Util/MUOptimize.h"

#ifndef OPTIMIZE
#pragma optimize("", off)
#endif

void UProductionInfoAccessor::GetRefundableItems
(
	class AFGCharacterPlayer* player,
	TSubclassOf<UFGRecipe> newTypeRecipe,
	const FProductionInfo& productionInfo,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToRefund,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& upgradeCost,
	float& amount
)
{
	auto newType = newTypeRecipe ? AFGBuildable::GetBuildableClassFromRecipe(newTypeRecipe) : nullptr;
	auto newCost = newTypeRecipe ? UFGRecipe::GetIngredients(newTypeRecipe) : TArray<FItemAmount>();

	// auto cheatManager = Cast<AFGPlayerController>(player->GetController())->GetCheatManager();
	//
	// auto gameState = Cast<AFGGameState>(player->GetWorld()->GetGameState());

	auto noCost = player->GetInventory()->GetNoBuildCost();

	for (const auto buildable : productionInfo.buildables)
	{
		if (!buildable)
		{
			continue;
		}

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
		else if (auto wire = Cast<AFGBuildableWire>(buildable))
		{
			amount += wire->GetLength() / 100;
			multiplier = wire->GetDismantleRefundReturnsMultiplier();
		}
		else
		{
			amount += 1;
		}

		auto buildableClass = buildable->GetClass();

		if (!noCost && buildableClass != newType)
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

	// auto cheatManager = Cast<AFGPlayerController>(player->GetController())->GetCheatManager();
	//
	// auto gameState = Cast<AFGGameState>(player->GetWorld()->GetGameState());

	auto noCost = player->GetInventory()->GetNoBuildCost();

	TSet<TSubclassOf<AActor>> newTypes;

	for (const auto& targetRecipe : targetRecipes)
	{
		if (!targetRecipe)
		{
			continue;
		}

		newTypes.Add(AFGBuildable::GetBuildableClassFromRecipe(targetRecipe));
	}

	if (!noCost)
	{
		auto centralStorageSubsystem = AFGCentralStorageSubsystem::Get(player->GetWorld());

		// Must afford all to be true
		for (const auto& entry : upgradeCost)
		{
			if (playerInventory->GetNumItems(entry.Key) + centralStorageSubsystem->GetNumItemsFromCentralStorage(entry.Key) < entry.Value)
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
	AMassUpgradeEquipment* massUpgradeEquipment,
	class AFGBuildable* targetBuildable,
	bool includeBelts,
	bool includeLifts,
	bool includeStorages,
	const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	if (!GetValid(targetBuildable))
	{
		return;
	}

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(targetBuildable->GetWorld());

	if (!targetBuildable->IsA(AFGBuildableConveyorBase::StaticClass()) && !commonInfoSubsystem->IsStorageContainer(targetBuildable))
	{
		return;
	}

	if (targetBuildable->HasAuthority())
	{
		CollectConveyorProductionInfo_Server(
			massUpgradeEquipment,
			targetBuildable,
			includeBelts,
			includeLifts,
			includeStorages,
			selectedTypes,
			crossAttachmentsAndStorages,
			collectProductionInfoIntent
			);
	}
	else
	{
		UMassUpgradeRCO::getRCO(targetBuildable->GetWorld())->CollectConveyorProductionInfo(
			massUpgradeEquipment,
			targetBuildable,
			includeBelts,
			includeLifts,
			includeStorages,
			selectedTypes.Array(),
			crossAttachmentsAndStorages,
			collectProductionInfoIntent
			);
	}
}

void UProductionInfoAccessor::CollectConveyorProductionInfo_Server
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	class AFGBuildable* targetBuildable,
	bool includeBelts,
	bool includeLifts,
	bool includeStorages,
	const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	TArray<FProductionInfo> infos;

	if (!GetValid(targetBuildable) || !targetBuildable->HasAuthority())
	{
		massUpgradeEquipment->SetProductionInfos(ToProductionInfoWithArray(infos), collectProductionInfoIntent);

		return;
	}

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(targetBuildable->GetWorld());

	if (!targetBuildable->IsA(AFGBuildableConveyorBase::StaticClass()) && !commonInfoSubsystem->IsStorageContainer(targetBuildable))
	{
		massUpgradeEquipment->SetProductionInfos(ToProductionInfoWithArray(infos), collectProductionInfoIntent);

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

		if ((includeBelts && buildable->IsA(AFGBuildableConveyorBelt::StaticClass())) ||
			(includeLifts && buildable->IsA(AFGBuildableConveyorLift::StaticClass())) ||
			(includeStorages && commonInfoSubsystem->IsStorageContainer(buildable)))
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
		else if (auto conveyor = Cast<AFGBuildableConveyorBase>(buildable))
		{
			if (conveyor->GetConnection0() && conveyor->GetConnection0()->GetConnection())
			{
				components.Add(conveyor->GetConnection0());
			}
			if (conveyor->GetConnection1() && conveyor->GetConnection1()->GetConnection())
			{
				components.Add(conveyor->GetConnection1());
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

			if (!component->GetConnection())
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
						commonInfoSubsystem->IsUndergroundSplitter(otherBuildable) ||
						commonInfoSubsystem->IsCounterLimiter(otherBuildable)
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

	massUpgradeEquipment->SetProductionInfos(ToProductionInfoWithArray(infos), collectProductionInfoIntent);
}

void UProductionInfoAccessor::CollectPipelineProductionInfo
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includePipelines,
	bool includePumps,
	const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	if (!GetValid(targetBuildable) ||
		(!targetBuildable->IsA(AFGBuildablePipeline::StaticClass()) && !targetBuildable->IsA(AFGBuildablePipelinePump::StaticClass())))
	{
		return;
	}

	if (targetBuildable->HasAuthority())
	{
		CollectPipelineProductionInfo_Server(
			massUpgradeEquipment,
			targetBuildable,
			includePipelines,
			includePumps,
			selectedTypes,
			crossAttachmentsAndStorages,
			collectProductionInfoIntent
			);
	}
	else
	{
		UMassUpgradeRCO::getRCO(targetBuildable->GetWorld())->CollectPipelineProductionInfo(
			massUpgradeEquipment,
			targetBuildable,
			includePipelines,
			includePumps,
			selectedTypes.Array(),
			crossAttachmentsAndStorages,
			collectProductionInfoIntent
			);
	}
}

void UProductionInfoAccessor::CollectPipelineProductionInfo_Server
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includePipelines,
	bool includePumps,
	const TSet<TSubclassOf<class UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	TArray<FProductionInfo> infos;

	if (!GetValid(targetBuildable) ||
		!targetBuildable->HasAuthority() ||
		(!targetBuildable->IsA(AFGBuildablePipeline::StaticClass()) && !targetBuildable->IsA(AFGBuildablePipelinePump::StaticClass())))
	{
		massUpgradeEquipment->SetProductionInfos(ToProductionInfoWithArray(infos), collectProductionInfoIntent);

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
				info.buildables.Add(pipeline);
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

			if (!component->GetConnection())
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

	massUpgradeEquipment->SetProductionInfos(ToProductionInfoWithArray(infos), collectProductionInfoIntent);
}

void UProductionInfoAccessor::CollectPowerPoleProductionInfo
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includeWires,
	bool includePowerPoles,
	bool includePowerPoleWalls,
	bool includePowerPoleWallDoubles,
	bool includePowerTowers,
	const TSet<TSubclassOf<UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	if (!GetValid(targetBuildable))
	{
		return;
	}

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(targetBuildable->GetWorld());

	if (!commonInfoSubsystem->IsPowerPole(targetBuildable) &&
		!commonInfoSubsystem->IsPowerPoleWall(targetBuildable) &&
		!commonInfoSubsystem->IsPowerPoleWallDouble(targetBuildable) &&
		!commonInfoSubsystem->IsPowerTower(targetBuildable))
	{
		return;
	}

	if (targetBuildable->HasAuthority())
	{
		CollectPowerPoleProductionInfo_Server(
			massUpgradeEquipment,
			targetBuildable,
			includeWires,
			includePowerPoles,
			includePowerPoleWalls,
			includePowerPoleWallDoubles,
			includePowerTowers,
			selectedTypes,
			crossAttachmentsAndStorages,
			collectProductionInfoIntent
			);
	}
	else
	{
		UMassUpgradeRCO::getRCO(targetBuildable->GetWorld())->CollectPowerPoleProductionInfo(
			massUpgradeEquipment,
			targetBuildable,
			includeWires,
			includePowerPoles,
			includePowerPoleWalls,
			includePowerPoleWallDoubles,
			includePowerTowers,
			selectedTypes.Array(),
			crossAttachmentsAndStorages,
			collectProductionInfoIntent
			);
	}
}

void UProductionInfoAccessor::CollectPowerPoleProductionInfo_Server
(
	AMassUpgradeEquipment* massUpgradeEquipment,
	AFGBuildable* targetBuildable,
	bool includeWires,
	bool includePowerPoles,
	bool includePowerPoleWalls,
	bool includePowerPoleWallDoubles,
	bool includePowerTowers,
	const TSet<TSubclassOf<UFGBuildDescriptor>>& selectedTypes,
	bool crossAttachmentsAndStorages,
	CollectProductionInfoIntent collectProductionInfoIntent
)
{
	TArray<FProductionInfo> infos;

	if (!GetValid(targetBuildable) || !targetBuildable->HasAuthority())
	{
		massUpgradeEquipment->SetProductionInfos(ToProductionInfoWithArray(infos), collectProductionInfoIntent);

		return;
	}

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(targetBuildable->GetWorld());

	if (!commonInfoSubsystem->IsPowerPole(targetBuildable) &&
		!commonInfoSubsystem->IsPowerPoleWall(targetBuildable) &&
		!commonInfoSubsystem->IsPowerPoleWallDouble(targetBuildable) &&
		!commonInfoSubsystem->IsPowerTower(targetBuildable))
	{
		massUpgradeEquipment->SetProductionInfos(ToProductionInfoWithArray(infos), collectProductionInfoIntent);

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
			handleTrainStationPowerPole(station, components);
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

			if (includeWires)
			{
				TArray<AFGBuildableWire*> wires;
				component->GetWires(wires);
				for (auto wire : wires)
				{
					auto& infoWire = infosMap[wire->GetClass()];
					infoWire.buildableType = wire->GetBuiltWithDescriptor<UFGBuildDescriptor>();
					infoWire.buildables.Add(wire);
				}
			}
		}
	}

	for (const auto& entry : infosMap)
	{
		if (!entry.second.buildableType ||
			entry.second.buildables.IsEmpty() ||
			selectedTypes.Contains(entry.second.buildableType))
		{
			continue;
		}

		infos.Add(entry.second);
	}

	enum BuildableType
	{
		Unknown,
		Wire,
		PowerPole,
		PowerPoleWall,
		PowerPoleWallDouble,
		PowerTower,
	};

	auto getBuildableType = [commonInfoSubsystem](const TSubclassOf<AActor>& buildableType)
	{
		if (buildableType->IsChildOf(AFGBuildableWire::StaticClass()))
		{
			return BuildableType::Wire;
		}
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
			auto buildable1 = Cast<AFGBuildable>(*info1.buildables.begin());
			auto buildable2 = Cast<AFGBuildable>(*info2.buildables.begin());

			auto type1 = getBuildableType(buildable1->GetClass());
			auto type2 = getBuildableType(buildable2->GetClass());

			float order = type1 - type2;

			if (order == 0)
			{
				auto pole1 = Cast<AFGBuildablePowerPole>(buildable1);
				auto pole2 = Cast<AFGBuildablePowerPole>(buildable2);

				if (pole1 && pole2)
				{
					order = pole1->GetPowerConnection(0)->GetMaxNumConnections() - pole2->GetPowerConnection(0)->GetMaxNumConnections();
				}
			}

			if (order == 0)
			{
				order = buildable1->GetClass()->GetName().Compare(buildable2->GetClass()->GetName());
			}

			return order < 0;
		}
		);

	massUpgradeEquipment->SetProductionInfos(ToProductionInfoWithArray(infos), collectProductionInfoIntent);
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

void UProductionInfoAccessor::FilterSelectedInfos
(
	class UWidget* container,
	TArray<FProductionInfo>& infos,
	TArray<FProductionInfo>& filteredInfos
)
{
	auto widgetTree = Cast<UWidgetTree>(container->GetOuter());

	for (const auto& info : infos)
	{
		auto checkbox = widgetTree->FindWidget<UCheckBox>(UWidgetMassUpgradePopupHelper::GetCheckboxName(info));

		if (checkbox && !checkbox->IsChecked())
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

	auto CommonInfoSubsystem = ACommonInfoSubsystem::Get(storageTeleporter->GetWorld());

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

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get(undergroundBelt->GetWorld());

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
	{
		TArray<UFGFactoryConnectionComponent*> tempComponents;
		trainPlatformCargo->GetComponents(tempComponents);

		components.Append(tempComponents);
	}
	
	auto railroadSubsystem = AFGRailroadSubsystem::Get(trainPlatformCargo->GetWorld());

	auto trackId = trainPlatformCargo->GetTrackGraphID();

	// Determine offsets from all the connected stations
	TSet<int> stationOffsets;
	TSet<AFGBuildableRailroadStation*> destinationStations;

	UMarcioCommonLibsUtils::getTrainPlatformIndexes(trainPlatformCargo, stationOffsets, destinationStations);

	TArray<AFGTrain*> trains;
	railroadSubsystem->GetTrains(trackId, trains);

	for (auto train : trains)
	{
		if (!train->HasTimeTable())
		{
			continue;
		}

		// Get train stations
		auto timeTable = train->GetTimeTable();
		
		TArray<FTimeTableStop> stops;
		timeTable->GetStops(stops);
		
		bool stopAtStations = false;

		for (const auto& stop : stops)
		{
			if (!stop.Station || !stop.Station->GetStation() || !destinationStations.Contains(stop.Station->GetStation()))
			{
				continue;
			}

			stopAtStations = true;

			break;
		}

		if (!stopAtStations)
		{
			continue;
		}
		
		for (const auto& stop : stops)
		{
			if (!stop.Station || !stop.Station->GetStation())
			{
				continue;
			}

			MU_LOG_Display_Condition(
				TEXT("    Stop = "),
				*stop.Station->GetStationName().ToString()
				);

			for (auto index : stationOffsets)
			{
				auto platform = UMarcioCommonLibsUtils::getNthTrainPlatform(stop.Station->GetStation(), index);

				auto stopCargo = Cast<AFGBuildableTrainPlatformCargo>(platform);
				if (!stopCargo || stopCargo == trainPlatformCargo)
				{
					// Not a cargo or the same as the current one. Skip
					continue;
				}

				TArray<UFGFactoryConnectionComponent*> tempComponents;
				platform->GetComponents(tempComponents);

				components.Append(tempComponents);
			}
		}
	}
}

void UProductionInfoAccessor::handleTrainPlatformCargoPipeline(AFGBuildableTrainPlatformCargo* trainPlatformCargo, TSet<UFGPipeConnectionComponent*>& components)
{
	{
		TArray<UFGPipeConnectionComponent*> tempComponents;
		trainPlatformCargo->GetComponents(tempComponents);

		components.Append(tempComponents);
	}
	
	auto railroadSubsystem = AFGRailroadSubsystem::Get(trainPlatformCargo->GetWorld());

	auto trackId = trainPlatformCargo->GetTrackGraphID();

	// Determine offsets from all the connected stations
	TSet<int> stationOffsets;
	TSet<AFGBuildableRailroadStation*> destinationStations;

	UMarcioCommonLibsUtils::getTrainPlatformIndexes(trainPlatformCargo, stationOffsets, destinationStations);

	TArray<AFGTrain*> trains;
	railroadSubsystem->GetTrains(trackId, trains);

	for (auto train : trains)
	{
		if (!train->HasTimeTable())
		{
			continue;
		}

		// Get train stations
		auto timeTable = train->GetTimeTable();
		
		TArray<FTimeTableStop> stops;
		timeTable->GetStops(stops);
		
		bool stopAtStations = false;

		for (const auto& stop : stops)
		{
			if (!stop.Station || !stop.Station->GetStation() || !destinationStations.Contains(stop.Station->GetStation()))
			{
				continue;
			}

			stopAtStations = true;

			break;
		}

		if (!stopAtStations)
		{
			continue;
		}
		
		for (const auto& stop : stops)
		{
			if (!stop.Station || !stop.Station->GetStation())
			{
				continue;
			}

			MU_LOG_Display_Condition(
				TEXT("    Stop = "),
				*stop.Station->GetStationName().ToString()
				);

			for (auto index : stationOffsets)
			{
				auto platform = UMarcioCommonLibsUtils::getNthTrainPlatform(stop.Station->GetStation(), index);

				auto stopCargo = Cast<AFGBuildableTrainPlatformCargo>(platform);
				if (!stopCargo || stopCargo == trainPlatformCargo)
				{
					// Not a cargo or the same as the current one. Skip
					continue;
				}

				TArray<UFGPipeConnectionComponent*> tempComponents;
				platform->GetComponents(tempComponents);

				components.Append(tempComponents);
			}
		}
	}
}

void UProductionInfoAccessor::handleTrainStationPowerPole(AFGBuildableRailroadStation* station, TSet<UFGPowerConnectionComponent*>& components)
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

TArray<FProductionInfoWithArray> UProductionInfoAccessor::ToProductionInfoWithArray(const TArray<FProductionInfo>& infos)
{
	TArray<FProductionInfoWithArray> infos2;

	for (const auto& info : infos)
	{
		infos2.Add(
			FProductionInfoWithArray{
				info.buildableType,
				info.buildables.Array()
			}
			);
	}

	return infos2;
}

TArray<FProductionInfo> UProductionInfoAccessor::ToProductionInfo(const TArray<FProductionInfoWithArray>& infos)
{
	TArray<FProductionInfo> infos2;

	for (const auto& info : infos)
	{
		infos2.Add(
			FProductionInfo{
				info.buildableType,
				TSet<AFGBuildable*>(info.buildables)
			}
			);
	}

	return infos2;
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

		for (auto buildable : info.buildables)
		{
			if (!buildable)
			{
				continue;
			}
			
			shoppingListObject_Class->IncreaseAmount(buildable->GetDismantleRefundReturnsMultiplier());
		}
	}
}

#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
