// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <HaptxPrimitives/Public/ihaptx_primitives.h>
#include <Runtime/Core/Public/Modules/ModuleManager.h>

DEFINE_LOG_CATEGORY(HaptxPrimitives);

class FHaptxPrimitives : public IHaptxPrimitives {};

IMPLEMENT_MODULE( FHaptxPrimitives, HaptxPrimitives )
