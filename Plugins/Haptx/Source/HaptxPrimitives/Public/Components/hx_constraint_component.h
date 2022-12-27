// Copyright (C) 2018-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <HaptxPrimitives/Public/haptx_primitives_shared.h>
#include <HaptxPrimitives/Public/hx_dof.h>
#include <Runtime/Engine/Classes/PhysicsEngine/PhysicsConstraintComponent.h>
#include "hx_constraint_component.generated.h"

//! The two anchors involved in a constraint.

// The two anchors involved in a constraint.
UENUM(BlueprintType)
enum class EAnchor : uint8 {
  ANCHOR1  UMETA(DisplayName = "Anchor1"),  //!< Anchor1 (the child).
  ANCHOR2  UMETA(DisplayName = "Anchor2")   //!< Anchor2 (the parent).
};

//! @brief A space in which anchor1 has meaning.
//! 
//! Gets used by UHxConstraintComponent::getAnchor1Transform();

// A space in which anchor1 has meaning.
UENUM(BlueprintType)
enum class EAnchor1Space : uint8 {
  WORLD         UMETA(DisplayName = "World"),       //!< Relative to the world.
  COMPONENT1    UMETA(DisplayName = "Component1"),  //!< Relative to component1.
  BONE1         UMETA(DisplayName = "Bone1")        //!< Relative to bone1.
};

//! @brief A space in which anchor2 has meaning.
//! 
//! Gets used by UHxConstraintComponent::getAnchor2Transform();

// A space in which anchor2 has meaning.
UENUM(BlueprintType)
enum class EAnchor2Space : uint8 {
  WORLD         UMETA(DisplayName = "World"),       //!< Relative to the world.
  COMPONENT2    UMETA(DisplayName = "Component2"),  //!< Relative to component2.
  BONE2         UMETA(DisplayName = "Bone2")        //!< Relative to bone2.
};

//! The spaces in which forces, torques, impulses, and angular impulses may be applied to 
//! anchors.

// The spaces in which forces, torques, impulses, and angular impulses may be applied to anchors.
UENUM(BlueprintType)
enum class EAnchorForceTorqueSpace : uint8 {
  WORLD    UMETA(DisplayName = "Anchor1"),  //!< In world space.
  ANCHOR2  UMETA(DisplayName = "Anchor2")   //!< In anchor2 space.
};

//! Serialized storage for 1 UHxDof.

// Serialized storage for 1 UHxDof.
USTRUCT()
struct FDof {
  GENERATED_BODY()

  //! The list of physical behaviors affecting the primitive on this UHxDof.

  // The list of physical behaviors affecting the primitive on this UHxDof.
  UPROPERTY(EditAnywhere, Instanced)
  TArray<UHxDofBehavior*> physical_behaviors;

  //! The list of state functions defining the primitive's state on this UHxDof.

  // The list of state functions defining the primitive's state on this UHxDof.
  UPROPERTY(EditAnywhere, Instanced)
  TArray<UHxStateFunction*> state_functions;
};

//! Serialized storage for 1 UHxLinearDof.

// Serialized storage for 1 UHxLinearDof.
USTRUCT()
struct FLinearDof : public FDof {
  GENERATED_BODY()

  //! The serialized UHxLinearDof.

  // The serialized UHxLinearDof.
  UPROPERTY(VisibleAnywhere, Instanced)
  UHxLinearDof* dof;
};

//! Serialized storage for 1 UHxAngularDof.

// Serialized storage for 1 UHxAngularDof.
USTRUCT()
struct FAngularDof : public FDof {
  GENERATED_BODY()

  //! The serialized UHxAngularDof.

  // The serialized UHxAngularDof.
  UPROPERTY(VisibleAnywhere, Instanced)
  UHxAngularDof* dof;
};

//! Storage for each UHxDof comprising full 6-degree-of-freedom motion.

// Storage for each UHxDof comprising full 6-degree-of-freedom motion.
USTRUCT()
struct FAllDofs {
  GENERATED_BODY()

  //! @brief [] operator for easy access to UHxDof pointers.
  //!
  //! The order is X, Y, Z linear then X, Y, Z angular.
  //!
  //! @param index The index of the UHxDof.
  //!
  //! @returns The UHxDof at @p index.

  // [] operator for easy access to UHxDof pointers.
  UHxDof* operator[](int index) const;

  //! The X axis linear degree of freedom and its settings.

  // The X UHxLinearDof.
  UPROPERTY(EditAnywhere)
  FLinearDof x_linear_dof;
  
  //! The Y axis linear degree of freedom and its settings.

  // The Y UHxLinearDof.
  UPROPERTY(EditAnywhere)
  FLinearDof y_linear_dof;
  
  //! The Z axis linear degree of freedom and its settings.
  
  // The Z UHxLinearDof.
  UPROPERTY(EditAnywhere)
  FLinearDof z_linear_dof;

  //! The X (twist) axis angular degree of freedom and its settings.

  // The X (twist) axis angular degree of freedom and its settings.
  UPROPERTY(EditAnywhere)
  FAngularDof x_angular_dof;

  //! The Y (swing1) axis angular degree of freedom and its settings.

  // The Y (swing1) axis angular degree of freedom and its settings.
  UPROPERTY(EditAnywhere)
  FAngularDof y_angular_dof;

  //! The Z (swing2) axis angular degree of freedom and its settings.

  // The Z (swing2) axis angular degree of freedom and its settings.
  UPROPERTY(EditAnywhere)
  FAngularDof z_angular_dof;
};

//! A custom FTickFunction so @link UHxConstraintComponent UHxConstraintComponents @endlink
//! can tick before and after physics.

// A custom FTickFunction so UHxConstraintComponents can tick before and after physics.
USTRUCT()
struct FHxConstraintSecondaryTickFunction : public FTickFunction {
  GENERATED_BODY()

  //! The UHxConstraintComponent that is ticking.
  class UHxConstraintComponent* Target;

  //! Abstract function. Actually execute the tick.
  //!
  //! @param DeltaTime Frame time to advance [s].
  //! @param TickType Kind of tick for this frame.
  //! @param CurrentThread Thread we are executing on, useful to pass along as new tasks are 
  //! created.
  //! @param CompletionGraphEvent Completion event for this task. Useful for holding the 
  //! completion of this task until certain child tasks are complete.
  HAPTXPRIMITIVES_API virtual void ExecuteTick(
    float DeltaTime,
    ELevelTick TickType,
    ENamedThreads::Type CurrentThread,
    const FGraphEventRef& CompletionGraphEvent) override;

  //! Abstract function to describe this tick. Used to print messages about illegal cycles 
  //! in the dependency graph.
  HAPTXPRIMITIVES_API virtual FString DiagnosticMessage() override;
};
template<>
struct TStructOpsTypeTraits<FHxConstraintSecondaryTickFunction> : public TStructOpsTypeTraitsBase2<FHxConstraintSecondaryTickFunction> {
  enum {
    WithCopy = false
  };
};

//! @brief An extension of 
//! [UPhysicsConstraintComponent](http://api.unrealengine.com/INT/API/Runtime/Engine/PhysicsEngine/UPhysicsConstraintComponent/index.html) 
//! that adds additional functionality and allows for an arbitrary number of custom @link 
//! UHxDofBehavior UHxDofBehaviors @endlink and @link UHxStateFunction UHxStateFunctions @endlink 
//! to be defined in constraint (anchor2) space about the six cardinal degrees of freedom.
//!
//! Rotations are represented in degrees. For rotations the absolute value might be > 180 if
//! UHxAngularDof::track_multiple_revolutions_ is true. We measure rotations around a 
//! single axis using a vector projection onto the associated plane. This does not use any
//! Euler angles. This means that functions related to rotations may not do what you're expecting 
//! if the constrained object can rotate around more than one axis.
//!
//! See the @ref section_unreal_hx_constraint_component "Unreal Haptic Primitive Guide" for a high 
//! level overview.
//!
//! @note Do not call UPhysicsConstraintComponent::SetConstrainedComponents(). That will lead to
//! undefined behavior. Use UHxConstraintComponent::hxSetConstrainedComponents instead.
//!
//! @ingroup group_unreal_haptic_primitives

// An extension of UPhysicsConstraintComponent that adds additional functionality and allows for an
// arbitrary number of custom UHxDofBehaviors and UHxStateFunctions to be defined in constraint
// (anchor2) space about the six cardinal degrees of freedom.
UCLASS(ClassGroup = ("HaptX"), DefaultToInstanced, EditInlineNew, meta = (BlueprintSpawnableComponent))
class HAPTXPRIMITIVES_API UHxConstraintComponent : public UPhysicsConstraintComponent {
  GENERATED_UCLASS_BODY()

  friend class UHxDof;

public:

  //! Called when the game starts, but before #BeginPlay().
  virtual void InitializeComponent() override;

#if WITH_EDITOR
  //! Called whenever a property is changed in the editor.
  //!
  //! @param PropertyChangedEvent Information about which property was changed.
  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

  //! Called at the beginning of the game.
  virtual void BeginPlay() override;

  //! Called every frame pre-physics.
  //!
  //! @param DeltaTime The time since the last tick.
  //! @param TickType The kind of tick this is, for example, are we paused, or 'simulating' in the
  //! editor.
  //! @param ThisTickFunction Internal tick function struct that caused this to run.
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  //! Called every frame post-physics.
  //!
  //! @param DeltaTime The time since the last tick.
  //! @param TickType The kind of tick this is, for example, are we paused, or 'simulating' in the
  //! editor.
  //! @param ThisTickFunction Internal tick function struct that caused this to run.
  virtual void TickComponentSecondary(
      float DeltaTime,
      ELevelTick TickType,
      FHxConstraintSecondaryTickFunction& ThisTickFunction);

  //! Settings for this component's second tick function.
  struct FHxConstraintSecondaryTickFunction SecondaryComponentTick;

  //! @brief Changes which physical bodies are constrained, and re-initializes the constraint.
  //! Do not call UPhysicsConstraintComponent::SetConstrainedComponents(). That will lead to
  //! undefined behavior.
  //!
  //! In prior versions of Unreal we were able to hide
  //! UPhysicsConstraintComponent::SetConstrainedComponents() by declaring a new UFUNCTION() with
  //! the same signature. That is no longer the case.
  //!
  //! @param component1 The name of the first component (the child) to constrain.
  //! @param bone_name1 The name of the bone on the first component to constrain.
  //! @param component2 The name of the second component (the parent) to constrain.
  //! @param bone_name2 The name of the bone on the second component to constrain.
  
  // Changes which physical bodies are constrained, and re-initializes the constraint. Do not call
  // UPhysicsConstraintComponent::SetConstrainedComponents(). That will lead to undefined
  // behavior.
  UFUNCTION(BlueprintCallable)
  void hxSetConstrainedComponents(
      UPrimitiveComponent* component1, FName bone_name1, 
      UPrimitiveComponent* component2, FName bone_name2);

  //! Updates the constraint with any changes made to underlying settings.

  // Updates the constraint with any changes made to underlying settings.
  UFUNCTION(BlueprintCallable)
  void updateConstraint();

  //! Changes the linear limits offset and updates the constraint.
  //!
  //! @param offset The new linear limits offset.

  // Changes the linear limits offset and updates the constraint.
  UFUNCTION(BlueprintCallable)
  void setLinearLimitsOffset(const FVector offset);

  //! Changes the angular limits offset and updates the constraint.
  //!
  //! @param offset The new angular limits offset.

  // Changes the angular limits offset and updates the constraint.
  UFUNCTION(BlueprintCallable)
  void setAngularLimitsOffset(const FRotator offset);

  //! @brief Get the transform of anchor1 in @p space.
  //!
  //! Scale is always one.
  //!
  //! @param space The space to get anchor1's transform in.
  //!
  //! @returns The transform of anchor1 in @p space.

  // Get the transform of anchor1 in space.
  UFUNCTION(BlueprintCallable)
  FTransform getAnchor1Transform(EAnchor1Space space = EAnchor1Space::WORLD) const;

  //! @brief Get the transform of anchor2 in @p space.
  //!
  //! Scale is always one.
  //!
  //! @param space The space to get anchor2's transform in.
  //!
  //! @returns The transform of anchor2 in @p space.

  // Get the transform of anchor2 in space.
  UFUNCTION(BlueprintCallable)
  FTransform getAnchor2Transform(EAnchor2Space space = EAnchor2Space::WORLD) const;

  //! @brief Calculates the constraint space transform being used to drive physical behaviors and 
  //! states. 
  //!
  //! This is the local transform of component1's anchor in component2's anchor's frame. Scale will 
  //! always be one. See @ref section_unreal_hx_constraint_component_constraint_space for more
  //! information about constraint space.
  //!
  //! @returns The constraint space transform being used to drive physical behaviors and 
  //! states. 

  // Calculates the constraint space transform being used to drive physical behaviors and states. 
  UFUNCTION(BlueprintCallable)
  FTransform calculateConstraintTransform() const;

  //! Teleports component1 such that anchor1 now has the given position and rotation in 
  //! anchor2's frame.
  //!
  //! @param new_location Anchor1's new location.
  //! @param new_rotation Anchor1's new rotation.

  // Teleports component1 such that anchor1 now has the given position and rotation in anchor2's 
  // frame.
  UFUNCTION(BlueprintCallable)
  void teleportAnchor1(FVector new_location, FRotator new_rotation);

  //! Teleports component1 such that anchor1 now has the given position in anchor2's frame 
  //! along a specified degree of freedom.
  //!
  //! @param new_position Anchor1's new position along anchor2's @p degree_of_freedom.
  //! @param degree_of_freedom Anchor2's degree of freedom to teleport along.

  // Teleports component1 such that anchor1 now has the given position in anchor2's frame along a 
  // specified degree of freedom.
  UFUNCTION(BlueprintCallable)
  void teleportAnchor1AlongDof(float new_position, EDegreeOfFreedom degree_of_freedom);

  //! Apply force at anchor.
  //!
  //! @param force The force value to apply.
  //! @param anchor Which anchor to apply @p force to.
  //! @param space The space to apply @p force in.
  //! @param accel_change Whether to apply @p force as an acceleration or a force.
  //! @param visualize Whether to visualize @p force. Lasts for one frame.

  // Apply force at anchor.
  UFUNCTION(BlueprintCallable)
  void addForceAtAnchor(FVector force, EAnchor anchor, 
      EAnchorForceTorqueSpace space = EAnchorForceTorqueSpace::WORLD, bool accel_change = false, 
      bool visualize = false);

  //! Apply torque at anchor.
  //!
  //! @param torque The torque value to apply.
  //! @param anchor Which anchor to apply @p torque to.
  //! @param space The space to apply @p torque in.
  //! @param accel_change Whether to apply @p torque as an acceleration or a torque.
  //! @param visualize Whether to visualize @p torque. Lasts for one frame.

  // Apply torque at anchor.
  UFUNCTION(BlueprintCallable)
  void addTorqueAtAnchor(FVector torque, EAnchor anchor, 
      EAnchorForceTorqueSpace space = EAnchorForceTorqueSpace::WORLD, bool accel_change = false, 
      bool visualize = false);

  //! Apply impulse at anchor.
  //!
  //! @param impulse The impulse value to apply.
  //! @param anchor Which anchor to apply @p impulse to.
  //! @param space The space to apply @p impulse in.
  //! @param vel_change Whether to apply @p impulse as a velocity change or an impulse.
  //! @param visualize Whether to visualize @p impulse. Lasts for one frame.

  // Apply impulse at anchor.
  UFUNCTION(BlueprintCallable)
  void addImpulseAtAnchor(FVector impulse, EAnchor anchor,
      EAnchorForceTorqueSpace space = EAnchorForceTorqueSpace::WORLD, bool vel_change = false,
      bool visualize = false);

  //! Apply angular impulse at anchor.
  //!
  //! @param angular_impulse The angular impulse value to apply.
  //! @param anchor Which anchor to apply @p angular_impulse to.
  //! @param space The space to apply @p angular_impulse in.
  //! @param vel_change Whether to apply @p angular_impulse as an angular velocity change or an 
  //! angular impulse.
  //! @param visualize Whether to visualize @p angular_impulse . Lasts for one frame.

  // Apply angular impulse at anchor.
  UFUNCTION(BlueprintCallable)
  void addAngularImpulseAtAnchor(FVector angular_impulse, EAnchor anchor,
      EAnchorForceTorqueSpace space = EAnchorForceTorqueSpace::WORLD, bool vel_change = false,
      bool visualize = false);

  //! Get the UHxDof for the given degree of freedom.
  //!
  //! @param degree_of_freedom The degree of freedom of interest.
  //!
  //! @returns The corresponding UHxDof.

  // Get the UHxDof for the given degree of freedom.
  UFUNCTION(BlueprintCallable)
  UHxDof* getDof(EDegreeOfFreedom degree_of_freedom) const;
  
  //! Get the current position of anchor1 in anchor2's frame about a given degree of 
  //! freedom.
  //!
  //! @param degree_of_freedom The degree of freedom of interest.
  //!
  //! @returns The current position of anchor1 in anchor2's frame.

  // Get the current position of anchor1 in anchor2's frame about a given degree of freedom.
  UFUNCTION(BlueprintCallable, BlueprintPure)
  float getPositionAlongDof(EDegreeOfFreedom degree_of_freedom);

  //! Freeze the constraint in place, locking all degrees of freedom.

  // Freeze the constraint in place, locking all degrees of freedom.
  UFUNCTION(BlueprintCallable)
  void freeze();

  //! Unfreeze the constraint, allowing motion defined by other settings once more.

  // Unfreeze the constraint, allowing motion defined by other settings once more.
  UFUNCTION(BlueprintCallable)
  void unfreeze();

  //! @brief Whether this constraint is sleeping.
  //!
  //! The joint will sleep if all rigid bodies participating in the joint are sleeping.
  //!
  //! @returns True if this constraint is sleeping.

  // Whether this constraint is sleeping.
  UFUNCTION(BlueprintCallable, BlueprintPure)
  bool isSleeping();

  //! Whether this constraint is formed.
  //!
  //! @returns True if the constraint is formed.

  // Whether the constraint is formed.
  UFUNCTION(BlueprintCallable, BlueprintPure)
  bool isConstraintFormed();

  //! @brief Whether to visualize the transforms of the anchors where each component is being 
  //! constrained.
  //!
  //! This is helpful for debugging the setup of your constraint.

  // Whether to visualize the transforms of the anchors where each component is being constrained.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HxConstraint | Visualizer")
  bool visualize_anchors_;

protected:
  //! Changing visibility because this function has been effectively overridden by
  //! UHxConstraintComponent::hxSetConstrainedComponents(), but not actually overridden because the
  //! parent function isn't virtual. In a future release we plan to change re-implement this class
  //! with a composition relationship instead of an inheritance one.
  using UPhysicsConstraintComponent::SetConstrainedComponents;

  //! @brief Configures HaptX settings on this component, and UPhysicsConstraintComponent settings
  //! in the given FConstraintInstance. 
  //!
  //! Gets called in PostEditChangeProperty() and just-in-time in #updateConstraint().
  //!
  //! @param in_constraint_instance The FConstraintInstance to configure.

  // Configures HaptX settings on this component, and UPhysicsConstraintComponent settings in
  // the given FConstraintInstance.
  virtual void configureConstraint(FConstraintInstance& in_constraint_instance) {}

  //! @brief An offset applied to the UPhysicsConstraintComponent linear limits of this constraint.
  //!
  //! E.G. if the linear limit is 10, and this value is 10 in the forward direction, 
  //! then the acceptable range of motion in the forward direction is 0 to 20 instead of -10 to 10.
  //! Only gets reflected after calls to #updateConstraint() or #hxSetConstrainedComponents().

  // An offset applied to the UPhysicsConstraintComponent linear limit of this constraint.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = HxConstraint)
  FVector linear_limits_offset_;

  //! The UHxDofs defining the degrees of freedom of this constraint.

  // The UHxDofs defining the degrees of freedom of this constraint.
  UPROPERTY(EditAnywhere, AdvancedDisplay, meta = (ShowOnlyInnerProperties), Category = HxConstraint)
  FAllDofs dofs_;

  //! @brief A pointer to constrained component1 (or its weld parent). 
  //!
  //! Will be nullptr if component2 is constrained to the world.

  // A pointer to constrained component1 (or its weld parent).
  UPROPERTY(BlueprintReadOnly, Category = HxConstraint)
  UPrimitiveComponent* component1_;

  //! @brief A pointer to constrained component2 (or its weld parent).
  //!
  //! Will be nullptr if component1 is constrained to the world.

  // A pointer to constrained component2 (or its weld parent).
  UPROPERTY(BlueprintReadOnly, Category = HxConstraint)
  UPrimitiveComponent* component2_;

  //! Whether this constraint is currently frozen by a call to #freeze().

  // Whether this constraint is currently frozen by a call to freeze().
  UPROPERTY(BlueprintReadOnly, Category = HxConstraint)
  bool frozen_;

  //! @brief The settings used to drive the underlying UPhysicsConstraintComponent
  //! (when not frozen). 
  //!
  //! May be configured at runtime by child classes.
  //!
  //! Gets copied from UPhysicsConstraintComponent::ConstraintInstance at InitializeComponent, and 
  //! copied into UPhysicsConstraintComponent::ConstraintInstance on every call to 
  //! #updateConstraint(). 
  //! 
  //! If #frozen_ is true, then the settings in UPhysicsConstraintComponent::ConstraintInstance 
  //! will deviate from #hx_constraint_instance_ as necessary.

  // The settings used to drive the underlying UPhysicsConstraintComponent (when not frozen).
  FConstraintInstance hx_constraint_instance_;

private:

  //! Initializes both UHxConstraintComponent and UPhysicsConstraintComponent functionality.
  void InitComponentConstraint();

  //! Apply UHxDofBehaviors to both components.
  void applyDofBehaviors();

  //! Updates internal UHxDofs once per frame.
  void updateDofs();

  //! Internal function called by either #addForceAtAnchor() or #addImpulseAtAnchor()
  //!
  //! @param force The force value to apply.
  //! @param anchor Which anchor to apply @p force to.
  //! @param space The space to apply @p force in.
  //! @param accel_change Whether to apply @p force as an acceleration or a force.
  //! @param visualize Whether to visualize @p force. Lasts for one frame.
  //! @param impulse Whether to apply @p force as an impulse or a force.
  void addForceAtAnchorInternal(FVector force, EAnchor anchor,
      EAnchorForceTorqueSpace space = EAnchorForceTorqueSpace::WORLD, bool accel_change = false,
      bool visualize = false, bool impulse = false);

  //! Internal function called by either #addTorqueAtAnchor() or 
  //! #addAngularImpulseAtAnchor()
  //!
  //! @param torque The torque value to apply.
  //! @param anchor Which anchor to apply @p torque to.
  //! @param space The space to apply @p torque in.
  //! @param accel_change Whether to apply @p torque as an acceleration or a torque.
  //! @param visualize Whether to visualize @p torque. Lasts for one frame.
  //! @param impulse Whether to apply @p as an angular impulse or a torque.
  void addTorqueAtAnchorInternal(FVector torque, EAnchor anchor,
      EAnchorForceTorqueSpace space = EAnchorForceTorqueSpace::WORLD, bool accel_change = false,
      bool visualize = false, bool impulse = false);

  //! @brief The transform of anchor2 in component2's bone's frame.
  //!
  //! Updated when #InitComponentConstraint() is called.
  FTransform b2_anchor2_;

  //! @brief The transform of component2's bone in anchor2's frame.
  //!
  //! Updated when #InitComponentConstraint() is called.
  FTransform a2_bone2_;

  //! @brief The transform of anchor1 in component1's bone's frame.
  //!
  //! Updated when #InitComponentConstraint() is called.
  FTransform b1_anchor1_;

  //! @brief The transform of component1's bone in anchor1's frame.
  //!
  //! Updated when #InitComponentConstraint() is called.
  FTransform a1_bone1_;

  //! The world scale of this UHxConstraintComponent as of the last call to 
  //! #InitComponentConstraint().
  FVector w_constraint_scale_;

  //! The inverse of w_constraint_scale_.
  FVector l_world_scale_;

  //! Scales visualizers based on the size of the constrained components and the lengths of
  //! their anchors.
  float w_viz_scale_;

};
