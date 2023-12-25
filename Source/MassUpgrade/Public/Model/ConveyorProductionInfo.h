#pragma once
#include "Templates/SubclassOf.h"

#include <ConveyorProductionInfo.generated.h>

USTRUCT(Blueprintable)
struct MASSUPGRADE_API FConveyorProductionInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UFGBuildDescriptor> conveyorType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<class AFGBuildableConveyorBase*> conveyors;

public:
	//FORCEINLINE ~FSimplePowerCircuitStats() = default;
};
