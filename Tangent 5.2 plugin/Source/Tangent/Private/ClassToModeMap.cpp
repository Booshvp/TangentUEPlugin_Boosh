// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 375 $

#include "ClassToModeMap.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"

// convenience map class for fname instances that can save/load as user friendly text to a file
ClassToModeMap::ClassToModeMap()
{
	// the persistent data is stored in a file with a fixed name, fetch it from the helper function
	FString fPath = SavedFileName();

	// on construction we load from the preset file if it exists, if not we init with defaults and then save
	if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*fPath))
	{
		// there is a file to load from, note that if the file is empty this results in an empty map
		LoadFromFile();
	}
	else
	{
		// reset to default map content and save to the persistent file
		ResetToDefault();
	}
}

// returns the predefined absolute path and name for the data in this fname map to be stored at
FString ClassToModeMap::SavedFileName()
{
	// prepare and return a string of the preset filename in the tangent subfolder in the project's config data area
	// (note - moved to config from saved in v1.1.2 (14) and changed to 'ini' suffix from 'txt')
	FString filePath = FPaths::ProjectConfigDir() + TEXT("Tangent/ClassModeMap.ini");
	return FPaths::ConvertRelativePathToFull(filePath);
}

// resets this instance of the map to the defaults as if it has been created for the first time, it empties the map of any current
// content, inserts the defaults and then saves this to the persistent file, replacing any existing file
void ClassToModeMap::ResetToDefault()
{
	// the persistent data is stored in a file with a fixed name, fetch it from the helper function
	FString fPath = SavedFileName();

	// delete the file if it exists, no need for a warning if not and even if read only
	IFileManager::Get().Delete(*fPath, false, true, true);

	// start with a clean slate by emptying the map data
	Empty();

	// init with defaults
	InsertDefaultMapData();

	// save this data to the file to create the initial data set
	SaveToFile();
}

// saves the contents of this map instance to the preset file
void ClassToModeMap::SaveToFile()
{
	// the file name is fixed, fetch it from the helper function
	FString fPath = SavedFileName();

	// we prepare the map contents as a string with each map entry key/value pairing as text on separate lines with a separator and
	// starting with a comment line explaining the file contents as a hint for any user who edits the file
	FString stringBuilder = TEXT("; Each line may have a class name and a mode name separated by :\n\n");
	for (const TPair<FName, FName>& pair : *this)
	{
		// each line contains the key and value pair separated by a character (that we assume is not in the values!!!)
		stringBuilder += pair.Key.ToString() + TEXT(":") + pair.Value.ToString() + TEXT("\n");
	}

	// use the standard file helper to store the resulting string to the file
	FFileHelper::SaveStringToFile(stringBuilder, *fPath);
}

// loads the map data from the preset file, replacing any existing data
void ClassToModeMap::LoadFromFile()
{
	// the file name is fixed, fetch it from the helper function
	FString fPath = SavedFileName();

	// start with a clean slate by emptying the map data
	Empty();

	// check for a file to load
 	if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*fPath))
	{
		// we load the string in with a visitor lamdba function that is called for every line of the file string contents
		auto lineVisitor = [this](FStringView lineView)
		{
			// trim off any and all whitespace from the start of the line
			auto strippedView = lineView.TrimStart();

			// only continue parsing if the line still has content and does not start with our defined comment character
			if (!strippedView.IsEmpty() && !strippedView.StartsWith(ANSI_TO_TCHAR(";")))
			{
				// we need to convert the string view into a separate string for parsing into an array of items
				FString line(strippedView);
				TArray<FString> pairs;

				// parse each line by splitting at the separator, looking for two and only two items per line - a key and a value
				if (line.ParseIntoArray(pairs, TEXT(":"), false) == 2)
				{
					// strip whitespace from the start and end of each item
					pairs[0].TrimStartAndEndInline();
					pairs[1].TrimStartAndEndInline();

					// successful parsing, add the pair to the map
					Add(*pairs[0], *pairs[1]);
				}
			}
		};

		// make the call to load the string data from the file, parsing in the visitor code above
		FFileHelper::LoadFileToStringWithLineVisitor(*fPath, lineVisitor);
	}
}

// inserts a set of default map data key/value pairs to the map instance
void ClassToModeMap::InsertDefaultMapData()
{
	// add our required map items
//	Add("ColorCorrectionWindow", "CCW");
}
