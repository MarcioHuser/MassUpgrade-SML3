#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "MassUpgrade_ConfigStruct.generated.h"

/* Struct generated from Mod Configuration Asset '/MassUpgrade/Configuration/MassUpgrade_Config' */
USTRUCT(BlueprintType)
struct FMassUpgrade_ConfigStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    int32 logLevel{};

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FMassUpgrade_ConfigStruct GetActiveConfig(UObject* WorldContext) {
        FMassUpgrade_ConfigStruct ConfigStruct{};
        FConfigId ConfigId{"", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FMassUpgrade_ConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

