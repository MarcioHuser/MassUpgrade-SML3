#include "Logic/MassUpgradeLogic.h"

#include <map>

#include "FGCharacterPlayer.h"
#include "FGCrate.h"
#include "FGItemPickup_Spawnable.h"
#include "FGFactoryConnectionComponent.h"
#include "FGGameState.h"
#include "FGPlayerController.h"
#include "MassUpgradeRCO.h"
#include "Buildables/FGBuildable.h"
#include "Hologram/FGBuildableHologram.h"
#include "Hologram/FGConveyorBeltHologram.h"
#include "Hologram/FGConveyorLiftHologram.h"
#include "Hologram/FGHologram.h"
#include "Model/ConveyorProductionInfo.h"
#include "Resources/FGBuildDescriptor.h"
#include "ShoppingList/FGShoppingListObject.h"
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
	const TArray<FConveyorProductionInfo>& infos
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
			infos
			);
	}
	else
	{
		UMassUpgradeRCO::getRCO(player->GetWorld())->UpgradeConveyors(
			player,
			newBeltTypeRecipe,
			newLiftTypeRecipe,
			infos
			);
	}
}

void UMassUpgradeLogic::UpgradeConveyors_Server
(
	AFGCharacterPlayer* player,
	const TSubclassOf<UFGRecipe>& newBeltTypeRecipe,
	const TSubclassOf<UFGRecipe>& newLiftTypeRecipe,
	const TArray<FConveyorProductionInfo>& infos
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
					infos
					);
			}
			);

		return;
	}

	std::map<TSubclassOf<UFGRecipe>, int32> recipeAmountMap;

	TArray<AFGBuildableConveyorBelt*> belts;
	TArray<AFGBuildableConveyorLift*> lifts;

	for (const auto& info : infos)
	{
		for (auto conveyor : info.conveyors)
		{
			if (auto belt = Cast<AFGBuildableConveyorBelt>(conveyor))
			{
				belts.Add(belt);
			}
			else if (auto lift = Cast<AFGBuildableConveyorLift>(conveyor))
			{
				lifts.Add(lift);
			}
		}
	}

	TMap<TSubclassOf<UFGItemDescriptor>, int32> itemsToAddOrRemoveFromInventory;

	if (newBeltTypeRecipe && !belts.IsEmpty())
	{
		recipeAmountMap[newBeltTypeRecipe] = UpgradeBelt(player, belts, newBeltTypeRecipe, itemsToAddOrRemoveFromInventory);
	}

	if (newLiftTypeRecipe && !lifts.IsEmpty())
	{
		recipeAmountMap[newLiftTypeRecipe] = UpgradeLifts(player, lifts, newLiftTypeRecipe, itemsToAddOrRemoveFromInventory);
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

int32 UMassUpgradeLogic::UpgradeBelt
(
	class AFGCharacterPlayer* player,
	const TArray<AFGBuildableConveyorBelt*>& belts,
	TSubclassOf<UFGRecipe> newBeltTypeRecipe,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
)
{
	if (!player)
	{
		return 0;
	}

	int32 amountBuilt = 0;

	auto world = player->GetWorld();
	auto newBeltType = AFGBuildable::GetBuildableClassFromRecipe(newBeltTypeRecipe);
	auto buildableSubsystem = AFGBuildableSubsystem::Get(world);
	auto playerInventory = player->GetInventory();
	auto gameState = Cast<AFGGameState>(world->GetGameState());

	for (auto belt : belts)
	{
		if (belt->GetClass() == newBeltType)
		{
			continue;
		}

		auto hologram = Cast<AFGConveyorBeltHologram>(
			AFGHologram::SpawnHologramFromRecipe(
				newBeltTypeRecipe,
				player->GetBuildGun(),
				belt->GetActorLocation(),
				player
				)
			);

		FHitResult hitResult(belt, hologram->GetComponentByClass<UPrimitiveComponent>(), belt->GetActorLocation(), belt->GetActorRotation().Vector());
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
			belt->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}
		}

		hologram->CheckValidPlacement();

		hologram->DoMultiStepPlacement(false);

		hologram->CheckValidPlacement();

		TArray<AActor*> children;
		auto newBelt = Cast<AFGBuildableConveyorBelt>(hologram->Construct(children, buildableSubsystem->GetNewNetConstructionID()));

		hologram->Destroy();

		belt->PreUpgrade_Implementation();
		belt->Upgrade_Implementation(newBelt);

		// Remake the connections
		if (auto connection = belt->GetConnection0()->GetConnection())
		{
			belt->GetConnection0()->ClearConnection(); // Disconnect
			newBelt->GetConnection0()->SetConnection(connection); // Connect to the new belt
		}

		if (auto connection = belt->GetConnection1()->GetConnection())
		{
			belt->GetConnection1()->ClearConnection(); // Disconnect
			newBelt->GetConnection1()->SetConnection(connection); // Connect to the new belt
		}

		belt->Destroy();

		amountBuilt += newBelt->GetDismantleRefundReturnsMultiplier();
	}

	return amountBuilt;
}

int32 UMassUpgradeLogic::UpgradeLifts
(
	class AFGCharacterPlayer* player,
	const TArray<AFGBuildableConveyorLift*>& lifts,
	TSubclassOf<UFGRecipe> newLiftTypeRecipe,
	TMap<TSubclassOf<UFGItemDescriptor>, int32>& itemsToAddOrRemoveFromInventory
)
{
	if (!player)
	{
		return 0;
	}

	int32 amountBuilt = 0;

	auto world = player->GetWorld();
	auto newLiftType = AFGBuildable::GetBuildableClassFromRecipe(newLiftTypeRecipe);
	auto buildableSubsystem = AFGBuildableSubsystem::Get(world);
	auto playerInventory = player->GetInventory();
	auto gameState = Cast<AFGGameState>(world->GetGameState());

	for (auto lift : lifts)
	{
		if (lift->GetClass() == newLiftType)
		{
			continue;
		}

		auto hologram = Cast<AFGConveyorLiftHologram>(
			AFGHologram::SpawnHologramFromRecipe(
				newLiftTypeRecipe,
				player->GetBuildGun(),
				lift->GetActorLocation(),
				player
				)
			);

		FHitResult hitResult(lift, hologram->GetComponentByClass<UPrimitiveComponent>(), lift->GetActorLocation(), lift->GetActorRotation().Vector());
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

		UMarcioCommonLibsUtils::DumpUnknownClass(lift);

		if (!gameState->GetCheatNoCost())
		{
			for (const auto& itemAmount : hologram->GetCost(false))
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(itemAmount.ItemClass) -= itemAmount.Amount;
			}

			TArray<FInventoryStack> refunds;
			lift->GetDismantleRefundReturns(refunds);

			for (const auto& inventory : refunds)
			{
				itemsToAddOrRemoveFromInventory.FindOrAdd(inventory.Item.GetItemClass()) += inventory.NumItems;
			}
		}

		hologram->CheckValidPlacement();

		hologram->DoMultiStepPlacement(false);

		hologram->CheckValidPlacement();

		TArray<AActor*> children;
		auto newLift = Cast<AFGBuildableConveyorLift>(hologram->Construct(children, buildableSubsystem->GetNewNetConstructionID()));

		hologram->Destroy();

		lift->PreUpgrade_Implementation();
		lift->Upgrade_Implementation(newLift);

		// Remake the connections
		if (auto connection = lift->GetConnection0()->GetConnection())
		{
			lift->GetConnection0()->ClearConnection(); // Disconnect
			newLift->GetConnection0()->SetConnection(connection); // Connect to the new lift
		}

		if (auto connection = lift->GetConnection1()->GetConnection())
		{
			lift->GetConnection1()->ClearConnection(); // Disconnect
			newLift->GetConnection1()->SetConnection(connection); // Connect to the new lift
		}

		UMarcioCommonLibsUtils::DumpUnknownClass(newLift);

		lift->Destroy();

		amountBuilt += newLift->GetDismantleRefundReturnsMultiplier();
	}

	return amountBuilt;
}


#ifndef OPTIMIZE
#pragma optimize("", on)
#endif
