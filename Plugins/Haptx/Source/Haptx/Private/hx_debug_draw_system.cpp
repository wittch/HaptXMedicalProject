// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_debug_draw_system.h>
#include <Runtime/Engine/Classes/Engine/StaticMesh.h>
#include <Haptx/Public/hx_core_actor.h>

void HxDebugDrawSystem::box(
    AActor* actor,
    const FVector& center,
    const FVector& extent,
    const FQuat& rotation,
    const FLinearColor& color) {
  if (!dds_.is_open_) {
    AHxCoreActor::logWarning(TEXT(
        "Attempting to call HxDebugDrawSystem::box() without an AHxCoreActor in the scene. Please add one to the scene for this feature to work properly."));
    return;
  }

  if (actor == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("Null actor pointer in HxDebugDrawSystem::box()."));
    return;
  }

  if (dds_.debug_cube_mesh_ == nullptr) {
    return;
  }

  auto ismc = dds_.getIsmcForColor(actor, dds_.debug_cube_mesh_, color);
  if (ismc == nullptr) {
    return;
  }
  const FBoxSphereBounds bounds = dds_.debug_cube_mesh_->GetBounds();
  const FVector scale = extent / bounds.BoxExtent;
  ismc->AddInstanceWorldSpace(FTransform(rotation, center, scale));
}

void HxDebugDrawSystem::coordinateSystem(
    AActor* actor,
    const FVector& location,
    const FQuat& rotation,
    float scale,
    float thickness) {
  if (!dds_.is_open_) {
    AHxCoreActor::logWarning(TEXT(
        "Attempting to call HxDebugDrawSystem::coordinateSystem() without an AHxCoreActor in the scene. Please add one to the scene for this feature to work properly."));
    return;
  }

  if (actor == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("Null actor pointer in HxDebugDrawSystem::coordinateSystem()."));
    return;
  }

  const FQuat x_rot = rotation;
  const FQuat y_rot = rotation * FQuat({0,0,1}, PI/2);
  const FQuat z_rot = rotation * FQuat({0,1,0}, -PI/2);

  const FVector x_pos = location + x_rot.RotateVector({scale/2, 0, 0});
  const FVector y_pos = location + y_rot.RotateVector({scale/2, 0, 0});
  const FVector z_pos = location + z_rot.RotateVector({scale/2, 0, 0});

  const FVector extents = {scale/2, thickness/2, thickness/2};

  box(actor, x_pos, extents, x_rot, FLinearColor::Red);
  box(actor, y_pos, extents, y_rot, FLinearColor::Green);
  box(actor, z_pos, extents, z_rot, FLinearColor::Blue);
}

void HxDebugDrawSystem::line(
    AActor* actor,
    const FVector& line_start,
    const FVector& line_end,
    const FLinearColor& color,
    float thickness) {
  if (!dds_.is_open_) {
    AHxCoreActor::logWarning(TEXT(
        "Attempting to call HxDebugDrawSystem::line() without an AHxCoreActor in the scene. Please add one to the scene for this feature to work properly."));
    return;
  }

  if (actor == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("Null actor pointer in HxDebugDrawSystem::line()."));
    return;
  }

  const FVector center = (line_start + line_end) / 2;
  const FVector v1 = {1,0,0};
  const float length = (line_end - line_start).Size();
  if (length == 0.f) {
    return;
  }
  const FVector v2 = (line_end - line_start) / length;
  FQuat rotation = FQuat(FVector::CrossProduct(v1, v2).GetSafeNormal(),
      acosf(FVector::DotProduct(v1, v2)));
  rotation.Normalize();

  box(actor, center, {length/2, thickness/2, thickness/2}, rotation, color);
}

void HxDebugDrawSystem::arrow(
    AActor* actor,
    const FVector& line_start,
    const FVector& line_end,
    float arrow_size,
    const FLinearColor& color,
    float thickness) {
  if (!dds_.is_open_) {
    AHxCoreActor::logWarning(TEXT(
        "Attempting to call HxDebugDrawSystem::arrow() without an AHxCoreActor in the scene. Please add one to the scene for this feature to work properly."));
    return;
  }

  if (actor == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("Null actor pointer in HxDebugDrawSystem::arrow()."));
    return;
  }

  const FVector v1 = FVector(1,0,0);
  const float length = (line_end - line_start).Size();
  if (length == 0.f) {
    return;
  }
  const FVector v2 = (line_end - line_start) / length;
  FVector axis = FVector::CrossProduct(v1, v2).GetSafeNormal();
  if (axis.SizeSquared() == 0) {
    axis = {0,1,0};
  }
  const FQuat main_rotation = FQuat(axis, acosf(FVector::DotProduct(v1, v2)));
  const FVector main_center = (line_start + line_end) / 2;

  box(actor, main_center, FVector(length/2, thickness/2, thickness/2), main_rotation, color);

  const FQuat left_rot = main_rotation * FQuat(FVector(0,0,1), PI/4);
  const FQuat right_rot = main_rotation * FQuat(FVector(0,0,1), -PI/4);

  const FVector left_center = line_end + left_rot.RotateVector(
      FVector(-arrow_size/2 + thickness/4, 0, 0));
  const FVector right_center = line_end + right_rot.RotateVector(
      FVector(-arrow_size/2 + thickness/4, 0, 0));

  box(actor, left_center, FVector(arrow_size/2 + thickness/4, thickness/2, thickness/2), left_rot,
      color);
  box(actor, right_center, FVector(arrow_size/2 + thickness/4, thickness/2, thickness/2),
      right_rot, color);
}

void HxDebugDrawSystem::sphere(
    AActor* actor,
    const FVector& center,
    float radius,
    const FLinearColor& color) {
  if (!dds_.is_open_) {
    AHxCoreActor::logWarning(TEXT(
        "Attempting to call HxDebugDrawSystem::sphere() without an AHxCoreActor in the scene. Please add one to the scene for this feature to work properly."));
    return;
  }

  if (actor == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("Null actor pointer in HxDebugDrawSystem::sphere()."));
    return;
  }

  if (dds_.debug_sphere_mesh_ == nullptr) {
    return;
  }

  auto ismc = dds_.getIsmcForColor(actor, dds_.debug_sphere_mesh_, color);
  if (ismc == nullptr) {
    return;
  }
  const FBoxSphereBounds bounds = dds_.debug_sphere_mesh_->GetBounds();
  const float scale = radius / bounds.SphereRadius;
  ismc->AddInstanceWorldSpace(FTransform(FQuat::Identity, center, FVector(scale)));
}

void HxDebugDrawSystem::open() {
  if (dds_.is_open_) {
    return;
  }
  dds_.debug_cube_mesh_ = nullptr;
  dds_.debug_sphere_mesh_ = nullptr;
  dds_.debug_material_ = nullptr;
  dds_.debug_cube_mesh_ = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr,
      TEXT("StaticMesh'/haptx/Misc/cube.cube'")));
  if (dds_.debug_cube_mesh_ == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("HxDebugDrawSystem::open(): Could not find debug cube mesh."));
    return;
  }
  dds_.debug_cube_mesh_->AddToRoot();
  dds_.debug_sphere_mesh_ = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr,
      TEXT("StaticMesh'/haptx/Misc/sphere.sphere'")));
  if (dds_.debug_sphere_mesh_ == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("HxDebugDrawSystem::open(): Could not find debug sphere mesh."));
    return;
  }
  dds_.debug_sphere_mesh_->AddToRoot();
  dds_.debug_material_ = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(),
      nullptr, TEXT("Material'/haptx/Misc/debug_material.debug_material'")));
  if (dds_.debug_material_ == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("HxDebugDrawSystem::open(): Could not find debug material."));
    return;
  }
  dds_.debug_material_->AddToRoot();
  dds_.is_open_ = true;
}

void HxDebugDrawSystem::reset() {
  if (!dds_.is_open_) {
    return;
  }
  for (auto iter : dds_.debug_instanced_cube_mesh_map_) {
    iter.second->ClearInstances();
  }
  dds_.debug_instanced_cube_mesh_map_.clear();
  for (auto iter : dds_.debug_instanced_sphere_mesh_map_) {
    iter.second->ClearInstances();
  }
  dds_.debug_instanced_sphere_mesh_map_.clear();
}

void HxDebugDrawSystem::close() {
  if (!dds_.is_open_) {
    return;
  }
  dds_.debug_instanced_cube_mesh_map_.clear();
  dds_.debug_instanced_sphere_mesh_map_.clear();
  dds_.ismcs_.clear();
  // Add our UObjects back to GC system
  if (IsValid(dds_.debug_cube_mesh_)) {
    dds_.debug_cube_mesh_->RemoveFromRoot();
  }
  if (IsValid(dds_.debug_sphere_mesh_)) {
    dds_.debug_sphere_mesh_->RemoveFromRoot();
  }
  if (IsValid(dds_.debug_material_)) {
    dds_.debug_material_->RemoveFromRoot();
  }
  dds_.is_open_ = false;
}

UInstancedStaticMeshComponent* HxDebugDrawSystem::getIsmcForColor(AActor* actor, UStaticMesh* mesh,
    const FLinearColor& color) {
  if (mesh == nullptr) {
    return nullptr;
  }

  const uint32 color_hash = GetTypeHash(color);
  std::unordered_map<uint32, UInstancedStaticMeshComponent*>& map = mesh == dds_.debug_cube_mesh_ ?
      dds_.debug_instanced_cube_mesh_map_ : dds_.debug_instanced_sphere_mesh_map_;
  auto mesh_iter = map.find(color_hash);

  // If an instanced static mesh does exists in the map
  if (mesh_iter != map.end()) {
    if (IsValid(mesh_iter->second)) {
      return mesh_iter->second;
    }
    else {
      map.erase(mesh_iter);
    }
  }

  // See if there is one we can recycle
  for (auto iter : dds_.ismcs_) {
    // If this one is free to be used
    if (IsValid(iter) && iter->GetInstanceCount() == 0) {
      iter->SetStaticMesh(mesh);
      Cast<UMaterialInstanceDynamic>(iter->GetMaterial(0))->SetVectorParameterValue(
          dds_.material_param_name_, color);
      map.insert({color_hash, iter});
      return iter;
    }
  }

  if (actor == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("Null actor pointer in getIsmcForColor()."));
    return nullptr;
  }

  if (dds_.debug_material_ == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("Could not find debug material."));
    return nullptr;
  }

  // Create a new one
  FString name = TEXT("InstancedMeshSpawner");
  name.AppendInt(dds_.ismcs_.size());
  UInstancedStaticMeshComponent* ismc = NewObject<UInstancedStaticMeshComponent>(actor, *name);
  if (!IsValid(ismc)) {
    return nullptr;
  }
  ismc->SetWorldTransform(FTransform::Identity);
  ismc->RegisterComponent();
  ismc->SetStaticMesh(mesh);
  ismc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  UMaterialInstanceDynamic* mat_inst = UMaterialInstanceDynamic::Create(dds_.debug_material_,
      actor);
  if (mat_inst == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("Could not create dynamic instance of debug_material."));
    return nullptr;
  }
  mat_inst->SetVectorParameterValue(dds_.material_param_name_, color);
  ismc->SetMaterial(0, mat_inst);
  // I have no idea what this boolean needs to be, change to false if you see problems in the visualizers.
  ismc->InitPerInstanceRenderData(true);
  ismc->CastShadow = false;

  // Record it in our data structures
  dds_.ismcs_.push_back(ismc);
  map.insert({color_hash, ismc});

  return ismc;
}

HxDebugDrawSystem::HxDebugDrawSystem() : debug_cube_mesh_(nullptr), debug_sphere_mesh_(nullptr),
    debug_material_(nullptr), material_param_name_(TEXT("Color")), is_open_(false) {}

HxDebugDrawSystem::~HxDebugDrawSystem() {}

HxDebugDrawSystem HxDebugDrawSystem::dds_ = {};
