// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 374 $

#include "Tangent.h"
#include "SequencerCommands.h"					// Sequencer module

#define LOCTEXT_NAMESPACE "FTangentModule"

//DEFINE_LOG_CATEGORY(LogTangent);

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION < 3)

// ** 5.1 and 5.2 only **
// 
// to be able to compile with registered sequencer commands we have to supply an implementation for this function, I don't know
// why but it's the only way it compiles and runs, it is defined as a pure virtual (abstract) function but the sequencer command
// class doesn't seem to expose it, even though it appears in the source, this stubbed implementation is not actually called
// 
// UPDATE - fixed in 5.3
void FSequencerCommands::RegisterCommands()
{
	// n/a
}

#endif // 5.1 and 5.2 only

void FTangentModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogTemp, Log, TEXT("Tangent plugin Startup..."));

	// first thing is to handle legacy data transfer if it is relevant
	MigrateLegacyData();

	// create a new engine instance to handle comms with the tangent hub and wrap it in a thred to run it
	_engine = new TangentEngine();
	_engineThread = FRunnableThread::Create(_engine, TEXT("Tangent Engine Thread"), 0);
}

void FTangentModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogTemp, Log, TEXT("Tangent plugin Shutdown..."));

	// safely release our resources
	if (_engine)
	{
		// call the engine to stop running
		_engine->Stop();

		// if the thread is valid we wait for it to honour the stop request
		if (_engineThread)
		{
			// wait for the thread to safely exit before releasing it
			_engineThread->WaitForCompletion();
			delete _engineThread;
			_engineThread = nullptr;
		}

		// release the engine instance
		delete _engine;
		_engine = nullptr;
		
	}
}

// from plugin version 1.1.2 (14) onwards we moved the persistent data the plugin uses from the project's saved folder to the config folder as
// it is a better location, to help users move to this version we scan the old location and copy any existing files to the new location the first
// time we are run, if the new location already has content we do nothing, we also handle a minor file rename to make it more context consistent
void FTangentModule::MigrateLegacyData()
{
	// data used to be in a tangent subfolder in the saved dir, new data is now in the config dir
	FString	oldPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() + TEXT("Tangent")),
			newPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() + TEXT("Tangent"));

	// prepare the file manager instance for handling data
	IPlatformFile& fileMan = FPlatformFileManager::Get().GetPlatformFile();

	// if the new data area does not exist but we have old data, we will copy this across
	if (!fileMan.DirectoryExists(*newPath) && fileMan.DirectoryExists(*oldPath))
	{
		// copy the data to the new location, it is contained in a single folder
		fileMan.CopyDirectoryTree(*newPath, *oldPath, false);

		// now we check for a file that we changed the suffix for, making it an ini file to be more consistent
		FString oldFileName = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() + TEXT("Tangent/ClassModeMap.txt"));
		if (fileMan.FileExists(*oldFileName))
		{
			// the file exists in the copied data, just rename it there
			FString newFileName = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() + TEXT("Tangent/ClassModeMap.ini"));
			fileMan.MoveFile(*newFileName, *oldFileName);
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTangentModule, Tangent)
