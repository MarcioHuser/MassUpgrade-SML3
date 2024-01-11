#include "Logic/MassUpgradeLogic.h"

#include <map>

#include "FGCharacterPlayer.h"
#include "FGCircuitSubsystem.h"
#include "FGCrate.h"
#include "FGItemPickup_Spawnable.h"
#include "FGFactoryConnectionComponent.h"
#include "FGGameState.h"
#include "FGPipeConnectionComponent.h"
#include "FGPlayerController.h"
#include "FGPowerCircuit.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
#include "MassUpgradeRCO.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildablePipeline.h"
#include "Buildables/FGBuildablePipelinePump.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Buildables/FGBuildableStorage.h"
#include "Buildables/FGBuildableWire.h"
#include "Hologram/FGHologram.h"
#include "Model/ProductionInfo.h"
#include "Resources/FGBuildDescriptor.h"
#include "ShoppingList/FGShoppingListObject.h"
#include "Subsystems/CommonInfoSubsystem.h"
#include "Util/MULogging.h"
#include "Util/MarcioCommonLibsUtils.h"

#ifndef OPTIMIZE
#pragma optimize("", off)
#endif

void UMassUpgradeLogic::UpgradeConveyors
(
	AFGCharacterPlayer* player,
	const TSubclassOf<UFGRecipe>& newBeltTypeRecipe,
	const TSubclassOf<UFGRecipe>& newLiftTypeRecipe,
	const TSubclassOf<UFGRecipe>& newStorageTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	if (!player)
	{
		return;
	}

	if (player->HasAuthority())
	{
		UpgradeConveyors_Server(
			player,
			newBeltTypeRecipe,
			newLiftTypeRecipe,
			newStorageTypeRecipe,
			infos
			);
	}
	else
	{
		UMassUpgradeRCO::getRCO(player->GetWorld())->UpgradeConveyors(
			player,
			newBeltTypeRecipe,
			newLiftTypeRecipe,
			newStorageTypeRecipe,
			infos
			);
	}
}

void UMassUpgradeLogic::UpgradeConveyors_Server
(
	AFGCharacterPlayer* player,
	const TSubclassOf<UFGRecipe>& newBeltTypeRecipe,
	const TSubclassOf<UFGRecipe>& newLiftTypeRecipe,
	const TSubclassOf<UFGRecipe>& newStorageTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	if (!player)
	{
		return;
	}

	if (!player->HasAuthority())
	{
		return;
	}

	if (!IsInGameThread())
	{
		MU_LOG_Display(TEXT("Schedulling UpgradeConveyors to main thread"));

		AsyncTask(
			ENamedThreads::GameThread,
			[=]()
			{
				UpgradeConveyors_Server(
					player,
					newBeltTypeRecipe,
					newLiftTypeRecipe,
					newStorageTypeRecipe,
					infos
					);
			}
			);

		return;
	}

	std::map<TSubclassOf<UFGRecipe>, int32> recipeAmountMap;

	TArray<AFGBuildableConveyorBase*> belts;
	TArray<AFGBuildableConveyorBase*> lifts;
	TArray<AFGBuildableStorage*> storages;

	for (const auto& info : infos)
	{
		for (auto buildable : info.buildables)
		{
			if (auto belt = Cast<AFGBuildableConveyorBelt>(buildable))
			{
				belts.Add(belt);
			}
			else if (auto lift = Cast<AFGBuildableConveyorLift>(buildable))
			{
				lifts.Add(lift);
			}
			else if (auto storage = Cast<AFGBuildableStorage>(buildable))
			{
				storages.Add(storage);
			}
		}
	}

	TMap<TSubclassOf<UFGItemDescriptor>, int32> itemsToAddOrRemoveFromInventory;

	if (newBeltTypeRecipe && !belts.IsEmpty())
	{
		recipeAmountMap[newBeltTypeRecipe] = UpgradeConveyor(player, belts, newBeltTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	if (newLiftTypeRecipe && !lifts.IsEmpty())
	{
		recipeAmountMap[newLiftTypeRecipe] = UpgradeConveyor(player, lifts, newLiftTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	if (newStorageTypeRecipe && !storages.IsEmpty())
	{
		recipeAmountMap[newStorageTypeRecipe] = UpgradeStorage(player, storages, newStorageTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	TMap<TSubclassOf<UFGItemDescriptor>, int32> excessToDrop;
	auto playerInventory = player->GetInventory();

	for (const auto& entry : itemsToAddOrRemoveFromInventory)
	{
		if (entry.Value > 0)
		{
			// Add to inventory
			auto itemsAdded = playerInventory->AddStack(FInventoryStack(entry.Value, entry.Key), true);

			if (itemsAdded < entry.Value)
			{
				// No more room. Add to the drop crate
				excessToDrop.FindOrAdd(entry.Key) += entry.Value - itemsAdded;
			}
		}
		else if (entry.Value < 0)
		{
			playerInventory->Remove(entry.Key, -entry.Value);
		}
	}

	if (!excessToDrop.IsEmpty())
	{
		TArray<FInventoryStack> stacks;

		for (const auto& entry : excessToDrop)
		{
			stacks.Add(
				FInventoryStack(
					entry.Value,
					entry.Key
					)
				);
		}

		FHitResult out_hitResult;
		float out_waterDepth = 0;

		if (!player->TraceForGround(
			player->GetActorLocation(),
			player->GetActorLocation() + FVector(0, 0, -60000),
			out_hitResult,
			out_waterDepth
			))
		{
			// Place it whenever the player is, even if flying over the ground
			out_hitResult.Location = player->GetActorLocation();
		}

		AFGCrate* out_Crate = nullptr;

		AFGItemPickup_Spawnable::SpawnInventoryCrate(
			player->GetWorld(),
			stacks,
			out_hitResult.Location + FVector(0, 0, out_waterDepth),
			// Place at water level, if any
			TArray<class AActor*>(),
			out_Crate,
			nullptr
			);

		UMarcioCommonLibsUtils::DumpUnknownClass(out_Crate);
	}

	auto shoppingListComponent = UFGShoppingListComponent::GetShoppingListComponent(player->GetFGPlayerController());

	for (const auto& entry : recipeAmountMap)
	{
		bool out_result;
		auto shoppingListObject_Class = Cast<UFGShoppingListObject_Class>(shoppingListComponent->GetShoppingListObjectFromClass(entry.first, out_result));

		if (out_result && shoppingListObject_Class->GetAmount() > 0)
		{
			auto amount = FMath::Min(shoppingListObject_Class->GetAmount(), entry.second);

			shoppingListObject_Class->DecreaseAmount(amount);
		}
	}
}

void UMassUpgradeLogic::UpgradePipelines
(
	AFGCharacterPlayer* player,
	const TSubclassOf<UFGRecipe>& newPipelineTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPumpTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	if (!player)
	{
		return;
	}

	if (player->HasAuthority())
	{
		UpgradePipelines_Server(
			player,
			newPipelineTypeRecipe,
			newPumpTypeRecipe,
			infos
			);
	}
	else
	{
		UMassUpgradeRCO::getRCO(player->GetWorld())->UpgradePipelines(
			player,
			newPipelineTypeRecipe,
			newPumpTypeRecipe,
			infos
			);
	}
}

void UMassUpgradeLogic::UpgradePipelines_Server
(
	AFGCharacterPlayer* player,
	const TSubclassOf<UFGRecipe>& newPipelineTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPumpTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	if (!player)
	{
		return;
	}

	if (!player->HasAuthority())
	{
		return;
	}

	if (!IsInGameThread())
	{
		MU_LOG_Display(TEXT("Schedulling UpgradeConveyors to main thread"));

		AsyncTask(
			ENamedThreads::GameThread,
			[=]()
			{
				UpgradePipelines_Server(
					player,
					newPipelineTypeRecipe,
					newPumpTypeRecipe,
					infos
					);
			}
			);

		return;
	}

	std::map<TSubclassOf<UFGRecipe>, int32> recipeAmountMap;

	TArray<AFGBuildablePipeline*> pipelines;
	TArray<AFGBuildablePipelinePump*> pumps;

	for (const auto& info : infos)
	{
		for (auto buildable : info.buildables)
		{
			if (auto pipeline = Cast<AFGBuildablePipeline>(buildable))
			{
				pipelines.Add(pipeline);
			}
			else if (auto pump = Cast<AFGBuildablePipelinePump>(buildable))
			{
				pumps.Add(pump);
			}
		}
	}

	TMap<TSubclassOf<UFGItemDescriptor>, int32> itemsToAddOrRemoveFromInventory;

	if (newPipelineTypeRecipe && !pipelines.IsEmpty())
	{
		recipeAmountMap[newPipelineTypeRecipe] = UpgradePipeline(player, pipelines, newPipelineTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	if (newPumpTypeRecipe && !pumps.IsEmpty())
	{
		recipeAmountMap[newPumpTypeRecipe] = UpgradePump(player, pumps, newPumpTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	TMap<TSubclassOf<UFGItemDescriptor>, int32> excessToDrop;
	auto playerInventory = player->GetInventory();

	for (const auto& entry : itemsToAddOrRemoveFromInventory)
	{
		if (entry.Value > 0)
		{
			// Add to inventory
			auto itemsAdded = playerInventory->AddStack(FInventoryStack(entry.Value, entry.Key), true);

			if (itemsAdded < entry.Value)
			{
				// No more room. Add to the drop crate
				excessToDrop.FindOrAdd(entry.Key) += entry.Value - itemsAdded;
			}
		}
		else if (entry.Value < 0)
		{
			playerInventory->Remove(entry.Key, -entry.Value);
		}
	}

	if (!excessToDrop.IsEmpty())
	{
		TArray<FInventoryStack> stacks;

		for (const auto& entry : excessToDrop)
		{
			stacks.Add(
				FInventoryStack(
					entry.Value,
					entry.Key
					)
				);
		}

		FHitResult out_hitResult;
		float out_waterDepth = 0;

		if (!player->TraceForGround(
			player->GetActorLocation(),
			player->GetActorLocation() + FVector(0, 0, -60000),
			out_hitResult,
			out_waterDepth
			))
		{
			// Place it whenever the player is, even if flying over the ground
			out_hitResult.Location = player->GetActorLocation();
		}

		AFGCrate* out_Crate = nullptr;

		AFGItemPickup_Spawnable::SpawnInventoryCrate(
			player->GetWorld(),
			stacks,
			out_hitResult.Location + FVector(0, 0, out_waterDepth),
			// Place at water level, if any
			TArray<class AActor*>(),
			out_Crate,
			nullptr
			);

		UMarcioCommonLibsUtils::DumpUnknownClass(out_Crate);
	}

	auto shoppingListComponent = UFGShoppingListComponent::GetShoppingListComponent(player->GetFGPlayerController());

	for (const auto& entry : recipeAmountMap)
	{
		bool out_result;
		auto shoppingListObject_Class = Cast<UFGShoppingListObject_Class>(shoppingListComponent->GetShoppingListObjectFromClass(entry.first, out_result));

		if (out_result && shoppingListObject_Class->GetAmount() > 0)
		{
			auto amount = FMath::Min(shoppingListObject_Class->GetAmount(), entry.second);

			shoppingListObject_Class->DecreaseAmount(amount);
		}
	}
}

void UMassUpgradeLogic::UpgradePowerPoles
(
	AFGCharacterPlayer* player,
	const TSubclassOf<UFGRecipe>& newWireTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPowerPoleTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPowerPoleWallTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPowerPoleWallDoubleTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPowerTowerTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	if (!player)
	{
		return;
	}

	if (player->HasAuthority())
	{
		UpgradePowerPoles_Server(
			player,
			newWireTypeRecipe,
			newPowerPoleTypeRecipe,
			newPowerPoleWallTypeRecipe,
			newPowerPoleWallDoubleTypeRecipe,
			newPowerTowerTypeRecipe,
			infos
			);
	}
	else
	{
		UMassUpgradeRCO::getRCO(player->GetWorld())->UpgradePowerPoles(
			player,
			newWireTypeRecipe,
			newPowerPoleTypeRecipe,
			newPowerPoleWallTypeRecipe,
			newPowerPoleWallDoubleTypeRecipe,
			newPowerTowerTypeRecipe,
			infos
			);
	}
}

void UMassUpgradeLogic::UpgradePowerPoles_Server
(
	AFGCharacterPlayer* player,
	const TSubclassOf<UFGRecipe>& newWireTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPowerPoleTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPowerPoleWallTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPowerPoleWallDoubleTypeRecipe,
	const TSubclassOf<UFGRecipe>& newPowerTowerTypeRecipe,
	const TArray<FProductionInfo>& infos
)
{
	if (!player)
	{
		return;
	}

	if (!player->HasAuthority())
	{
		return;
	}

	if (!IsInGameThread())
	{
		MU_LOG_Display(TEXT("Schedulling UpgradeConveyors to main thread"));

		AsyncTask(
			ENamedThreads::GameThread,
			[=]()
			{
				UpgradePowerPoles_Server(
					player,
					newWireTypeRecipe,
					newPowerPoleTypeRecipe,
					newPowerPoleWallTypeRecipe,
					newPowerPoleWallDoubleTypeRecipe,
					newPowerTowerTypeRecipe,
					infos
					);
			}
			);

		return;
	}

	auto commonInfoSubsystem = ACommonInfoSubsystem::Get();

	std::map<TSubclassOf<UFGRecipe>, int32> recipeAmountMap;

	TArray<AFGBuildableWire*> wires;
	TArray<AFGBuildablePowerPole*> powerPoles;
	TArray<AFGBuildablePowerPole*> powerPoleWalls;
	TArray<AFGBuildablePowerPole*> powerPoleWallDoubles;
	TArray<AFGBuildablePowerPole*> powerTowers;

	for (const auto& info : infos)
	{
		for (auto buildable : info.buildables)
		{
			if (buildable->IsA(AFGBuildableWire::StaticClass()))
			{
				wires.Add(Cast<AFGBuildableWire>(buildable));
			}
			else if (commonInfoSubsystem->IsPowerPole(buildable))
			{
				powerPoles.Add(Cast<AFGBuildablePowerPole>(buildable));
			}
			else if (commonInfoSubsystem->IsPowerPoleWall(buildable))
			{
				powerPoleWalls.Add(Cast<AFGBuildablePowerPole>(buildable));
			}
			else if (commonInfoSubsystem->IsPowerPoleWallDouble(buildable))
			{
				powerPoleWallDoubles.Add(Cast<AFGBuildablePowerPole>(buildable));
			}
			else if (commonInfoSubsystem->IsPowerTower(buildable))
			{
				powerTowers.Add(Cast<AFGBuildablePowerPole>(buildable));
			}
		}
	}

	TMap<TSubclassOf<UFGItemDescriptor>, int32> itemsToAddOrRemoveFromInventory;

	if (newWireTypeRecipe && !wires.IsEmpty())
	{
		recipeAmountMap[newWireTypeRecipe] = UpgradeWire(player, wires, newWireTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	if (newPowerPoleTypeRecipe && !powerPoles.IsEmpty())
	{
		recipeAmountMap[newPowerPoleTypeRecipe] = UpgradePowerPole(player, powerPoles, newPowerPoleTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	if (newPowerPoleWallTypeRecipe && !powerPoles.IsEmpty())
	{
		recipeAmountMap[newPowerPoleWallTypeRecipe] = UpgradePowerPole(player, powerPoleWalls, newPowerPoleWallTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	if (newPowerPoleWallDoubleTypeRecipe && !powerPoles.IsEmpty())
	{
		recipeAmountMap[newPowerPoleWallDoubleTypeRecipe] = UpgradePowerPole(player, powerPoleWallDoubles, newPowerPoleWallDoubleTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	if (newPowerTowerTypeRecipe && !powerTowers.IsEmpty())
	{
		recipeAmountMap[newPowerTowerTypeRecipe] = UpgradePowerPole(player, powerTowers, newPowerTowerTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	TMap<TSubclassOf<UFGItemDescriptor>, int32> excessToDrop;
	auto playerInventory = player->GetInventory();

	for (const auto& entry : itemsToAddOrRemoveFromInventory)
	{
		if (entry.Value > 0)
		{
			// Add to inventory
			auto itemsAdded = playerInventory->AddStack(FInventoryStack(entry.Value, entry.Key), true);

			if (itemsAdded < entry.Value)
			{
				// No more room. Add to the drop crate
				excessToDrop.FindOrAdd(entry.Key) += entry.Value - itemsAdded;
			}
		}
		else if (entry.Value < 0)
		{
			playerInventory->Remove(entry.Key, -entry.Value);
		}
	}

	if (!excessToDrop.IsEmpty())
	{
		TArray<FInventoryStack> stacks;

		for (const auto& entry : excessToDrop)
		{
			stacks.Add(
				FInventoryStack(
					entry.Value,
					entry.Key
					)
				);
		}

		FHitResult out_hitResult;
		float out_waterDepth = 0;

		if (!player->TraceForGround(
			player->GetActorLocation(),
			player->GetActorLocation() + FVector(0, 0, -60000),
			out_hitResult,
			out_waterDepth
			))
		{
			// Place it whenever the player is, even if flying over the ground
			out_hitResult.Location = player->GetActorLocation();
		}

		AFGCrate* out_Crate = nullptr;

		AFGItemPickup_Spawnable::SpawnInventoryCrate(
			player->GetWorld(),
			stacks,
			out_hitResult.Location + FVector(0, 0, out_waterDepth),
			// Place at water level, if any
			TArray<class AActor*>(),
			out_Crate,
			nullptr
			);

		UMarcioCommonLibsUtils::DumpUnknownClass(out_Crate);
	}

	auto shoppingListComponent = UFGShoppingListComponent::GetShoppingListComponent(player->GetFGPlayerController());

	for (const auto& entry : recipeAmountMap)
	{
		bool out_result;
		auto shoppingListObject_Class = Cast<UFGShoppingListObject_Class>(shoppingListComponent->GetShoppingListObjectFromClass(entry.first, out_result));

		if (out_result && shoppingListObject_Class->GetAmount() > 0)
		{
			auto amount = FMath::Min(shoppingListObject_Class->GetAmount(), entry.second);

			shoppingListObject_Class->DecreaseAmount(amount);
		}
	}
}

int32 UMassUpgradeLogic::UpgradeConveyor
(
	class AFGCharacterPlayer* player,
	const TArray<AFGBuildableConveyorBase*>& conveyors,
	TSubclassOf<UFGRecipe> newConveyorTypeRecipe,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
)
{
	if (!player)
	{
		return 0;
	}

	int32 amountBuilt = 0;

	auto world = player->GetWorld();
	auto newConveyorType = AFGBuildable::GetBuildableClassFromRecipe(newConveyorTypeRecipe);
	auto buildableSubsystem = AFGBuildableSubsystem::Get(world);
	auto playerInventory = player->GetInventory();
	auto gameState = Cast<AFGGameState>(world->GetGameState());

	for (auto conveyor : conveyors)
	{
		if (conveyor->GetClass() == newConveyorType)
		{
			continue;
		}

		auto hologram = AFGHologram::SpawnHologramFromRecipe(
			newConveyorTypeRecipe,
			player->GetBuildGun(),
			conveyor->GetActorLocation(),
			player
			);

		FHitResult hitResult(conveyor, hologram->GetComponentByClass<UPrimitiveComponent>(), conveyor->GetActorLocation(), conveyor->GetActorRotation().Vector());
		if (!hologram->TryUpgrade(hitResult))
		{
			hologram->Destroy();
			continue;
		}

		hologram->ValidatePlacementAndCost(playerInventory);

		if (!hologram->IsUpgrade())
		{
			hologram->Destroy();
			continue;
		}

		if (!gameState->GetCheatNoCost())
		{
			for (const auto& itemAmount : hologram->GetCost(false))
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(itemAmount.ItemClass) -= itemAmount.Amount;
			}

			TArray<FInventoryStack> refunds;
			conveyor->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}
		}

		hologram->CheckValidPlacement();

		hologram->DoMultiStepPlacement(false);

		hologram->CheckValidPlacement();

		TArray<AActor*> children;
		auto newConveyor = Cast<AFGBuildableConveyorBase>(hologram->Construct(children, buildableSubsystem->GetNewNetConstructionID()));

		hologram->Destroy();

		conveyor->PreUpgrade_Implementation();
		conveyor->Upgrade_Implementation(newConveyor);

		// Remake the connections
		if (auto connection = conveyor->GetConnection0()->GetConnection())
		{
			conveyor->GetConnection0()->ClearConnection(); // Disconnect
			newConveyor->GetConnection0()->SetConnection(connection); // Connect to the new conveyor
		}

		if (auto connection = conveyor->GetConnection1()->GetConnection())
		{
			conveyor->GetConnection1()->ClearConnection(); // Disconnect
			newConveyor->GetConnection1()->SetConnection(connection); // Connect to the new conveyor
		}

		TArray<AActor*> childDismantleActors;
		conveyor->GetChildDismantleActors_Implementation(childDismantleActors);

		for (auto actor : childDismantleActors)
		{
			actor->Destroy();
		}

		conveyor->Destroy();

		amountBuilt += newConveyor->GetDismantleRefundReturnsMultiplier();
	}

	return amountBuilt;
}

int32 UMassUpgradeLogic::UpgradeStorage
(
	AFGCharacterPlayer* player,
	const TArray<AFGBuildableStorage*>& storages,
	TSubclassOf<UFGRecipe> newStorageTypeRecipe,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
)
{
	if (!player)
	{
		return 0;
	}

	int32 amountBuilt = 0;

	auto world = player->GetWorld();
	auto newStorageType = AFGBuildable::GetBuildableClassFromRecipe(newStorageTypeRecipe);
	//auto buildableSubsystem = AFGBuildableSubsystem::Get(world);
	//auto playerInventory = player->GetInventory();
	auto gameState = Cast<AFGGameState>(world->GetGameState());

	for (auto storage : storages)
	{
		if (storage->GetClass() == newStorageType)
		{
			continue;
		}

		auto transform = storage->GetActorTransform();

		if (!gameState->GetCheatNoCost())
		{
			for (const auto& itemAmount : UFGRecipe::GetIngredients(newStorageTypeRecipe))
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(itemAmount.ItemClass) -= itemAmount.Amount;
			}

			TArray<FInventoryStack> refunds;
			storage->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}
		}

		FActorSpawnParameters spawnParams;
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		spawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		auto newStorage = Cast<AFGBuildableStorage>(world->SpawnActor(newStorageType, &transform, spawnParams));

		newStorage->SetBuiltWithRecipe(newStorageTypeRecipe);
		newStorage->SetOriginalBuildableVariant(newStorageType);

		// storage->PreUpgrade_Implementation();
		// storage->Upgrade_Implementation(newStorage);

		TArray<UFGFactoryConnectionComponent*> oldComponents;
		storage->GetComponents(oldComponents);

		TArray<UFGFactoryConnectionComponent*> newComponents;
		newStorage->GetComponents(newComponents);

		for (auto oldConnection : oldComponents)
		{
			if (!oldConnection->IsConnected())
			{
				continue;
			}

			auto connection = oldConnection->GetConnection();

			// Disconnect from old
			oldConnection->ClearConnection();

			// Find a matching connection
			for (auto newConnection : newComponents)
			{
				if (newConnection->GetDirection() != oldConnection->GetDirection())
				{
					continue;
				}

				// Check if it is less than 10cm apart
				if (FVector::Distance(newConnection->GetComponentLocation(), oldConnection->GetComponentLocation()) < 10)
				{
					newConnection->SetConnection(connection);
				}
			}
		}

		// Move stacks
		for (auto x = 0; x < storage->mInventorySizeX * storage->mInventorySizeY; x++)
		{
			FInventoryStack stack;
			if (!storage->GetStorageInventory()->GetStackFromIndex(x, stack))
			{
				continue;
			}

			if (!stack.HasItems())
			{
				continue;
			}

			auto itemsAdded = newStorage->GetStorageInventory()->AddStack(stack, true);

			// Excess stacks go into inventory
			if (itemsAdded < stack.NumItems)
			{
				// No more room. Add to the drop crate
				itemsToAddOrRemoveFromInventory.FindOrAdd(stack.Item.GetItemClass()) += stack.NumItems - itemsAdded;
			}
		}

		newStorage->PlayBuildEffects(player);
		newStorage->ExecutePlayBuildEffects();

		TArray<AActor*> childDismantleActors;
		storage->GetChildDismantleActors_Implementation(childDismantleActors);

		for (auto actor : childDismantleActors)
		{
			actor->Destroy();
		}

		storage->Destroy();

		amountBuilt++;
	}

	return amountBuilt;
}

int32 UMassUpgradeLogic::UpgradePipeline
(
	AFGCharacterPlayer* player,
	const TArray<AFGBuildablePipeline*>& pipelines,
	TSubclassOf<UFGRecipe> newPipelineTypeRecipe,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
)
{
	if (!player)
	{
		return 0;
	}

	int32 amountBuilt = 0;

	auto world = player->GetWorld();
	auto newPipelineType = AFGBuildable::GetBuildableClassFromRecipe(newPipelineTypeRecipe);
	auto buildableSubsystem = AFGBuildableSubsystem::Get(world);
	auto playerInventory = player->GetInventory();
	auto gameState = Cast<AFGGameState>(world->GetGameState());

	for (auto pipeline : pipelines)
	{
		if (pipeline->GetClass() == newPipelineType)
		{
			continue;
		}

		auto hologram = AFGHologram::SpawnHologramFromRecipe(
			newPipelineTypeRecipe,
			player->GetBuildGun(),
			pipeline->GetActorLocation(),
			player
			);

		FHitResult hitResult(pipeline, hologram->GetComponentByClass<UPrimitiveComponent>(), pipeline->GetActorLocation(), pipeline->GetActorRotation().Vector());
		if (!hologram->TryUpgrade(hitResult))
		{
			hologram->Destroy();
			continue;
		}

		hologram->ValidatePlacementAndCost(playerInventory);

		if (!hologram->IsUpgrade())
		{
			hologram->Destroy();
			continue;
		}

		if (!gameState->GetCheatNoCost())
		{
			for (const auto& itemAmount : hologram->GetCost(false))
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(itemAmount.ItemClass) -= itemAmount.Amount;
			}

			TArray<FInventoryStack> refunds;
			pipeline->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}
		}

		hologram->CheckValidPlacement();

		hologram->DoMultiStepPlacement(false);

		hologram->CheckValidPlacement();

		TArray<AActor*> children;
		auto newPipeline = Cast<AFGBuildablePipeline>(hologram->Construct(children, buildableSubsystem->GetNewNetConstructionID()));

		hologram->Destroy();

		pipeline->PreUpgrade_Implementation();
		pipeline->Upgrade_Implementation(newPipeline);

		// Remake the connections
		if (auto connection = pipeline->GetPipeConnection0()->GetConnection())
		{
			pipeline->GetPipeConnection0()->ClearConnection(); // Disconnect
			newPipeline->GetPipeConnection0()->SetConnection(connection); // Connect to the new pipeline
		}

		if (auto connection = pipeline->GetPipeConnection1()->GetConnection())
		{
			pipeline->GetPipeConnection1()->ClearConnection(); // Disconnect
			newPipeline->GetPipeConnection1()->SetConnection(connection); // Connect to the new pipeline
		}

		// Delete children, if any
		TArray<AActor*> childDismantleActors;
		pipeline->GetChildDismantleActors_Implementation(childDismantleActors);

		for (auto actor : childDismantleActors)
		{
			actor->Destroy();
		}

		pipeline->Destroy();

		amountBuilt += newPipeline->GetDismantleRefundReturnsMultiplier();
	}

	return amountBuilt;
}

int32 UMassUpgradeLogic::UpgradePump
(
	AFGCharacterPlayer* player,
	const TArray<AFGBuildablePipelinePump*>& pumps,
	TSubclassOf<UFGRecipe> newPumpTypeRecipe,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
)
{
	if (!player)
	{
		return 0;
	}

	int32 amountBuilt = 0;

	auto world = player->GetWorld();
	auto newPumpType = AFGBuildable::GetBuildableClassFromRecipe(newPumpTypeRecipe);
	//auto buildableSubsystem = AFGBuildableSubsystem::Get(world);
	//auto playerInventory = player->GetInventory();
	auto gameState = Cast<AFGGameState>(world->GetGameState());
	auto powerCircuitSubsystem = AFGCircuitSubsystem::Get(world);

	for (auto pump : pumps)
	{
		if (pump->GetClass() == newPumpType)
		{
			continue;
		}

		auto transform = pump->GetActorTransform();

		if (!gameState->GetCheatNoCost())
		{
			for (const auto& itemAmount : UFGRecipe::GetIngredients(newPumpTypeRecipe))
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(itemAmount.ItemClass) -= itemAmount.Amount;
			}

			TArray<FInventoryStack> refunds;
			pump->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}
		}

		FActorSpawnParameters spawnParams;
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		spawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		auto newPump = Cast<AFGBuildablePipelinePump>(world->SpawnActor(newPumpType, &transform, spawnParams));

		newPump->SetBuiltWithRecipe(newPumpTypeRecipe);
		newPump->SetOriginalBuildableVariant(newPumpType);

		// pump->PreUpgrade_Implementation();
		// pump->Upgrade_Implementation(newPump);

		auto oldComponents = pump->GetPipeConnections();
		auto newComponents = newPump->GetPipeConnections();

		UMarcioCommonLibsUtils::DumpUnknownClass(pump, TEXT(""), TEXT("Old-"));
		UMarcioCommonLibsUtils::DumpUnknownClass(pump->GetPowerInfo(), TEXT(""),TEXT("Old-"));

		// Remake the connections
		if (auto connection = oldComponents[0]->GetConnection())
		{
			oldComponents[0]->ClearConnection(); // Disconnect
			newComponents[0]->SetConnection(connection); // Connect to the new pipeline
		}

		if (auto connection = oldComponents[1]->GetConnection())
		{
			oldComponents[1]->ClearConnection(); // Disconnect
			newComponents[1]->SetConnection(connection); // Connect to the new pipeline
		}

		newPump->PlayBuildEffects(player);
		newPump->ExecutePlayBuildEffects();

		TArray<UFGPowerConnectionComponent*> oldPowerConnections;
		pump->GetComponents(oldPowerConnections);

		TArray<UFGPowerConnectionComponent*> newPowerConnections;
		newPump->GetComponents(newPowerConnections);

		for (auto x = 0; x < oldPowerConnections.Num() && x < newPowerConnections.Num(); x++)
		{
			auto oldPowerComponent = oldPowerConnections[x];
			auto newPowerComponent = newPowerConnections[x];

			UMarcioCommonLibsUtils::DumpUnknownClass(oldPowerComponent, TEXT(""), TEXT("Old-"));
			UMarcioCommonLibsUtils::DumpUnknownClass(newPowerComponent, TEXT(""),TEXT("New-"));

			TArray<AFGBuildableWire*> wires;
			oldPowerComponent->GetWires(wires);

			for (auto wire : wires)
			{
				auto oppositeConnection = wire->GetOppositeConnection(oldPowerComponent);

				//powerCircuitSubsystem->DisconnectComponents(oldPowerComponent, oppositeConnection);
				wire->Disconnect();

				//powerCircuitSubsystem->ConnectComponents(newPowerComponent, oppositeConnection);
				wire->Connect(newPowerComponent, oppositeConnection);

				// if(oppositeConnection)
				// {
				// 	wire->TurnOffAndDestroy();
				//
				// 	TArray< FWireInstance > instances;
				// 	AFGBuildableWire::CreateWireInstancesBetweenConnections(instances, newPowerComponent, oppositeConnection, FTransform());
				// }
			}
		}

		TArray<AActor*> childDismantleActors;
		pump->GetChildDismantleActors_Implementation(childDismantleActors);

		for (auto actor : childDismantleActors)
		{
			actor->Destroy();
		}

		UMarcioCommonLibsUtils::DumpUnknownClass(newPump, TEXT(""),TEXT("New-"));
		UMarcioCommonLibsUtils::DumpUnknownClass(newPump->GetPowerInfo(), TEXT(""),TEXT("New-"));

		pump->TurnOffAndDestroy();

		newPump->TryStartProductionLoopEffects(true);

		amountBuilt++;
	}

	return amountBuilt;
}

int32 UMassUpgradeLogic::UpgradeWire
(
	AFGCharacterPlayer* player,
	const TArray<AFGBuildableWire*>& wires,
	TSubclassOf<UFGRecipe> newWireTypeRecipe,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
)
{
	if (!player)
	{
		return 0;
	}

	int32 amountBuilt = 0;

	auto world = player->GetWorld();
	auto newWireType = AFGBuildable::GetBuildableClassFromRecipe(newWireTypeRecipe);
	auto buildableSubsystem = AFGBuildableSubsystem::Get(world);
	auto playerInventory = player->GetInventory();
	auto gameState = Cast<AFGGameState>(world->GetGameState());

	for (auto wire : wires)
	{
		if (wire->GetClass() == newWireType)
		{
			continue;
		}

		auto hologram = AFGHologram::SpawnHologramFromRecipe(
			newWireTypeRecipe,
			player->GetBuildGun(),
			wire->GetActorLocation(),
			player
			);

		FHitResult hitResult(wire, hologram->GetComponentByClass<UPrimitiveComponent>(), wire->GetActorLocation(), wire->GetActorRotation().Vector());
		if (!hologram->TryUpgrade(hitResult))
		{
			hologram->Destroy();
			continue;
		}

		hologram->ValidatePlacementAndCost(playerInventory);

		if (!hologram->IsUpgrade())
		{
			hologram->Destroy();
			continue;
		}

		if (!gameState->GetCheatNoCost())
		{
			for (const auto& itemAmount : hologram->GetCost(false))
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(itemAmount.ItemClass) -= itemAmount.Amount;
			}

			TArray<FInventoryStack> refunds;
			wire->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}
		}

		hologram->CheckValidPlacement();

		hologram->DoMultiStepPlacement(false);

		hologram->CheckValidPlacement();

		TArray<AActor*> children;
		auto newWire = Cast<AFGBuildableWire>(hologram->Construct(children, buildableSubsystem->GetNewNetConstructionID()));

		hologram->Destroy();

		wire->PreUpgrade_Implementation();
		wire->Upgrade_Implementation(newWire);
		
		// Delete children, if any
		TArray<AActor*> childDismantleActors;
		wire->GetChildDismantleActors_Implementation(childDismantleActors);

		for (auto actor : childDismantleActors)
		{
			actor->Destroy();
		}

		wire->Destroy();

		amountBuilt += newWire->GetDismantleRefundReturnsMultiplier();
	}

	return amountBuilt;
}

int32 UMassUpgradeLogic::UpgradePowerPole
(
	AFGCharacterPlayer* player,
	const TArray<AFGBuildablePowerPole*>& powerPoles,
	TSubclassOf<UFGRecipe> newPowerPoleTypeRecipe,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
)
{
	if (!player)
	{
		return 0;
	}

	int32 amountBuilt = 0;

	auto world = player->GetWorld();
	auto newPowerPoleType = AFGBuildable::GetBuildableClassFromRecipe(newPowerPoleTypeRecipe);
	auto buildableSubsystem = AFGBuildableSubsystem::Get(world);
	auto playerInventory = player->GetInventory();
	auto gameState = Cast<AFGGameState>(world->GetGameState());

	for (auto powerPole : powerPoles)
	{
		if (powerPole->GetClass() == newPowerPoleType)
		{
			continue;
		}

		auto hologram = AFGHologram::SpawnHologramFromRecipe(
			newPowerPoleTypeRecipe,
			player->GetBuildGun(),
			powerPole->GetActorLocation(),
			player
			);

		FHitResult hitResult(powerPole, hologram->GetComponentByClass<UPrimitiveComponent>(), powerPole->GetActorLocation(), powerPole->GetActorRotation().Vector());
		if (!hologram->TryUpgrade(hitResult))
		{
			hologram->Destroy();
			continue;
		}

		hologram->ValidatePlacementAndCost(playerInventory);

		if (!hologram->IsUpgrade())
		{
			hologram->Destroy();
			continue;
		}

		if (!gameState->GetCheatNoCost())
		{
			for (const auto& itemAmount : hologram->GetCost(false))
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(itemAmount.ItemClass) -= itemAmount.Amount;
			}

			TArray<FInventoryStack> refunds;
			powerPole->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}
		}

		hologram->CheckValidPlacement();

		hologram->DoMultiStepPlacement(false);

		hologram->CheckValidPlacement();

		TArray<AActor*> children;
		auto newPowerPole = Cast<AFGBuildablePowerPole>(hologram->Construct(children, buildableSubsystem->GetNewNetConstructionID()));

		hologram->Destroy();

		powerPole->PreUpgrade_Implementation();
		powerPole->Upgrade_Implementation(newPowerPole);

		// Delete children, if any
		TArray<AActor*> childDismantleActors;
		powerPole->GetChildDismantleActors_Implementation(childDismantleActors);

		for (auto actor : childDismantleActors)
		{
			actor->Destroy();
		}

		powerPole->Destroy();

		amountBuilt++;
	}

	return amountBuilt;
}


#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
