// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <unordered_map>
#include <vector>
#include <Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h>
#include <Runtime/Engine/Classes/Components/InstancedStaticMeshComponent.h>
#include <Runtime/Engine/Classes/Materials/MaterialInstanceDynamic.h>

//! A custom debug draw system to replace Unreal's DrawDebug* functions. Allows us to use our
//! key visualizers in all game build configurations.
class HAPTX_API HxDebugDrawSystem {
 public:
  //! A custom debug draw implementation of Unreal's DrawDebugSolidBox() function. This one
  //! works in all build configurations.
  //!
  //! @param actor The actor to attach new components to if we need to create them.
  //! @param center The location where the center of the box should be in cm.
  //! @param extent The extents of the box along each of its axes in cm.
  //! @param rotation The rotation with which to draw the box.
  //! @param color The color with which to draw the box.
  static void box(
      AActor* actor,
      const FVector& center,
      const FVector& extent,
      const FQuat& rotation,
      const FLinearColor& color);

  //! A custom debug draw implementation of Unreal's DrawDebugCoordinateSystem() function.
  //! This one works in all build configurations.
  //!
  //! @param actor The actor to attach new components to if we need to create them.
  //! @param location The location where the center of the coordinate system should be in cm.
  //! @param rotation The rotation with which to draw the coordinate system.
  //! @param scale The length of the drawn axes in cm.
  //! @param thickness The thickness of the drawn axes in cm.
  static void coordinateSystem(
      AActor* actor,
      const FVector& location,
      const FQuat& rotation,
      float scale,
      float thickness);

  //! A custom debug draw implementation of Unreal's DrawDebugLine() function. This one works
  //! in all build configurations.
  //!
  //! @param actor The actor to attach new components to if we need to create them.
  //! @param line_start The location where the start of the line should be in cm.
  //! @param line_end The location where the end of the line should be in cm.
  //! @param color The color with which to draw the line.
  //! @param thickness The thickness of the drawn line in cm.
  static void line(
      AActor* actor,
      const FVector& line_start,
      const FVector& line_end,
      const FLinearColor& color,
      float thickness);

  //! A custom debug draw implementation of Unreal's DrawDebugDirectionalArrow() function.
  //! This one works in all build configurations.
  //!
  //! @param actor The actor to attach new components to if we need to create them.
  //! @param line_start The location where the start of the line should be in cm.
  //! @param line_end The location where the end of the line should be in cm.
  //! @param arrow_size The length of the wings of the arrowhead in cm.
  //! @param color The color with which to draw the arrow.
  //! @param thickness The thickness of the arrow's lines in cm.
  static void arrow(
      AActor* actor,
      const FVector& line_start,
      const FVector& line_end,
      float arrow_size,
      const FLinearColor& color,
      float thickness);

  //! A custom debug draw implementation of Unreal's DrawDebugSphere() function. This one
  //! works in all build configurations.
  //!
  //! @param actor The actor to attach new components to if we need to create them.
  //! @param center The location where the center of the sphere should be in cm.
  //! @param radius The radius of the sphere in cm.
  //! @param color The color with which to draw the sphere.
  static void sphere(
      AActor* actor,
      const FVector& center,
      float radius,
      const FLinearColor& color);

  //! Initializes the debug drawing system. Doing this work after construction avoids static
  //! memory issues.
  static void open();

  //! Call this immediately after each visual frame has been rendered to ready the debug
  //! drawing system for the next frame.
  static void reset();

  //! Tears down the debug drawing system. Doing this work before destruction avoids static
  //! memory issues.
  static void close();

 private:
  //! Hidden default constructor.
  HxDebugDrawSystem();

  //! Hidden destructor.
  ~HxDebugDrawSystem();

  //! Get the UInstancedStaticMeshComponent for the color you want to draw. This will create
  //! the component if one isn't already assigned and we don't have any to recycle from previous
  //! frames.
  //!
  //! @param actor The actor to attach the new component to if we need to create one.
  //! @param mesh The mesh we want an ISMC for. Should be one of the ones defined in this file.
  //! @param color The color you're trying to draw a shape with.
  //! @returns The UInstancedStaticMeshComponent for the color you want to draw.
  static UInstancedStaticMeshComponent* getIsmcForColor(AActor* actor, UStaticMesh* mesh,
      const FLinearColor& color);

  //! A list of ISMCs to reuse between frames.
  std::list<UInstancedStaticMeshComponent*> ismcs_;

  //! Map from color hashes to cube mesh ISMCs.
  std::unordered_map<uint32, UInstancedStaticMeshComponent*> debug_instanced_cube_mesh_map_;

  //! Map from color hashes to sphere mesh ISMCs.
  std::unordered_map<uint32, UInstancedStaticMeshComponent*> debug_instanced_sphere_mesh_map_;

  //! The singleton instance of this class.
  static HxDebugDrawSystem dds_;

  //! The mesh to use for debug draws drawing cubes.
  UPROPERTY()
  UStaticMesh* debug_cube_mesh_;

  //! The mesh to use for debug draws drawing spheres.
  UPROPERTY()
  UStaticMesh* debug_sphere_mesh_;

  //! The material to draw on debug meshes.
  UPROPERTY()
  UMaterialInterface* debug_material_;

  //! The name of the parameter on debug_material_ dynamic instances to set color values for.
  const FName material_param_name_;

  //! Whether the debug drawing system is currently open.
  bool is_open_;
};
