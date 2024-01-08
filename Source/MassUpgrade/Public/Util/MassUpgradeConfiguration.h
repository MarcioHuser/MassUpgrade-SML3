#pragma once

#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "MassUpgrade_ConfigStruct.h"

#include "MassUpgradeConfiguration.generated.h"

UCLASS()
class MASSUPGRADE_API UMassUpgradeConfiguration : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="MassUpgradeConfiguration")
	static void SetMassUpgradeConfiguration
	(
		UPARAM(DisplayName = "Configuration") const struct FMassUpgrade_ConfigStruct& in_configuration
	);

	UFUNCTION(BlueprintCallable, Category="MassUpgradeConfiguration")
	static void GetMassUpgradeConfiguration
	(
		UPARAM(DisplayName = "Log Level") int& out_logLevel
	);

	UFUNCTION(BlueprintCallable, Category = "MassUpgradeConfiguration", BlueprintPure)
	static int GetLogLevelMU();

public:
	static FMassUpgrade_ConfigStruct configuration;
};
