// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <Runtime/Engine/Classes/Components/PrimitiveComponent.h>
#include <HaptxApi/simulation_callbacks.h>

//! Callbacks associated with UPrimitiveComponents being used by the HaptX SDK.
class HAPTX_API PrimitiveComponentCallbacks : public HaptxApi::SimulationCallbacks {

public:
  //! Associate a UPrimitiveComponent/bone with a set of callbacks.
  //!
  //! @param component The component to associate these callbacks with.
  //! @param bone The bone to associate these callbacks with.
  PrimitiveComponentCallbacks(UPrimitiveComponent* component, FName bone = NAME_None);

  //! Gets the world position of the component/bone's center of mass [m].
  //!
  //! @returns The world position of the component/bone's center of mass [m].
  HaptxApi::Vector3D getPositionM() const override;

  //! Gets the world rotation of the component/bone.
  //!
  //! @returns The world rotation of the component/bone.
  HaptxApi::Quaternion getRotation() const override;

  //! Gets the lossy scale of the component/bone.
  //!
  //! @returns The lossy scale of the component/bone.
  HaptxApi::Vector3D getLossyScale() const override;

  //! Gets the world transform of the component/bone.
  //!
  //! @returns The world transform of the component/bone.
  HaptxApi::Transform getTransform() const override;

  //! Gets the world linear velocity of the component/bone [m/s].
  //!
  //! @returns The world linear velocity of the component/bone [m/s].
  HaptxApi::Vector3D getLinearVelocityM_S() const override;

  //! Gets the world angular velocity of the component/bone [rad/s].
  //!
  //! @returns The world angular velocity of the component/bone [rad/s].
  HaptxApi::Vector3D getAngularVelocityRad_S() const override;

private:
  //! The component these callbacks are associated with.
  TWeakObjectPtr<UPrimitiveComponent> component_;

  //! The bone these callbacks are associated with.
  FName bone_;
};

//! Special callbacks used to represent a UPrimitiveComponent that doesn't have a physical
//! presence in the world but is welded to another UPrimitiveComponent that does.
class HAPTX_API WeldedComponentCallbacks : public HaptxApi::SimulationCallbacks {

public:
  //! Associate a UPrimitiveComponent with a set of HaptxApi::ContactInterpreter
  //! callbacks.
  //!
  //! @param component The component to associate these callbacks with.
  //! @param socket The socket to associate these callbacks with.
  //! @param l_transform A local transform applied to the socket transform.
  WeldedComponentCallbacks(UPrimitiveComponent* component, FName socket = NAME_None,
      FTransform l_transform = FTransform::Identity);

  //! Gets the world position of the component's socket [m].
  //!
  //! @returns The world position of the component's socket [m].
  HaptxApi::Vector3D getPositionM() const override;

  //! Gets the world rotation of the component's socket.
  //!
  //! @returns The world rotation of the component's socket.
  HaptxApi::Quaternion getRotation() const override;

  //! Gets the lossy scale of the component's socket.
  //!
  //! @returns The lossy scale of the component's socket.
  HaptxApi::Vector3D getLossyScale() const override;

  //! Gets the world transform of the component's socket.
  //!
  //! @returns The world transform of the component's socket.
  HaptxApi::Transform getTransform() const override;

  //! Gets the world linear velocity of the weld parent [m/s].
  //!
  //! @returns The world linear velocity of the weld parent [m/s].
  HaptxApi::Vector3D getLinearVelocityM_S() const override;

  //! Gets the world angular velocity of the weld parent [rad/s].
  //!
  //! @returns The world angular velocity of the weld parent [rad/s].
  HaptxApi::Vector3D getAngularVelocityRad_S() const override;

  //! Get the world transform being used by these callbacks.
  //!
  //! @returns The world transform being used by these callbacks.
  FTransform getUnrealWorldTransform() const;

  //! Get the local transform being used by these callbacks.
  //!
  //! @returns The local transform being used by these callbacks.
  FTransform getUnrealLocalTransform() const;

private:
  //! The component these callbacks are associated with.
  TWeakObjectPtr<UPrimitiveComponent> component_;

  //! The socket these callbacks are associated with.
  FName socket_;

  //! A local transform applied to all return values.
  FTransform l_transform_;
};
