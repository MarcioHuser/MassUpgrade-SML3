// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMassUpgradeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	inline FString
	getTagName() const
	{
		return /*getTimeStamp() +*/ TEXT("MassUpgradeModule: ");
	}
};
