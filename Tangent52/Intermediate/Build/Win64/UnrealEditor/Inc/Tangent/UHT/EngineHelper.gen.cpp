// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Tangent/Private/EngineHelper.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeEngineHelper() {}
// Cross Module References
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_ACameraActor_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UActorComponent_NoRegister();
	TANGENT_API UClass* Z_Construct_UClass_UEngineHelper();
	TANGENT_API UClass* Z_Construct_UClass_UEngineHelper_NoRegister();
	UPackage* Z_Construct_UPackage__Script_Tangent();
// End Cross Module References
	void UEngineHelper::StaticRegisterNativesUEngineHelper()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UEngineHelper);
	UClass* Z_Construct_UClass_UEngineHelper_NoRegister()
	{
		return UEngineHelper::StaticClass();
	}
	struct Z_Construct_UClass_UEngineHelper_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp__selectedActor_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp__selectedActor;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp__selectedComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp__selectedComponent;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp__lastSelectedCamera_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp__lastSelectedCamera;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UEngineHelper_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_Tangent,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UEngineHelper_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "EngineHelper.h" },
		{ "ModuleRelativePath", "Private/EngineHelper.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedActor_MetaData[] = {
		{ "ModuleRelativePath", "Private/EngineHelper.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedActor = { "_selectedActor", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(UEngineHelper, _selectedActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedActor_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedActor_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedComponent_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Private/EngineHelper.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedComponent = { "_selectedComponent", nullptr, (EPropertyFlags)0x0040000000080008, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(UEngineHelper, _selectedComponent), Z_Construct_UClass_UActorComponent_NoRegister, METADATA_PARAMS(Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedComponent_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedComponent_MetaData)) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UEngineHelper_Statics::NewProp__lastSelectedCamera_MetaData[] = {
		{ "ModuleRelativePath", "Private/EngineHelper.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UEngineHelper_Statics::NewProp__lastSelectedCamera = { "_lastSelectedCamera", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, 1, nullptr, nullptr, STRUCT_OFFSET(UEngineHelper, _lastSelectedCamera), Z_Construct_UClass_ACameraActor_NoRegister, METADATA_PARAMS(Z_Construct_UClass_UEngineHelper_Statics::NewProp__lastSelectedCamera_MetaData, UE_ARRAY_COUNT(Z_Construct_UClass_UEngineHelper_Statics::NewProp__lastSelectedCamera_MetaData)) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UEngineHelper_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedActor,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UEngineHelper_Statics::NewProp__selectedComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UEngineHelper_Statics::NewProp__lastSelectedCamera,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UEngineHelper_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UEngineHelper>::IsAbstract,
	};
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
		METADATA_PARAMS(Z_Construct_UClass_UEngineHelper_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UEngineHelper_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UEngineHelper()
	{
		if (!Z_Registration_Info_UClass_UEngineHelper.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UEngineHelper.OuterSingleton, Z_Construct_UClass_UEngineHelper_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UEngineHelper.OuterSingleton;
	}
	template<> TANGENT_API UClass* StaticClass<UEngineHelper>()
	{
		return UEngineHelper::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UEngineHelper);
	struct Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_TangentDesk52_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_TangentDesk52_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UEngineHelper, UEngineHelper::StaticClass, TEXT("UEngineHelper"), &Z_Registration_Info_UClass_UEngineHelper, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UEngineHelper), 463190680U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_TangentDesk52_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h_3970536779(TEXT("/Script/Tangent"),
		Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_TangentDesk52_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_james_Documents_Unreal_Projects_TangentDesk52_Plugins_Tangent_Source_Tangent_Private_EngineHelper_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
