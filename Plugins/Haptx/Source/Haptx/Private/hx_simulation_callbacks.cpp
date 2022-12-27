// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_simulation_callbacks.h>
#include <Haptx/Private/haptx_shared.h>

PrimitiveComponentCallbacks::PrimitiveComponentCallbacks(
    UPrimitiveComponent* component, FName bone) : component_(component), bone_(bone) {}

HaptxApi::Vector3D PrimitiveComponentCallbacks::getPositionM() const {
  if (component_.IsValid()) {
    return hxFromUnrealLength(component_->GetCenterOfMass(bone_));
  }
  else {
    return HaptxApi::Vector3D::zero();
  }
}

HaptxApi::Quaternion PrimitiveComponentCallbacks::getRotation() const {
  if (component_.IsValid()) {
    return hxFromUnreal(FQuat(component_->GetSocketRotation(bone_)));
  }
  else {
    return HaptxApi::Quaternion::identity();
  }
}

HaptxApi::Vector3D PrimitiveComponentCallbacks::getLossyScale() const {
  if (component_.IsValid()) {
    return hxFromUnrealScale(component_->GetSocketTransform(bone_).GetScale3D());
  }
  else {
    return HaptxApi::Vector3D::one();
  }
}

HaptxApi::Transform PrimitiveComponentCallbacks::getTransform() const {
  return HaptxApi::Transform(getPositionM(), getRotation(), getLossyScale());
}

HaptxApi::Vector3D PrimitiveComponentCallbacks::getLinearVelocityM_S() const {
  if (component_.IsValid()) {
    return hxFromUnrealLength(component_->GetPhysicsLinearVelocity(bone_));
  }
  else {
    return HaptxApi::Vector3D::zero();
  }
}

HaptxApi::Vector3D PrimitiveComponentCallbacks::getAngularVelocityRad_S() const {
  if (component_.IsValid()) {
    return hxFromUnrealAngularVelocity(component_->GetPhysicsAngularVelocityInDegrees(bone_));
  }
  else {
    return HaptxApi::Vector3D::zero();
  }
}

WeldedComponentCallbacks::WeldedComponentCallbacks(UPrimitiveComponent* component, FName socket,
    FTransform l_transform) : component_(component), socket_(socket), l_transform_(l_transform) {}

HaptxApi::Vector3D WeldedComponentCallbacks::getPositionM() const {
  if (component_.IsValid()) {
    return hxFromUnrealLength(
        component_->GetSocketTransform(socket_).TransformPosition(l_transform_.GetLocation()));
  }
  else {
    return HaptxApi::Vector3D::zero();
  }
}

HaptxApi::Quaternion WeldedComponentCallbacks::getRotation() const {
  if (component_.IsValid()) {
    return hxFromUnreal(component_->GetSocketQuaternion(socket_) * l_transform_.GetRotation());
  }
  else {
    return HaptxApi::Quaternion::identity();
  }
}

HaptxApi::Vector3D WeldedComponentCallbacks::getLossyScale() const {
  if (component_.IsValid()) {
    return hxFromUnrealScale(UKismetMathLibrary::ComposeTransforms(l_transform_,
        component_->GetSocketTransform(socket_)).GetScale3D());
  }
  else {
    return HaptxApi::Vector3D::one();
  }
}

HaptxApi::Transform WeldedComponentCallbacks::getTransform() const {
  return hxFromUnreal(getUnrealWorldTransform());
}

HaptxApi::Vector3D WeldedComponentCallbacks::getLinearVelocityM_S() const {
  if (component_.IsValid()) {
    return hxFromUnrealLength(
        component_->GetPhysicsLinearVelocityAtPoint(
        component_->GetSocketTransform(socket_).TransformPosition(l_transform_.GetLocation())));
  }
  else {
    return HaptxApi::Vector3D::zero();
  }
}

HaptxApi::Vector3D WeldedComponentCallbacks::getAngularVelocityRad_S() const {
  if (component_.IsValid()) {
    return hxFromUnrealAngularVelocity(component_->GetPhysicsAngularVelocityInDegrees());
  }
  else {
    return HaptxApi::Vector3D::zero();
  }
}

FTransform WeldedComponentCallbacks::getUnrealWorldTransform() const {
  if (component_.IsValid()) {
    return UKismetMathLibrary::ComposeTransforms(l_transform_,
        component_->GetSocketTransform(socket_));
  } else {
    return FTransform::Identity;
  }
}

FTransform WeldedComponentCallbacks::getUnrealLocalTransform() const {
  return l_transform_;
}
