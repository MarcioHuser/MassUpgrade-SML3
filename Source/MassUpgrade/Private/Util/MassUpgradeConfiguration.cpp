#include "Util/MassUpgradeConfiguration.h"

#include "Util/MULogging.h"

#include "Util/MUOptimize.h"

FMassUpgrade_ConfigStruct UMassUpgradeConfiguration::configuration;

void UMassUpgradeConfiguration::SetMassUpgradeConfiguration(const FMassUpgrade_ConfigStruct& in_configuration)
{
	configuration = in_configuration;

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	MU_LOG_Display(TEXT("setConfiguration"));

	MU_LOG_Display(TEXT("logLevel = "), configuration.logLevel);

	MU_LOG_Display(TEXT("==="));
}

void UMassUpgradeConfiguration::GetMassUpgradeConfiguration(int& out_logLevel)
{
	out_logLevel = configuration.logLevel;
}

int UMassUpgradeConfiguration::GetLogLevelMU()
{
	return configuration.logLevel;
}
