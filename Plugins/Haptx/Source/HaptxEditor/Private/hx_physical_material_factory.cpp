// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#ifdef WITH_EDITOR
#include <HaptxEditor/Public/hx_physical_material_factory.h>
#include <Haptx/Public/hx_physical_material.h>

UHxPhysicalMaterialFactory::UHxPhysicalMaterialFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    SupportedClass = UHxPhysicalMaterial::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UHxPhysicalMaterialFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
    EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	return NewObject<UObject>(InParent, Class, Name, Flags);
}
#endif // WITH_EDITOR
