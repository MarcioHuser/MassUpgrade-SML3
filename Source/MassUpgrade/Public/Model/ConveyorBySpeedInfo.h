#pragma once
#include "Templates/SubclassOf.h"

#include <ConveyorBySpeedInfo.generated.h>

USTRUCT(Blueprintable)
struct MASSUPGRADE_API FConveyorBySpeedInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 speed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UFGRecipe> beltRecipe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UFGRecipe> liftRecipe;

public:
	//FORCEINLINE ~FSimplePowerCircuitStats() = default;
};
