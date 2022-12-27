// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#pragma once

#include <deque>
#include <sstream>
#include <unordered_map>
#include <Runtime/Engine/Classes/Components/SphereComponent.h>
#include <Runtime/Engine/Classes/GameFramework/Actor.h>
#include <Runtime/Engine/Classes/PhysicsEngine/PhysicsConstraintComponent.h>
#include <HaptxApi/contact_interpreter.h>
#include <HaptxApi/grasp_detector.h>
#include <HaptxApi/haptx_system.h>
#include <HaptxApi/system_logger.h>
#include <Haptx/Private/haptx_shared.h>
#include <Haptx/Public/contact_interpreter_parameters.h>
#include <Haptx/Public/hx_on_screen_log.h>
#include <Haptx/Public/hx_physical_material.h>
#include <Haptx/Public/ihaptx.h>
#include "hx_core_actor.generated.h"

DECLARE_STATS_GROUP_IF_PROFILING(TEXT("HxCore"), STATGROUP_HxCore, STATCAT_Advanced)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrasp, UPrimitiveComponent*, component);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRelease, UPrimitiveComponent*, component);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdate, UPrimitiveComponent*, component);

//! @brief Different techniques for managing physics network authority. Each technique has
//! strengths and weaknesses suited to different applications.
//!
//! Physics authority determines which physics simulation broadcasts its results to be assumed by
//! all other nodes (all other clients and the server) in an Unreal networking environment. When a
//! player has physics authority, their experiences are indistinguishable from a non-networked
//! environment.
UENUM(BlueprintType)
enum class EPhysicsAuthorityMode : uint8 {
  //! "Dynamic" authority mode attempts to dynamically manage whether clients or the server have
  //! physics authority based on interaction contextUse "Dynamic" authority in projects that have
  //! joint interactions and reasonable latencies.
  //!
  //! @note This mode is experimental. There are some unresolved artifacts that can occur during
  //! physics authority transitions. Proceed with caution.
  DYNAMIC    UMETA(DisplayName = "Dynamic"),
  //! "Client" authority always assigns clients authority over their interactions. Use this mode in
  //! projects that only have solo interactions as it will provide each player the best experience
  //! possible in all latency environments. Using this mode with joint interactions will result in
  //! undefined behavior.
  CLIENT     UMETA(DisplayName = "Client"),
  //! "Server" authority always gives the server authority over interactions. Use this mode in high
  //! latency environments.
  SERVER     UMETA(DisplayName = "Server")
};

//! @brief Represents parameters used in the grasp visualizer.
//!
//! This struct only exists for organizational purposes in the details panel.

// Represents parameters used in the grasp visualizer.
USTRUCT(BlueprintType)
struct FGraspVisualizationParameters {
  GENERATED_BODY()

  //! @brief The ratio of bar height to grasp score [cm].
  //!
  //! Increase to make the grasp visualizer taller.

  // The ratio of bar height to grasp score [cm].
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization",
      meta=(DisplayName="Score to cm", UIMin="0.0", ClampMin="0.0"))
  float score_to_cm = 0.01f;
};

//! @brief The information associated with a body for grasping purposes.
//!
//! This struct only gets used inside AHxCoreActor.

// The information associated with a body for grasping purposes.
USTRUCT()
struct FGraspBodyInfo {
  GENERATED_BODY()

  FGraspBodyInfo() : id(0), component(nullptr), is_anchor(false) {};

  //! Constructor to populate all fields.
  //!
  //! @param id This body's ID.
  //! @param component The component associated with this body.
  //! @param bone_name The bone associated with this body.
  //! @param is_anchor Whether this body is a valid anchor.
  FGraspBodyInfo(int64_t id, UPrimitiveComponent* component, FName bone_name,
      bool is_anchor = false) : id(id), component(component), bone_name(bone_name),
      is_anchor(is_anchor) {}

  //! This body's ID.

  // This body's ID.
  UPROPERTY()
  int64 id;

  //! The component associated with this body.

  // The component associated with this body.
  UPROPERTY()
  UPrimitiveComponent* component;

  //! The bone associated with this body.

  // The bone associated with this body.
  UPROPERTY()
  FName bone_name;

  //! @brief Whether this body is a valid anchor.
  //!
  //! Grasps whose parent bodies are valid anchors will have an extra constraint formed "anchoring"
  //! the grasped object to the parent body.

  // Whether this body is a valid anchor.
  UPROPERTY()
  bool is_anchor;
};

//! @brief The information associated with an object for grasping purposes.
//!
//! This struct only gets used inside AHxCoreActor.

// The information associated with an object for grasping purposes.
USTRUCT()
struct FGraspObjectInfo {
  GENERATED_BODY()

    FGraspObjectInfo() : component(nullptr) {};

  //! Constructor to populate all fields.
  //!
  //! @param component The component associated with this body.
  //! @param bone_name The bone associated with this body.
  FGraspObjectInfo(UPrimitiveComponent* component, FName bone_name) : component(component),
      bone_name(bone_name) {}

  //! The component associated with this object.

  // The component associated with this body.
  UPROPERTY()
  UPrimitiveComponent* component;

  //! The bone associated with this object.

  // The bone associated with this object.
  UPROPERTY()
  FName bone_name;
};

//! @brief The information associated with a single grasp.
//!
//! This struct only gets used inside AHxCoreActor.

// The information associated with a single grasp.
USTRUCT()
struct FGrasp {
  GENERATED_BODY()

  //! All bodies participating in the grasp.
  std::vector<int64_t> body_ids;

  //! @brief A map of the bodies participating in a grasp to their associated stick constraints.
  //!
  //! Stick constraints consist of linear springs in each axis. They help objects stick to bodies
  //! in a way that matches our physical intuition. Every body participating in a grasp is
  //! guaranteed to be represented in this map.

  // A map of the bodies participating in a grasp to their associated stick constraints.
  UPROPERTY()
  TMap<int64, UPhysicsConstraintComponent*> stick_constraints;

  //! @brief A map of the bodies participating in a pinch to their associated pinch constraints.
  //!
  //! Pinch constraints consist of angular springs in each axis. They only form when exactly two
  //! bodies are participating in a grasp, and they help provide a sense of rotational friction in
  //! this case which is otherwise under-constrained.

  // A map of the bodies participating in a pinch to their associated pinch constraints.
  UPROPERTY()
  TMap<int64, UPhysicsConstraintComponent*> pinch_constraints;

  //! @brief The anchor constraint for this grasp. Can be null if the grasp parent isn't an anchor.
  //!
  //! Anchor constraints consist of linear and angular limits. They allow constrained objects to
  //! move relative to bodies dubbed "anchors" within acceptable limits, then provide a hard stop.
  //! This protects against really large forces that may occur when bodies are accelerating rapidly
  //! or encountering static objects.

  // The anchor constraint for this grasp. Can be null if the grasp parent isn't an anchor.
  UPROPERTY()
  UPhysicsConstraintComponent* anchor_constraint;
};

//! A custom FTickFunction so @link AHxCoreActor AHxCoreActors @endlink can tick
//! before any other HaptX class ticks.

// A custom FTickFunction so AHxCoreActors can tick before any other HaptX class ticks.
USTRUCT()
struct FHxCoreGlobalFirstTickFunction : public FTickFunction {
  GENERATED_BODY()

  //! The UHxHandComponent that is ticking.
  class AHxCoreActor* Target;

  //! Abstract function. Actually execute the tick.
  //!
  //! @param DeltaTime Frame time to advance [s].
  //! @param TickType Kind of tick for this frame.
  //! @param CurrentThread Thread we are executing on, useful to pass along as new tasks are
  //! created.
  //! @param CompletionGraphEvent Completion event for this task. Useful for holding the
  //! completion of this task until certain child tasks are complete.
  HAPTX_API virtual void ExecuteTick(
      float DeltaTime,
      ELevelTick TickType,
      ENamedThreads::Type CurrentThread,
      const FGraphEventRef& CompletionGraphEvent) override;

  //! Abstract function to describe this tick. Used to print messages about illegal cycles
  //! in the dependency graph.
  HAPTX_API virtual FString DiagnosticMessage() override;
};
template<>
struct TStructOpsTypeTraits<FHxCoreGlobalFirstTickFunction> : public TStructOpsTypeTraitsBase2<FHxCoreGlobalFirstTickFunction> {
  enum {
    WithCopy = false
  };
};

//! @brief Responsible for global parameters and configuration of the HaptX system.
//!
//! See the @ref section_haptx_core_blueprint "Unreal Plugin Guide" for a high level overview.
//!
//! @ingroup group_unreal_plugin

// Responsible for global parameters and configuration of the HaptX system.
UCLASS(ClassGroup = (HaptX), Blueprintable)
class HAPTX_API AHxCoreActor : public AActor {
  GENERATED_UCLASS_BODY()

public:
  //! Returns the properties used for network replication. This needs to be overridden by all actor
  //! classes with native replicated properties.
  //!
  //! @param [out] OutLifetimeProps The properties used for network replication.
  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty>& OutLifetimeProps) const override;

  //! Called when the game starts.
  virtual void BeginPlay() override;

  //! Called every frame before other HaptX classes' Tick functions.
  //!
  //! @param DeltaTime The time since the last tick.
  //! @param TickType The kind of tick this is, for example, are we paused, or 'simulating' in the
  //! editor.
  //! @param ThisTickFunction Internal tick function struct that caused this to run.
  virtual void TickGlobalFirst(
    float DeltaTime,
    ELevelTick TickType,
    FHxCoreGlobalFirstTickFunction& ThisTickFunction);

  //! Settings for this actor's global first tick function.
  struct FHxCoreGlobalFirstTickFunction GlobalFirstTick;

  //! Called every frame.
  //!
  //! @param delta_seconds The time that has passed since last frame in seconds.
  virtual void Tick(float delta_seconds) override;

  //! Overridable function called whenever this actor is being removed from a level.
  //!
  //! @param end_play_reason Why EndPlay() was called.
  virtual void EndPlay(EEndPlayReason::Type end_play_reason) override;

  //! Called when game ends after everything has their EndPlay()s called.
  virtual void BeginDestroy() override;

  //! Initializes all interfaces with HaptX systems.
  //!
  //! @returns Whether this object is successfully interfaced with HaptX systems.
  bool initializeHaptxSystem();

  //! Get a handle to the underlying HaptxApi::HaptxSystem.
  //!
  //! @returns A handle to the underlying HaptxApi::HaptxSystem.
  HaptxApi::HaptxSystem& getHaptxSystem();

  //! Get a handle to the underlying HaptxApi::ContactInterpreter.
  //!
  //! @returns A handle to the underlying HaptxApi::ContactInterpreter.
  HaptxApi::ContactInterpreter& getContactInterpreter();

  //! Get a handle to the underlying HaptxApi::GraspDetector.
  //!
  //! @returns A handle to the underlying HaptxApi::GraspDetector.
  HaptxApi::GraspDetector& getGraspDetector();

  //! Register the existence of a simulated peripheral.
  //!
  //! @param peripheral The simulated peripheral.
  void registerSimulatedPeripheral(const HaptxApi::Peripheral& peripheral);

  //! True if this object is successfully interfaced with HaptX systems.
  //!
  //! @returns True if this object is successfully interfaced with HaptX systems.

  // True if this object is successfully interfaced with HaptX systems.
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HaptX Core")
  bool isHaptxSystemInitialized() const;

  //! Sets the state of the #enable_tactile_feedback_ flag.
  //!
  //! This permits the HaptxApi::ContactInterpreter to request actions of tactors and
  //! their pressure regulators.
  //!
  //! @param enabled True to enable tactile feedback.

  // Sets the state of the "enable tactile feedback" flag.
  UFUNCTION(BlueprintCallable, Category = "Contact Interpreter")
  void setEnableTactileFeedbackState(bool enabled);

  //! Sets the state of the #enable_force_feedback_ flag.
  //!
  //! This permits the HaptxApi::ContactInterpreter to request actions of retractuators and
  //! their pressure regulators.
  //!
  //! @param enabled True to enable force feedback.

  // Sets the state of the "enable force feedback" flag.
  UFUNCTION(BlueprintCallable, Category = "Contact Interpreter")
  void setEnableForceFeedbackState(bool enabled);

  //! Sets the default #grasp_threshold_ value.
  //!
  //! This controls how easily the grasp assistance algorithm kicks in by default. This value may
  //! be overridden on a per-object basis by assigning a UHxPhysicalMaterial.
  //!
  //! @param threshold Smaller values make grasp assistance trigger more easily.

  // Sets the default "grasp threshold" value.
  UFUNCTION(BlueprintCallable, Category = "Grasping")
  void setDefaultGraspThreshold(float threshold);

  //! @brief A multiplier applied to #grasp_threshold_ that determines how easily objects are
  //! released by default.
  //!
  //! Increase it to make releasing easier. This value may be overridden on a per-object basis by
  //! assigning a UHxPhysicalMaterial.

  // Sets the default "release hysteresis" value.
  UFUNCTION(BlueprintCallable, Category = "Grasping")
  void setDefaultReleaseHysteresis(float release_hysteresis);

  //! Sets the state of the #enable_grasping_ flag.
  //!
  //! This permits the HaptxApi::GraspDetector to assist the user with grasping physically
  //! simulating objects.
  //!
  //! @param enabled True to enable grasping.

  // Sets the state of the "enable grasping" flag.
  UFUNCTION(BlueprintCallable, Category = "Grasping")
  void setEnableGraspingState(bool enabled);

  //! Get the current EPhysicsAuthorityMode.
  //!
  //! @returns The current EPhysicsAuthorityMode.

  // Get the current EPhysicsAuthorityMode.
  UFUNCTION(BlueprintCallable, Category = "Networking")
  EPhysicsAuthorityMode getPhysicsAuthorityMode() const;

  //! Set the current EPhysicsAuthorityMode.
  //!
  //! @param mode The new EPhysicsAuthorityMode.

  // Set the current EPhysicsAuthorityMode.
  UFUNCTION(BlueprintCallable, Category = "Networking")
  void setPhysicsAuthorityMode(EPhysicsAuthorityMode mode);

  //! Toggles the state of the #visualize_grasps_ flag.

  // Toggles the state of the "visualize grasps" flag.
  UFUNCTION(BlueprintCallable, Category = "Visualization")
  void toggleGraspVisualizer();

  //! Toggle the networking state visualizer.

  // Toggle the networking state visualizer.
  UFUNCTION(BlueprintCallable, Category = "Visualization")
  void toggleNetworkStateVisualizer();

  //! Returns the current AHxCoreActor in the level.
  //!
  //! If none can be found, one will spawn and attempt to open all HaptX interfaces. There should
  //! always be an AHxCoreActor in the level when using the HaptX API, but one does not have to be
  //! added to a level manually. After a call to this function it is guaranteed that there exists at
  //! most one AHxCoreActor in the level. This function can return null, and does so if the call to
  //! the underlying HaptxApi::Core singleton fails to open all HaptX interfaces.
  //!
  //! @param world The current world pointer. Typically acquired via AActor::GetWorld().
  //!
  //! @returns The current AHxCoreActor in the level. Returns null if any HaptX interfaces fail to
  //! open.

  // Returns the current AHxCoreActor in the level.
  UFUNCTION(BlueprintCallable, Category = "HaptX Core")
  static AHxCoreActor* getAndMaintainPseudoSingleton(UWorld* world);

  //! Registers an object with the HaptxApi::ContactInterpreter.
  //!
  //! Anything that UHxHandComponents may come into contact with constitutes a
  //! HaptxApi::ContactInterpreter object. Objects will be registered automatically the
  //! first time they are contacted.
  //!
  //! @param comp The component being registered as a HaptxApi::ContactInterpreter object.
  //! @param bone The bone being registered as a HaptxApi::ContactInterpreter object.
  //! @param register_again Registers the object even if it's already registered.
  //! @param [out] object_id The registered ID of the object.
  //!
  //! @returns Whether the object is registered at the end of this call.
  bool tryRegisterObjectWithCi(UPrimitiveComponent* comp, FName bone,
      bool register_again, int64_t& object_id);

  //! Associates a component/bone with a HaptxApi::ContactInterpreter body ID.
  //!
  //! Any independently moving part of an UHxHandComponent may be associated with its own
  //! HaptxApi::ContactInterpreter body ID, or it can share one with other parts of the hand.
  //!
  //! @param ci_body_id The HaptxApi::ContactInterpreter body ID being associated with
  //! the following component/bone.
  //! @param comp The component being associated with the preceding
  //! HaptxAPi::ContactInterpreter body Id.
  //! @param bone The bone being associated with the preceding
  //! HaptxAPi::ContactInterpreter body Id
  //! @param parameters Additional information about this body.
  //! @param rigid_body_part The HaptxApi::RigidBodyPart associated with the component/bone.
  void registerBodyWithCi(int64_t ci_body_id,
      UPrimitiveComponent* comp, FName bone, const FBodyParameters& parameters,
      HaptxApi::RigidBodyPart rigid_body_part);

  //! Registers an object with the HaptxApi::GraspDetector.
  //!
  //! Anything that UHxHandComponents may come into contact with constitutes a
  //! HaptxApi::GraspDetector object. Objects will be registered automatically the first
  //! time they are contacted. Welded objects will be grouped together under the same registered
  //! ID.
  //!
  //! @param comp The component being registered as a HaptxApi::GraspDetector object.
  //! @param bone The bone being registered as a HaptxApi::GraspDetector object.
  //! @param register_again Registers the object even if it's already registered.
  //! @param [out] object_id The registered ID of the object.
  //!
  //! @returns Whether the object is registered at the end of this call.
  bool tryRegisterObjectWithGd(UPrimitiveComponent* comp, FName bone,
      bool register_again, int64_t& object_id);

  //! Associates a component and bone name with a HaptxApi::GraspDetector body ID.
  //!
  //! Any independently moving part of a UHxHandComponent may be associated with its own
  //! HaptxApi::GraspDetector body ID, or it can share one with other parts of the hand.
  //!
  //! @param gd_body_id The HaptxApi::GraspDetector body ID being associated with the
  //! following component and bone.
  //! @param comp The component being associated with the preceding
  //! HaptxApi::GraspDetector body ID.
  //! @param bone The bone being associated with the preceding HaptxApi::GraspDetector
  //! body ID.
  //! @param is_anchor Whether this body is a valid anchor.
  void registerGdBody(int64_t gd_body_id, UPrimitiveComponent* comp, FName bone,
      bool is_anchor = false);

  //! Logs a message to the Unreal log and optionally to the screen.
  //!
  //! @param message The message to log.
  //! @param add_to_screen True to log this message to the screen.
  //! @param message_key The key/ID for this message. Only one message with a given key can display
  //!     at a time and will replace existing messages with the same key. If negative then a unique
  //!     key will be chosen automatically.
  //! @param duration The amount of time [s] to display this message on-screen.
  //! @returns The key of the message if @p add_to_screen is true, a negative number otherwise.
  static int32 log(const TCHAR* message, bool add_to_screen = false, int32 message_key = -1,
      float duration = 10.0f);

  //! Logs a warning message to the Unreal log and optionally to the screen.
  //!
  //! @param message The warning message to log.
  //! @param add_to_screen True to log this warning message to the screen.
  //! @param message_key The key/ID for this message. Only one message with a given key can display
  //!     at a time and will replace existing messages with the same key. If negative then a unique
  //!     key will be chosen automatically.
  //! @returns The key of the message if @p add_to_screen is true, a negative number otherwise.
  static int32 logWarning(const TCHAR* message, bool add_to_screen = false,
      int32 message_key = -1);

  //! Logs an error message to the Unreal log and optionally to the screen.
  //!
  //! @param message The error message to log.
  //! @param add_to_screen True to log this error message to the screen.
  //! @param message_key The key/ID for this message. Only one message with a given key can display
  //!     at a time and will replace existing messages with the same key. If negative then a unique
  //!     key will be chosen automatically.
  //! @returns The key of the message if @p add_to_screen is true, a negative number otherwise.
  static int32 logError(const TCHAR* message, bool add_to_screen = false, int32 message_key = -1);

  //! Logs a message to the Unreal log and optionally to the screen.
  //!
  //! @param message The message to log.
  //! @param add_to_screen True to log this message to the screen.
  //! @param message_key The key/ID for this message. Only one message with a given key can display
  //!     at a time and will replace existing messages with the same key. If negative then a unique
  //!     key will be chosen automatically.
  //! @param duration The amount of time [s] to display this message on-screen.
  //! @returns The key of the message if @p add_to_screen is true, a negative number otherwise.

  // Logs a message to the Unreal log and optionally to the screen.
  UFUNCTION(BlueprintCallable, Category = "Logging")
  static int32 log(const FString& message, bool add_to_screen = false, int32 message_key = -1,
      float duration = 10.0f);

  //! Logs a warning message to the Unreal log and optionally to the screen.
  //!
  //! @param message The warning message to log.
  //! @param add_to_screen True to log this warning message to the screen.
  //! @param message_key The key/ID for this message. Only one message with a given key can display
  //!     at a time and will replace existing messages with the same key. If negative then a unique
  //!     key will be chosen automatically.
  //! @returns The key of the message if @p add_to_screen is true, a negative number otherwise.

  // Logs a warning message to the Unreal log and optionally to the screen.
  UFUNCTION(BlueprintCallable, Category = "Logging")
  static int32 logWarning(const FString& message, bool add_to_screen = false,
      int32 message_key = -1);

  //! Logs an error message to the Unreal log and optionally to the screen.
  //!
  //! @param message The error message to log.
  //! @param add_to_screen True to log this error message to the screen.
  //! @param message_key The key/ID for this message. Only one message with a given key can display
  //!     at a time and will replace existing messages with the same key. If negative then a unique
  //!     key will be chosen automatically.
  //! @returns The key of the message if @p add_to_screen is true, a negative number otherwise.

  // Logs an error message to the Unreal log and optionally to the screen.
  UFUNCTION(BlueprintCallable, Category = "Logging")
  static int32 logError(const FString& message, bool add_to_screen = false,
      int32 message_key = -1);

  //! @brief Logs a message asking the user to restart their game to the output log and the screen.
  //!
  //! Only HaptX classes should call this function.
  static void logRestartMessage();

  //! @brief Gets fired when a grasp is created.
  //!
  //! Bind this event to perform actions when a grasp is created.
  //! See @ref section_haptx_core_blueprint_grasping for an example of binding.

  // Gets fired when a grasp is created.
  UPROPERTY(BlueprintAssignable, Category = "Events")
  FOnGrasp on_grasp_;

  //! @brief Gets fired when a grasp is destroyed.
  //!
  //! Bind this event to perform actions when a grasp is destroyed.
  //! See @ref section_haptx_core_blueprint_grasping for an example of binding.

  // Gets fired when a grasp is destroyed.
  UPROPERTY(BlueprintAssignable, Category = "Events")
  FOnRelease on_release_;

  //! @brief Gets fired when a grasp is updated.
  //!
  //! Bind this event to perform actions when a grasp is updated. Grasps will be updated when the
  //! set of participating bodies changes without the grasp score dropping below the
  //! grasp threshold.
  //! See @ref section_haptx_core_blueprint_grasping for an example of binding.

  // Gets fired when a grasp is updated.
  UPROPERTY(BlueprintAssignable, Category = "Events")
  FOnUpdate on_update_;

  //! This inline flag toggles grasp visualization.

  // This inline flag toggles grasp visualization.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grasping", meta =
      (InlineEditConditionToggle))
  bool visualize_grasps_;

  //! @brief A visualizer that displays detected and assisted grasps.
  //!
  //! Elements include:
  //! - Black squares: The base of each grasp visualizer
  //! - Gray bars: Thresholds under which grasps are not being assisted
  //! - Blue bars: Scores for each grasp
  //! - Teal: Indicates grasp assistance

  // A visualizer that displays detected and assisted grasps.
  UPROPERTY(EditAnywhere, Category = "Grasping", meta =
    (editcondition = "visualize_grasps_", DisplayName = "Visualize Grasps"))
  FGraspVisualizationParameters grasp_visualization_parameters_;

  //! @brief Whether to allow the #grasp_collision_types_ whitelist to be used.
  //!
  //! If disabled, all object types are accepted (so #grasp_collision_types_ won't be used as a
  //! whitelist).

  // Whether to allow the grasp_collision_types_ whitelist to be used.
  UPROPERTY(EditAnywhere, Category = "Grasping", meta = (InlineEditConditionToggle))
  bool allow_grasp_collision_type_whitelist_ = true;

  //! @brief The list of object collision types that can be grasped.
  //!
  //! Only gets used if #allow_grasp_collision_type_whitelist_ is true.

  // The list of object collision types that can be grasped.
  UPROPERTY(EditAnywhere, Category = "Grasping",
      meta = (editcondition = "allow_grasp_collision_type_whitelist_"))
  TArray<TEnumAsByte<ECollisionChannel>> grasp_collision_types_;

  //! @brief Whether to allow the #tactile_feedback_collision_types_ whitelist to be used.
  //!
  //! If disabled, all object types are accepted (so #tactile_feedback_collision_types_ won't be
  //! used as a whitelist).

  // Whether to allow the tactile_feedback_collision_types_ whitelist to be used.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta = (InlineEditConditionToggle))
  bool allow_tactile_feedback_collision_type_whitelist_ = true;

  //! @brief The list of object collision types that can generate tactile feedback.
  //!
  //! Only gets used if #allow_tactile_feedback_collision_type_whitelist_ is true.

  // The list of object collision types that can generate tactile feedback.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter",
      meta = (editcondition = "allow_tactile_feedback_collision_type_whitelist_"))
  TArray<TEnumAsByte<ECollisionChannel>> tactile_feedback_collision_types_;

  //! @brief Whether to allow the #force_feedback_collision_types_ whitelist to be used.
  //!
  //! If disabled, all object types are accepted (so #force_feedback_collision_types_ won't be used
  //! as a whitelist).

  // Whether to allow the force_feedback_collision_types_ whitelist to be used.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta = (InlineEditConditionToggle))
  bool allow_force_feedback_collision_type_whitelist_ = true;

  //! @brief The list of object collision types that can generate force feedback.
  //!
  //! Only gets used if #force_feedback_collision_types_ is true.

  // The list of object collision types that can generate force feedback.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter",
      meta = (editcondition = "allow_force_feedback_collision_type_whitelist_"))
  TArray<TEnumAsByte<ECollisionChannel>> force_feedback_collision_types_;

  //! Whether to visualize networking state.

  // Whether to visualize networking state.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
  bool visualize_network_state_;

protected:

  //! The name of the input action that controls toggling the grasp visualizer.

  // The name of the input action that controls toggling the grasp visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualization")
  FName toggle_grasp_vis_action_;

  //! The name of the input action that controls toggling the networking state visualizer.

  // The name of the input action that controls toggling the networking state visualizer.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Visualization")
  FName toggle_network_state_vis_action_;

  //! @brief True to enable tactile feedback. If disabled, per-object properties set to enable
  //! tactile feedback will have no effect.
  //!
  //! Permits the HaptxApi::ContactInterpreter to request actions of tactors and their
  //! pressure regulators. Add a UHxPhysicalMaterial to your component to configure this value on a
  //! per-object basis.

  // True to enable tactile feedback. If disabled, per-object properties set to enable tactile
  // feedback will have no effect.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contact Interpreter")
  bool enable_tactile_feedback_;

  //! @brief True to enable force feedback. If disabled, per-object properties set to enable
  //! force feedback will have no effect.
  //!
  //! Permits the HaptxApi::ContactInterpreter to request actions of retractuators and their
  //! pressure regulators. Add a UHxPhysicalMaterial to your component to configure this value on a
  //! per-object basis.

  // True to enable force feedback. If disabled, per-object properties set to enable force feedback
  // will have no effect.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contact Interpreter")
  bool enable_force_feedback_;

  //! @brief The ratio that we decrease the current compression filter scale by when a haptic
  //! signal containing effect output doesn't fit entirely within the max inflation for a tactor.
  //!
  //! Should always be between 0 and 1 (inclusive). A value of 1 disables attacking; small values
  //! mean an aggressive attack; 0 means a perfect/infinite attack rate, and you'll never see
  //! signal output above max inflation.

  // The ratio that we decrease the current compression filter scale by when a haptic signal
  // containing effect output doesn't fit entirely within the max inflation for a tactor. Should
  // always be between 0 and 1 (inclusive). A value of 1 disables attacking; small values mean an
  // aggressive attack; 0 means a perfect/infinite attack rate, and you'll never see signal output
  // above max inflation.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(ClampMin=0.0,ClampMax=1.0))
  float tactor_compression_filter_attack_ratio_{
      HaptxApi::ContactInterpreter::DEFAULT_COMPRESSION_FILTER_ATTACK_RATIO};

  //! @brief The ratio that we increase the current compression filter scale by when a haptic
  //! signal comfortably fits entirely within the max inflation for a tactor.
  //!
  //! Should always be at least 1 (inclusive). A value of 1 disables releasing; large values mean
  //! an aggressive release.

  // The ratio that we increase the current compression filter scale by when a haptic signal
  // comfortably fits entirely within the max inflation for a tactor. Should always be at least 1
  // (inclusive). A value of 1 disables releasing; large values mean an aggressive release.
  UPROPERTY(EditAnywhere, Category = "Contact Interpreter", meta=(ClampMin=1.0))
  float tactor_compression_filter_release_ratio_{
      HaptxApi::ContactInterpreter::DEFAULT_COMPRESSION_FILTER_RELEASE_RATIO};

  //! @brief True to enable grasping. If disabled, per-object properties set to enable grasping
  //! will have no effect.
  //!
  //! Determines whether the HaptxApi::GraspDetector assists the user with grasping
  //! physically simulating objects. Add a UHxPhysicalMaterial to your component to configure this
  //! value on a per-object basis.

  // True to enable grasping. If disabled, per-object properties set to enable grasping will have
  // no effect.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grasping")
  bool enable_grasping_;

  //! @brief A threshold that determines how difficult objects are to grasp by default.
  //!
  //! Increase it to make grasping happen less often. We don't recommend values above 200. Add a
  //! UHxPhysicalMaterial to your component to configure this value on a per-object basis.

  // A threshold that determines how difficult objects are to grasp by default.
  UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"), Category = "Grasping")
  float grasp_threshold_;

  //! @brief A multiplier applied to #grasp_threshold_ that determines how easily objects are
  //! released by default.
  //!
  //! Increase it to make releasing easier. Add a UHxPhysicalMaterial to your component to
  //! configure this value on a per-object basis.

  // A multiplier applied to grasp threshold that determines how easily objects are released by
  // default.
  UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0",
      UIMax = "1.0"), Category = "Grasping")
  float release_hysteresis_;

  //! The linear drive parameters that assist when grasping an object.

  // The linear drive parameters that assist when grasping an object.
  UPROPERTY(EditAnywhere, Category = "Grasping")
  FConstraintDrive grasp_linear_drive_;

  //! @brief The angular drive parameters that assist when pinching an object.
  //!
  //! These parameters will only get used if a "pinching" grasp is detected.

  // The angular drive parameters that assist when pinching an object.
  UPROPERTY(EditAnywhere, Category = "Grasping")
  FConstraintDrive pinch_angular_drive_;

  //! The linear limit settings that get applied to grasps. This happens in the anchor's
  //! reference frame, which, for now means the palms' frames.

  // The linear limit settings that get applied to grasps. This happens in the anchor's reference
  // frame, which, for now means the palms' frames.
  UPROPERTY(EditAnywhere, Category = "Grasping")
  FLinearConstraint grasp_linear_limit_;

  //! The swing1 and swing2 limit settings that get applied to grasps. This happens in the
  //! anchor's reference frame, which, for now means the palms' frames.

  // The swing1 and swing2 limit settings that get applied to grasps. This happens in the anchor's
  // reference frame, which, for now means the palms' frames.
  UPROPERTY(EditAnywhere, Category = "Grasping")
  FConeConstraint grasp_cone_limit_;

  //! The twist limit settings that get applied to grasps. This happens in the anchor's
  //! reference frame, which, for now means the palms' frames.

  // The twist limit settings that get applied to grasps. This happens in the anchor's reference
  // frame, which, for now means the palms' frames.
  UPROPERTY(EditAnywhere, Category = "Grasping")
  FTwistConstraint grasp_twist_limit_;

  //! @brief Which method gets used to evaluate physics authority in a networked environment.
  //!
  //! See EPhysicsAuthorityMode for details.
  //!
  //! @note Dynamic authority is experimental. There are some unresolved artifacts that can occur
  //! during physics authority transitions. Proceed with caution.

  // Which method gets used to evaluate physics authority in a networked environment. Dynamic
  // authority is experimental. There are some unresolved artifacts that can occur during physics
  // authority transitions. Proceed with caution.
  UPROPERTY(EditAnywhere, Category = "Networking", Replicated)
  EPhysicsAuthorityMode physics_authority_mode_;

  //! @brief True to enable on-screen logging.
  //!
  //! Determines whether messages get rendered on-screen.

  // True to enable on-screen logging.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logging")
  bool display_on_screen_messages_;

  //! The minimum level of severity an on-screen message must have to be displayed.

  // The minimum level of severity an on-screen message must have to be displayed.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logging")
  EOnScreenMessageSeverity min_severity_;

  //! The size of the text.

  // The size of the text.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logging",
      meta = (ClampMin = "0.0", UIMin = "0.0"))
  float text_size_;

  //! The max number of characters in a line.

  // The max number of characters in a line.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logging",
      meta = (ClampMin = "1", UIMin = "1"))
  uint8 max_line_length_;

  //! The fraction of horizontal screen space reserved for the left margin.

  // The fraction of horizontal screen space reserved for the left margin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logging",
      meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
  float left_margin_;

  //! The fraction of vertical screen space reserved for the top margin.

  // The fraction of vertical screen space reserved for the top margin.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logging",
      meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
  float top_margin_;

private:
  //! Execute any actions recommended by the HaptxApi::GraspDetector.
  //!
  //! @param delta_seconds The amount of time [s] since the last call to updateGrasps().
  void updateGrasps(float delta_seconds);

  //! @brief Performs any steps necessary to cease a given grasp.
  //!
  //! Restores the physical material on any body no longer participating in a grasp, and
  //! destroys any constraints that were created in the formation of the given grasp.
  //!
  //! @param grasp The grasp to destroy.
  void destroyGrasp(FGrasp &grasp);

  //! @brief Performs any steps necessary to form a grasp.
  //!
  //! Updates physical materials on new bodies participating in a grasp. Creates rotational
  //! constraints in cases where exactly two bodies are pinching the object.
  //!
  //! @param grasp A place to store information about this grasp.
  //! @param result The grasp to create.
  void createGrasp(FGrasp &grasp, const HaptxApi::GraspDetector::GraspResult &result);

  //! Performs any steps necessary to update an existing grasp.
  //!
  //! @param grasp Where to update grasp information.
  //! @param result Information about the new grasp.
  void updateGrasp(FGrasp &grasp, const HaptxApi::GraspDetector::GraspResult &result);

  //! Creates a stick constraint between an object and a body.
  //!
  //! @param grasp The struct to store the constraint in.
  //! @param body The body being constrained.
  //! @param object The object being constrained.
  void createStickConstraint(FGrasp& grasp, FGraspBodyInfo& body, FGraspObjectInfo& object);

  //! Creates a pinch constraint between an object and a body.
  //!
  //! @param grasp The struct to store the constraint in.
  //! @param body The body being constrained.
  //! @param object The object being constrained.
  //! @param constraint_location The location to position the constraint before forming.
  void createPinchConstraint(FGrasp& grasp, FGraspBodyInfo& body, FGraspObjectInfo& object,
      FVector constraint_location);

  //! Creates an anchor constraint between an object and an anchor body.
  //!
  //! @param grasp The struct to store the constraint in.
  //! @param anchor The anchor body being constrained.
  //! @param object The object being constrained.
  //! @param constraint_location The location to position the constraint before forming.
  void createAnchorConstraint(FGrasp& grasp, FGraspBodyInfo& anchor, FGraspObjectInfo& object,
      FVector constraint_location);

  //! Destroys a grasp constraint.
  //!
  //! @param body_id The body associated with the constraint being destroyed.
  //! @param map The map containing the constraint being destroyed.
  void destroyGraspConstraint(int64_t body_id, TMap<int64, UPhysicsConstraintComponent*>& map);

  //! Destroys a grasp constraint.
  //!
  //! @param constraint_ptr_ptr The constraint to destroy.
  void destroyGraspConstraint(UPhysicsConstraintComponent** constraint_ptr_ptr);

  //! Notifies participating AHxHandActors that a constraint has been formed.
  //!
  //! @param constraint The constraint that has been formed.
  void notifyHandsOfConstraintCreation(UPhysicsConstraintComponent* constraint);

  //! Notifies participating AHxHandActors that a constraint has been destroyed.
  //!
  //! @param constraint The constraint that has been destroyed.
  void notifyHandsOfConstraintDestruction(UPhysicsConstraintComponent* constraint);

  //! @brief Visualize calculations happening within the HaptxApi::GraspDetector.
  //!
  //! Lasts for one frame.
  void visualizeGrasps();

  //! Server implementation of AHxCoreActor::setPhysicsAuthorityMode().
  //!
  //! @param mode The new EPhysicsAuthorityMode.
  UFUNCTION(Server, Reliable, WithValidation)
  void serverSetPhysicsAuthorityMode(EPhysicsAuthorityMode mode);

  //! Prints all HaptX Log messages currently stored in the HaptxApi::Core to the Unreal
  //! log.
  void printLogMessages();

  //! Records the actual physics delta time.
  void customPhysics(float delta_time_s, FBodyInstance* body_instance);

  //! Exists solely for access to FBodyInstance::AddCustomPhysics().
  UPROPERTY()
  USphereComponent* custom_physics_target_{nullptr};

  //! Custom physics delegate.
  FCalculateCustomPhysics on_calculate_custom_physics_;

  //! The amount of time that has passed in the physics simulation since the last Tick().
  float physics_delta_time_s_{0.0f};

  //! A map of grasp-capable body Ids to information associated with them for grasping.
  TMap<int64, FGraspBodyInfo> gd_body_id_to_component_and_bone_;

  //! A map of object Ids to information associated with
  //! them for the purpose of grasping.
  TMap<int64, FGraspObjectInfo> gd_object_id_to_component_and_bone_;

  //! A map of HaptxApi::ContactInterpreter object IDs to the callbacks being used
  //! to get simulation information about said objects.
  TMap<int64, std::shared_ptr<const HaptxApi::SimulationCallbacks>>
      ci_object_id_to_callbacks_;

  //! A map of HaptxApi::ContactInterpreter body IDs to the callbacks being used to
  //! get simulation information about said bodies.
  TMap<int64, std::shared_ptr<const HaptxApi::SimulationCallbacks>>
      ci_body_id_to_callbacks_;

  //! A map of grasp Ids to the corresponding grasp structs.
  TMap<int64, FGrasp> grasp_id_to_grasp_;

  //! A centralized interface with HaptX systems.
  HaptxApi::HaptxSystem haptx_system_;

  //! Whether a call to initializeHaptxSystem has ever been made.
  bool initialize_haptx_system_attempted_;

  //! The return value from the first meaningful call to initializeHaptxSystem().
  bool initialize_haptx_system_result_;

  //! Used for converting physics data from the game engine to haptic feedback set points.
  HaptxApi::ContactInterpreter contact_interpreter_;

  //! Used for detecting grasps based on physics data from the game engine.
  HaptxApi::GraspDetector grasp_detector_;

  //! Maps from Dk2AirController ID to the HsvController that was configured to mirror it.
  std::unordered_map<std::string, std::shared_ptr<HaptxApi::HsvController>>
      hsv_controller_from_air_controller_id_;

  //! The HsvController being used to render simulated peripherals.
  std::shared_ptr<HaptxApi::HsvController> simulated_hardware_hsv_{nullptr};

  //! Populated with Log messages from HaptxApi.
  std::deque<HaptxApi::LogMessage> haptx_log_messages_;

  //! Whether or not we've displayed a restart message yet (we only print one per session).
  static bool has_printed_restart_message_;

  //! The instance that was responsible for opening all the interfaces, and thus should be
  //! responsible for updating and closing all of them.
  static AHxCoreActor* designated_core_actor_;
};
