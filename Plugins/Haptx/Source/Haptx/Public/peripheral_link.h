// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Core/Public/CoreMinimal.h>
#include <HaptxApi/peripheral.h>
#include "peripheral_link.generated.h"

//! Blueprint delegate for IPeripheralLink.

// Blueprint delegate for IPeripheralLink.
UINTERFACE()
class UPeripheralLink : public UInterface {
  GENERATED_BODY()
};

//! Interface for HaptX classes associated with a HaptxApi::Peripheral.
class IPeripheralLink {
  GENERATED_BODY()

  public:
    //! Retrieve the HaptxApi::Peripheral associated with the inheriting class. 
    //!
    //! TODO: Make this function return a value once the CI doesn't need pointers anymore.
    //!
    //! @returns The HaptxApi::Peripheral associated with the inheriting class if successfully
    //! determined or nullptr otherwise.
    virtual std::shared_ptr<HaptxApi::Peripheral> getPeripheral() = 0;
};
