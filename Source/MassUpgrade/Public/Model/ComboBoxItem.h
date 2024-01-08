#pragma once
#include "Templates/SubclassOf.h"

#include <ComboBoxItem.generated.h>

USTRUCT(Blueprintable)
struct MASSUPGRADE_API FComboBoxItem
{
	GENERATED_USTRUCT_BODY()
public:
	FComboBoxItem()
		: recipe(nullptr),
		  amount(0)
	{
	}

	FComboBoxItem(const TSubclassOf<class UFGRecipe>& Recipe, float Amount)
		: recipe(Recipe),
		  amount(Amount)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UFGRecipe> recipe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float amount;

public:
};
