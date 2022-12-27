// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/ihaptx.h>
#include <Runtime/Core/Public/Modules/ModuleManager.h>

DEFINE_LOG_CATEGORY(HaptX);

class FHaptx : public IHaptx {};

IMPLEMENT_MODULE( FHaptx, HaptX )
