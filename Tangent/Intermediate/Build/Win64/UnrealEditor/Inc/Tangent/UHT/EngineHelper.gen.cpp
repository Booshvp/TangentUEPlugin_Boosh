// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "EngineHelper.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeEngineHelper() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_ACameraActor_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UActorComponent_NoRegister();
TANGENT_API UClass* Z_Construct_UClass_UEngineHelper();
TANGENT_API UClass* Z_Construct_UClass_UEngineHelper_NoRegister();
UPackage* Z_Construct_UPackage__Script_Tangent();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UEngineHelper ************************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UEngineHelper;
UClass* UEngineHelper::GetPrivateStaticClass()
{
	using TClass = UEngineHelper;
	if (!Z_Registration_Info_UClass_UEngineHelper.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("EngineHelper"),
			Z_Registration_Info_UClass_UEngineHelper.InnerSingleton,
			StaticRegisterNativesUEngineHelper,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_UEngineHelper.InnerSingleton;
}
UClass* Z_Construct_UClass_UEngineHelper_NoRegister()
{
	return UEngineHelper::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UEngineHelper_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "EngineHelper.h" },
		{ "ModuleRelativePath", "Private/EngineHelper.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp__selectedActor_MetaData[] = {
		{ "ModuleRelativePath", "Private/EngineHelper.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp__selectedComponent_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Private/EngineHelper.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp__lastSelectedCamera_MetaData[] = {
		{ "ModuleRelativePath", "Private/EngineHelper.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UEngineHelper constinit property declarations ****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp__selectedActor;
	static const UECodeGen_Private::FObjectPropertyParams NewProp__selectedComponent;
	static const UECodeGen_Private::FObjectPropertyParams NewProp__lastSelectedCamera;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UEngineHelper constinit property declarations ******************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UEngineHelper>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UEngineHelper_Statics

// ********** Begin Class UEngineHelper Property Definitions ***************************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedActor = { "_selectedActor", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UEngineHelper, _selectedActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp__selectedActor_MetaData), NewProp__selectedActor_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedComponent = { "_selectedComponent", nullptr, (EPropertyFlags)0x0040000000080008, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UEngineHelper, _selectedComponent), Z_Construct_UClass_UActorComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp__selectedComponent_MetaData), NewProp__selectedComponent_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UEngineHelper_Statics::NewProp__lastSelectedCamera = { "_lastSelectedCamera", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UEngineHelper, _lastSelectedCamera), Z_Construct_UClass_ACameraActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp__lastSelectedCamera_MetaData), NewProp__lastSelectedCamera_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UEngineHelper_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedActor,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedComponent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UEngineHelper_Statics::NewProp__lastSelectedCamera,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UEngineHelper_Statics::PropPointers) < 2048);
// ********** End Class UEngineHelper Property Definitions *****************************************
UObject* (*const Z_Construct_UClass_UEngineHelper_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_Tangent,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UEngineHelper_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UEngineHelper_Statics::ClassParams = {
	&UEngineHelper::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UEngineHelper_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UEngineHelper_Statics::PropPointers),
	0,
	0x009000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UEngineHelper_Statics::Class_MetaDataParams), Z_Construct_UClass_UEngineHelper_Statics::Class_MetaDataParams)
};
void UEngineHelper::StaticRegisterNativesUEngineHelper()
{
}
UClass* Z_Construct_UClass_UEngineHelper()
{
	if (!Z_Registration_Info_UClass_UEngineHelper.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UEngineHelper.OuterSingleton, Z_Construct_UClass_UEngineHelper_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UEngineHelper.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UEngineHelper);
// ********** End Class UEngineHelper **************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_Tangent1_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h__Script_Tangent_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UEngineHelper, UEngineHelper::StaticClass, TEXT("UEngineHelper"), &Z_Registration_Info_UClass_UEngineHelper, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UEngineHelper), 2979201012U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_Tangent1_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h__Script_Tangent_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_Tangent1_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h__Script_Tangent_430325802{
	TEXT("/Script/Tangent"),
	Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_Tangent1_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h__Script_Tangent_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_Tangent1_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h__Script_Tangent_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
