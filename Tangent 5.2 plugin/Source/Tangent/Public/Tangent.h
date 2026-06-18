// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 374 $

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "TangentEngine.h"

//DECLARE_LOG_CATEGORY_EXTERN(LogTangent, Log, All);

class FTangentModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void MigrateLegacyData();

private:
	TangentEngine* _engine = nullptr;
	FRunnableThread* _engineThread = nullptr;
};
