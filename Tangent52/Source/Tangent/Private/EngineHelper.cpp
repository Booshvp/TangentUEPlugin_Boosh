// Tangent Panels Plugin for Unreal Editor
// Copyright 2022 Tangent Wave Ltd.
// SVN: $Revision: 385 $

#include "EngineHelper.h"
#include "ResetCache.h"
#include "Engine/Selection.h"								// Engine module
#include "Editor.h"											// UnrealEd module
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "GameFramework/WorldSettings.h"
#include "Async/Async.h"									// Core module
#include "LevelEditor.h"									// LevelEditor module
#include "CineCameraActor.h"								// CinematicCamera module
#include "CineCameraComponent.h"							// CinematicCamera module
#include "ISequencerModule.h"								// Sequencer module
#include "SequencerCommands.h"								// Sequencer module
#include "LevelSequenceActor.h"								// LevelSequence module
#include "ColorCorrectRegion.h"								// ColorCorrectRegions plugin
#include "Engine/SkyLight.h"

// class static event that may be bound to to receive notification when the selected object is changed with
// a possible request to change mode to match the type of the new selection
UEngineHelper::FSelectionChangedEvent UEngineHelper::SelectionChangedEvent;

// class static event to notify of actor property changes made in the editor ui
UEngineHelper::FActorPropertyChangedEvent UEngineHelper::ActorPropertyChangedEvent;

// class static event to notify of a train session being completed with a response triggered by a property change in the editor ui
UEngineHelper::FTrainResponseEvent UEngineHelper::TrainResponseEvent;

// class static event to allow the passing back of menu strings as required
UEngineHelper::FMenuString UEngineHelper::MenuStringEvent;

// class static event to allow the passing back of parameter values as required
UEngineHelper::FParameterValue UEngineHelper::ParameterValueEvent;

// class static event to allow the passing back of custom parameter values as required
UEngineHelper::FCustomParameterValue UEngineHelper::CustomParameterValueEvent;

// class static event to allow the passing back of custom menu strings as required
UEngineHelper::FCustomMenuString UEngineHelper::CustomMenuStringEvent;

// prefix increment operator for the transform reference enum that handles wrapping
UEngineHelper::TransformReference& operator++(UEngineHelper::TransformReference& ref) 
{
	// use the first and last special case values to handle increments
	return ref = (ref == UEngineHelper::TransformReference::LAST)? UEngineHelper::TransformReference::FIRST: static_cast<UEngineHelper::TransformReference>(ref + 1);
}

// prefix decrement operator for the transform reference enum that handles wrapping
UEngineHelper::TransformReference& operator--(UEngineHelper::TransformReference& ref) 
{
	// use the first and last special case values to handle decrements
	return ref = (ref == UEngineHelper::TransformReference::FIRST)? UEngineHelper::TransformReference::LAST: static_cast<UEngineHelper::TransformReference>(ref - 1);
}

// default constructor
UEngineHelper::UEngineHelper()
{
	// we filter out irrelevant property change events for anything other than actor instances, however there are a small number of
	// properties of their root components that we want to trigger panel updates which are stored by name in an array to prepare here
	_objectPropertyChangedExceptions.Add(FName(TEXT("bAbsoluteLocation")));
	_objectPropertyChangedExceptions.Add(FName(TEXT("bAbsoluteRotation")));
	_objectPropertyChangedExceptions.Add(FName(TEXT("bAbsoluteScale")));

	// this checks if the instance being created is the default object created by unreal or the actual instance created by the plugin, we only
	// subscribe to system events in the plugin
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// we subscribe to the selection changed event
		USelection::SelectionChangedEvent.AddUObject(this, &UEngineHelper::OnSelectionChanged);

		// listen for object property changes to be able to detect when actors are changed in the editor ui
		FCoreUObjectDelegates::OnPreObjectPropertyChanged.AddUObject(this, &UEngineHelper::OnPreObjectPropertyChanged);
		FDelegateHandle h = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UEngineHelper::OnObjectPropertyChanged);

		// listen for a pie/sie session starting/ending to perform certain updates
		FEditorDelegates::PostPIEStarted.AddUObject(this, &UEngineHelper::PostPIEStarted);
		FEditorDelegates::EndPIE.AddUObject(this, &UEngineHelper::OnEndPIE);

		// listen to the event triggered when the current map is changed, we take this generally as having a level loaded
		FEditorDelegates::MapChange.AddUObject(this, &UEngineHelper::OnMapChanged);

		// if auto-save (or a standard save) is operating then we cannot interact with objects, listen to the start and end events for
		// saves to block activity during this period
		FEditorDelegates::PreSaveWorldWithContext.AddUObject(this, &UEngineHelper::OnPreSaveWorldWithContext);
		FEditorDelegates::PostSaveWorldWithContext.AddUObject(this, &UEngineHelper::OnPostSaveWorldWithContext);

		// we listen for sequencer instances being created to be able to perform sequencer based control mappings, we need the module for
		// sequencers to register a delegate for the relevant event
		ISequencerModule&				mod = FModuleManager::GetModuleChecked<ISequencerModule>(TEXT("Sequencer"));
		FOnSequencerCreated::FDelegate	onNewSequencer;

		// bind the delegate to the function in this instance to call on the event being fired
		onNewSequencer.BindUObject(this, &UEngineHelper::OnSequencerCreated);

		// store the delegate handle that is returned so we can clean up when being destroyed
		_onSequencerCreatedDelegate = mod.RegisterOnSequencerCreated(onNewSequencer);
	}
}

// destructor
UEngineHelper::~UEngineHelper()
{
	// again only release event listeners for non-cdo instances
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// tidy up event handling
		USelection::SelectionChangedEvent.RemoveAll(this);
		FCoreUObjectDelegates::OnPreObjectPropertyChanged.RemoveAll(this);
		FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
		FEditorDelegates::PostPIEStarted.RemoveAll(this);
		FEditorDelegates::EndPIE.RemoveAll(this);
		FEditorDelegates::MapChange.RemoveAll(this);
		FEditorDelegates::PreSaveWorldWithContext.RemoveAll(this);
		FEditorDelegates::PostSaveWorldWithContext.RemoveAll(this);

		// the sequencer event is removed via the associated module
		ISequencerModule& mod = FModuleManager::GetModuleChecked<ISequencerModule>(TEXT("Sequencer"));
		mod.UnregisterOnSequencerCreated(_onSequencerCreatedDelegate);
	}
}

// called by the editor before a world is saved out, used to block interation with data during the save which can cause crashes
void UEngineHelper::OnPreSaveWorldWithContext(UWorld* world, FObjectPreSaveContext context)
{
	// safely set the flag that is used to track save activity
	_saveWorldActive.AtomicSet(true);
}

// called by the editor after a world is saved out, used to re-enable interation with data once the save is completed
void UEngineHelper::OnPostSaveWorldWithContext(UWorld* world, FObjectPostSaveContext context)
{
	// safely clear the flag that is used to track save activity
	_saveWorldActive.AtomicSet(false);
}

// allows access to the flag that tracks when a save activity is running, we use this to block interaction with world objects during saves which
// can cause crashes in the editor
bool UEngineHelper::IsSaveWorldActive()
{
	// return the flag safe value
	return _saveWorldActive;
}

// function that marks the actor as about to be modified with a specific property value change, must always be paired
// with a call to the post change function and must only be used in the game thread by the caller, it kicks off a transaction
// to surround the pending changes using the optional description if one is not already in place
void UEngineHelper::OnPreChange(AActor* actor, FProperty *property, const FString& description)
{
	// to prevent any kind of race overlap with ongoing changes and the completion timer, stop the timer here before continuing
	TSharedRef<FTimerManager> timeMan = GEditor->GetTimerManager();
	timeMan->ClearTimer(_postChangeTimer);

	// we only need to kick off a transaction if not already in place
	if (!GEditor->IsTransactionActive())
	{
		// a new transaction is required using the supplied description
		GEditor->BeginTransaction(FText::FromString(description));
	}

	// as a precautionary measure we log any attempts to call the pre change with a null property
	if (!property)
	{
		// log this warning as a marker to investigate
		UE_LOG(LogTemp, Warning, TEXT("EngineHelper: OnPreChange() called with null property"));
	}
	// we need to mark the target actor as modified/about to change, note that this call internally calls Modify() for us
	actor->PreEditChange(property);
}

// takes an actor/property pairing and stores it in a local bin so we can follow up a series of interactive updates with a value set
// one when the change session has been completed, technically we really ought to store as weak or shared pointers but this is over such
// a short time frame that we take the keep it simple approach here
void UEngineHelper::RegisterOnChangeActorProperty(AActor* actor, FProperty* prop)
{
	// we only want to store valid pairs where both pointers are defined
	if (actor && prop)
	{ 
		// we only want to store a pairing once, but a single actor could theoretically have more than one property changed during a change
		// transaction we we scan each time to make sure we are not repeating data, most of the time there is going to be a single entry here
		// so we are not going to waste a lot of time in this loop so no need to put the effort into making it more efficient
		for (int i = 0; i < _onChangeActors.Num(); i++)
		{
			// if we find this actor with this property then we can just stop here as we do not store duplicates
			if ((_onChangeActors[i] == actor) && (_onChangeProperties[i] == prop))
			{
				// already stored, exit here now
				return;
			}
		}

		// this is a new actor/property pairing, store both
		_onChangeActors.Add(actor);
		_onChangeProperties.Add(prop);
	}
}

// called at the end of a transaction when we have generated one or more interactive property change events, the docs say we have to follow
// up with value set events so we use the stored actor/property data built up during the session to send these in now, note that most of the
// time there will only be one entry
void UEngineHelper::CompleteOnChangeActorProperties()
{
	// allow for multiple properties for multiple actors during the transaction, most of the time there will be just one actor with one or two
	// properties
	for (int i = 0; i < _onChangeActors.Num(); i++)
	{
		// feed in the value set property changed events as required
		FPropertyChangedEvent changeEvent = FPropertyChangedEvent(_onChangeProperties[i], EPropertyChangeType::ValueSet);
		_onChangeActors[i]->PostEditChangeProperty(changeEvent);
	}

	// we have consumed the stored data so empty the data sets
	_onChangeActors.Empty();
	_onChangeProperties.Empty();
}

// function that marks the actor as having been modified, must always be paired with a call to the pre change function and must 
// only be used in the game thread by the caller, it handles the subsequent call to the change completion function after a preset 
// time period to wait until the user stops making changes before closing the transaction
void UEngineHelper::OnPostChange(AActor* actor, FProperty *property)
{
	// remember this actor and property pairing so we can do a value set changed event later which is a requirement of sending in an 
	// interactive one now
	RegisterOnChangeActorProperty(actor, property);

	// update the transaction with a changed event, note that the alternative PostEditChange() function calls
	// PostEditChangeProperty() with a null property event
	FPropertyChangedEvent changeEvent = FPropertyChangedEvent(property, EPropertyChangeType::Interactive);
	actor->PostEditChangeProperty(changeEvent);

	// also update movement separately, by saying not finished (false)  we can get on-the-go updates for the tranaction
	actor->PostEditMove(false);

	// our handling of properties is generic and so it may affect the actor's location, in which case we may need to update the pivot
	// positions, do this every time to catch the occasions when it is actually required but as in the other code for specific
	// controls, we only do this when editing or simulating, pivots do not appear in play sessions
	if (!GEditor->IsPlayingSessionInEditor() || GEditor->IsSimulatingInEditor())
	{
		// make the pivot track the object
		GUnrealEd->UpdatePivotLocationForSelection();
		GUnrealEd->SetPivotMovedIndependently(false);
	}

	// set a timer for completion check via the editor shared ref timer manager
	TSharedRef<FTimerManager> timeMan = GEditor->GetTimerManager();
	timeMan->SetTimer(_postChangeTimer, this, &UEngineHelper::OnPostChangeCompletion, 0.5, false);
}

// fired by a timer a specific time period following a call to the post change function, if called this marks the correct time to complete
// the undo transaction and tidy up all the pre/post change processing
void UEngineHelper::OnPostChangeCompletion()
{
	// we deal with all the pre and post change processing on the game thread
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// check if there is an active transaction, it should always be the case but check anyway
		if (GEditor->IsTransactionActive())
		{
			// follow up the interactive changed events with value set events as required, note that we allow for multiple actors with
			// one or more properties each, but in reality there is really just going to be one actor/property pair
			CompleteOnChangeActorProperties();

			// we have waited for an idle period after recent changes and so we can now close this transaction
			GEditor->EndTransaction();
		}
		// we trigger a refresh of the details window to catch at default changes
		RefreshDetailsView();
	});
}

// allows access to the property flag that chooses if we change mode to follow the selection type
bool UEngineHelper::GetSelectionChangesMode()
{
	// return the value
	return _selectionChangesMode;
}

// allows access to the property flag that chooses if we change mode to follow the selection type
void UEngineHelper::SetSelectionChangesMode(bool value)
{
	// store the value
	_selectionChangesMode = value;
}

// associates the specified target id with the property path so it may be used with the generic functions to update, reset and request
// values for parameters and menus that take the property approach
void UEngineHelper::RegisterTargetIdProperty(uint32 targetId, const FName propertyPath)
{
	// simply add to the map
	_targetIdPropertyMap.Add(targetId, propertyPath);
}

// called after some object properties have been changed to refresh the details view(s) in the unreal ui to capture
// changes to default states, it doesn't need to be called for changes that automatically handle this, it is called 
// after a short period of inactivity to avoid excessive updates
void UEngineHelper::RefreshDetailsView()
{
	// these identifiers are defined in the level editor class but will not link, they are fixed though so we simply list the names
	static const FName DetailsTabIdentifiers[] = { "LevelEditorSelectionDetails", "LevelEditorSelectionDetails2", "LevelEditorSelectionDetails3", "LevelEditorSelectionDetails4" };

	// we need the property editor to find the details views
	FPropertyEditorModule& mod = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// there are multiple possible views for details so iterate through them all 
	for (const FName& detailsTabId : DetailsTabIdentifiers)
	{
		// get the named detail view and if valid, call for a refresh
		TSharedPtr<IDetailsView> detailsView = mod.FindDetailView(detailsTabId);
		if (detailsView.IsValid())
		{
			// this view is valid so ask it to refresh
			detailsView->ForceRefresh();
		}
	}
}

// returns a plugin defined value that specifies the supported type of the given actor reference, used when the selection
// object is changed to trigger required updates
UEngineHelper::ObjectType UEngineHelper::GetSupportedTypeOfActor(AActor* actor)
{
	// choose a value to return based on our current list of supported types, or the special case none value
	if (!actor)
	{
		// no selected actor
		return ObjectType::None;
	}
	else if (actor->IsA(ALight::StaticClass()) || actor->IsA(ASkyLight::StaticClass()))
	{
		// light and sub-classes, skylights are derived from other classes but are included in this branch too
		return ObjectType::Light;
	}
	else if (actor->IsA(ACameraActor::StaticClass()))
	{
		// camera
		return ObjectType::Camera;
	}
	else if (actor->IsA(APostProcessVolume::StaticClass()))
	{
		// post process
		return ObjectType::PostProcess;
	}
	else if (actor->IsA(ALevelSequenceActor::StaticClass()))
	{
		// sequencer
		return ObjectType::Sequencer;
	}
	else if (actor->IsA(AColorCorrectRegion::StaticClass()))
	{
		// color correcting region
		return ObjectType::ColorCorrect;
	}
	else
	{
		// an actor is selected but not of a type we have specific support for, define as other
		return ObjectType::Other;
	}
}

// called to keep the last-selected caches for certain actor types up to date
void UEngineHelper::UpdateSelectionCaches(AActor* newSelection)
{
	// if there is no selection then there is nothing to update
	if (!newSelection)
	{
		// n/a so return
		return;
	}

	// we choose to keep track of the last selected instance of some actor types, check for each in turn, they are
	// stored as uprops so the local pointer will be nulled by UE if the instance is deleted in the meantime
	if (ACameraActor* camera = Cast<ACameraActor>(newSelection))
	{
		// camera
		_lastSelectedCamera = camera;
	}
}

// called when the selection of the unreal editor changes, the obj is an instance of the uselection class
void UEngineHelper::OnSelectionChanged(UObject* obj)
{
	// init to no selected actor/component
	AActor*				newActorSelection = nullptr;
	UActorComponent*	newComponentSelection = nullptr;

	// the single argument is the new selection container
	if (USelection* selection = Cast<USelection>(obj))
	{
		// in this implementation we allow actors and components to be selected, if a component is the main selection we auto-fill the
		// containing actor too, first check if the selection is an actor
		newActorSelection = selection->GetTop<AActor>();

		// it is possible for the user to actually directly select a component instead of the actor that contains it, in this context
		// we handle this checking for the actor at the top of the selection first, if it is null we then check for a component
		if (!newActorSelection)
		{
			// there is no selected actor to be found, try and get the top selected component
			UActorComponent *component = selection->GetTop<UActorComponent>();
			if (component)
			{
				// the selection is a component, we take note of this and also get its owning actor as part of the selection too 
				newComponentSelection = component;
				newActorSelection = component->GetOwner();
			}
		}
	}

	// it is possible for the selected object to be opened in an editor, in which case we have a situation where there is a selected component
	// without a containing selected owning actor, this currently breaks the plugin so we test for this and blank any partial selections
	if (newComponentSelection && !newActorSelection)
	{
		// we have a selected component but no selected actor, null both to prevent crashes
		newActorSelection = nullptr;
		newComponentSelection = nullptr;
	}

	// we track the selection of certain actor types to support extra functionality, update the caches as required
	UpdateSelectionCaches(newActorSelection);

	// update the local stores before firing off the broadcast event as the registered functions may want to call back, a selected
	// component will always have an associated selected owning actor, but an actor may be selected without a specific component
	_selectedActor = newActorSelection;
	_selectedComponent = newComponentSelection;
	UEngineHelper::SelectionChangedEvent.Broadcast();
}

// called by unreal when a pie or sie session has fully started
void UEngineHelper::PostPIEStarted(bool isSimulating)
{
	// there is an unconsistency in the unreal ui where entering a simulation leaves the selected object in place in the details window
	// but is not tagged as such with the pivots in the viewport, to make the system consistent between pie and sie we explicitly remove
	// any selection when starting a sie session, we use the post pie event and not begin pie as we need to have properly started
	// the session before removing the selection so it can potentially be reinstated when the session is left, this keeps pie and sie the same
	if (isSimulating)
	{
		// remove any selection inside the session now it has started
		DeselectAll();
	}
}

// called by unreal when a pie or sie session is ending
void UEngineHelper::OnEndPIE(bool isSimulating)
{
	// we force an update of the pivot location for any selected object as this can end up in the wrong location if moved in the sim
	// with optionally keeping the changes, doing this here tidies things up on exit of pie/sie
	GUnrealEd->UpdatePivotLocationForSelection();
}

/*
static void ReportProp(FProperty* prop)
{
	if (prop)
	{
		UE_LOG(LogTemp, Warning, TEXT("  prop name is %s"), *prop->GetName());
		UE_LOG(LogTemp, Warning, TEXT("  class description is %s"), *prop->GetClass()->GetDescription());
		UE_LOG(LogTemp, Warning, TEXT("  path name is %s"), *prop->GetPathName());
		UE_LOG(LogTemp, Warning, TEXT("  full name is %s"), *prop->GetFullName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NO prop"));
	}
}
*/

// this function is called when a panel control is being trained and a property has been changed in the unreal ui, we take
// the object and property path and fetch the target property so we can try and match this to the control that is being trained
// to make sure the mapping is consistent, if data can be generated it is signalled out and the function returns true
bool UEngineHelper::TrainingArbitrator(UObject *obj, FString &propertyPath)
{
	PropertyData*	pData = FetchPropertyData(obj, propertyPath);
	bool			trainingResult = false;

	// we do the initial look up for the property path to get access to all the property data here and to make sure it's valid
	if (pData)
	{
		// we decide what to map by looking at the control we are trying to map something to
		uint32	controlType = ControlId::controlTypeOf(_trainingPanelControl);

		// encoder => float or double or byte or int value => parameter
		// button => bool toggle or byte value or enum value => menu

		// we only try to map encoders to parameter float, double, byte or int values, in theory we could also map to enumerated menus
		// but not in this implementation
		// TODO: perhaps extend to include menus
		if (controlType == ControlType::Encoder)
		{
			// we have an encoder, check what type of property we have
			if (FFloatProperty* floatProp = CastField<FFloatProperty>(pData->Prop))
			{
				// this is a parameter based float value candidate, to complete the mapping we want to supply a base range and step
				// which we guess at from property data
				float	currentValue = floatProp->GetFloatingPointPropertyValue(pData->DataPtr),
						defaultValue = floatProp->GetFloatingPointPropertyValue(pData->DefaultDataPtr),
						min = 0,
						max = 0,
						step = 0;
				FName	name = pData->Prop->GetFName();
				FName	controlString = pData->PropertyPath;

				// as an initial stab at guessing sensible min/max/step settings we assume that a small current and default value means a small step
				if ((fabsf(currentValue) < 5) && (fabsf(defaultValue) < 5))
				{
					// use a small step with an associated small range, these arbitrary numbers have been chosen as they generally give a decent 'feel'
					min = 0;
					max = 1;
					step = 0.001;
				}
				else
				{
					// use a unity step with an associated larger range
					min = 0;
					max = 1000;
					step = 1;
				}

				// TODO - shorten the display name to better fit the target panel type
				// apply this data as the train response and return with success
				UE_LOG(LogTemp, Log, TEXT("EngineHelper: training encoder with property path '%s'"), *controlString.ToString());
				UEngineHelper::TrainResponseEvent.Broadcast(MappingDataType::Parameter, name, controlString, min, max, step);
				trainingResult = true;
			}
			else if (FDoubleProperty* doubleProp = CastField<FDoubleProperty>(pData->Prop))
			{
				// this is a parameter based double value candidate, to complete the mapping we want to supply a base range and step
				// which we guess at from property data
				double	currentValue = doubleProp->GetFloatingPointPropertyValue(pData->DataPtr),
						defaultValue = doubleProp->GetFloatingPointPropertyValue(pData->DefaultDataPtr);
				float	min = 0,
						max = 0,
						step = 0;
				FName	name = pData->Prop->GetFName();
				FName	controlString = pData->PropertyPath;

				// as an initial stab at guessing sensible min/max/step settings we assume that a small current and default value means a small step
				if ((fabs(currentValue) < 5) && (fabs(defaultValue) < 5))
				{
					// use a small step with an associated small range, these arbitrary numbers have been chosen as they generally give a decent 'feel'
					min = 0;
					max = 1;
					step = 0.001;
				}
				else
				{
					// use a unity step with an associated larger range
					min = 0;
					max = 1000;
					step = 1;
				}

				// TODO - shorten the display name to better fit the target panel type
				// apply this data as the train response and return with success
				UE_LOG(LogTemp, Log, TEXT("EngineHelper: training encoder with property path '%s'"), *controlString.ToString());
				UEngineHelper::TrainResponseEvent.Broadcast(MappingDataType::Parameter, name, controlString, min, max, step);
				trainingResult = true;
			}
			else if (FByteProperty* byteProp = CastField<FByteProperty>(pData->Prop))
			{
				// as an initial stab at guessing sensible min/max/step settings we assume a unity step with an associated large range
				float	min = 0,
						max = 1000,
						step = 1;
				FName	name = pData->Prop->GetFName();
				FName	controlString = pData->PropertyPath;

				// TODO - shorten the display name to better fit the target panel type
				// apply this data as the train response and return with success
				UE_LOG(LogTemp, Log, TEXT("EngineHelper: training encoder with property path '%s'"), *controlString.ToString());
				UEngineHelper::TrainResponseEvent.Broadcast(MappingDataType::Parameter, name, controlString, min, max, step);
				trainingResult = true;
			}
			else if (FIntProperty* intProp = CastField<FIntProperty>(pData->Prop))
			{
				// as an initial stab at guessing sensible min/max/step settings we assume a unity step with an associated medium range
				float	min = 0,
						max = 100,
						step = 1;
				FName	name = pData->Prop->GetFName();
				FName	controlString = pData->PropertyPath;

				// TODO - shorten the display name to better fit the target panel type
				// apply this data as the train response and return with success
				UE_LOG(LogTemp, Log, TEXT("EngineHelper: training encoder with property path '%s'"), *controlString.ToString());
				UEngineHelper::TrainResponseEvent.Broadcast(MappingDataType::Parameter, name, controlString, min, max, step);
				trainingResult = true;
			}
			else
			{
				// log a warning about why training can't be completed
				UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unhandled %s type ignored when training an encoder control"), *pData->Prop->GetClass()->GetDescription());
			}
		}
		else if (controlType == ControlType::Button)
		{
			// byte based, enumerated and bool properties can be mapped as custom menus
			if (CastField<FEnumProperty>(pData->Prop) || CastField<FByteProperty>(pData->Prop) || CastField<FBoolProperty>(pData->Prop))
			{
				FName	name = pData->Prop->GetFName();
				FName	controlString = pData->PropertyPath;

				// apply this data as the train response and return with success, for menus the min/max/step values are not used but need to be defined
				UE_LOG(LogTemp, Log, TEXT("EngineHelper: training button with property path '%s'"), *controlString.ToString());
				UEngineHelper::TrainResponseEvent.Broadcast(MappingDataType::Menu, name, controlString, 0, 0, 0);
				trainingResult = true;
			}
			else
			{
				// log a warning about why training can't be completed
				UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unhandled %s type ignored when training a button control"), *pData->Prop->GetClass()->GetDescription());
			}
		}
		else
		{
			// getting here means something is really messed up
			UE_LOG(LogTemp, Warning, TEXT("EngineHelper: invalid call to train an unknown control type %d"), controlType);
		}
	}

	// return with the result of this process, true means we dispatched a train response event and may consider this training session complete
	return trainingResult;
/*
	if (FFloatProperty* floatProp = CastField<FFloatProperty>(pData->_prop))
	{
		UE_LOG(LogTemp, Warning, TEXT("-- float property"));
	}
	else if (FBoolProperty* boolProp = CastField<FBoolProperty>(pData->_prop))
	{
		UE_LOG(LogTemp, Warning, TEXT("-- bool property"));
	}
	else if (FByteProperty* byteProp = CastField<FByteProperty>(pData->_prop))
	{
		UE_LOG(LogTemp, Warning, TEXT("-- byte/menu property"));
	}
	else if (FEnumProperty* enumProp = CastField<FEnumProperty>(pData->_prop))
	{
		UE_LOG(LogTemp, Warning, TEXT("-- enum/menu property"));
	}
	else if (FArrayProperty* arrayProp = CastField<FArrayProperty>(pData->_prop))
	{
		UE_LOG(LogTemp, Warning, TEXT("-- array property"));
	}
	else if (FStructProperty* structProp = CastField<FStructProperty>(pData->_prop))
	{
		UE_LOG(LogTemp, Warning, TEXT("-- struct property"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("-- OTHER property"));
	}
*/
}

// called by unreal before a property is about to be changed in the editor ui, the main difference between this and the
// property changed event is that this one supplies the entire property chain which is desirable
void UEngineHelper::OnPreObjectPropertyChanged(UObject* obj, const class FEditPropertyChain& propChain)
{
//	UE_LOG(LogTemp, Warning, TEXT(">> OnPreObjectPropertyChanged called object name is %s of class %s"), *obj->GetName(), *obj->GetClass()->GetDescription());

	// this feature is only required when we are training a control on a panel from a property change event, this is why we need the
	// full property path as we will use this to drive changes in the values later
	if (_trainingPanelControl != ControlId::Undefined)
	{
// UPDATE - we no longer filter on actors to support component based changes, early days so I've left this in place as a note
		// this pre-edit event is often generated twice for a property change, involving both the actor and a component, to simplify
		// things we only continue with actor based calls
//		if (AActor* actor = Cast<AActor>(obj))
		{
			// we want to be able to find the qualified path of object to property for this change event so we can track and modify
			// the property at a later date, we begin at the head of the property chain linked list and work our way down
			TDoubleLinkedList<FProperty*>::TDoubleLinkedListNode* chainNode = propChain.GetHead();
			FString		path;
			FProperty*	prop;

			// work through the entire chain length
			while (chainNode)
			{
				// the property itself is the node value
				prop = chainNode->GetValue();

				// sanity check before we use it
				if (prop)
				{
					// append the property name to the path with a trailing dot, note the last property will have this removed later
					path += prop->GetName() + TEXT(".");
				}
				else
				{
					// it would be useful to know if there is ever a situation where this is null so log a warning
					UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unexpected null property in pre-edit property chain"));
				}

				// simply traverse the whole chain to generate the full path to the end property
				chainNode = chainNode->GetNextNode();
			}

			// as we now allow component property change tracking we may need to insert the component name to the property path, this is needed as we change
			// property based params via the selected actor that contains the component so we need the full path including the component to be trained
			if (UActorComponent* comp = Cast<UActorComponent>(obj))
			{				
				// the object that has changed is a component inside an actor, get its name and check if the generated property path starts there
				FString	prefixName;
				comp->GetName(prefixName);
				if (!path.StartsWith(prefixName))
				{
					// the property path we currently have does not include the component itself, so prepend it now with the separator
					path = prefixName + TEXT(".") + path;
				}
			}

			// if we got any property data we need to trim the end dot and then use it
			if (!path.IsEmpty())
			{
				// take off the last character which is the redundant spacer
				path.LeftChopInline(1);

				// pass the selected actor and property defining path to the function to try and match this data to the control type we are training, it
				// returns true if a new control mapping request was dispatched effectively completing this training session, we use the actor here as it
				// will be the target for subsequent property based value changes, regardless of whether we trained via the actor or component
				if (TrainingArbitrator(_selectedActor, path))
				{
					// we have a successfull result, training is a one shot deal so we clear the target control id to turn off the
					// training processing until another session is required
					_trainingPanelControl = ControlId::Undefined;
				}
			}
		}
	}
}

// called by unreal when any object has a property changed in the editor ui
void UEngineHelper::OnObjectPropertyChanged(UObject* obj, struct FPropertyChangedEvent& event)
{
//	UE_LOG(LogTemp, Warning, TEXT(">> OnObjectPropertyChanged called object name is %s of class %s"), *obj->GetName(), *obj->GetClass()->GetDescription());

	// we only need to monitor changes made to actors in the editor context as these are the only ones we interact with, filter
	// to allow value set events (not drags) and only for actor objects or a small number of exceptions that are properties of their
	// components, this reduces the number of events as much as possible while catching all those that we want to trigger updates
	AActor* actor = Cast<AActor>(obj);
	if ((event.ChangeType == EPropertyChangeType::ValueSet) && (actor || _objectPropertyChangedExceptions.Contains(event.GetPropertyName())))
	{
		// tell any subscribers of the event of the change, no details need to be passed along
		UEngineHelper::ActorPropertyChangedEvent.Broadcast();
	}
}

// called by unreal when the current map is changed, we take this generally speaking as having a level loaded
void UEngineHelper::OnMapChanged(uint32 flags)
{
	UE_LOG(LogTemp, Log, TEXT("EngineHelper: map changed"));

	// trigger the release of all the reset data in the singleton cache instance as it doesn't carry between levels
	ResetCache::Clear();
}

// called by unreal when a sequencer instance has been created, we store this to be able to perform sequencer based control mappings
void UEngineHelper::OnSequencerCreated(TSharedRef<ISequencer> sequencerRef)
{
	// store the sequencer wrapping it in a weak pointer, this way we don't interfere with ownership but can maintain safe access
	_sequencerWeakPtr = TWeakPtr<ISequencer>(sequencerRef);
	UE_LOG(LogTemp, Log, TEXT("EngineHelper: sequencer has been created"));
}

// called to update the value for a parameter in the currently active sequencer, the call is ignored if one is not available,  if the delta 
// is zero it is treated as a reset, the new value will be sent to the hub if available
void UEngineHelper::UpdateSequencerParameter(uint32 targetId, float delta)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, delta]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// this safely accesses any sequencer we have been supplied with which will remain valid via the shared pointer until it goes out of scope
		if (TSharedPtr<ISequencer> sequencer = _sequencerWeakPtr.Pin())
		{
			// select which parameter value to update
			switch (targetId)
			{
				case TargetId::SequencerPlaybackSpeed:
					// playback speed
					{
						// in the current implementation the parameter playback speed value is always 0+ but we may set it in the sequencer as the
						// negated value for reverse play
						float newValue;

						// get the current speed of the sequencer as the starting point, we use this as the base for the change and also to detect
						// active forward or reverse playback
						float currentSpeed = sequencer->GetPlaybackSpeed();

						// a zero delta means reset
						if (delta == 0.0f)
						{
							// reset the speed to unity, note we don't use the special function that restores the playback speed
							// as it starts playing automatically which we may not want
							newValue = 1.0f;
						}
						else
						{
							// we want to apply the delta to the current speed, take the absolute value then apply the change
							newValue = fabsf(currentSpeed) + delta;
							
							// stop a negative delta taking us below zero 
							if (newValue < 0.0f)
							{
								// clip the new value to zero
								newValue = 0.0f;
							}
						}

						// if we are currently playing, we need to apply the new value to the sequencer as a negative value for reverse play
						// which we detect by looking at the sign of the existing speed
						if ((sequencer->GetPlaybackStatus() == EMovieScenePlayerStatus::Playing) && (currentSpeed < 0.0f))
						{
							// we are currently playing in reverse so make the new value to be applied also negative to continue playing in the same 
							// direction (if the current speed is positive for forward playback then we don't need to do this step)
							newValue = -newValue;
						}
							
						// we now apply the new value to the sequencer with the appropriate sign
						sequencer->SetPlaybackSpeed(newValue);

						// use the parameter value reporting event to pass this data back, taking the absolute version of what was calculated above
						ParameterValueEvent.Broadcast(targetId, fabsf(newValue));
					}
					break;
				default:
					// a command we don't know, log a warning
					UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unknown sequencer parameter 0x%08X"), targetId);
					break;
			}
		}
	});
}

// resets the value for the parameter in the currently active sequencer, the call is ignored if one is not available, the new value is reported
// back to the hub 
void UEngineHelper::ResetSequencerParameter(uint32 targetId)
{
	// use the sister function, the delta value of 0 means reset
	UpdateSequencerParameter(targetId, 0);
}

// asynchronously reports back the value of the specified parameter for the currently active sequencer, the call is ignored if one is not available
void UEngineHelper::RequestSequencerParameterValue(uint32 targetId)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// this safely accesses any sequencer we have been supplied with which will remain valid via the shared pointer until it goes out of scope
		if (TSharedPtr<ISequencer> sequencer = _sequencerWeakPtr.Pin())
		{
			// select which parameter value to update
			switch (targetId)
			{
				case TargetId::SequencerPlaybackSpeed:
					{
						// get the current playback speed, could be negative
						float speed = sequencer->GetPlaybackSpeed();

						// in the current implementation the speed param value is always positive
						speed = fabsf(speed);

						// use the parameter value reporting event to pass this data back
						ParameterValueEvent.Broadcast(targetId, speed);
					}
					break;
				default:
					// a command we don't know, log a warning
					UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unknown sequencer parameter 0x%08X"), targetId);
					break;
			}
		}
	});

}

// called to perform an action in the currently active sequencer, the call is ignored if one is not available
void UEngineHelper::SequencerAction(uint32 actionId)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, actionId]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// this safely accesses any sequencer we have been supplied with which will remain valid via the shared pointer until it goes out of scope
		if (TSharedPtr<ISequencer> sequencer = _sequencerWeakPtr.Pin())
		{
			// we could call the IsRegistered() function and then Register() if needed, but the test just checks the IsValid() status which is the first thing
			// the Register() function does anyway, so it's more efficient to just jump in and register and let that function decide if anything needs doing
			// NOTE - this call was initially in the tangent module startup code but we have had to move it here for the latest 5.1 build as they have changed
			// how objects are created and referenced at runtime so now we prepare the sequencer commands when we need to use them
			FSequencerCommands::Register();

			// get the commands to use now we know the commands must be registered and available
			const FSequencerCommands& commands = FSequencerCommands::Get();

			// select which action to perform
			switch (actionId)
			{
				case TargetId::SequencerPlayForward:
					// play forward, the dedicated command will make sure the play speed is positive, to have ui behaviour consistency we only fire
					// off the command if not already playing in the required direction
					if ((sequencer->GetPlaybackStatus() != EMovieScenePlayerStatus::Playing) || (sequencer->GetPlaybackSpeed() < 0.0f))
					{
						// we know it's not playing forward so ask it nicely
						sequencer->GetCommandBindings()->ExecuteAction(commands.PlayForward.ToSharedRef());
					}
					break;
				case TargetId::SequencerPlayReverse:
					// play reverse
					{
						// there is no specific play reverse command so we make sure the current play speed is negative
						float speed = sequencer->GetPlaybackSpeed();
						if (speed > 0.0f)
						{
							// the sequencer is currently set to play forward so flip the direction
							sequencer->SetPlaybackSpeed(-speed);
						}

						// now we just need to make sure it is playing
						if (sequencer->GetPlaybackStatus() != EMovieScenePlayerStatus::Playing)
						{
							// we know it's not playing so toggle to start reverse playback
							sequencer->GetCommandBindings()->ExecuteAction(commands.TogglePlay.ToSharedRef());
						}
					}
					break;
				case TargetId::SequencerStop:
					// stop/pause playback
					sequencer->GetCommandBindings()->ExecuteAction(commands.Pause.ToSharedRef());
					break;
				case TargetId::SequencerStepForward:
					// forward one 
					sequencer->GetCommandBindings()->ExecuteAction(commands.StepForward.ToSharedRef());
					break;
				case TargetId::SequencerStepBackward:
					// backward one 
					sequencer->GetCommandBindings()->ExecuteAction(commands.StepBackward.ToSharedRef());
					break;
				case TargetId::SequencerJumpToStart:
					// start of playback 
					sequencer->GetCommandBindings()->ExecuteAction(commands.JumpToStart.ToSharedRef());
					break;
				case TargetId::SequencerJumpToEnd:
					// end of playback 
					sequencer->GetCommandBindings()->ExecuteAction(commands.JumpToEnd.ToSharedRef());
					break;
				case TargetId::SequencerShuttleForward:
					// shuttle fwd
					sequencer->GetCommandBindings()->ExecuteAction(commands.ShuttleForward.ToSharedRef());
					break;
				case TargetId::SequencerShuttleBackward:
					// shuttle bwd
					sequencer->GetCommandBindings()->ExecuteAction(commands.ShuttleBackward.ToSharedRef());
					break;
				case TargetId::SequencerSetStartPlayback:
					// set start of playback
					sequencer->GetCommandBindings()->ExecuteAction(commands.SetStartPlaybackRange.ToSharedRef());
					break;
				case TargetId::SequencerSetEndPlayback:
					// set end of playback
					sequencer->GetCommandBindings()->ExecuteAction(commands.SetEndPlaybackRange.ToSharedRef());
					break;
				case TargetId::SequencerZoomIn:
					// zoom in view
					sequencer->GetCommandBindings()->ExecuteAction(commands.ZoomInViewRange.ToSharedRef());
					break;
				case TargetId::SequencerZoomOut:
					// zoom out view
					sequencer->GetCommandBindings()->ExecuteAction(commands.ZoomOutViewRange.ToSharedRef());
					break;
				case TargetId::SequencerZoomToFit:
					// zoom to fit view
					sequencer->GetCommandBindings()->ExecuteAction(commands.ZoomToFit.ToSharedRef());
					break;
				default:
					// a command we don't know, log a warning
					UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unknown sequencer action 0x%08X"), actionId);
					break;
			}

			// assume something was done so trigger an update
			sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChangedRefreshImmediately);
		}
	});
}

// called to perform an undo or redo action on the transaction history
void UEngineHelper::TransactionHistoryAction(uint32 actionId)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, actionId]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// select which action to perform
		switch (actionId)
		{
			case TargetId::Undo:
				// ask to undo any relevant transaction, preserving the abililty to redo it by default
				GEditor->UndoTransaction();
				break;
			case TargetId::Redo:
				// redo any relevant transaction
				GEditor->RedoTransaction();
				break;
			default:
				// a command we don't know, log a warning
				UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unknown transaction history action 0x%08X"), actionId);
				break;
		}
	});
}

// controls play or simulate in editor sessions, a request to start a session is only possible when one is not already active or
// queued, if a session is running you can switch between pie and sie types or stop it
void UEngineHelper::PlaySessionControl(uint32 actionId)
{
	// in the later versions of ue (5+) we have to do these actions on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, actionId]()
	{
		// handle by action
		switch (actionId)
		{
		case TargetId::StartPIESession:
			// in progress means pie, sie or queued to start next tick
			if (!GEditor->IsPlaySessionInProgress())
			{
				FLevelEditorModule&			mod = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				FRequestPlaySessionParams	params;

				// queue up a new pie session in the first available active viewport in the editor
				params.WorldType = EPlaySessionWorldType::PlayInEditor;
				params.DestinationSlateViewport = mod.GetFirstActiveViewport();
				GEditor->RequestPlaySession(params);
			}
			break;
		case TargetId::StartSIESession:
			// in progress means pie, sie or queued to start next tick
			if (!GEditor->IsPlaySessionInProgress())
			{
				FLevelEditorModule&			mod = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				FRequestPlaySessionParams	params;

				// queue up a new sie session in the first available active viewport in the editor
				params.WorldType = EPlaySessionWorldType::SimulateInEditor;
				params.DestinationSlateViewport = mod.GetFirstActiveViewport();
				GEditor->RequestPlaySession(params);
			}
			break;
		case TargetId::ToggleSession:
			// playing a session is true if in pie or sie now
			if (GEditor->IsPlayingSessionInEditor())
			{
				// swap between the two possible session types
				GEditor->RequestToggleBetweenPIEandSIE();
			}
			break;
		case TargetId::StopSession:
			// playing a session is true if in pie or sie now
			if (GEditor->IsPlayingSessionInEditor())
			{
				// ask to end the current session
				GEditor->RequestEndPlayMap();
			}
			break;
		default:
			// log a warning
			UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unknown play session command"));
			break;
		}
	});
}

// returns the class name of any currently selected actor or an empty name if nothing is selected
FName UEngineHelper::GetSelectedClassName()
{
	// safely check for actor class name availability
	if (_selectedActor)
	{
		// return the actor class name
		return _selectedActor->GetClass()->GetFName();
	}

	// no selection so return an empty name
	return FName();
}

// attempts to return the name of the selected component or actor, in the current implementation an actor or a specific component
// within that actor may be selected, if the full name is asked for and both are defined we return both names separated by a colon 
// character (actor first), if we don't need the full name we return just the component or actor name with the component having priority
void UEngineHelper::GetSelectedName(FString& name, bool fullName)
{
	// this is not done in the game thread as a simple solution to needing to pass back data, it seems to work ok though
	// we need to use the currently selected actor or component, if any, it is tracked by selection change events
	if (fullName && _selectedComponent)
	{
		FString actorName,
				componentName;

		// asked for a full name when a component is selected, the result is the component name preceded by the owning actor name with
		// a predefined separating character, if a component is selected we will always have an actor too
		_selectedActor->GetName(actorName);
		_selectedComponent->GetName(componentName);
		name = FString::Printf(TEXT("%s.%s"), *actorName, *componentName);
	}
	else if (_selectedComponent)
	{
		// not asked for a full name and we have a selected component
		_selectedComponent->GetName(name);
	}
	else if (_selectedActor)
	{
		// not asked for a full name and just a selected actor
		_selectedActor->GetName(name);
	}
}

// convenience wrapper to directly return the full name of the selected component or actor if there is one
FString UEngineHelper::GetSelectedName()
{
	FString result;

	// call the sister function and return the result 
	GetSelectedName(result, true);
	return result;
}

// returns a plugin defined value that specifies the supported type of the currently selected actor, it abstracts away some types as specifically 
// supported and generalises others, if there is no currently selected actor it returns the special case none value
UEngineHelper::ObjectType UEngineHelper::GetSelectedObjectType()
{
	// return the sister function value for the selected actor, if any
	return GetSupportedTypeOfActor(_selectedActor);
}

// helper function that returns the editor world
UWorld* UEngineHelper::GetEditorWorld()
{
	// fetch the world using the proper calls, note there is a GEditor->EditorWorld property but it is only set when playing in editor
	return GEditor->GetEditorWorldContext().World();
}

// helper function that safely returns the play world if it exists, will be null if not in any play mode
UWorld* UEngineHelper::GetPlayWorld()
{
/*
	// this is null if in edit mode
	FWorldContext* worldContext = GEditor->GetPIEWorldContext();
	if (worldContext)
	{
		// we are in play mode, return the world ref
		return worldContext->World();
	}
	
	// not in play mode, return with null
	return nullptr;
*/

	// this property always seems to match the results of the more complicated calls above so we will use this until we learn otherwise
	return GEditor->PlayWorld;
}

// helper function to return the world that is to be used for the current game context, this will be the play world when in play mode or
// the editor world if not, this function should be used in preference to the two functions above in most cases
UWorld* UEngineHelper::GetActiveWorld()
{
	// the play world is only set when in play mode, try that first
	UWorld* world = GetPlayWorld();
	if (!world)
	{
		// not playing so we fallback to using the editor world
		world = GetEditorWorld();
	}

	// return the current level of the chosen world
	return world;
}

// returns a persistent level reference for the current world we are using, if in play mode this is in the play world, if not
// playing this falls back to using the editor world
ULevel* UEngineHelper::GetPersistentLevel()
{
	// get the world currently in use, the play world is only set when in play mode, editor mode otherwise
	UWorld* world = GetActiveWorld();
	if (world)
	{
		// return the persistent level of the chosen world
		return world->PersistentLevel;
	}

	// the editor world should always be in place, but just in case...
	return nullptr;
}

// internal function to select an actor/component by name, the target name must be either a simple actor name or an actor/component combination
// with the actor name followed by a character-separated component name, the call removes any previously existing selection, a selection change 
// notification event is generated for all calls
void UEngineHelper::ExclusivelySelectNamedObject(const FString &targetName)
{
	// default to taking the supplied target as being just an actor name until we know otherwise
	FString actorName = targetName,
			componentName;

	// try to split the supplied name by the predefined separating character to give us both an actor and component, if there is no separator
	// the call will not change the default actor name as set above
	targetName.Split(FString(TEXT(".")), &actorName, &componentName);

	// if playing there are two overall packages, one for the editor and one for play, this leads to ambiguous matching by name
	// unless we restrict the search to one or the other, depending on the current context, we use the levels of either
	// the editor or play world to find the object we actually want to select

	// first remove any existing selection but with no change notification
	GEditor->SelectNone(false, true, false);

	// get the world currently in use, the play world is only set when in play mode, editor mode otherwise
	UWorld* world = GetActiveWorld();
	if (world)
	{
		// we want to be able to select the actor whatever level it is in, so we iterate through all listed levels
		FConstLevelIterator levelIterator = world->GetLevelIterator();
		while (levelIterator)
		{
			// get this iteration's level instance
			ULevel *level = *levelIterator;

			// search for the object using the level outer object restriction and watching for invalid deleted instances
			AActor* actor = FindObject<AActor>(level, *actorName, false);
			if (actor && IsValid(actor) && !actor->IsA(AWorldSettings::StaticClass()))
			{
				// found it, select it triggering a selection change notification
				GEditor->SelectActor(actor, true, true);
				
				// now check if we have a specific component to select within this actor, if the name is empty we skip this step
				if (!componentName.IsEmpty())
				{
					// look for the named component within this actor
					UActorComponent *component = FindObject<UActorComponent>(actor, *componentName, false);
					if (component)
					{
						// we found the component, select it too, note that the containing actor must already be selected for this to be accepted
						GEditor->SelectComponent(component, true, true);
					}
				}
				return;
			}

			// no luck so far, so iterate to the next level, if available
			levelIterator++;
		}
	}

	// if the named actor could not be found and selected then we generate the selection change
	// notification here so this is correctly done in all calling contexts as we removed any previous selection
	GEditor->NoteSelectionChange();
}

// attemps to exclusively select the named object, generating a selection change notification in unreal 
void UEngineHelper::SelectObjectByName(FString &name)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, name]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// try to select the object by name, selection change notifications are done for us by this call
		ExclusivelySelectNamedObject(name);
	});
}

// deselects all objects in the editor, triggering a selection change notification
void UEngineHelper::DeselectAll()
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// remove any existing selection with a change notification
		GEditor->SelectNone(true, true, false);
	});
}

// starts a single instance timer for the specified delay in seconds, if the timer fires then the delegate will be called
void UEngineHelper::SetActionTimer(FTimerDelegate& gate, float delay)
{
	// use of the timer manager requires the game thread
	AsyncTask(ENamedThreads::GameThread, [this, gate, delay]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we set the timer via the editor shared ref timer manager
		TSharedRef<FTimerManager> timeMan = GEditor->GetTimerManager();
		// setting a timer automatically clears any existing timer associated with the timer handle
		timeMan.Get().SetTimer(_actionTimer, gate, delay, false);
	});
}

// if there is a pending timer for any action it will be removed and will not fire
void UEngineHelper::ClearActionTimer()
{
	// use of the timer manager requires the game thread
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// clear any timer associated with the single timer handle via the editor shared ref timer manager
		TSharedRef<FTimerManager> timeMan = GEditor->GetTimerManager();
		timeMan.Get().ClearTimer(_actionTimer);
	});
}

// recursive resolver for a property path defined by an array of property name segments, the index is for the next property we are searching for and
// the supplied property and data pointer settings are for the current property as resolved up to the point of call, if the index is beyond the end
// of the segment array then we are finished and the function will return reporting a succesful call
bool UEngineHelper::ResolvePropertyRecursive(TArray<FString>& nameSegments, int32 nameIndex, FProperty *&currentProp, void *&currentDataPtr, void *&currentDefaultDataPtr)
{
	// we allow this function to handle the termination of the search by checking the index against the name array
	if (nameSegments.IsValidIndex(nameIndex))
	{
		// we have another property to search for by name
		FName propertyName = FName(*nameSegments[nameIndex]);
		
		// the current property is the parent of the one we are looking for, it may be one of several different types of container which
		// require different processing so we check for the current property type and handle as required
		if (FStructProperty* structProp = CastField<FStructProperty>(currentProp))
		{
			// the current property is a struct, we find the next step using the script struct data
			UScriptStruct* sStructProp = structProp->Struct;
			currentProp = sStructProp->FindPropertyByName(propertyName);
			if (currentProp)
			{
				// we have successfully updated the current prop to the next level, update the data pointers as well, they
				// are relative to the current data pointers				
				currentDataPtr = currentProp->ContainerPtrToValuePtr<void>(currentDataPtr);
				currentDefaultDataPtr = currentProp->ContainerPtrToValuePtrForDefaults<void>(sStructProp, currentDefaultDataPtr);

				// shift up a property segment and try the next recursion, it will stop when we hit the end of the path
				nameIndex++;
				return ResolvePropertyRecursive(nameSegments, nameIndex, currentProp, currentDataPtr, currentDefaultDataPtr);
			}
		}
		else if (FObjectProperty* objProp = CastField<FObjectProperty>(currentProp))
		{
			// the current property is actually a reference to another object, we need to get the object pointer first
			UObject* targetObj = objProp->GetObjectPropertyValue(currentDataPtr);
			if (targetObj)
			{
				// we now have the object contained by this level's property, as an object we find the property by name
				currentProp = targetObj->GetClass()->FindPropertyByName(propertyName);
				if (currentProp)
				{
					// we have successfully updated the current prop to the next level, update the data pointers as well, in this
					// context they are relative to the new object we have just found
					currentDataPtr = currentProp->ContainerPtrToValuePtr<void>(targetObj);
					currentDefaultDataPtr = currentProp->ContainerPtrToValuePtrForDefaults<void>(targetObj->GetClass(), targetObj->GetArchetype());

					// shift up a property segment and try the next recursion, it will stop when we hit the end of the path
					nameIndex++;
					return ResolvePropertyRecursive(nameSegments, nameIndex, currentProp, currentDataPtr, currentDefaultDataPtr);
				}
			}
		}

		// getting here means the current property is not a container type that we support, log a warning and return with fail
		UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unhandled property container type %s"), currentProp? *currentProp->GetClass()->GetName(): TEXT("<nullptr>"));
		return false;
	}

	// getting here means we are at the end of the property path and so have successfully completed the resolving process
	return true;
}

// searches the object's properties for one that matches the names supplied as an array of path segments, if a match is found the function returns 
// true and the property instance is passed back along with the correctly calculated data value pointer that may be used to get/set values
// according to the type of the property as well as the default data pointer for resets
bool UEngineHelper::ResolveProperty(UObject *rootObject, TArray<FString>& nameSegments, FProperty *&outProperty, void *&outDataPtr, void *&outDefaultDataPtr)
{
	// start the search with the actor itself for the top level property
	UObject*	target = rootObject;
	int32		nameIndex = 0;
	FName		propertyName = FName(*nameSegments[nameIndex]);
	outProperty = target->GetClass()->FindPropertyByName(propertyName);

	// if this was not successful then check the if we have access to a root component instead
	if (!outProperty)
	{
		// no property hit, if the supplied object is an actor instance we try its root component 
		if (AActor * actor = Cast<AActor>(rootObject))
		{
			// switch to using the root component as the target and try again
			target = actor->GetRootComponent();
			outProperty = target->GetClass()->FindPropertyByName(propertyName);
		}
	}

	// if we have an initial property then we may continue
	if (outProperty)
	{
		// prepare the data and default pointers for the top level property as we'll need these
		outDataPtr = outProperty->ContainerPtrToValuePtr<void>(target);
		outDefaultDataPtr = outProperty->ContainerPtrToValuePtrForDefaults<void>(target->GetClass(), target->GetArchetype());

		// shift up a property segment and call the recursive resolvers, it will stop when we hit the end of the path
		nameIndex++;
		return ResolvePropertyRecursive(nameSegments, nameIndex, outProperty, outDataPtr, outDefaultDataPtr);
	}

	// getting here means no matching property was located, returning false means the out values are invalid
	return false;
}

// convenience function that returns a property pointer for the given property path specifically for the supplied 
// object, the path may have any number of property name segments separated by periods, the data is cached as much
// as possible, may be null if not available
FProperty* UEngineHelper::FetchProperty(UObject* obj, const FName propertyPath)
{
	// use the sister function with a name path, this is just a convenience wrapper
	if (PropertyData* pData = FetchPropertyData(obj, propertyPath))
	{
		// we only want the cached property for this call
		return pData->Prop;
	}

	// getting here means the data is not available
	return nullptr;
}

// returns a helper data container specifically for the supplied object identified via a property path registered with the given
// target id, the data is cached as much as possible, may be null if not available
PropertyData* UEngineHelper::FetchPropertyData(UObject* obj, uint32 targetId)
{
	// look up the registered property path in the map, it may be null if it hasn't been registered
	if (FName* propertyPath = _targetIdPropertyMap.Find(targetId))
	{ 
		// use the sister function with the property path to fetch the data container
		return FetchPropertyData(obj, *propertyPath);
	}

	// getting here means the data is not available
	UE_LOG(LogTemp, Warning, TEXT("EngineHelper: no property data found for 0x%08X"), targetId);
	return nullptr;
}

// returns a helper data container for the given property path specifically for the supplied object, the path may have any
// number of property name segments separated by periods, the data is cached as much as possible, may be null if not available
PropertyData* UEngineHelper::FetchPropertyData(UObject* obj, const FString& propertyPath)
{
	// use the sister function with a name path, this is just a convenience wrapper
	return FetchPropertyData(obj, FName(*propertyPath));
}

// returns a helper data container for the given property path specifically for the supplied object, the path may have any
// number of property name segments separated by periods, the data is cached as much as possible, may be null if not available
PropertyData* UEngineHelper::FetchPropertyData(UObject* obj, const FName propertyPath)
{
	// use the id of the actor for the cache key, note these can actually be reused so they are really only unique for the 
	// lifetime of the objects, should perhaps invalidate the cache every now and then
	uint32					key = obj->GetUniqueID();
	TArray<PropertyData*>	matches;

	// we cache all properties for the same object under the object's unique id, so there may be multiple hits in this first step
	_propertyDataCache.MultiFind(key, matches, false);
	
	// scan through this data set for the actual property path we are after
	for (PropertyData* pData : matches)
	{
		// a simple string check is all that is required, we store the path data as fnames so the comparisons are very quick
		if (pData->PropertyPath == propertyPath)
		{
			// found a hit, return the cached data
			return pData;
		}
	}

	// getting here means there is nothing cached for this actor instance/property, prepare to make a new entry by taking the 
	// property path and splitting into a segment strings if relevant, they are separated by period characters
	TArray<FString> nameSegments;
	int32 count = propertyPath.ToString().ParseIntoArray(nameSegments, TEXT("."));

	// we expect one or more segements from the parse
	if (count > 0)
	{
		FProperty*	prop = nullptr;
		void*		dataPtr = nullptr;
		void*		defaultDataPtr = nullptr;

		// we have the property names so we need to turn this into a property instance that we can use to get/set actual values, call
		// a helper function to look it up for us, if it returns true then we have the property and data pointer that we need
		if (ResolveProperty(obj, nameSegments, prop, dataPtr, defaultDataPtr))
		{
			// we have everything we need, so create a new data instance and add to the cache
			PropertyData* newData = new PropertyData(propertyPath, prop, dataPtr, defaultDataPtr);
			_propertyDataCache.Add(key, newData);
			// return this as the call result, any subsequent calls will get this same instance from the cache
			return newData;
		}
	}

	// getting here means there is no available property data for the specified actor/property set up
	UE_LOG(LogTemp, Warning, TEXT("EngineHelper: no property data found for '%s'"), *propertyPath.ToString());
	return nullptr;
}

// looks for a parameter with the same property path as the supplied control string and applies the delta if the property is of
// the correct type, the target id will be a locally assigned unique one that can be used to differentiate the custom params, if 
// the delta is zero it is treated as a reset, the new value will be sent to the hub if available
void UEngineHelper::UpdateCustomParameter(uint32 targetId, const FName controlString, float delta)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, controlString, delta]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the custom control string and apply the delta, if the delta is zero it is treated as a reset
			float			newValue;
			PropertyData*	pData = FetchPropertyData(_selectedActor, controlString);

			// check for availability and access the data if possible
			if (pData && pData->AdjustParameterValue(this, _selectedActor, targetId, delta, newValue))
			{
				// value changed, we don't request the new value to be sent back to the hub using the sister function as that
				// is rather inefficient with the selection/actor/property calls so the adjust function gives us the value
				CustomParameterValueEvent.Broadcast(controlString, newValue);
			}
		}
	});
}

// resets the property with the name as supplied on the currently selected object to the standard default if available and reports back
// the new value to the hub the target id will be a locally assigned unique one that can be used to differentiate the custom params
void UEngineHelper::ResetCustomParameter(uint32 targetId, const FName controlString)
{
	// use the sister function, the delta value of 0 means reset
	UpdateCustomParameter(targetId, controlString, 0);
}

// asynchronously reports the value of the named property of the current selection back via the custom parameter value event
void UEngineHelper::RequestCustomParameterValue(const FName controlString)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, controlString]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the custom control string so we can access the value
			float			value;
			PropertyData*	pData = FetchPropertyData(_selectedActor, controlString);

			// check for availability and access the data if possible
			if (pData && pData->GetParameterValue(value))
			{
				// use the custom parameter value reporting event to pass this data back
				CustomParameterValueEvent.Broadcast(controlString, value);
			}
		}
	});
}

// steps the enumerated menu as defined by the custom control string of the selected actor, asynchronously returning the new 
// string value via the associated custom menu string event, a step of zero triggers a reset
void UEngineHelper::StepCustomMenu(const FName controlString, int32 step)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, controlString, step]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the custom control string so we can process the value
			PropertyData*	pData = FetchPropertyData(_selectedActor, controlString);
			FString			strValue;

			// check for availability and process the data if possible
			if (pData && pData->AdjustMenuValue(this, _selectedActor, step, strValue))
			{
				// use the custom menu string reporting event to pass this data back
				CustomMenuStringEvent.Broadcast(controlString, strValue);
			}
		}
	});
}

// resets the enumerated menu as defined by the custom control string of the selected actor, asynchronously returning the new 
// string value via the associated custom menu string event, a step of zero triggers a reset
void UEngineHelper::ResetCustomMenu(const FName controlString)
{
	// use the sister function, the step value of 0 means a reset
	StepCustomMenu(controlString, 0);
}

// asynchronously reports the value of the named property of the current selection back via the custom menu string event
void UEngineHelper::RequestCustomMenuString(const FName controlString)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, controlString]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the custom control string so we can access the value
			FString			strValue;
			PropertyData*	pData = FetchPropertyData(_selectedActor, controlString);

			// check for availability and access the data if possible
			if (pData && pData->GetMenuValueAsString(strValue))
			{
				// use the custom menu string reporting event to pass this data back
				CustomMenuStringEvent.Broadcast(controlString, strValue);
			}
		}
	});
}

// performs a custom action based on the custom control string and on/off state flag
void UEngineHelper::OnCustomAction(const FName controlString, bool state)
{
	// we don't support custom actions in the current implementation
}

// returns true if there is a selected actor and it is of the templated class type
template<class T>
FORCEINLINE bool UEngineHelper::SelectedActorIsA() const
{
	// we need a current selection of this type to return with success
	return (_selectedActor && _selectedActor->IsA(T::StaticClass()));
}

// uses the target id to look up a registered property name and applies the delta if the property is of the correct type, this
// call applies range clipping if necessary, if the delta is zero it is treated as a reset, the new value will be sent to the hub if available
void UEngineHelper::UpdateParameterProperty(uint32 targetId, float minimum, float maximum, float delta)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, minimum, maximum, delta]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the target id and apply the delta, if the delta is zero it is treated as a reset
			float			newValue;
			PropertyData*	pData = FetchPropertyData(_selectedActor, targetId);

			// check for property availability
			if (pData)
			{
				// this call allows for range clipping to the supplied values
				pData->Clipped = true;
				pData->Minimum = minimum;
				pData->Maximum = maximum;

				// access the data if possible
				if (pData->AdjustParameterValue(this, _selectedActor, targetId, delta, newValue))
				{
					// value changed, we don't request the new value to be sent back to the hub using the sister function as that
					// is rather inefficient with the selection/actor/property calls so the adjust function gives us the value
					ParameterValueEvent.Broadcast(targetId, newValue);
				}
			}
		}
	});
}

// uses the target id to look up a registered property name and applies the delta if the property is of the correct type, if 
// the delta is zero it is treated as a reset, the new value will be sent to the hub if available
void UEngineHelper::UpdateParameterProperty(uint32 targetId, float delta)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, delta]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the target id and apply the delta, if the delta is zero it is treated as a reset
			float			newValue;
			PropertyData*	pData = FetchPropertyData(_selectedActor, targetId);

			// check for property availability
			if (pData)
			{
				// this call disables range clipping
				pData->Clipped = false;

				// access the data if possible
				if (pData->AdjustParameterValue(this, _selectedActor, targetId, delta, newValue))
				{
					// value changed, we don't request the new value to be sent back to the hub using the sister function as that
					// is rather inefficient with the selection/actor/property calls so the adjust function gives us the value
					ParameterValueEvent.Broadcast(targetId, newValue);
				}
			}
		}
	});
}

// uses the target id to look up a registered property name on the currently selected object to reset to the standard default and reports back
// the new value to the hub
void UEngineHelper::ResetParameterProperty(uint32 targetId)
{
	// use the sister function, the delta value of 0 means reset
	UpdateParameterProperty(targetId, 0);
}

// asynchronously reports the value of the property registered with the target id for the current selection back via the parameter value event
void UEngineHelper::RequestParameterPropertyValue(uint32 targetId)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the target id so we can access the value
			float			value;
			PropertyData*	pData = FetchPropertyData(_selectedActor, targetId);

			// check for availability and access the data if possible
			if (pData && pData->GetParameterValue(value))
			{
				// use the parameter value reporting event to pass this data back
				ParameterValueEvent.Broadcast(targetId, value);
			}
		}
	});
}

// uses the target id to look up a registered boolean property name on the currently selected object and sets the specified state, asynchronously
// returning the new string value via the associated menu string event
void UEngineHelper::SetMenuProperty(uint32 targetId, bool value)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, value]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the target id so we can process the value
			PropertyData*	pData = FetchPropertyData(_selectedActor, targetId);
			FString			strValue;

			// check for availability and process the data if possible
			if (pData && pData->SetMenuValue(this, _selectedActor, value, strValue))
			{
				// use the menu string reporting event to pass this data back
				MenuStringEvent.Broadcast(targetId, strValue);
			}
		}
	});
}

// uses the target id to look up a registered property name on the currently selected object and steps the enumerated menu, asynchronously returning the new 
// string value via the associated menu string event, a step of zero triggers a reset
void UEngineHelper::StepMenuProperty(uint32 targetId, int32 step)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, step]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the target id so we can process the value
			PropertyData*	pData = FetchPropertyData(_selectedActor, targetId);
			FString			strValue;

			// check for availability and process the data if possible
			if (pData && pData->AdjustMenuValue(this, _selectedActor, step, strValue))
			{
				// use the menu string reporting event to pass this data back
				MenuStringEvent.Broadcast(targetId, strValue);
			}
		}
	});
}

// uses the target id to look up a registered property name on the currently selected object to reset to the standard default and reports back
// the new value to the hub
void UEngineHelper::ResetMenuProperty(uint32 targetId)
{
	// use the sister function, the step value of 0 means a reset
	StepMenuProperty(targetId, 0);
}

// asynchronously reports the value of the property registered with the target id for the current selection back via the menu string event
void UEngineHelper::RequestMenuPropertyString(uint32 targetId)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// try to find the property that matches the target id so we can access the value
			FString			strValue;
			PropertyData*	pData = FetchPropertyData(_selectedActor, targetId);

			// check for availability and access the data if possible
			if (pData && pData->GetMenuValueAsString(strValue))
			{
				// use the menu string reporting event to pass this data back
				MenuStringEvent.Broadcast(targetId, strValue);
			}
		}
	});
}

// uses the target id to select an hsv component value and applies the delta, if the delta is zero it is treated as a 
// reset, the new value will be sent to the hub if available
void UEngineHelper::UpdateLightHSV(uint32 targetId, float delta)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, delta]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// only applicable to light actors
		if (ALight* light = Cast<ALight>(_selectedActor))
		{
			FLinearColor	hsvColor;
			float			value;

			// we use a cache for colour handling here as rounding in the various conversions make the panel controls feel really stuttery 
			// with small increments, the cache will always return a valid hsv colour for the light instance, it may be cached if valid
			_hsvColorCache.getHSV(light, hsvColor);

			// choose which hsv component is required from the target id
			switch (targetId)
			{
				case TargetId::LightColorHue:
					// hue (r) reset or update
					if (!HandleResetOrUpdate(hsvColor.R, 0.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					hsvColor.R = (delta == 0.0)? 0.0: hsvColor.R + delta;
					// apply some sensible limits to the new value, we want the hue (r) to be in the range 0-359
					hsvColor.R = (hsvColor.R < 0) ? hsvColor.R + 360.0 : hsvColor.R;
					hsvColor.R = (hsvColor.R >= 360.0) ? hsvColor.R - 360.0 : hsvColor.R;
					// track the value to send to the hub
					value = hsvColor.R;
					break;
				case TargetId::LightColorSaturation:
					// saturation (g) reset or update
					if (!HandleResetOrUpdate(hsvColor.G, 0.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					hsvColor.G = (delta == 0.0) ? 0.0 : hsvColor.G + delta;
					// we want the saturation (g) to be 0-1
					hsvColor.G = (hsvColor.G < 0) ? 0.0 : hsvColor.G;
					hsvColor.G = (hsvColor.G > 1.0) ? 1.0 : hsvColor.G;
					// track the value to send to the hub
					value = hsvColor.G;
					break;
				case TargetId::LightColorValue:
					// value (b) reset or update
					if (!HandleResetOrUpdate(hsvColor.B, 1.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					hsvColor.B = (delta == 0.0) ? 1.0 : hsvColor.B + delta;
					// we want the value (b) to be 0-1
					hsvColor.B = (hsvColor.B < 0) ? 0.0 : hsvColor.B;
					hsvColor.B = (hsvColor.B > 1.0) ? 1.0 : hsvColor.B;
					// track the value to send to the hub
					value = hsvColor.B;
					break;
				default:
					// unknown id, return
					return;
			}

			// update our cache with the latest hsv colour
			_hsvColorCache.setHSV(hsvColor);

			// convert the hsv back to rgb to apply to the light
			FLinearColor rgbColor = hsvColor.HSVToLinearRGB();

			// we need the associated property for change events
			FProperty *targetProperty = FetchProperty(_selectedActor, FName("LightColor"));

			// mark the light as being changed and set the new colour
			OnPreChange(light, targetProperty, "Light HSV");
			light->SetLightColor(rgbColor);
			OnPostChange(light, targetProperty);

			// finally send back the new value to the hub via the parameter value event
			ParameterValueEvent.Broadcast(targetId, value);
		}
	});
}

// resets the hsv colour channel assosciated with the target id parameter for any currently selected light, the new value will be sent to the hub if available
void UEngineHelper::ResetLightHSV(uint32 targetId)
{
	// call the sister function, a delta of zero indicates reset
	UpdateLightHSV(targetId, 0);
}

// asynchronously reports the current value of the hsv colour channel associated with the target id for the currently selected light
// back via the parameter value event
void UEngineHelper::RequestLightHSV(uint32 targetId)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// only applicable to light actors
		if (ALight* light = Cast<ALight>(_selectedActor))
		{
			// get the light colour in hsv format, when hsv the fields are interpreted as r = hue, g = sat, b = value
			FLinearColor rgbColor = light->GetLightColor();
			FLinearColor hsvColor = rgbColor.LinearRGBToHSV();
			
			// choose which value to return from the target id
			switch (targetId)
			{
				case TargetId::LightColorHue:
					// hue (r) value
					ParameterValueEvent.Broadcast(targetId, hsvColor.R);
					break;
				case TargetId::LightColorSaturation:
					// saturation (g) value
					ParameterValueEvent.Broadcast(targetId, hsvColor.G);
					break;
				case TargetId::LightColorValue:
					// value (b) value
					ParameterValueEvent.Broadcast(targetId, hsvColor.B);
					break;
				default:
					// unknown id, return
					return;
			}
		}
	});
}

// adjusts the selected light colour using the supplied vector as x/y/z deltas to be applied to the yu pip 
// and v slider in a colour wheel representation of the colourspace using the target id to determine which
// to process, a zero adjustment triggers a reset
void UEngineHelper::UpdateLightXYZ(uint32 targetId, float delta)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, delta]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// only applicable to light actors
		if (ALight* light = Cast<ALight>(_selectedActor))
		{
			FLinearColor hsvColor;

			// we use a cache for colour handling here as rounding in the various conversions make the panel controls feel really stuttery 
			// with small increments, the cache will always return a valid hsv colour for the light instance, it may be cached if valid
			_hsvColorCache.getHSV(light, hsvColor);

			// convert the polar co-ord of hue as the angle (converted to radians) and saturation as the amplitude
			// to the corresponding x/y which are normalised
			float	xPos,
					yPos,
					angle = FMath::DegreesToRadians(hsvColor.R);
			FMath::PolarToCartesian(hsvColor.G, angle, xPos, yPos);

			// apply the delta or reset by which xyz component is required from the target id
			switch (targetId)
			{
				case TargetId::LightColorX:
					// x pos reset or change
					if (!HandleResetOrUpdate(xPos, 0.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
					break;
				case TargetId::LightColorY:
					// y pos reset or change, note y is inverted
					if (!HandleResetOrUpdate(yPos, 0.0, -delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
					break;
				case TargetId::LightColorZ:
					// z reset or change is applied to the color instance directly
					if (!HandleResetOrUpdate(hsvColor.B, 1.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
					break;
				default:
					// not relevant to this context, log a warning and abort
					UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unhandled XYZ component"));
					return;
			}

			// convert back to polar to get a new hue and saturation setting
			float	hue,
					saturation;
			FMath::CartesianToPolar(xPos, yPos, saturation, hue);

			// store the hue (converted back to degrees) and saturation in the hsv store as the rg channels
			hsvColor.R = FMath::RadiansToDegrees(hue);
			hsvColor.G = saturation;

			// apply some sensible limits to the new values, we want the hue (r) to be in the range 0-359
			hsvColor.R = (hsvColor.R < 0) ? hsvColor.R + 360.0 : hsvColor.R;
			hsvColor.R = (hsvColor.R >= 360.0) ? hsvColor.R - 360.0 : hsvColor.R;

			// we want the saturation (g) to be 0-1
			hsvColor.G = (hsvColor.G < 0) ? 0.0 : hsvColor.G;
			hsvColor.G = (hsvColor.G > 1.0) ? 1.0 : hsvColor.G;

			// we want the value (b) to be 0-1
			hsvColor.B = (hsvColor.B < 0) ? 0.0 : hsvColor.B;
			hsvColor.B = (hsvColor.B > 1.0) ? 1.0 : hsvColor.B;

			// update our cache with the latest hsv colour
			_hsvColorCache.setHSV(hsvColor);

			// generate a new rgb colour to apply
			FLinearColor rgbColor = hsvColor.HSVToLinearRGB();

			// we need the associated property for change events
			FProperty *targetProperty = FetchProperty(_selectedActor, FName("LightColor"));

			// mark the light as being changed and set the new colour
			OnPreChange(light, targetProperty, "Light Color Wheel");
			light->SetLightColor(rgbColor);
			OnPostChange(light, targetProperty);

			// note - we don't send the x/y/z values back to the hub as they don't really have any meaning although
			// we could maybe consider sending back the updated hsv values, but that means mapping between the target ids
		}
	});
}

// resets the xyz value associated with the encoded target id parameter for the currently selected light
void UEngineHelper::ResetLightXYZ(uint32 targetId)
{
	// call the sister function, a delta of zero indicates reset
	UpdateLightXYZ(targetId, 0);
}

// helper function that looks up the settings section and targeted value vector for the specified post process volume that is relevant 
// to the encoded target id and returns a pointer to the vector instance, this can be used to read and modify the values, returns null if not found
FVector4* UEngineHelper::SelectPostProcessSettingsVector(uint32 targetId, APostProcessVolume* volume)
{
	// get the settings by reference from the post process volume so we may modify values
	FPostProcessSettings& settings = volume->Settings;

	// choose which settings section is relevant to this encoded target id
	switch (TargetId::GradingSettingsSection(targetId))
	{
		case TargetId::GRADING_GlobalSaturation:
			// global > saturation
			return &settings.ColorSaturation;
		case TargetId::GRADING_GlobalContrast:
			// global > contrast
			return &settings.ColorContrast;
		case TargetId::GRADING_GlobalGamma:
			// global > gamma
			return &settings.ColorGamma;
		case TargetId::GRADING_GlobalGain:
			// global > gain
			return &settings.ColorGain;
		case TargetId::GRADING_GlobalOffset:
			// global > offset
			return &settings.ColorOffset;
		case TargetId::GRADING_ShadowsSaturation:
			// shadows > saturation
			return &settings.ColorSaturationShadows;
		case TargetId::GRADING_ShadowsContrast:
			// shadows > contrast
			return &settings.ColorContrastShadows;
		case TargetId::GRADING_ShadowsGamma:
			// shadows > gamma
			return &settings.ColorGammaShadows;
		case TargetId::GRADING_ShadowsGain:
			// shadows > gain
			return &settings.ColorGainShadows;
		case TargetId::GRADING_ShadowsOffset:
			// shadows > offset
			return &settings.ColorOffsetShadows;
		case TargetId::GRADING_MidtonesSaturation:
			// midtones > saturation
			return &settings.ColorSaturationMidtones;
		case TargetId::GRADING_MidtonesContrast:
			// midtones > contrast
			return &settings.ColorContrastMidtones;
		case TargetId::GRADING_MidtonesGamma:
			// midtones > gamma
			return &settings.ColorGammaMidtones;
		case TargetId::GRADING_MidtonesGain:
			// midtones > gain
			return &settings.ColorGainMidtones;
		case TargetId::GRADING_MidtonesOffset:
			// midtones > offset
			return &settings.ColorOffsetMidtones;
		case TargetId::GRADING_HighlightsSaturation:
			// highlights > saturation
			return &settings.ColorSaturationHighlights;
		case TargetId::GRADING_HighlightsContrast:
			// highlights > contrast
			return &settings.ColorContrastHighlights;
		case TargetId::GRADING_HighlightsGamma:
			// highlights > gamma
			return &settings.ColorGammaHighlights;
		case TargetId::GRADING_HighlightsGain:
			// highlights > gain
			return &settings.ColorGainHighlights;
		case TargetId::GRADING_HighlightsOffset:
			// highlights > offset
			return &settings.ColorOffsetHighlights;
		default:
			// not relevant to this context, log a warning and abort
			UE_LOG(LogTemp, Warning, TEXT("EngineHelper: post process volume unhandled target id 0x%08X"), targetId);
			return nullptr;
	}
}

// helper function that looks up the settings section and targeted value vector for the specified color correct region that is relevant
// to the encoded target id  and returns a pointer to the vector instance, this can be used to read and modify the values, returns null if not found
FVector4* UEngineHelper::SelectColorCorrectSettingsVector(uint32 targetId, AColorCorrectRegion* region)
{
	// get the settings by reference from the color correct region so we may modify values
	FColorGradingSettings& settings = region->ColorGradingSettings;

	// choose which settings section is relevant to this encoded target id
	switch (TargetId::GradingSettingsSection(targetId))
	{
		case TargetId::GRADING_GlobalSaturation:
			// global > saturation
			return &settings.Global.Saturation;
		case TargetId::GRADING_GlobalContrast:
			// global > contrast
			return &settings.Global.Contrast;
		case TargetId::GRADING_GlobalGamma:
			// global > gamma
			return &settings.Global.Gamma;
		case TargetId::GRADING_GlobalGain:
			// global > gain
			return &settings.Global.Gain;
		case TargetId::GRADING_GlobalOffset:
			// global > offset
			return &settings.Global.Offset;
		case TargetId::GRADING_ShadowsSaturation:
			// shadows > saturation
			return &settings.Shadows.Saturation;
		case TargetId::GRADING_ShadowsContrast:
			// shadows > contrast
			return &settings.Shadows.Contrast;
		case TargetId::GRADING_ShadowsGamma:
			// shadows > gamma
			return &settings.Shadows.Gamma;
		case TargetId::GRADING_ShadowsGain:
			// shadows > gain
			return &settings.Shadows.Gain;
		case TargetId::GRADING_ShadowsOffset:
			// shadows > offset
			return &settings.Shadows.Offset;
		case TargetId::GRADING_MidtonesSaturation:
			// midtones > saturation
			return &settings.Midtones.Saturation;
		case TargetId::GRADING_MidtonesContrast:
			// midtones > contrast
			return &settings.Midtones.Contrast;
		case TargetId::GRADING_MidtonesGamma:
			// midtones > gamma
			return &settings.Midtones.Gamma;
		case TargetId::GRADING_MidtonesGain:
			// midtones > gain
			return &settings.Midtones.Gain;
		case TargetId::GRADING_MidtonesOffset:
			// midtones > offset
			return &settings.Midtones.Offset;
		case TargetId::GRADING_HighlightsSaturation:
			// highlights > saturation
			return &settings.Highlights.Saturation;
		case TargetId::GRADING_HighlightsContrast:
			// highlights > contrast
			return &settings.Highlights.Contrast;
		case TargetId::GRADING_HighlightsGamma:
			// highlights > gamma
			return &settings.Highlights.Gamma;
		case TargetId::GRADING_HighlightsGain:
			// highlights > gain
			return &settings.Highlights.Gain;
		case TargetId::GRADING_HighlightsOffset:
			// highlights > offset
			return &settings.Highlights.Offset;
		default:
			// not relevant to this context, log a warning and abort
			UE_LOG(LogTemp, Warning, TEXT("EngineHelper: color correct region unhandled target id 0x%08X"), targetId);
			return nullptr;
	}
}

// helper function that looks up the settings section and targeted value vector for the specified camera that is relevant 
// to the encoded target id and returns a pointer to the vector instance, this can be used to read and modify the 
// values, returns null if not found
FVector4* UEngineHelper::SelectCameraPostProcessSettingsVector(uint32 targetId, ACameraActor* camera)
{
	// get the settings by reference from the camera component so we may modify values
	FPostProcessSettings& settings = camera->GetCameraComponent()->PostProcessSettings;

	// choose which settings section is relevant to this encoded target id
	switch (TargetId::GradingSettingsSection(targetId))
	{
		case TargetId::GRADING_GlobalSaturation:
			// global > saturation
			return &settings.ColorSaturation;
		case TargetId::GRADING_GlobalContrast:
			// global > contrast
			return &settings.ColorContrast;
		case TargetId::GRADING_GlobalGamma:
			// global > gamma
			return &settings.ColorGamma;
		case TargetId::GRADING_GlobalGain:
			// global > gain
			return &settings.ColorGain;
		case TargetId::GRADING_GlobalOffset:
			// global > offset
			return &settings.ColorOffset;
		case TargetId::GRADING_ShadowsSaturation:
			// shadows > saturation
			return &settings.ColorSaturationShadows;
		case TargetId::GRADING_ShadowsContrast:
			// shadows > contrast
			return &settings.ColorContrastShadows;
		case TargetId::GRADING_ShadowsGamma:
			// shadows > gamma
			return &settings.ColorGammaShadows;
		case TargetId::GRADING_ShadowsGain:
			// shadows > gain
			return &settings.ColorGainShadows;
		case TargetId::GRADING_ShadowsOffset:
			// shadows > offset
			return &settings.ColorOffsetShadows;
		case TargetId::GRADING_MidtonesSaturation:
			// midtones > saturation
			return &settings.ColorSaturationMidtones;
		case TargetId::GRADING_MidtonesContrast:
			// midtones > contrast
			return &settings.ColorContrastMidtones;
		case TargetId::GRADING_MidtonesGamma:
			// midtones > gamma
			return &settings.ColorGammaMidtones;
		case TargetId::GRADING_MidtonesGain:
			// midtones > gain
			return &settings.ColorGainMidtones;
		case TargetId::GRADING_MidtonesOffset:
			// midtones > offset
			return &settings.ColorOffsetMidtones;
		case TargetId::GRADING_HighlightsSaturation:
			// highlights > saturation
			return &settings.ColorSaturationHighlights;
		case TargetId::GRADING_HighlightsContrast:
			// highlights > contrast
			return &settings.ColorContrastHighlights;
		case TargetId::GRADING_HighlightsGamma:
			// highlights > gamma
			return &settings.ColorGammaHighlights;
		case TargetId::GRADING_HighlightsGain:
			// highlights > gain
			return &settings.ColorGainHighlights;
		case TargetId::GRADING_HighlightsOffset:
			// highlights > offset
			return &settings.ColorOffsetHighlights;
		default:
			// not relevant to this context, log a warning and abort
			UE_LOG(LogTemp, Warning, TEXT("EngineHelper: camera post process unhandled target id 0x%08X"), targetId);
			return nullptr;
	}
}

// helper function used by some updates to choose either an appropriate reset value or the adjusted value depending on the 
// calling context, a zero delta means reset so we choose a single tap cached value or a double tap supplied value to
// apply, if the single tap has no cached value the function will return false to show we cannot continue, if there is a valid
// delta we will cache the current value before adjusting (on first change only), the function will return true if the value is to be set
template<typename T1, typename T2>
bool UEngineHelper::HandleResetOrUpdate(T1& value, T2 defaultValue, float delta, uint32 targetId, AActor* actor)
{
	// first check for a zero delta meaning a reset
	if (delta == 0)
	{
		// the reset action was registered when it first came in - single tap means try for a cached value, double tap means a preset default as supplied
		if (ResetCache::IsSingleTap(targetId))
		{
			// single tap reset to last cached, there may not be a cached value in which case we stop here
			if (!ResetCache::GetValue(targetId, actor, &value))
			{
				// no cached value so nothing to reset to, returning false signals to the caller we stop processing
				return false;
			}
		}
		else
		{
			// a double tap reset sets the value to the one supplied
			value = defaultValue;
		}
	}
	else
	{
		// not a reset - apply delta, but first we may need to cache the pre-change current value, only the first value is stored as a potential
		// single tap reset in the cache, subsequent calls for the same target id and actor combo will be ignored
		ResetCache::CacheInitialValue(targetId, actor, value);
		value += delta;
	}

	// return true to say the value has been set to the required value and can be applied
	return true;
}

// applies a delta change to the currently selected post processing volume, color correct region or camera actor for HSV changes for all sections
// and settings that are supported by the encoded target ids, if the delta is zero it is treated as a reset, the new value will be sent 
// to the hub if available
void UEngineHelper::UpdateGradingHSV(uint32 targetId, float delta)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, delta]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		FVector4*	settingsVector = nullptr;		// this is used to detect a valid set of data including the property path and description
		FName		propertyPath;
		FString		description;

		// only applicable to post process volume, color correct region or camera actors with an appropriate target control id, each having their own settings property
		// but with common functionality, we use a pointer to the actual target setting vector as we can't assign references on the fly, we choose which settings 
		// section is relevant to this target id, we use this to read the current values and then to apply any changes, it is encoded in the target id
		if (TargetId::IsPostProcessGrading(targetId))
		{
			// the target control is ppv based, only applicable to selected post process volume actors
			if (APostProcessVolume* volume = Cast<APostProcessVolume>(_selectedActor))
			{
				// fetch the settings vector for this volume and target id, may be null if not available
				settingsVector = SelectPostProcessSettingsVector(targetId, volume);
				propertyPath = FName("Settings");
				description = FString("PostProcess HSV");
			}
		}
		else if (TargetId::IsColorCorrectGrading(targetId))
		{
			// the target control is ccr based, only applicable to selected color correct region actors
			if (AColorCorrectRegion* region = Cast<AColorCorrectRegion>(_selectedActor))
			{
				// fetch the settings vector for this region and target id, may be null if not available
				settingsVector = SelectColorCorrectSettingsVector(targetId, region);
				propertyPath = FName("ColorGradingSettings");
				description = FString("ColorCorrect HSV");
			}
		}
		else if (TargetId::IsCameraPostProcessGrading(targetId))
		{
			// the target control is camera based, only applicable to selected camera actors
			if (ACameraActor* camera = Cast<ACameraActor>(_selectedActor))
			{
				// fetch the settings vector for this camera and target id, may be null if not available
				settingsVector = SelectCameraPostProcessSettingsVector(targetId, camera);
				propertyPath = FName("PostProcessSettings");
				description = FString("Camera Post Process HSV");
			}
		}

		// if a settings vector is set then we may proceeed, if this is null then the target id and/or selected actor is incorrect for this context
		if (settingsVector)
		{
			// convert the vector to a color and then to hsv for changing
			FLinearColor	rgbColor = FLinearColor(*settingsVector);
			FLinearColor	hsvColor = rgbColor.LinearRGBToHSV();
			float			value;

			// choose which hsv component is required from the target id
			switch (TargetId::GradingComponent(targetId))
			{
				case TargetId::GRADING_H:
					// hue (r) reset or update
					if (!HandleResetOrUpdate(hsvColor.R, 0.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					hsvColor.R = (delta == 0.0) ? 0.0 : hsvColor.R + delta;
					// apply some sensible limits to the new value, we want the hue (r) to be in the range 0-359
					hsvColor.R = (hsvColor.R < 0) ? hsvColor.R + 360.0 : hsvColor.R;
					hsvColor.R = (hsvColor.R >= 360.0) ? hsvColor.R - 360.0 : hsvColor.R;
					// track the value to send to the hub
					value = hsvColor.R;
					break;
				case TargetId::GRADING_S:
					// saturation (g) reset or update
					if (!HandleResetOrUpdate(hsvColor.G, 0.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					hsvColor.G = (delta == 0.0) ? 0.0 : hsvColor.G + delta;
					// we want the saturation (g) to be 0-1
					hsvColor.G = (hsvColor.G < 0) ? 0.0 : hsvColor.G;
					hsvColor.G = (hsvColor.G > 1.0) ? 1.0 : hsvColor.G;
					// track the value to send to the hub
					value = hsvColor.G;
					break;
				case TargetId::GRADING_V:
					// value (b) reset or update
					if (!HandleResetOrUpdate(hsvColor.B, 1.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					hsvColor.B = (delta == 0.0) ? 1.0 : hsvColor.B + delta;
					// we want the value (b) to be 0-2
					hsvColor.B = (hsvColor.B < 0) ? 0.0 : hsvColor.B;
					hsvColor.B = (hsvColor.B > 2.0) ? 2.0 : hsvColor.B;
					// track the value to send to the hub
					value = hsvColor.B;
					break;
				default:
					// not relevant to this context, log a warning and abort
					UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unhandled HSV component"));
					return;
			}

			// generate a new rgb colour to apply
			rgbColor = hsvColor.HSVToLinearRGB();

			// we need the associated property for change events
			FProperty *targetProperty = FetchProperty(_selectedActor, propertyPath);

			// mark the actor as about to be modified
			OnPreChange(_selectedActor, targetProperty, description);

			// apply the new rgb (converted to the vector format) to the stored settings 
			*settingsVector = FVector4(rgbColor);

			// flag the edit has been done
			OnPostChange(_selectedActor, targetProperty);

			// send back the new value to the hub via the parameter value event
			ParameterValueEvent.Broadcast(targetId, value);
		}
	});
}

// resets the hsv colour channel associated with the target id parameter for the currently selected post processing volume or color correct region
// actor for all sections and settings that are supported by the encoded target ids, the new value will be sent to the hub if available
void UEngineHelper::ResetGradingHSV(uint32 targetId)
{
	// call the sister function, a delta of zero indicates reset
	UpdateGradingHSV(targetId, 0);
}

// requests the value of the hsv colour channel associated with the target id parameter for the currently selected post processing volume actor 
// or color correct region for all sections and settings that are supported by the encoded target ids, the new value will be sent to the hub if available
void UEngineHelper::RequestGradingHSV(uint32 targetId)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		FVector4*	settingsVector = nullptr;		// this is used to detect a valid set of data

		// only applicable to post process volume or color correct region actors with an appropriate target control id, each having their own settings property
		// but with common functionality, we use a pointer to the actual target setting vector as we can't assign references on the fly, we choose which settings 
		// section is relevant to this target id, we use this to read the current values, it is encoded in the target id
		if (TargetId::IsPostProcessGrading(targetId))
		{
			// the target control is ppv based, only applicable to selected post process volume actors
			if (APostProcessVolume* volume = Cast<APostProcessVolume>(_selectedActor))
			{
				// fetch the settings vector for this volume and target id, may be null if not available
				settingsVector = SelectPostProcessSettingsVector(targetId, volume);
			}
		}
		else if (TargetId::IsColorCorrectGrading(targetId))
		{
			// the target control is ccr based, only applicable to selected color correct region actors
			if (AColorCorrectRegion* region = Cast<AColorCorrectRegion>(_selectedActor))
			{
				// fetch the settings vector for this region and target id, may be null if not available
				settingsVector = SelectColorCorrectSettingsVector(targetId, region);
			}
		}

		// if a settings vector is set then we may proceeed, if this is null then the target id and/or selected actor is incorrect for this context
		if (settingsVector)
		{
			// convert the vector to a color and then to hsv
			FLinearColor	rgbColor = FLinearColor(*settingsVector);
			FLinearColor	hsvColor = rgbColor.LinearRGBToHSV();

			// choose which hsv component is required from the target id
			switch (TargetId::GradingComponent(targetId))
			{
				case TargetId::GRADING_H:
					// hue (r) value
					ParameterValueEvent.Broadcast(targetId, hsvColor.R);
					break;
				case TargetId::GRADING_S:
					// saturation (g) value
					ParameterValueEvent.Broadcast(targetId, hsvColor.G);
					break;
				case TargetId::GRADING_V:
					// value (b) value
					ParameterValueEvent.Broadcast(targetId, hsvColor.B);
					break;
				default:
					// not relevant to this context, log a warning and abort
					UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unhandled HSV component"));
					return;
			}
		}
	});
}

// this value is used to apply a log adjustment to the saturation radius of values for the hsv functions, it is derived from the color grading
// wheel code in unreal as this is what we are driving so the maths have to match, the unreal ui defaults to 2.4
#define EXPONENT_DISPLACEMENT		(2.4f)

// these values are the adjustment made to the angle in the convert x and y to/from hue and saturation, and also an inverting multiplier for the x
// axis, tailored by engine versioning
#if (ENGINE_MAJOR_VERSION >= 5) && (ENGINE_MINOR_VERSION >= 5)
	// UE 5.5+, rotate and invert
	#define ANGULAR_DISPLACEMENT		(PI / 3.0f)
	#define X_AXIS_INVERTER				(-1.0f)
#else
	// all previous versions, no changes
	#define ANGULAR_DISPLACEMENT		(0.0f)
	#define X_AXIS_INVERTER				(1.0f)
#endif

// helper function to convert an hsv colour instance with hue (in degrees) and saturation radius into the corresponding x/y
// co-ordinates that are returned in the vector instance
void UEngineHelper::ConvertHSToXY(const FLinearColor& color, FVector2D& coords)
{
	// the red color component is the hue angle which we convert to radians, recent engine versions have a modified colour wheel layout and 
	// so we may need to apply a rotational displacement, defined at compilation
	float angle = (color.R / 180.0f * PI) - ANGULAR_DISPLACEMENT;

	// the green is the saturation to which we apply a log component that matches the code for the colour wheel in the unreal editor as that is what we are driving
	float radius = FMath::Pow(color.G, 1.0f / EXPONENT_DISPLACEMENT);

	// convert the polar values to the x/y to return allowing for possible x axis inverting, defined at compilation
	coords = FVector2D(X_AXIS_INVERTER * FMath::Cos(angle), FMath::Sin(angle)) * radius;
}

// helper function that converts an x/y vector into the corresponding hue angle (in degrees) and saturation radius, note the
// value of the color is not set or modified by this function
void UEngineHelper::ConvertXYToHS(const FVector2D& coords, FLinearColor& color)
{
	// convert the x/y into the angle, recent engine versions have a modified colour wheel layout and so we may need to apply a
	// rotational displacement and x axis invertion, defined at compilation
	float angle = FMath::Atan2(coords.Y, X_AXIS_INVERTER * coords.X) + ANGULAR_DISPLACEMENT;

	// allow for possibly negative values
	if (angle < 0.0f)
	{
		// adjust for negative
		angle += 2.0f * PI;
	}

	// the hue angle is in the red channel, converted to degrees and the saturation radius has the logarithmic adjustment applied with the 
	// result stored in the green channel
	color.R = angle * 180.0f * INV_PI;
	color.G = FMath::Pow(coords.Size(), EXPONENT_DISPLACEMENT);
}

// adjusts the selected post process or color correct encoded xyz target parameter using the supplied delta to be applied to the yu pip 
// and v slider in a colour wheel representation of the colourspace, a zero adjustment triggers a reset
void UEngineHelper::UpdateGradingXYZ(uint32 targetId, float delta)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, delta]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		FVector4*	settingsVector = nullptr;		// this is used to detect a valid set of data including the property path and description
		FName		propertyPath;
		FString		description;

		// only applicable to post process volume, color correct region or camera actors with an appropriate target control id, each having their own settings property
		// but with common functionality, we use a pointer to the actual target setting vector as we can't assign references on the fly, we choose which settings 
		// section is relevant to this target id, we use this to read the current values and then to apply any changes, it is encoded in the target id
		if (TargetId::IsPostProcessGrading(targetId))
		{
			// the target control is ppv based, only applicable to selected post process volume actors
			if (APostProcessVolume* volume = Cast<APostProcessVolume>(_selectedActor))
			{
				// fetch the settings vector for this volume and target id, may be null if not available
				settingsVector = SelectPostProcessSettingsVector(targetId, volume);
				propertyPath = FName("Settings");
				description = FString("PostProcess Color Wheel");
			}
		}
		else if (TargetId::IsColorCorrectGrading(targetId))
		{
			// the target control is ccr based, only applicable to selected color correct region actors
			if (AColorCorrectRegion* region = Cast<AColorCorrectRegion>(_selectedActor))
			{
				// fetch the settings vector for this region and target id, may be null if not available
				settingsVector = SelectColorCorrectSettingsVector(targetId, region);
				propertyPath = FName("ColorGradingSettings");
				description = FString("ColorCorrect Color Wheel");
			}
		}
		else if (TargetId::IsCameraPostProcessGrading(targetId))
		{
			// the target control is camera based, only applicable to selected camera actors
			if (ACameraActor* camera = Cast<ACameraActor>(_selectedActor))
			{
				// fetch the settings vector for this camera and target id, may be null if not available
				settingsVector = SelectCameraPostProcessSettingsVector(targetId, camera);
				propertyPath = FName("PostProcessSettings");
				description = FString("Camera Post Process Color Wheel");
			}
		}

		// if a settings vector is set then we may proceeed, if this is null then the target id and/or selected actor is incorrect for this context
		if (settingsVector)
		{
			// convert the vector to a color
			FLinearColor	rgbColor = FLinearColor(*settingsVector);

			// the range of offset values are different to the rest, we adjust these before processing to match and then allow for this afterwards
			if (TargetId::IsGradingOffset(targetId))
			{
				// shift the offset value ranges from -1/+1 up to the range of 0/2
				rgbColor.R += 1;
				rgbColor.G += 1;
				rgbColor.B += 1;
			}

			// convert the rgb to hsv
			FLinearColor	hsvColor = rgbColor.LinearRGBToHSV();
			FVector2D		xyPos;

			// convert the hsv colour to the x and y that is used to place the 'pip'in the ui
			ConvertHSToXY(hsvColor, xyPos);

			// apply the delta or reset by which xyz component is required from the target id
			switch (TargetId::GradingComponent(targetId))
			{
				case TargetId::GRADING_X:
					// x pos reset or change
					if (!HandleResetOrUpdate(xyPos.X, 0.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					xyPos.X = (delta == 0.0)? 0.0: xyPos.X + delta;
					break;
				case TargetId::GRADING_Y:
					// y pos reset or change, note y is inverted
					if (!HandleResetOrUpdate(xyPos.Y, 0.0, -delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					xyPos.Y = (delta == 0.0)? 0.0: xyPos.Y - delta;
					break;
				case TargetId::GRADING_Z:
					// z reset or change is applied to the color instance directly
					if (!HandleResetOrUpdate(hsvColor.B, 1.0, delta, targetId, _selectedActor))
					{
						// we have no appropriate reset value to continue so stop now
						return;
					}
//					hsvColor.B = (delta == 0.0) ? 1.0 : hsvColor.B + delta;
					break;
				default:
					// not relevant to this context, log a warning and abort
					UE_LOG(LogTemp, Warning, TEXT("EngineHelper: unhandled XYZ component"));
					return;
			}

			// convert the new x and y into the hsv colour
			ConvertXYToHS(xyPos, hsvColor);

			// apply some sensible limits to the new hsv values, we want the hue (r) to be in the range 0-359
			hsvColor.R = (hsvColor.R < 0) ? hsvColor.R + 360.0 : hsvColor.R;
			hsvColor.R = (hsvColor.R >= 360.0) ? hsvColor.R - 360.0 : hsvColor.R;

			// we want the saturation (g) to be 0-1
			hsvColor.G = (hsvColor.G < 0) ? 0.0 : hsvColor.G;
			hsvColor.G = (hsvColor.G > 1.0) ? 1.0 : hsvColor.G;

			// we want the value (b) to be 0-2
			hsvColor.B = (hsvColor.B < 0) ? 0.0 : hsvColor.B;
			hsvColor.B = (hsvColor.B > 2.0) ? 2.0 : hsvColor.B;

			// generate a new rgb colour to apply
			rgbColor = hsvColor.HSVToLinearRGB();

			// now the processing has been done we allow for the offset exception where we adjusted the values
			if (TargetId::IsGradingOffset(targetId))
			{
				// bring the offset value ranges back to their required -1/1
				rgbColor.R -= 1;
				rgbColor.G -= 1;
				rgbColor.B -= 1;
			}

			// we need the associated property for change events
			FProperty *targetProperty = FetchProperty(_selectedActor, propertyPath);

			// mark the volume as about to be modified
			OnPreChange(_selectedActor, targetProperty, description);

			// apply the new rgb (converted to the vector format) to the stored settings 
			*settingsVector = FVector4(rgbColor);

			// flag the edit has been done
			OnPostChange(_selectedActor, targetProperty);

			// note - we don't send the x/y/z values back to the hub as they don't really have any meaning although
			// we could maybe consider sending back the updated hsv values, but that means mapping between the target ids
		}
	});
}

// resets the xyz value associated with the encoded target id parameter for the currently selected post processing volume or color correct
// region actor for all sections and settings that are supported by the encoded target ids
void UEngineHelper::ResetGradingXYZ(uint32 targetId)
{
	// call the sister function, a delta of zero indicates reset
	UpdateGradingXYZ(targetId, 0);
}

// rotates the selected camera around the local vertical axis, the call has no effect if the selection is not an instance of a camera based actor
void UEngineHelper::UpdateCameraPan(float delta)
{
	// only applicable to camera actor selections
	if (SelectedActorIsA<ACameraActor>())
	{
		// camera pan is yaw around the local z axis, note - going through the internal apply function rather than the 
		// api rotate function means this value is not used by the rest cache
		ApplyRotateSelected(FRotator(0, delta, 0), TransformReference::Local, false);
	}
}

// rotates the selected camera around the local left/right axis, the call has no effect if the selection is not an instance of a camera based actor
void UEngineHelper::UpdateCameraTilt(float delta)
{
	// only applicable to camera actor selections
	if (SelectedActorIsA<ACameraActor>())
	{
		// camera tilt is pitch around the local y axis, note - going through the internal apply function rather than the 
		// api rotate function means this value is not used by the rest cache
		ApplyRotateSelected(FRotator(delta, 0, 0), TransformReference::Local, false);
	}
}

// rotates the selected camera around the local forwards/back axis, the call has no effect if the selection is not an instance of a camera based actor
void UEngineHelper::UpdateCameraRoll(float delta)
{
	// only applicable to camera actor selections
	if (SelectedActorIsA<ACameraActor>())
	{
		// camera roll is roll around the local x axis, note - going through the internal apply function rather than the 
		// api rotate function means this value is not used by the rest cache
		ApplyRotateSelected(FRotator(0, 0, delta), TransformReference::Local, false);
	}
}

// moves the selected camera actor forwards/back by the specified amount, always relative to the camera's local axes, the call
// has no effect if the selection is not an instance of a camera based actor
void UEngineHelper::UpdateCameraDolly(float delta)
{
	// only applicable to camera actor selections
	if (SelectedActorIsA<ACameraActor>())
	{
		// camera dolly is forward/back movement on the local x axis, note - going through the internal apply function rather than the 
		// api move function means this value is not used by the rest cache
		ApplyMoveSelected(FVector(delta, 0, 0), UEngineHelper::TransformReference::Local, false);
	}
}

// moves the selected camera actor left/right by the specified amount, always relative to the camera's local axes, the call
// has no effect if the selection is not an instance of a camera based actor
void UEngineHelper::UpdateCameraTruck(float delta)
{
	// only applicable to camera actor selections
	if (SelectedActorIsA<ACameraActor>())
	{
		// camera truck is left/right movement on the local y axis, note - going through the internal apply function rather than the 
		// api move function means this value is not used by the rest cache
		ApplyMoveSelected(FVector(0, delta, 0), UEngineHelper::TransformReference::Local, false);
	}
}

// moves the selected camera actor up/down by the specified amount, always relative to the camera's local axes, the call
// has no effect if the selection is not an instance of a camera based actor
void UEngineHelper::UpdateCameraPedestal(float delta)
{
	// only applicable to camera actor selections
	if (SelectedActorIsA<ACameraActor>())
	{
		// camera pedestal is up/down movement on the local z axis, note - going through the internal apply function rather than the 
		// api move function means this value is not used by the rest cache
		ApplyMoveSelected(FVector(0, 0, delta), UEngineHelper::TransformReference::Local, false);
	}
}

// steps through the defined preset settings for the currently selected camera, asynchronously returning the new string value via
// the associated menu string event, a step of zero triggers a reset
void UEngineHelper::StepCineCameraLensSettingsPreset(int32 step)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, step]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// only applicable to camera actors
		if (ACameraActor* camera = Cast<ACameraActor>(_selectedActor))
		{
			// we require the camera to have a cine camera component to access the lens settings preset data
			if (UCineCameraComponent* cineCameraComponent = Cast<UCineCameraComponent>(camera->GetCameraComponent()))
			{
				// fetch the array of all defined presets
				TArray<FNamedLensPreset> const& presets = UCineCameraSettings::GetLensPresets();

				// we need the associated property for change events
				FProperty *targetProperty = FetchProperty(_selectedActor, FName("CameraComponent.LensSettings"));

				// mark the camera as about to be modified
				// TODO - camera or component?
				OnPreChange(camera, targetProperty, "CineCamera Lens Settings");

				// check for a reset or step
				if (step == 0)
				{
					// a zero step resets to the special case last entry
					cineCameraComponent->SetLensPresetByName(presets.Last().Name);
				}
				else
				{
					// extract the current preset name as a string
					FString currentPreset = cineCameraComponent->GetLensPresetName();

					// find the preset that matches the current name so we can apply the step to that index
					int32 const numPresets = presets.Num();
					for (int32 index = 0; index < numPresets; index++)
					{
						// check for a matching name for the current preset
						if (presets[index].Name == currentPreset)
						{
							// we have the index of the current setting, apply the step watching array boundaries to wrap to the 
							// next/previous preset as required
							index += step;
							index = (index < 0)? numPresets - 1: index;
							index = (index >= numPresets)? 0: index;

							// apply the new index and leave the loop
							cineCameraComponent->SetLensPresetByName(presets[index].Name);
							break;
						}
					}
				}

				// flag the edit has been done
				// TODO - camera or component?
				OnPostChange(camera, targetProperty);

				// send out the new preset name using the sister function
				RequestCineCameraLensSettingsPreset();
			}
		}
	});
}

// resets to the default lens setting default on the currently selected camera actor and reports back the new value to the hub
void UEngineHelper::ResetCineCameraLensSettingsPreset()
{
	// use the sister function, the step value of 0 means a reset
	StepCineCameraLensSettingsPreset(0);
}

// asynchronously reports the value of the camera lens preset for the currently selection camera actor back via the menu string event
void UEngineHelper::RequestCineCameraLensSettingsPreset()
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// only applicable to camera actors
		if (ACameraActor* camera = Cast<ACameraActor>(_selectedActor))
		{
			// we require the camera to have a cine camera component to access the lens settings preset data
			if (UCineCameraComponent* cineCameraComponent = Cast<UCineCameraComponent>(camera->GetCameraComponent()))
			{
				// extract the current preset name as a string
				FString strValue = cineCameraComponent->GetLensPresetName();

				// use the menu string reporting event to pass this data back
				UEngineHelper::MenuStringEvent.Broadcast(TargetId::CineCameraLensSettings, strValue);
			}
		}
	});
}

// main api function to allow the selected actor to be moved by a set of deltas across all of the x, y and z axis in a single update call, if
// any movement is the first for an axis we cache the current world location for that axis to be used as the single tap reset when required
void UEngineHelper::MoveSelected(const FVector& deltas)
{
	// we need to use the currently selected actor, if there is one, it is tracked by selection change events
	if (_selectedActor)
	{
		// fetch the current location as we may want to cache the positions for resets
		FVector currentLocation = _selectedActor->GetActorLocation();

		// check each axis in turn for a change, if one is present we may need to seed the reset cache with the current world location for resets
		if (deltas.X != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorLocationX, _selectedActor, currentLocation.X);
		}

		// same for y
		if (deltas.Y != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorLocationY, _selectedActor, currentLocation.Y);
		}

		// same for z
		if (deltas.Z != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorLocationZ, _selectedActor, currentLocation.Z);
		}
	
		// call the sister function with the delta vector, using whatever the user has as the current transform reference and marking it as not a reset (i.e. delta change)
		ApplyMoveSelected(deltas, TransformReference::Current, false);
	}
}

// main api function to reset any of the axis values for the location of the selected actor, the flags are bitwise settings that indicate which axes are to be
// processed, this allows us to rest multiple values with one update, we handle a single or double tap reset, the single tap resets to the cached initial value
// for that axis and a double tap resets to zero
void UEngineHelper::ResetMoveSelected(uint32 axisFlags)
{
	// we need to use the currently selected actor, if there is one, it is tracked by selection change events
	if (_selectedActor)
	{
		// as we may not be selecting all the axes we need the current world location to start with for the other axes we do not reset
		FVector location = _selectedActor->GetActorLocation();

		// check the flags for the bit that indicates what axes to reset, then for each we check for single or double taps, the tap
		// was registered when it was first received, a single tap uses the cached value, a double tap goes to the origin
		if (axisFlags & AxisX)
		{
			// check what to reset to - the single tap cached value or zero
			if (ResetCache::IsSingleTap(TargetId::ActorLocationX))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original location is left unchanged here
				ResetCache::GetValue(TargetId::ActorLocationX, _selectedActor, &location.X);
			}
			else
			{
				// a double tap reset simply goes back to the origin
				location.X = 0;
			}
		}

		// same for y
		if (axisFlags & AxisY)
		{
			// check what to reset to - the single tap cached value or zero
			if (ResetCache::IsSingleTap(TargetId::ActorLocationY))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original location is left unchanged here
				ResetCache::GetValue(TargetId::ActorLocationY, _selectedActor, &location.Y);
			}
			else
			{
				// a double tap reset simply goes back to the origin
				location.Y = 0;
			}
		}

		// same for z
		if (axisFlags & AxisZ)
		{
			// check what to reset to - the single tap cached value or zero
			if (ResetCache::IsSingleTap(TargetId::ActorLocationZ))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original location is left unchanged here
				ResetCache::GetValue(TargetId::ActorLocationZ, _selectedActor, &location.Z);
			}
			else
			{
				// a double tap reset simply goes back to the origin
				location.Z = 0;
			}
		}

		// call the sister function with the setting vector requesting it to be applied as a world location as a reset, we reset using world co-ords to
		// side step any problems caused by having different transform reference settings during runtime
		ApplyMoveSelected(location, UEngineHelper::TransformReference::World, true);
	}
}

// moves the selected actor by the specified amount, local or world offsets may be chosen, the caller may choose to use the current
// transform reference setting or to override with a specific local or world selection, a reset call sets the specified value, this 
// function is called by other code in the helper as required
void UEngineHelper::ApplyMoveSelected(const FVector& value, TransformReference transformReference, bool reset)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, value, transformReference, reset]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// we need the associated property for change events
			FProperty *targetProperty = FetchProperty(_selectedActor, FName("RelativeLocation"));

			// mark the actor as about to be modified
			OnPreChange(_selectedActor, targetProperty, "Move Actor");

			// the caller can choose to use the current setting for the transform reference or to override with a specific value
			TransformReference transformRef = (transformReference == TransformReference::Current)? _transformReference: transformReference;

			// we are ready to apply the reset or delta, local or world
			if (reset && (transformRef == TransformReference::Local))
			{
				// apply the reset location value in local space
				_selectedActor->SetActorRelativeLocation(value);
			}
			else if (reset && (transformRef == TransformReference::World))
			{
				// apply the reset location value in world space
				_selectedActor->SetActorLocation(value);
			}
			else if (reset && (transformRef == TransformReference::Camera))
			{
				// TODO: what should a reset in camera reference do?
				// apply the reset location value in world space
				_selectedActor->SetActorLocation(value);
			}
			else if (reset && (transformRef == TransformReference::Viewport))
			{
				// TODO: what should a reset in viewport reference do?
				// apply the reset location value in world space
				_selectedActor->SetActorLocation(value);
			}
			else if (transformRef == TransformReference::Local)
			{
				// apply the delta value as a local offset
				_selectedActor->AddActorLocalOffset(value);
			}
			else if (transformRef == TransformReference::World)
			{
				// apply the delta value as a world offset
				_selectedActor->AddActorWorldOffset(value);
			}
			else if (transformRef == TransformReference::Camera)
			{
				// apply the delta values as offsets relative to the last selected camera if available
				if (_lastSelectedCamera)
				{
					// we need the directional vectors of the camera's orientation to drive movement relative to it
					FVector cameraFV = _lastSelectedCamera->GetActorForwardVector(),
							cameraRV = _lastSelectedCamera->GetActorRightVector(),
							cameraUV = _lastSelectedCamera->GetActorUpVector();

					// for ue: x is forward, y is right and z is up, so multiply the unit vectors of the appropriate directions by the associated delta values
					cameraFV *= value.X;
					cameraRV *= value.Y;
					cameraUV *= value.Z;

					// sum the individual movements and apply to the selected actor as a world move
					FVector result = cameraFV + cameraRV + cameraUV;
					_selectedActor->AddActorWorldOffset(result);
				}
			}
			else if (transformRef == TransformReference::Viewport)
			{
				// apply the delta values relative to the active viewport
				if (FViewport* vpActive = GEditor->GetActiveViewport())
				{
					// get the forward facing vector of the viewport client instance
					FEditorViewportClient *vpClient = (FEditorViewportClient*) vpActive->GetClient();
					FVector vpVector = vpClient->GetViewRotation().Vector();

					// the easiest way to get the forward/right/up unit vectors is by getting the viewport orientation as a quat and taking
					// the vector values from that helper class
					FQuat	vpQuat = vpVector.ToOrientationQuat();
					FVector vpFV = vpQuat.GetForwardVector(),
							vpRV = vpQuat.GetRightVector(),
							vpUV = vpQuat.GetUpVector();

					// for ue: x is forward, y is right and z is up, so multiply the unit vectors of the appropriate directions by the associated delta values
					vpFV *= value.X;
					vpRV *= value.Y;
					vpUV *= value.Z;

					// sum the individual movements and apply to the selected actor as a world move
					FVector result = vpFV + vpRV + vpUV;
					_selectedActor->AddActorWorldOffset(result);
				}
			}

			// flag the edit has been done
			OnPostChange(_selectedActor, targetProperty);

			// as part of a change value process we should return the new value to the hub for display purposes, we
			// use the sister function to be sure to report the current setting
			RequestSelectedLocation();
		}
	});
}

// asynchronously reports the location of the current selection back via the parameter value event
void UEngineHelper::RequestSelectedLocation()
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// actors may have an absolute location being set depending on its config, this is available via the root component, note this is
			// not the flag we use in the plugin to decide how to apply location moves
			USceneComponent* root = _selectedActor->GetRootComponent();
			if (root)
			{
				FVector location;
					
				// if absolute for this object then get the world based values, otherwise it is relative to any parent
				if (root->IsUsingAbsoluteLocation())
				{
					// get the actor's location in world space
					location = _selectedActor->GetActorLocation();
				}
				else
				{
					// get the actor's location relative to any parent from the root component
					location = root->GetRelativeLocation();
				}

				// use the parameter value reporting event to pass this data back
				ParameterValueEvent.Broadcast(TargetId::ActorLocationX, location.X);
				ParameterValueEvent.Broadcast(TargetId::ActorLocationY, location.Y);
				ParameterValueEvent.Broadcast(TargetId::ActorLocationZ, location.Z);
			}
		}
	});
}

// main api function to allow the selected actor to be rotated by a set of deltas across all of the x, y and z axis in a single update call, if
// any movement is the first for an axis we cache the current world rotation for that axis to be used as the single tap reset when required
void UEngineHelper::RotateSelected(const FRotator& deltas)
{
	// we need to use the currently selected actor, if there is one, it is tracked by selection change events
	if (_selectedActor)
	{
		// fetch the current world rotation as we may want to cache the values for resets
		FRotator currentRotation = _selectedActor->GetActorRotation();

		// check each axis in turn for a change, if one is present we may need to seed the reset cache with the current world rotation for resets
		// x is roll, y is pitch, z is yaw
		if (deltas.Roll != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorRotationX, _selectedActor, currentRotation.Roll);
		}

		// same for y and pitch
		if (deltas.Pitch != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorRotationY, _selectedActor, currentRotation.Pitch);
		}

		// same for z and yaw
		if (deltas.Yaw != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorRotationZ, _selectedActor, currentRotation.Yaw);
		}
	
		// call the sister function with the delta rotator, using whatever the user has as the current transform reference and marking it as not a reset (i.e. delta change)
		ApplyRotateSelected(deltas, TransformReference::Current, false);
	}
}

// main api function to reset any of the axis values for the rotation of the selected actor, the flags are bitwise settings that indicate which axes are to be
// processed, this allows us to rest multiple values with one update, we handle a single or double tap reset, the single tap resets to the cached initial value
// for that axis and a double tap resets to zero
void UEngineHelper::ResetRotateSelected(uint32 axisFlags)
{
	// we need to use the currently selected actor, if there is one, it is tracked by selection change events
	if (_selectedActor)
	{
		// as we may not be selecting all the axes we need the current world rotation to start with for the other axes we do not reset
		FRotator rotation = _selectedActor->GetActorRotation();

		// check the flags for the bit that indicates what axes to reset, then for each we check for single or double taps, the tap
		// was registered when it was first received, a single tap uses the cached value, a double tap goes to zero
		// x is roll, y is pitch, z is yaw
		if (axisFlags & AxisX)
		{
			// check what to reset to - the single tap cached value or zero
			if (ResetCache::IsSingleTap(TargetId::ActorRotationX))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original rotation is left unchanged here
				ResetCache::GetValue(TargetId::ActorRotationX, _selectedActor, &rotation.Roll);
			}
			else
			{
				// a double tap reset simply goes back to zero
				rotation.Roll = 0;
			}
		}

		// same for y and pitch
		if (axisFlags & AxisY)
		{
			// check what to reset to - the single tap cached value or zero
			if (ResetCache::IsSingleTap(TargetId::ActorRotationY))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original rotation is left unchanged here
				ResetCache::GetValue(TargetId::ActorRotationY, _selectedActor, &rotation.Pitch);
			}
			else
			{
				// a double tap reset simply goes back to zero
				rotation.Pitch = 0;
			}
		}

		// same for z and yaw
		if (axisFlags & AxisZ)
		{
			// check what to reset to - the single tap cached value or zero
			if (ResetCache::IsSingleTap(TargetId::ActorRotationZ))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original rotation is left unchanged here
				ResetCache::GetValue(TargetId::ActorRotationZ, _selectedActor, &rotation.Yaw);
			}
			else
			{
				// a double tap reset simply goes back to zero
				rotation.Yaw = 0;
			}
		}

		// call the sister function with the setting rotator requesting it to be applied as a world rotation as a reset, we reset using world values to
		// side step any problems caused by having different transform reference settings during runtime
		ApplyRotateSelected(rotation, UEngineHelper::TransformReference::World, true);
	}
}

// rotates the selected actor by the specified amount, local or world offsets may be chosen, the caller may choose to use the current
// transform reference setting or to override with a specific local or world selection, a reset call sets the specified value
void UEngineHelper::ApplyRotateSelected(const FRotator& value, TransformReference transformReference, bool reset)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, value, transformReference, reset]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// we need the associated property for change events
			FProperty *targetProperty = FetchProperty(_selectedActor, FName("RelativeRotation"));

			// mark the actor as about to be modified
			OnPreChange(_selectedActor, targetProperty, "Rotate Actor");

			// the caller can choose to use the current setting for the transform reference or to override with a specific value
			TransformReference transformRef = (transformReference == TransformReference::Current) ? _transformReference : transformReference;

			// we are ready to apply the reset or delta, local or world
			if (reset && (transformRef == TransformReference::Local))
			{
				// apply the reset as the rotation value in local space
				_selectedActor->SetActorRelativeRotation(value);
			}
			else if (reset && (transformRef == TransformReference::World))
			{
				// apply the reset as the rotation value in world space
				_selectedActor->SetActorRotation(value);
			}
			else if (reset && (transformRef == TransformReference::Camera))
			{
				// TODO: what should a reset in camera reference do?
				// apply the reset as the rotation value in world space
				_selectedActor->SetActorRotation(value);
			}
			else if (reset && (transformRef == TransformReference::Viewport))
			{
				// TODO: what should a reset in viewport reference do?
				// apply the reset as the rotation value in world space
				_selectedActor->SetActorRotation(value);
			}
			else if (transformRef == TransformReference::Local)
			{
				// apply the delta value as a local offset
				_selectedActor->AddActorLocalRotation(value);
			}
			else if (transformRef == TransformReference::World)
			{
				// apply the delta value as a world offset
				_selectedActor->AddActorWorldRotation(value);
			}
			else if (transformRef == TransformReference::Camera)
			{
				// apply the delta values as offsets relative to the last selected camera if available
				if (_lastSelectedCamera)
				{
					// we need the directional vectors of the camera's orientation to drive rotation relative to it
					FVector cameraFV = _lastSelectedCamera->GetActorForwardVector(),
							cameraRV = _lastSelectedCamera->GetActorRightVector(),
							cameraUV = _lastSelectedCamera->GetActorUpVector();					

					// as we are using the camera directional vectors to create the quats we don't need to safeguard the vectors by making calls
					// to GetSafeNormal() on them first, note that the x and y rotations are inverted to make the operation visually correct
					// for ue: x is forward, y is right, z is up and x is roll, y is pitch, z is yaw
					FQuat xQuat = FQuat(cameraFV, FMath::DegreesToRadians(-value.Roll));
					FQuat yQuat = FQuat(cameraRV, FMath::DegreesToRadians(-value.Pitch));
					FQuat zQuat = FQuat(cameraUV, FMath::DegreesToRadians(value.Yaw));

					// apply the results to the selected actor directly from the quats, no need to convert to rotators
					_selectedActor->AddActorWorldRotation(xQuat);
					_selectedActor->AddActorWorldRotation(yQuat);
					_selectedActor->AddActorWorldRotation(zQuat);
				}
			}
			else if (transformRef == TransformReference::Viewport)
			{
				// apply the delta values as offsets relative to the viewport client instance
				if (FViewport* vpActive = GEditor->GetActiveViewport())
				{
					// get the forward facing vector of the viewport client instance
					FEditorViewportClient *vpClient = (FEditorViewportClient*) vpActive->GetClient();
					FVector vpVector = vpClient->GetViewRotation().Vector();

					// the easiest way to get the forward/right/up unit vectors is by getting the viewport orientation as a quat and taking
					// the vector values from that helper class
					FQuat	vpQuat = vpVector.ToOrientationQuat();
					FVector vpFV = vpQuat.GetForwardVector(),
							vpRV = vpQuat.GetRightVector(),
							vpUV = vpQuat.GetUpVector();

					// as we are using the viewport directional vectors to create the quats we don't need to safeguard the vectors by making calls
					// to GetSafeNormal() on them first, note that the x and y rotations are inverted to make the operation visually correct
					// for ue: x is forward, y is right, z is up and x is roll, y is pitch, z is yaw
					FQuat xQuat = FQuat(vpFV, FMath::DegreesToRadians(-value.Roll));
					FQuat yQuat = FQuat(vpRV, FMath::DegreesToRadians(-value.Pitch));
					FQuat zQuat = FQuat(vpUV, FMath::DegreesToRadians(value.Yaw));

					// apply the results to the selected actor directly from the quats, no need to convert to rotators
					_selectedActor->AddActorWorldRotation(xQuat);
					_selectedActor->AddActorWorldRotation(yQuat);
					_selectedActor->AddActorWorldRotation(zQuat);
				}
			}

			// flag the edit has been done
			OnPostChange(_selectedActor, targetProperty);

			// as part of a change value process we should return the new value to the hub for display purposes, we
			// use the sister function to be sure to report the current setting
			RequestSelectedRotation();
		}
	});
}

// asynchronously reports the rotation of the current selection back via the parameter value event
void UEngineHelper::RequestSelectedRotation()
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// actors may have an absolute rotation being set depending on its config, this is available via the root component, note this is
			// not the flag we use in the plugin to decide how to apply rotation changes
			USceneComponent* root = _selectedActor->GetRootComponent();
			if (root)
			{
				FRotator rotation;

				// if absolute for this object then get the world based values, otherwise it is relative to any parent
				if (root->IsUsingAbsoluteRotation())
				{
					// get the actor's rotation in world space
					rotation = _selectedActor->GetActorRotation();
				}
				else
				{
					// get the actor's rotation relative to any parent
					rotation = root->GetRelativeRotation();
				}

				// use the parameter value reporting event to pass this data back
				// x is roll, y is pitch, z is yaw
				ParameterValueEvent.Broadcast(TargetId::ActorRotationX, rotation.Roll);
				ParameterValueEvent.Broadcast(TargetId::ActorRotationY, rotation.Pitch);
				ParameterValueEvent.Broadcast(TargetId::ActorRotationZ, rotation.Yaw);
			}
		}
	});
}

// main api function to allow the selected actor to be scaled by a set of deltas across all of the x, y and z axis in a single update call, if
// any change is the first for an axis we cache the current world scale for that axis to be used as the single tap reset when required
void UEngineHelper::ScaleSelected(const FVector& deltas)
{
	// we need to use the currently selected actor, if there is one, it is tracked by selection change events
	if (_selectedActor)
	{
		// fetch the current scale as we may want to cache the values for resets
		FVector currentScale = _selectedActor->GetActorScale3D();

		// check each axis in turn for a change, if one is present we may need to seed the reset cache with the current world scale for resets
		if (deltas.X != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorScaleX, _selectedActor, currentScale.X);
		}

		// same for y
		if (deltas.Y != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorScaleY, _selectedActor, currentScale.Y);
		}

		// same for z
		if (deltas.Z != 0)
		{
			// if a value is already cached this call has no effect
			ResetCache::CacheInitialValue(TargetId::ActorScaleZ, _selectedActor, currentScale.Z);
		}
	
		// call the sister function with the delta vector, using whatever the user has as the current transform reference and marking it as not a reset (i.e. delta change)
		ApplyScaleSelected(deltas, TransformReference::Current, false);
	}
}

// main api function to reset any of the axis values for the scale of the selected actor, the flags are bitwise settings that indicate which axes are to be
// processed, this allows us to rest multiple values with one update, we handle a single or double tap reset, the single tap resets to the cached initial value
// for that axis and a double tap resets to unity
void UEngineHelper::ResetScaleSelected(uint32 axisFlags)
{
	// we need to use the currently selected actor, if there is one, it is tracked by selection change events
	if (_selectedActor)
	{
		// as we may not be selecting all the axes we need the current world scaling to start with for the other axes we do not reset
		FVector scale = _selectedActor->GetActorScale3D();

		// check the flags for the bit that indicates what axes to reset, then for each we check for single or double taps, the tap
		// was registered when it was first received, a single tap uses the cached value, a double tap goes to unity
		if (axisFlags & AxisX)
		{
			// check what to reset to - the single tap cached value or one
			if (ResetCache::IsSingleTap(TargetId::ActorScaleX))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original scale is left unchanged here
				ResetCache::GetValue(TargetId::ActorScaleX, _selectedActor, &scale.X);
			}
			else
			{
				// a double tap reset simply goes back to unity
				scale.X = 1;
			}
		}

		// same for y
		if (axisFlags & AxisY)
		{
			// check what to reset to - the single tap cached value or one
			if (ResetCache::IsSingleTap(TargetId::ActorScaleY))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original scale is left unchanged here
				ResetCache::GetValue(TargetId::ActorScaleY, _selectedActor, &scale.Y);
			}
			else
			{
				// a double tap reset simply goes back to unity
				scale.Y = 1;
			}
		}

		// same for z
		if (axisFlags & AxisZ)
		{
			// check what to reset to - the single tap cached value or one
			if (ResetCache::IsSingleTap(TargetId::ActorScaleZ))
			{
				// a single tap reset fetches the cached initial value, if it is not available we don't do anything more, if there is no cached
				// value then the original scale is left unchanged here
				ResetCache::GetValue(TargetId::ActorScaleZ, _selectedActor, &scale.Z);
			}
			else
			{
				// a double tap reset simply goes back to unity
				scale.Z = 1;
			}
		}

		// call the sister function with the setting vector requesting it to be applied as a world scaling as a reset, we reset using world values to
		// side step any problems caused by having different transform reference settings during runtime
		ApplyScaleSelected(scale, UEngineHelper::TransformReference::World, true);
	}
}

// scales the selected actor by the specified amount, local or world, a reset call sets the specified value
void UEngineHelper::ApplyScaleSelected(const FVector& value,  TransformReference transformReference, bool reset)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, value, transformReference, reset]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// we need the associated property for change events
			FProperty *targetProperty = FetchProperty(_selectedActor, FName("RelativeScale3D"));

			// mark the actor as about to be modified
			OnPreChange(_selectedActor, targetProperty, "Scale Actor");

			// the caller can choose to use the current setting for the transform reference or to override with a specific value
			TransformReference transformRef = (transformReference == TransformReference::Current)? _transformReference: transformReference;

			// we are ready to apply the reset or delta, local or world
			if (reset && (transformRef == TransformReference::Local))
			{
				// apply the reset scale in local space
				_selectedActor->SetActorRelativeScale3D(value);
			}
			else if (reset && (transformRef == TransformReference::World))
			{
				// apply the reset value as the scale value in world space
				_selectedActor->SetActorScale3D(value);
			}
			else if (reset && (transformRef == TransformReference::Camera))
			{
				// TODO: what should a reset in camera reference do?
				// apply the reset value as the scale value in world space
				_selectedActor->SetActorScale3D(value);
			}
			else if (reset && (transformRef == TransformReference::Viewport))
			{
				// TODO: what should a reset in viewport reference do?
				// apply the reset value as the scale value in world space
				_selectedActor->SetActorScale3D(value);
			}
			else if (transformRef == TransformReference::Local)
			{
				// apply the delta value component-wise as a local scale to the current scale vector of the object
				FVector scale = _selectedActor->GetActorRelativeScale3D() * (value + 1);
				_selectedActor->SetActorRelativeScale3D(scale);
			}
			else if (transformRef == TransformReference::World)
			{
				// apply the delta value component-wise as a world scale to the current scale vector of the object
				FVector scale = _selectedActor->GetActorScale3D() * (value + 1);
				_selectedActor->SetActorScale3D(scale);
			}
			else if (transformRef == TransformReference::Camera)
			{
				// apply the delta values as offsets relative to the last selected camera if available
				if (_lastSelectedCamera)
				{
					// we need the directional vectors of the camera's orientation to drive scaling relative to it
					FVector cameraFV = _lastSelectedCamera->GetActorForwardVector(),
							cameraRV = _lastSelectedCamera->GetActorRightVector(),
							cameraUV = _lastSelectedCamera->GetActorUpVector();

					// we want to apply the delta value component-wise as a world scale to the current scale vector of the object
					// so we have to allow for the directional vectors effectively reversing the delta directions by taking the 
					// absolute adjustments and scaling those for each component, it's a bit odd but it seems to work ok
					cameraFV = cameraFV.GetAbs() * value.X;
					cameraRV = cameraRV.GetAbs() * value.Y;
					cameraUV = cameraUV.GetAbs() * value.Z;

					// combine all the component values
					FVector sum = cameraFV + cameraRV + cameraUV;

					// modify the current selected object scale by the trim of the deltas
					FVector newScale = _selectedActor->GetActorScale3D() * (sum + 1);

					// finally apply the new scale in world space
					_selectedActor->SetActorScale3D(newScale);
				}
			}
			else if (transformRef == TransformReference::Viewport)
			{
				// apply the delta values as offsets relative to the active viewport
				if (FViewport* vpActive = GEditor->GetActiveViewport())
				{
					// get the forward facing vector of the viewport client instance
					FEditorViewportClient *vpClient = (FEditorViewportClient*) vpActive->GetClient();
					FVector vpVector = vpClient->GetViewRotation().Vector();

					// the easiest way to get the forward/right/up unit vectors is by getting the viewport orientation as a quat and taking
					// the vector values from that helper class
					FQuat	vpQuat = vpVector.ToOrientationQuat();
					FVector vpFV = vpQuat.GetForwardVector(),
							vpRV = vpQuat.GetRightVector(),
							vpUV = vpQuat.GetUpVector();

					// we want to apply the delta value component-wise as a world scale to the current scale vector of the object
					// so we have to allow for the directional vectors effectively reversing the delta directions by taking the 
					// absolute adjustments and scaling those for each component, it's a bit odd but it seems to work ok
					vpFV = vpFV.GetAbs() * value.X;
					vpRV = vpRV.GetAbs() * value.Y;
					vpUV = vpUV.GetAbs() * value.Z;

					// combine all the component values
					FVector sum = vpFV + vpRV + vpUV;

					// modify the current selected object scale by the trim of the deltas
					FVector newScale = _selectedActor->GetActorScale3D() * (sum + 1);

					// finally apply the new scale in world space
					_selectedActor->SetActorScale3D(newScale);
				}
			}

			// flag the edit has been done
			OnPostChange(_selectedActor, targetProperty);

			// as part of a change value process we should return the new value to the hub for display purposes, we
			// use the sister function to be sure to report the current setting
			RequestSelectedScale();
		}
	});
}

// asynchronously reports the scale of the current selection back via the parameter value event
void UEngineHelper::RequestSelectedScale()
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we need to use the currently selected actor, if there is one, it is tracked by selection change events
		if (_selectedActor)
		{
			// actors may have an absolute scale being set depending on its config, this is available via the root component, note this is
			// not the flag we use in the plugin to decide how to apply scale changes
			USceneComponent* root = _selectedActor->GetRootComponent();
			if (root)
			{
				FVector scale;

				// if absolute for this object then get the world based values, otherwise it is relative to any parent
				if (root->IsUsingAbsoluteScale())
				{
					// get the actor's scale in world space
					scale = _selectedActor->GetActorScale3D();
				}
				else
				{
					// get the actor's scale relative to any parent
					scale = _selectedActor->GetActorRelativeScale3D();
				}

				// use the parameter value reporting event to pass this data back, note we don't report a value
				// for the scale all param as this doesn't really have a value, it is a delta that is applied to others
				ParameterValueEvent.Broadcast(TargetId::ActorScaleX, scale.X);
				ParameterValueEvent.Broadcast(TargetId::ActorScaleY, scale.Y);
				ParameterValueEvent.Broadcast(TargetId::ActorScaleZ, scale.Z);
			}
		}
	});
}

// steps the transform reference space flag for transform changes, the new state string is returned via the menu string 
// event, a step of zero triggers a reset
void UEngineHelper::StepTransformReference(int32 step)
{
	// reset or change as required, the step value is always zero for reset or +1/-1
	if (step == 0)
	{
		// reset to predefined default
		_transformReference = TransformReference::Default;
	}
	else if (step > 0)
	{
		// step up one value, wrapping
		_transformReference = ++_transformReference;
	}
	else
	{
		// step down one value, wrapping
		_transformReference = --_transformReference;
	}
	
	// as part of a change value process we should return the new value to the hub for display purposes, we
	// use the sister function to be sure to report the current setting
	RequestTransformReference();
}

// resets the transform reference setting
void UEngineHelper::ResetTransformReference()
{
	// use the sister function, the step value of 0 means a reset
	StepTransformReference(0);
}

// asynchronously reports the string value of the transform reference setting for the current selection back via the menu string event
void UEngineHelper::RequestTransformReference()
{
	// this relies on the actual values being so take care if changed
	static FString strValues[] = { "Local", "World", "Camera", "Viewport" };

	// use the menu string reporting event to pass this data back
	UEngineHelper::MenuStringEvent.Broadcast(TargetId::TransformReference, strValues[_transformReference]);
}

// called to start a training session for the specified control id, this activates the property change detection routine and will generate a train
// response event if a property/control match is found
void UEngineHelper::StartTrainingForPanelControl(uint32 controlId)
{
	// set the local control id to the specified value to allow event based training processing
	_trainingPanelControl = controlId;
}

// steps the flag that determines if the mode changes to follow actor selections, in this implementation any step
// is just a toggle of the state, the new state string is returned via the menu string event, a step of zero triggers a reset
void UEngineHelper::StepSelectionChangesMode(int32 step)
{
	// reset or toggle as required
	if (step == 0)
	{
		// reset to local default
		_selectionChangesMode = true;
	}
	else
	{
		// toggle on any step, in the current implementation the step is always +/-1
		_selectionChangesMode = !_selectionChangesMode;
	}
	
	// as part of a change value process we should return the new value to the hub for display purposes, we
	// use the sister function to be sure to report the current setting
	RequestSelectionChangesMode();
}

// resets the flag that determines if the mode changes to follow actor selection
void UEngineHelper::ResetSelectionChangesMode()
{
	// use the sister function, the step value of 0 means a reset
	StepSelectionChangesMode(0);
}

// asynchronously reports the string value of the flag that determines if the mode changes to follow actor selection
void UEngineHelper::RequestSelectionChangesMode()
{
	// choose an appropriate string
	FString strValue = _selectionChangesMode? "On": "Off";

	// use the menu string reporting event to pass this data back
	UEngineHelper::MenuStringEvent.Broadcast(TargetId::SelectionChangesMode, strValue);
}

// steps the flag that determines if the main set of post process enabled flags are set or cleared as a batch, in this implementation any step
// is just a toggle of the state, the new state string is returned via the menu string event, a step of zero triggers a reset, the function handles
// both the post process volume and camera color grading targets
void UEngineHelper::StepUseAllColorGrading(uint32 targetId, int32 step)
{
	// seems sensible to apply the changes on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, targetId, step]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// we have two target ids to handle, one for post process volumes and one for cameras
		if (targetId == TargetId::UseAllColorGrading)
		{
			// only applicable to post process volume actors
			if (APostProcessVolume* volume = Cast<APostProcessVolume>(_selectedActor))
			{
				// reset or toggle as required
				if (step == 0)
				{
					// reset to local default
					_ppvUseAllColorGrading = false;
				}
				else
				{
					// toggle on any step, in the current implementation the step is always +/-1
					_ppvUseAllColorGrading = !_ppvUseAllColorGrading;
				}

				// we need the associated property for change events
				FProperty *targetProperty = FetchProperty(_selectedActor, FName("Settings"));

				// mark the volume as about to be modified
				OnPreChange(volume, targetProperty, "PostProcess Use All Color Gradings");

				// apply the new master flag to all relevant use properties, start with the global set
				SetMenuProperty(TargetId::UseGlobalSaturation, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseGlobalContrast, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseGlobalGamma, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseGlobalGain, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseGlobalOffset, _ppvUseAllColorGrading);

				// the shadows set
				SetMenuProperty(TargetId::UseShadowsSaturation, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseShadowsContrast, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseShadowsGamma, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseShadowsGain, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseShadowsOffset, _ppvUseAllColorGrading);

				// the midtones set
				SetMenuProperty(TargetId::UseMidtonesSaturation, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseMidtonesContrast, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseMidtonesGamma, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseMidtonesGain, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseMidtonesOffset, _ppvUseAllColorGrading);

				// the highlights set
				SetMenuProperty(TargetId::UseHighlightsSaturation, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseHighlightsContrast, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseHighlightsGamma, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseHighlightsGain, _ppvUseAllColorGrading);
				SetMenuProperty(TargetId::UseHighlightsOffset, _ppvUseAllColorGrading);

				// flag the edit has been done
				OnPostChange(volume, targetProperty);

				// as part of a change value process we should return the new value to the hub for display purposes, we
				// use the sister function to be sure to report the current setting
				RequestUseAllColorGrading(targetId);
			}
		}
		else if (targetId == TargetId::CPPUseAllColorGrading)
		{
			// only applicable to camera actors
			if (ACameraActor* camera = Cast<ACameraActor>(_selectedActor))
			{
				// reset or toggle as required
				if (step == 0)
				{
					// reset to local default
					_cppUseAllColorGrading = false;
				}
				else
				{
					// toggle on any step, in the current implementation the step is always +/-1
					_cppUseAllColorGrading = !_cppUseAllColorGrading;
				}

				// we need the associated property for change events
				FProperty *targetProperty = FetchProperty(_selectedActor, FName("CameraComponent.PostProcessSettings"));

				// mark the camera as about to be modified
				OnPreChange(camera, targetProperty, "Camera Use All Color Gradings");

				// apply the new master flag to all relevant use properties, start with the global set
				SetMenuProperty(TargetId::CPPUseGlobalSaturation, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseGlobalContrast, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseGlobalGamma, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseGlobalGain, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseGlobalOffset, _cppUseAllColorGrading);

				// the shadows set
				SetMenuProperty(TargetId::CPPUseShadowsSaturation, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseShadowsContrast, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseShadowsGamma, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseShadowsGain, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseShadowsOffset, _cppUseAllColorGrading);

				// the midtones set
				SetMenuProperty(TargetId::CPPUseMidtonesSaturation, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseMidtonesContrast, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseMidtonesGamma, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseMidtonesGain, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseMidtonesOffset, _cppUseAllColorGrading);

				// the highlights set
				SetMenuProperty(TargetId::CPPUseHighlightsSaturation, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseHighlightsContrast, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseHighlightsGamma, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseHighlightsGain, _cppUseAllColorGrading);
				SetMenuProperty(TargetId::CPPUseHighlightsOffset, _cppUseAllColorGrading);

				// flag the edit has been done
				OnPostChange(camera, targetProperty);

				// as part of a change value process we should return the new value to the hub for display purposes, we
				// use the sister function to be sure to report the current setting
				RequestUseAllColorGrading(targetId);
			}
		}
	});
}

// resets the enabled states for all the main set of post process enabled flags, the function handles both the post process volume and camera color grading targets
void UEngineHelper::ResetUseAllColorGrading(uint32 targetId)
{
	// use the sister function, the step value of 0 means a reset
	StepUseAllColorGrading(targetId, 0);
}

// asynchronously reports the string value of the flag that batch drives the main set of post process enabled flags, note that this does not guarantee that
// each flag is at this state as the user may change any inidividual enabled checkbox at any time, this value is just the last known state of the use all
// flag, the function handles both the post process volume and camera color grading targets
void UEngineHelper::RequestUseAllColorGrading(uint32 targetId)
{
	// we have two target ids to handle, one for post process volumes and one for cameras
	if (targetId == TargetId::UseAllColorGrading)
	{
		// choose an appropriate string
		FString strValue = _ppvUseAllColorGrading? "On": "Off";

		// use the menu string reporting event to pass this data back
		UEngineHelper::MenuStringEvent.Broadcast(TargetId::UseAllColorGrading, strValue);
	}
	else if (targetId == TargetId::CPPUseAllColorGrading)
	{
		// choose an appropriate string
		FString strValue = _cppUseAllColorGrading? "On": "Off";

		// use the menu string reporting event to pass this data back
		UEngineHelper::MenuStringEvent.Broadcast(TargetId::CPPUseAllColorGrading, strValue);
	}
}

// spawns an actor of the specified type and selects it
void UEngineHelper::SpawnActor(UClass* typeUClass)
{
	// seems sensible to do the work on the game thread
	AsyncTask(ENamedThreads::GameThread, [this, typeUClass]()
	{
		// if the gate flag says a save activity is running we block input, this guards against trying to change things while the editor is
		// (auto) saving the objects in the world, we have to do this inside the game thread lambda to allow for call latency
		if (_saveWorldActive)
		{
			// ignore this call as a save is being processed
			return;
		}

		// wrap the action inside a transaction with a contextual description
		FText				description = FText::Format(FText::FromString("Add {0}"), typeUClass->GetDisplayNameText());
		FScopedTransaction	Transaction(description);

		// get the world currently in use, the play world is only set when in play mode, editor mode otherwise
		UWorld* world = GetActiveWorld();

		// create the actor in the appropriate world
		if (world)
		{
			FVector		location;

			// find a location relative to the viewport to place the new actor or use a safefallback if the viewport is not available
			if (FViewport* vpActive = GEditor->GetActiveViewport())
			{
				// get the look-at location from the viewport client as a sensible starting point
				FEditorViewportClient *vpClient = (FEditorViewportClient*) vpActive->GetClient();
				location = vpClient->GetLookAtLocation();
			}
			else
			{
				// just in case - set the location to the origin and up a bit
				location.X = 0.0f;
				location.Y = 0.0f;
				location.Z = 200.0f;
			}
			
			// populate the transform with no rotation, our location and unity scale
			FTransform transform = FTransform(FQuat::Identity, location, FVector::OneVector);

			// (silently) add the actor, this call would usually do a transaction but we already have one in place, it also marks the
			// package as dirty and exclusively selects the new actor for us, triggering all the updates we require
			GEditor->AddActor(world->GetCurrentLevel(), typeUClass, transform, true, RF_Transactional);
		}
	});
}