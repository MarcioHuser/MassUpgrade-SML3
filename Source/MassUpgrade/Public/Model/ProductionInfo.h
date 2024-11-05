#pragma once
#include "Templates/SubclassOf.h"

#include <ProductionInfo.generated.h>

USTRUCT(Blueprintable)
struct MASSUPGRADE_API FProductionInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UFGBuildDescriptor> buildableType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<class AFGBuildable*> buildables;
public:
};

USTRUCT(Blueprintable)
struct MASSUPGRADE_API FProductionInfoWithArray
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UFGBuildDescriptor> buildableType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class AFGBuildable*> buildables;
public:
};
