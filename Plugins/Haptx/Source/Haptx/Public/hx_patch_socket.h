// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include "hx_patch_socket.generated.h"

//! Blueprint delegate for IHxPatchSocket.

// Blueprint delegate for IHxPatchSocket.
UINTERFACE()
class HAPTX_API UHxPatchSocket : public UInterface {
  GENERATED_BODY()
};

class UHxPatchComponent;

//! This interface is a means for @link UHxPatchComponent UHxPatchComponents @endlink to 
//! discover which tactors they're supposed to load.
class HAPTX_API IHxPatchSocket {
  GENERATED_BODY()

public:
  //! @brief Get the world transform of a locating feature.
  //!
  //! @param locating_feature Which locating feature.
  //! @param [out] w_transform Populated with the locating feature's world transform.
  //! @returns True if @p w_transform is successfully populated.
  virtual bool tryGetLocatingFeatureTransform(FName locating_feature, FTransform* w_transform) = 0;

  //! @brief Get the relative direction the patch should use.
  //!
  //! @param [out] rel_dir Populated with the relative direction the patch should use.
  //! @returns True if @p rel_dir is successfully populated.
  virtual bool tryGetRelativeDirection(ERelativeDirection* rel_dir) = 0;

  //! Get the HaptxApi::ContactInterpreter body ID of the attach parent.
  //!
  //! @param patch The patch calling this function.
  //! @param[out] ci_body_id The body ID associated with the bone the patch is attached to. Null
  //! if not associated.
  //!
  //! @returns Whether a body ID was found.
  virtual bool tryGetCiBodyId(const UHxPatchComponent &patch, int64_t *ci_body_id) = 0;
};
