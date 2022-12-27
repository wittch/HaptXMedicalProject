// Copyright (C) 2017-2020 by HaptX Incorporated - All Rights Reserved.
// Unauthorized copying of this file via any medium is strictly prohibited.
// The contents of this file are proprietary and confidential.

#include <Haptx/Public/hx_core_actor.h>
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include <Runtime/Engine/Classes/Kismet/KismetStringLibrary.h>
#include <Runtime/Engine/Public/CanvasItem.h>
#include <Runtime/Engine/Public/Net/UnrealNetwork.h>
#include <HaptxApi/direct_pneumatic_calculator.h>
#include <Haptx/Private/haptx_shared.h>
#include <Haptx/Public/hx_debug_draw_system.h>
#include <Haptx/Public/hx_hand_actor.h>
#include <Haptx/Public/hx_simulation_callbacks.h>

DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxCore::Tick"),
    STAT_Tick, STATGROUP_HxCore)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxCore::core_::update"),
    STAT_core_update, STATGROUP_HxCore)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxCore::updateGrasps"),
    STAT_updateGrasps, STATGROUP_HxCore)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxCore::visualizeGrasps"),
    STAT_visualizeGrasps, STATGROUP_HxCore)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxCore::registerObjectIfNotAlready - CI"),
    STAT_registerObjectIfNotAlready_CI, STATGROUP_HxCore)
DECLARE_CYCLE_STAT_IF_PROFILING(TEXT("HxCore::registerObjectIfNotAlready - GD"),
    STAT_registerObjectIfNotAlready_GD, STATGROUP_HxCore)

void FHxCoreGlobalFirstTickFunction::ExecuteTick(
    float DeltaTime,
    ELevelTick TickType,
    ENamedThreads::Type CurrentThread,
    const FGraphEventRef& CompletionGraphEvent) {
  if (Target && !Target->IsPendingKill() && !Target->IsUnreachable()) {
    FScopeCycleCounterUObject ActorScope(Target);
    Target->TickGlobalFirst(DeltaTime, TickType, *this);
  }
}

FString FHxCoreGlobalFirstTickFunction::DiagnosticMessage() {
  return Target->GetFullName() + TEXT("AHxCoreActor Global First Tick.");
}

// Initialize static variables.
AHxCoreActor* AHxCoreActor::designated_core_actor_ = nullptr;
bool AHxCoreActor::has_printed_restart_message_ = false;

AHxCoreActor::AHxCoreActor(const FObjectInitializer& object_initializer) :
    Super(object_initializer), visualize_network_state_(false),
    toggle_grasp_vis_action_(TEXT("HxToggleGraspVis")),
    toggle_network_state_vis_action_(TEXT("HxToggleNetworkStateVis")),
    enable_tactile_feedback_(true), enable_force_feedback_(true), enable_grasping_(true),
    grasp_threshold_(18.0f), release_hysteresis_(0.75f),
    physics_authority_mode_(EPhysicsAuthorityMode::SERVER), display_on_screen_messages_(true),
    min_severity_(EOnScreenMessageSeverity::INFO), text_size_(4.0f), max_line_length_(80u),
    left_margin_(0.33f), top_margin_(0.33f), haptx_system_(),
    initialize_haptx_system_attempted_(false), initialize_haptx_system_result_(false),
    contact_interpreter_(), hsv_controller_from_air_controller_id_(), haptx_log_messages_() {
  PrimaryActorTick.bCanEverTick = true;
  // Tick after all other tick logic in the game has completed
  SetTickGroup(ETickingGroup::TG_PostPhysics);

  GlobalFirstTick.bCanEverTick = true;
  GlobalFirstTick.bStartWithTickEnabled = false;  // Ticking gets enabled in BeginPlay().
  GlobalFirstTick.TickGroup = TG_PrePhysics;

  bReplicates = true;

  grasp_linear_drive_.bEnablePositionDrive = true;
  grasp_linear_drive_.bEnableVelocityDrive = true;
  grasp_linear_drive_.Stiffness = 1.0e5f;
  grasp_linear_drive_.Damping = 1.0e3f;
  grasp_linear_drive_.MaxForce = 3.0e3f;
  pinch_angular_drive_.bEnablePositionDrive = true;
  pinch_angular_drive_.bEnableVelocityDrive = true;
  pinch_angular_drive_.Stiffness = 1.0e6f;
  pinch_angular_drive_.Damping = 1.0e3f;
  pinch_angular_drive_.MaxForce = 3.0e4f;
  AutoReceiveInput = EAutoReceiveInput::Player0;
  tactile_feedback_collision_types_ = { ECC_PhysicsBody, ECC_WorldDynamic, ECC_WorldStatic,
      ECC_Destructible, ECC_Vehicle, ECC_Pawn };
  force_feedback_collision_types_ = tactile_feedback_collision_types_;
  grasp_collision_types_ = tactile_feedback_collision_types_;

  grasp_linear_limit_.XMotion = ELinearConstraintMotion::LCM_Free;
  grasp_linear_limit_.YMotion = ELinearConstraintMotion::LCM_Free;
  grasp_linear_limit_.ZMotion = ELinearConstraintMotion::LCM_Free;
  grasp_linear_limit_.Limit = 1.0f;

  grasp_cone_limit_.Swing1Motion = EAngularConstraintMotion::ACM_Free;
  grasp_cone_limit_.Swing2Motion = EAngularConstraintMotion::ACM_Free;
  grasp_twist_limit_.TwistMotion = EAngularConstraintMotion::ACM_Free;

  custom_physics_target_= CreateDefaultSubobject<USphereComponent>(TEXT("CustomPhysicsTarget"));
  custom_physics_target_->SetupAttachment(RootComponent);
  custom_physics_target_->SetSphereRadius(1.0f, false);
  custom_physics_target_->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
  custom_physics_target_->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
  custom_physics_target_->SetEnableGravity(false);
  custom_physics_target_->SetSimulatePhysics(true);
  on_calculate_custom_physics_.BindUObject(this, &AHxCoreActor::customPhysics);
}

void AHxCoreActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(AHxCoreActor, physics_authority_mode_);
}

void AHxCoreActor::BeginPlay() {
  Super::BeginPlay();
  // If a designated core already exists, and it's not us, destroy ourselves.
  if (IsValid(designated_core_actor_) && designated_core_actor_ != this) {
    // Yield to the chosen one, destroy myself
    UE_LOG(HaptX, Warning,
        TEXT("More than one AHxCoreActor detected in the level at BeginPlay(). Deleting myself!\nMake sure to only have one if you want to configure the HaptX system, otherwise the one you configure might be deleted due to the arbitrary order of BeginPlay() calls."));
    Destroy();
    return;
  }

  HxDebugDrawSystem::open();
  AHxOnScreenLog* on_screen_log = AHxOnScreenLog::getInstance(GetWorld());
  if (IsValid(on_screen_log)) {
    on_screen_log->display_on_screen_messages_ = display_on_screen_messages_;
    on_screen_log->min_severity_ = min_severity_;
    on_screen_log->text_size_ = text_size_;
    on_screen_log->max_line_length_ = max_line_length_;
    on_screen_log->left_margin_ = left_margin_;
    on_screen_log->top_margin_ = top_margin_;
  }

  if (!IsTemplate() && GlobalFirstTick.bCanEverTick) {
    GlobalFirstTick.Target = this;
    GlobalFirstTick.SetTickFunctionEnable(true);
    GlobalFirstTick.RegisterTickFunction(GetLevel());
  }

  // Can be nullptr when simulating vs play.
  if (InputComponent != nullptr) {
    InputComponent->BindAction(toggle_grasp_vis_action_, IE_Pressed, this,
        &AHxCoreActor::toggleGraspVisualizer);
    InputComponent->BindAction(toggle_network_state_vis_action_, IE_Pressed, this,
        &AHxCoreActor::toggleNetworkStateVisualizer).bConsumeInput = false;
  }
}

void AHxCoreActor::TickGlobalFirst(
    float DeltaTime,
    ELevelTick TickType,
    FHxCoreGlobalFirstTickFunction& ThisTickFunction) {
  HxDebugDrawSystem::reset();

  if (IsValid(custom_physics_target_)) {
    custom_physics_target_->BodyInstance.AddCustomPhysics(on_calculate_custom_physics_);
  }
}

void AHxCoreActor::Tick(float delta_seconds) {
  Super::Tick(delta_seconds);
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_Tick)

  // If I'm not the one that should be handling this logic
  if (designated_core_actor_ != this) {
    return;
  }

  // Print out any System Log messages we know about
  printLogMessages();

  // Skip tick if nothing opened
  if (!isHaptxSystemInitialized()) {
    return;
  }
  {
    SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_core_update)
    HaptxApi::AirController::maintainComms();
    std::unordered_map<HaptxApi::HaptxUuid, HaptxApi::HapticFrame> haptic_frames;
    contact_interpreter_.commit(physics_delta_time_s_, &haptic_frames);
    for (auto& air_controller : haptx_system_.getDk2AirControllers()) {
      if (air_controller == nullptr) {
        continue;
      }
      HaptxApi::PneumaticFrame pneumatic_frame;
      std::map<int, std::shared_ptr<HaptxApi::Peripheral>> peripheral_from_slot;
      HaptxApi::AirController::ReturnCode ret =
          air_controller->getAttachedPeripherals(&peripheral_from_slot);
      if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
        UE_LOG(HaptX, Error,
            TEXT("Dk2AirController::getAttachedPeripherals() failed with return code %d: %s."),
            (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
      } else {
        for(const auto& it : peripheral_from_slot) {
          const std::shared_ptr<const HaptxApi::Peripheral> peripheral = it.second;
          if (peripheral == nullptr) {
            continue;
          }

          const auto& haptic_frame_it = haptic_frames.find(peripheral->id);
          if (haptic_frame_it == haptic_frames.end()) {
            continue;
          }

          if (!HaptxApi::DirectPneumaticCalculator::addToPneumaticFrame(*peripheral,
              haptic_frame_it->second, *air_controller, &pneumatic_frame)) {
            UE_LOG(HaptX, Error, TEXT(
                "HaptxApi::DirectPneumaticCalculator::addToPneumaticFrame() failed for Peripheral %s."),
                *STRING_TO_FSTRING(peripheral->casual_name))
          }
        }
        ret = air_controller->render(pneumatic_frame);
        if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
          UE_LOG(HaptX, Error,
              TEXT("HaptxApi::Dk2AirController::render() failed with error code %d: %s."),
              (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
        }

        std::string air_controller_id;
        if (air_controller->getId(&air_controller_id) ==
            HaptxApi::AirController::ReturnCode::SUCCESS) {
          auto const& hsv_it = hsv_controller_from_air_controller_id_.find(air_controller_id);
          if (hsv_it != hsv_controller_from_air_controller_id_.end() &&
              hsv_it->second != nullptr) {
            ret = hsv_it->second->render(pneumatic_frame);
            if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
              UE_LOG(HaptX, Error,
                  TEXT("HaptxApi::HsvAirController::render() failed with error code %d: %s."),
                  (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
            }
          }
        }
      }
    }
    if (simulated_hardware_hsv_ != nullptr) {
      HaptxApi::PneumaticFrame pneumatic_frame;
      std::map<int, std::shared_ptr<HaptxApi::Peripheral>> peripheral_from_slot;
      HaptxApi::AirController::ReturnCode ret =
          simulated_hardware_hsv_->getAttachedPeripherals(&peripheral_from_slot);
      if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
        UE_LOG(HaptX, Error,
            TEXT("HsvController::getAttachedPeripherals() failed with return code %d: %s."),
            (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
      } else {
        for(const auto& it : peripheral_from_slot) {
          const std::shared_ptr<const HaptxApi::Peripheral> peripheral = it.second;
          if (peripheral == nullptr) {
            continue;
          }

          const auto& haptic_frame_it = haptic_frames.find(peripheral->id);
          if (haptic_frame_it == haptic_frames.end()) {
            continue;
          }

          if (!HaptxApi::DirectPneumaticCalculator::addToPneumaticFrame(*peripheral,
              haptic_frame_it->second, *simulated_hardware_hsv_, &pneumatic_frame)) {
            UE_LOG(HaptX, Error, TEXT(
                "HaptxApi::DirectPneumaticCalculator::addToPneumaticFrame() failed for Peripheral %s."),
                *STRING_TO_FSTRING(peripheral->casual_name))
          }
        }
        ret = simulated_hardware_hsv_->render(pneumatic_frame);
        if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
          UE_LOG(HaptX, Error,
              TEXT("HaptxApi::HsvController::render() failed with error code %d: %s."),
              (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
        }
      }
    }
  }
  updateGrasps(physics_delta_time_s_);
  physics_delta_time_s_ = 0.0f;

  if (visualize_grasps_) {
    visualizeGrasps();
  }
}

void AHxCoreActor::EndPlay(EEndPlayReason::Type end_play_reason) {
  if (designated_core_actor_ == this || !IsValid(designated_core_actor_)) {
    HxDebugDrawSystem::close();
    AHxOnScreenLog::close();
    has_printed_restart_message_ = false;
  }
  Super::EndPlay(end_play_reason);
}

void AHxCoreActor::BeginDestroy() {
  if (designated_core_actor_ == this) {
    designated_core_actor_ = nullptr;

    // Print any final log messages.
    printLogMessages();
    HaptxApi::SystemLogger::unregisterOutput(FSTRING_TO_CSTR(GetName()));
  }
  Super::BeginDestroy();
}

bool AHxCoreActor::initializeHaptxSystem() {
  // If we've already tried once, quit early.
  if (initialize_haptx_system_attempted_) {
    return isHaptxSystemInitialized();
  }
  initialize_haptx_system_attempted_ = true;

  // Another core has claimed this role.
  if (designated_core_actor_ != nullptr) {
    return false;
  }
  designated_core_actor_ = this;

  // Initialize static interfaces.
  bool something_went_wrong = false;
  if (!HaptxApi::SystemLogger::registerOutput(FSTRING_TO_CSTR(GetName()),
      haptx_log_messages_)) {
    UE_LOG(HaptX, Error, TEXT("HaptxApi::SystemLogger::registerOutput() failed."))
    something_went_wrong = true;
  }

  // Inflates HaptxSystem with the full picture of connected hardware.
  haptx_system_.discoverDevices();

  // Setup HsvControllers to mirror Dk2AirControllers.
  auto air_controller_it = haptx_system_.getDk2AirControllers().begin();
  for (auto& hsv_controller : haptx_system_.getHsvControllers()) {
    if (hsv_controller == nullptr) {
      continue;
    }
    while (air_controller_it != haptx_system_.getDk2AirControllers().end() &&
        *air_controller_it == nullptr) {
      air_controller_it++;
    }
    if (air_controller_it == haptx_system_.getDk2AirControllers().end()) {
      // We've run out of Dk2AirControllers to mirror. We can use this last hsv_controller to
      // render simulated hardware.
      simulated_hardware_hsv_ = hsv_controller;
      break;
    }
    auto& air_controller = *air_controller_it;

    std::map<int, std::shared_ptr<HaptxApi::Peripheral>> peripheral_from_slot;
    HaptxApi::AirController::ReturnCode ret =
        air_controller->getAttachedPeripherals(&peripheral_from_slot);
    if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
      UE_LOG(HaptX, Error, TEXT(
          "HaptxApi::Dk2AirController::getAttachedPeripherals() failed with error code %d: %s."),
          (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
      something_went_wrong = true;
    } else {
      for (auto it : peripheral_from_slot) {
        if (it.second == nullptr) {
          continue;
        }
        ret = hsv_controller->attachPeripheral(it.first, *it.second);
        if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
          UE_LOG(HaptX, Error,
              TEXT("HaptxApi::HsvController::attachPeripheral() failed with error code %d: %s."),
              (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
          something_went_wrong = true;
        }
      }
      std::string air_controller_id;
      ret = air_controller->getId(&air_controller_id);
      if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
        UE_LOG(HaptX, Error,
            TEXT("HaptxApi::Dk2AirController::getId() failed with error code %d: %s."),
            (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
        something_went_wrong = true;
      } else {
        hsv_controller_from_air_controller_id_.insert({air_controller_id, hsv_controller});
      }
      air_controller_it++;
    }
  }

  // Sync serialized settings with underlying objects.
  contact_interpreter_.setEnableTactileFeedbackState(enable_tactile_feedback_);
  contact_interpreter_.setEnableForceFeedbackState(enable_force_feedback_);
  contact_interpreter_.setCompressionFilterAttackRatio(tactor_compression_filter_attack_ratio_);
  contact_interpreter_.setCompressionFilterReleaseRatio(
      tactor_compression_filter_release_ratio_);
  grasp_detector_.setEnabled(enable_grasping_);
  grasp_detector_.setDefaultGraspThreshold(grasp_threshold_);
  grasp_detector_.setDefaultReleaseHysteresis(release_hysteresis_);

  // Print out log messages generated above.
  printLogMessages();

  initialize_haptx_system_result_ = !something_went_wrong;
  return initialize_haptx_system_result_;
}

HaptxApi::HaptxSystem& AHxCoreActor::getHaptxSystem() {
  return haptx_system_;
}

HaptxApi::ContactInterpreter& AHxCoreActor::getContactInterpreter() {
  return contact_interpreter_;
}

HaptxApi::GraspDetector& AHxCoreActor::getGraspDetector() {
  return grasp_detector_;
}

void AHxCoreActor::registerSimulatedPeripheral(const HaptxApi::Peripheral& peripheral) {
  if (simulated_hardware_hsv_ == nullptr) {
    return;
  }

  int slot = 0;
  const HaptxApi::Glove* glove = dynamic_cast<const HaptxApi::Glove*>(&peripheral);
  if (glove != nullptr) {
    slot = glove->handedness == HaptxApi::RelativeDirection::RD_LEFT ? 1 : 2;
  } else {
    std::map<int, std::shared_ptr<HaptxApi::Peripheral>> peripheral_from_slot;
    HaptxApi::AirController::ReturnCode ret =
        simulated_hardware_hsv_->getAttachedPeripherals(&peripheral_from_slot);
    if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
      UE_LOG(HaptX, Error,
          TEXT("HsvController::getAttachedPeripherals() failed with return code %d: %s."),
          (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
      return;
    }

    if (peripheral_from_slot.size() > 0) {
      // Map is sorted so last entry should be the highest.
      slot = peripheral_from_slot.rbegin()->first + 1;
    }
  }

  HaptxApi::AirController::ReturnCode ret =
      simulated_hardware_hsv_->attachPeripheral(slot, peripheral);
  if (ret != HaptxApi::AirController::ReturnCode::SUCCESS) {
    UE_LOG(HaptX, Error,
        TEXT("HsvController::attachPeripheral() failed with return code %d: %s."),
        (int)ret, *STRING_TO_FSTRING(HaptxApi::AirController::toString(ret)))
  }
}

bool AHxCoreActor::isHaptxSystemInitialized() const {
  return initialize_haptx_system_result_;
}

void AHxCoreActor::setEnableTactileFeedbackState(bool enabled) {
  enable_tactile_feedback_ = enabled;
  contact_interpreter_.setEnableTactileFeedbackState(enable_tactile_feedback_);
}

void AHxCoreActor::setEnableForceFeedbackState(bool enabled) {
  enable_force_feedback_ = enabled;
  contact_interpreter_.setEnableForceFeedbackState(enable_force_feedback_);
}

void AHxCoreActor::setDefaultGraspThreshold(float threshold) {
  grasp_threshold_ = threshold;
  grasp_detector_.setDefaultGraspThreshold(threshold);
}

void AHxCoreActor::setDefaultReleaseHysteresis(float release_hysteresis) {
  release_hysteresis_ = release_hysteresis;
  grasp_detector_.setDefaultReleaseHysteresis(release_hysteresis);
}

void AHxCoreActor::setEnableGraspingState(bool enabled) {
  grasp_detector_.setEnabled(enabled);
  enable_grasping_ = grasp_detector_.isEnabled();
}

EPhysicsAuthorityMode AHxCoreActor::getPhysicsAuthorityMode() const {
  return physics_authority_mode_;
}

void AHxCoreActor::setPhysicsAuthorityMode(EPhysicsAuthorityMode mode) {
  serverSetPhysicsAuthorityMode(mode);
}

void AHxCoreActor::toggleGraspVisualizer() {
  visualize_grasps_ = !visualize_grasps_;
}

void AHxCoreActor::toggleNetworkStateVisualizer() {
  visualize_network_state_ = !visualize_network_state_;
}

void AHxCoreActor::updateGrasps(float delta_seconds) {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_updateGrasps)
  // Execute any actions recommended by the HaptxApi::GraspDetector.
  grasp_detector_.detectGrasps(delta_seconds);
  for (auto grasp_event = grasp_detector_.getGraspHistory().begin();
      grasp_event != grasp_detector_.getGraspHistory().end(); grasp_event++) {
    const HaptxApi::GraspDetector::Grasp &grasp = grasp_event->grasp;
    FGrasp *fgrasp;
    switch (grasp_event->action) {
    case HaptxApi::GraspDetector::GraspAction::DESTROY:
      fgrasp = grasp_id_to_grasp_.Find(grasp.id);
      if (fgrasp) {
        destroyGrasp(*fgrasp);

        if (on_release_.IsBound()) {
          FGraspObjectInfo *object =
            gd_object_id_to_component_and_bone_.Find(grasp.result.object_id);
          if (object) {
            on_release_.Broadcast(object->component);
          }
        }

        grasp_id_to_grasp_.Remove(grasp.id);
      }
      else {
        UE_LOG(HaptX, Error, TEXT("Attempted to destroy grasp %d, but it didn't exist."), grasp.id)
      }

      break;
    case HaptxApi::GraspDetector::GraspAction::CREATE:
      fgrasp = grasp_id_to_grasp_.Find(grasp.id);
      if (!fgrasp) {
        grasp_id_to_grasp_.Add(grasp.id, FGrasp());

        createGrasp(grasp_id_to_grasp_[grasp.id], grasp.result);

        if (on_grasp_.IsBound()) {
          FGraspObjectInfo *object =
            gd_object_id_to_component_and_bone_.Find(grasp.result.object_id);
          if (object) {
            on_grasp_.Broadcast(object->component);
          }
        }
      }
      else {
        UE_LOG(HaptX, Error, TEXT("Attempted to create grasp %d, but it already existed."), grasp.id)
      }

      break;
    case HaptxApi::GraspDetector::GraspAction::UPDATE:
      fgrasp = grasp_id_to_grasp_.Find(grasp.id);
      if (fgrasp) {
        updateGrasp(*fgrasp, grasp.result);

        if (on_update_.IsBound()) {
          FGraspObjectInfo *object =
            gd_object_id_to_component_and_bone_.Find(grasp.result.object_id);
          if (object) {
            on_update_.Broadcast(object->component);
          }
        }
      }
      else {
        UE_LOG(HaptX, Error, TEXT("Attempted to update grasp %d, but it didn't exist."), grasp.id)
      }

      break;
    default:
      UE_LOG(HaptX, Warning, TEXT("HaptxApi::GraspDetector recommended an invalid action."))
    }
  }
  grasp_detector_.clearGraspHistory();
}

void AHxCoreActor::destroyGrasp(FGrasp &grasp) {
  for (int64_t body_id : grasp.body_ids) {
    // Look for the body.
    FGraspBodyInfo *body = gd_body_id_to_component_and_bone_.Find(body_id);
    if (body && body->component) {
      // Destroy any constraints.
      destroyGraspConstraint(body_id, grasp.stick_constraints);
      destroyGraspConstraint(body_id, grasp.pinch_constraints);
    }
    else {
      UE_LOG(HaptX, Error,
          TEXT("Attempted to destroy a grasp on body %d, but couldn't find it."),
          body_id)
    }
  }
  grasp.body_ids.clear();

  destroyGraspConstraint(&grasp.anchor_constraint);
}

void AHxCoreActor::createGrasp(FGrasp &grasp, const HaptxApi::GraspDetector::GraspResult &result) {
  grasp.body_ids = result.body_ids;
  // Look for the object.
  FGraspObjectInfo *object = gd_object_id_to_component_and_bone_.Find(result.object_id);
  if (object && object->component) {
    for (int64_t body_id : result.body_ids) {
      // Look for the body.
      FGraspBodyInfo *body = gd_body_id_to_component_and_bone_.Find(body_id);
      if (body && body->component) {
        // Create constraint to help objects stick to the hand.
        createStickConstraint(grasp, *body, *object);

        // If this grasp only consists of two bodies, create rotational constraints.
        if (result.body_ids.size() == 2) {
          createPinchConstraint(grasp, *body, *object,
              unrealFromHxLength(result.avg_contact_location));
        }
      }
      else {
        UE_LOG(HaptX, Error,
            TEXT("Attempted to create a grasp on body %d, but couldn't find it."),
            body_id)
      }
    }

    // Look for the parent body if it's not the global root.
    if (result.parent_body_id > 0) {
      FGraspBodyInfo *parent_body = gd_body_id_to_component_and_bone_.Find(result.parent_body_id);
      if (parent_body && parent_body->component) {
        if (parent_body->is_anchor) {
          createAnchorConstraint(grasp, *parent_body, *object,
            unrealFromHxLength(result.avg_contact_location));
        }
      }
      else {
        UE_LOG(HaptX, Error,
          TEXT("Attempted to create a grasp with parent body %d, but couldn't find it."),
          result.parent_body_id)
      }
    }
  }
  else {
    UE_LOG(HaptX, Error,
        TEXT("Attempted to create a grasp on object %d, but couldn't find it."),
        result.object_id)
  }
}

void AHxCoreActor::updateGrasp(FGrasp &grasp,
    const HaptxApi::GraspDetector::GraspResult &result) {
  // Destroy old constraints.
  for (int64_t body_id : grasp.body_ids) {
    destroyGraspConstraint(body_id, grasp.stick_constraints);
    destroyGraspConstraint(body_id, grasp.pinch_constraints);
  }
  grasp.body_ids = result.body_ids;

  // Look for the object.
  FGraspObjectInfo *object =
      gd_object_id_to_component_and_bone_.Find(result.object_id);
  if (object && object->component) {
    for (int64_t body_id : result.body_ids) {
      // Look for the body.
      FGraspBodyInfo *body =
          gd_body_id_to_component_and_bone_.Find(body_id);
      if (body && body->component) {
        // Create new stick constraints.
        createStickConstraint(grasp, *body, *object);

        // Create new pinch constraints.
        if (result.body_ids.size() == 2) {
          createPinchConstraint(grasp, *body, *object,
              unrealFromHxLength(result.avg_contact_location));
        }
      }
      else {
        UE_LOG(HaptX, Error,
            TEXT("Attempted to update a grasp on body %d, but couldn't find it."),
            body_id)
      }
    }
  }
  else {
    UE_LOG(HaptX, Error,
        TEXT("Attempted to update a grasp on object %d, but couldn't find it."),
        result.object_id)
  }
}

void AHxCoreActor::createStickConstraint(FGrasp& grasp, FGraspBodyInfo& body,
    FGraspObjectInfo& object) {
  UPhysicsConstraintComponent* linear_constraint = NewObject<UPhysicsConstraintComponent>(this,
      UPhysicsConstraintComponent::StaticClass());
  linear_constraint->RegisterComponent();

  linear_constraint->ConstraintInstance.SetLinearXMotion(ELinearConstraintMotion::LCM_Free);
  linear_constraint->ConstraintInstance.SetLinearYMotion(ELinearConstraintMotion::LCM_Free);
  linear_constraint->ConstraintInstance.SetLinearZMotion(ELinearConstraintMotion::LCM_Free);
  linear_constraint->ConstraintInstance.SetAngularTwistMotion(EAngularConstraintMotion::ACM_Free);
  linear_constraint->ConstraintInstance.SetAngularSwing1Motion(EAngularConstraintMotion::ACM_Free);
  linear_constraint->ConstraintInstance.SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Free);
  UHxPhysicalMaterial* hx_phys_mat = Cast<UHxPhysicalMaterial>(getPhysicalMaterial(
	  object.component, object.bone_name));
  const FConstraintDrive& drive_setting = (hx_phys_mat != nullptr && hx_phys_mat->override_grasp_drives_) ?
	  hx_phys_mat->grasp_drives_ : grasp_linear_drive_;
  linear_constraint->ConstraintInstance.ProfileInstance.LinearDrive.XDrive = drive_setting;
  linear_constraint->ConstraintInstance.ProfileInstance.LinearDrive.YDrive = drive_setting;
  linear_constraint->ConstraintInstance.ProfileInstance.LinearDrive.ZDrive = drive_setting;
  linear_constraint->SetWorldLocation(body.component->GetSocketLocation(body.bone_name));
  linear_constraint->SetConstrainedComponents(body.component, body.bone_name, object.component,
      object.bone_name);
  grasp.stick_constraints.Add(body.id, linear_constraint);
  notifyHandsOfConstraintCreation(linear_constraint);
}

void AHxCoreActor::createPinchConstraint(FGrasp& grasp, FGraspBodyInfo& body,
    FGraspObjectInfo& object, FVector constraint_location) {
  UPhysicsConstraintComponent* pinch_constraint = NewObject<UPhysicsConstraintComponent>(this,
      UPhysicsConstraintComponent::StaticClass());
  pinch_constraint->RegisterComponent();

  pinch_constraint->ConstraintInstance.SetLinearXMotion(ELinearConstraintMotion::LCM_Free);
  pinch_constraint->ConstraintInstance.SetLinearYMotion(ELinearConstraintMotion::LCM_Free);
  pinch_constraint->ConstraintInstance.SetLinearZMotion(ELinearConstraintMotion::LCM_Free);
  pinch_constraint->ConstraintInstance.SetAngularTwistMotion(EAngularConstraintMotion::ACM_Free);
  pinch_constraint->ConstraintInstance.SetAngularSwing1Motion(EAngularConstraintMotion::ACM_Free);
  pinch_constraint->ConstraintInstance.SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Free);
  pinch_constraint->ConstraintInstance.ProfileInstance.AngularDrive.AngularDriveMode = EAngularDriveMode::SLERP;
  UHxPhysicalMaterial* hx_phys_mat = Cast<UHxPhysicalMaterial>(getPhysicalMaterial(
	  object.component, object.bone_name));
  pinch_constraint->ConstraintInstance.ProfileInstance.AngularDrive.SlerpDrive = (hx_phys_mat != nullptr && hx_phys_mat->override_grasp_drives_) ?
	  hx_phys_mat->grasp_drives_ : pinch_angular_drive_;
  pinch_constraint->ConstraintInstance.SetAngularOrientationTarget(FQuat::Identity);
  pinch_constraint->SetWorldLocation(constraint_location);
  pinch_constraint->SetAngularVelocityTarget(FVector::ZeroVector);
  pinch_constraint->SetConstrainedComponents(body.component, body.bone_name, object.component,
      object.bone_name);
  grasp.pinch_constraints.Add(body.id, pinch_constraint);
  notifyHandsOfConstraintCreation(pinch_constraint);
}

void AHxCoreActor::createAnchorConstraint(FGrasp& grasp, FGraspBodyInfo& anchor,
    FGraspObjectInfo& object, FVector constraint_location) {
  UPhysicsConstraintComponent* anchor_constraint = NewObject<UPhysicsConstraintComponent>(this,
      UPhysicsConstraintComponent::StaticClass());
  anchor_constraint->RegisterComponent();

  UHxPhysicalMaterial* hx_phys_mat = Cast<UHxPhysicalMaterial>(getPhysicalMaterial(
      object.component, object.bone_name));
  anchor_constraint->ConstraintInstance.ProfileInstance.LinearLimit =
    (hx_phys_mat != nullptr && hx_phys_mat->override_grasp_linear_limit_) ?
    hx_phys_mat->grasp_linear_limit_ : grasp_linear_limit_;
  anchor_constraint->ConstraintInstance.ProfileInstance.TwistLimit =
    (hx_phys_mat != nullptr && hx_phys_mat->override_grasp_twist_limit_) ?
    hx_phys_mat->grasp_twist_limit_ : grasp_twist_limit_;
  anchor_constraint->ConstraintInstance.ProfileInstance.ConeLimit =
    (hx_phys_mat != nullptr && hx_phys_mat->override_grasp_cone_limit_) ?
    hx_phys_mat->grasp_cone_limit_ : grasp_cone_limit_;

  anchor_constraint->SetWorldLocationAndRotation(constraint_location,
      anchor.component->GetSocketQuaternion(anchor.bone_name));
  anchor_constraint->SetConstrainedComponents(object.component, object.bone_name,
      anchor.component, anchor.bone_name);
  grasp.anchor_constraint = anchor_constraint;
  notifyHandsOfConstraintCreation(anchor_constraint);
}

void AHxCoreActor::destroyGraspConstraint(int64_t body_id,
    TMap<int64, UPhysicsConstraintComponent*>& map) {
  destroyGraspConstraint(map.Find(body_id));
  map.Remove(body_id);
}

void AHxCoreActor::destroyGraspConstraint(UPhysicsConstraintComponent** constraint_ptr_ptr) {
  if (constraint_ptr_ptr != nullptr) {
    notifyHandsOfConstraintDestruction(*constraint_ptr_ptr);
    destroyPhysicsConstraintComponent(*constraint_ptr_ptr);
    *constraint_ptr_ptr = nullptr;
  }
}

void AHxCoreActor::notifyHandsOfConstraintCreation(UPhysicsConstraintComponent* constraint) {
  if (!IsValid(constraint)) {
    return;
  }

  USkeletalMeshComponent* skel =
      Cast<USkeletalMeshComponent>(constraint->OverrideComponent1.Get());
  if (!IsValid(skel)) {
    skel = Cast<USkeletalMeshComponent>(constraint->OverrideComponent2.Get());
  }

  if (IsValid(skel)) {
    AHxHandActor* hand = Cast<AHxHandActor>(skel->GetOwner());
    if (IsValid(hand)) {
      hand->notifyLocalConstraintCreated(constraint);
    }
  }
}

void AHxCoreActor::notifyHandsOfConstraintDestruction(UPhysicsConstraintComponent* constraint) {
  if (!IsValid(constraint)) {
    return;
  }

  USkeletalMeshComponent* skel =
      Cast<USkeletalMeshComponent>(constraint->OverrideComponent1.Get());
  if (!IsValid(skel)) {
    skel = Cast<USkeletalMeshComponent>(constraint->OverrideComponent2.Get());
  }

  if (IsValid(skel)) {
    AHxHandActor* hand = Cast<AHxHandActor>(skel->GetOwner());
    if (IsValid(hand)) {
      hand->notifyLocalConstraintDestroyed(constraint);
    }
  }
}

void AHxCoreActor::visualizeGrasps() {
  SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_visualizeGrasps)
  // The color to use when an object is grasped.
  const FColor GR_GRASPED_COLOR = DEBUG_PURPLE_OR_TEAL;
  // The color to use when an object is not grasped.
  const FColor GR_NOT_GRASPED_COLOR = DEBUG_BLUE_OR_YELLOW;
  // The color of the score bar base.
  const FColor GR_BASE_COLOR = DEBUG_BLACK;
  // The color of the threshold bar.
  const FColor GR_THRESHOLD_COLOR = DEBUG_GRAY;
  // The extent of the score bar base.
  const FVector W_GR_BASE_EXTENT_CM = FVector(1.0f, 1.0f, 0.1f);
  // The vertical offset of the score bars from objects being grasped.
  const float W_GR_VERTICAL_OFFSET_CM = 3.0f;
  // The horizontal amount by which to space out score bars when multiple grasps are present.
  const float W_GR_HORIZONTAL_OFFSET_CM = 3.0f;
  // The width and length of the threshold bar.
  const float W_GR_THRESHOLD_EXTENT_CM = 0.5f;
  // The width and length of the grasp score bar.
  const float W_GR_WIDTH_CM = 0.75f;
  // The thickness that body lines will be drawn.
  const float W_GR_OBJECT_LINE_THICKNESS_CM = 0.3f;

  // Keep track of how many grasps have formed per object.
  TMap<int64_t, unsigned int> object_id_to_grasp_count;
  // Loop over all grasp results (even those that do not constitute a grasp).
  HaptxApi::GraspDetector& gd = grasp_detector_;
  for (const auto &result : gd.getAllGraspResults()) {
    // Verify that the object exists.
    FGraspObjectInfo *object = gd_object_id_to_component_and_bone_.Find(result.object_id);
    if (object && object->component) {
      // Increment the grasp count for this object.
      int grasp_index = object_id_to_grasp_count.FindOrAdd(result.object_id);
      object_id_to_grasp_count[result.object_id] = grasp_index + 1;

      // Draw the bar at about the object's mesh.
      FVector w_bar_location = object->component->GetCenterOfMass(object->bone_name)
          + W_GR_HORIZONTAL_OFFSET_CM * grasp_index * FVector::RightVector
          + (object->component->Bounds.SphereRadius + W_GR_VERTICAL_OFFSET_CM) * FVector::UpVector;

      // Draw the base of the bar with half its height offset down.
      HxDebugDrawSystem::box(
          this,
          w_bar_location + (W_GR_BASE_EXTENT_CM * -FVector::UpVector),
          W_GR_BASE_EXTENT_CM,
          FQuat::Identity,
          GR_BASE_COLOR);

      // Draw the threshold.
      float w_threshold_height = grasp_visualization_parameters_.score_to_cm *
          (result.parameters->override_default_grasp_threshold ?
          result.parameters->grasp_threshold : gd.getDefaultGraspThreshold());
      FColor score_color = GR_NOT_GRASPED_COLOR;
      const HaptxApi::GraspDetector::Grasp* grasp =
          gd.getGrasp(result.object_id, result.parent_body_id);
      if (grasp != nullptr) {
        w_threshold_height *= (result.parameters->override_default_release_hysteresis ?
            result.parameters->release_hysteresis : gd.getDefaultReleaseHysteresis());
        score_color = GR_GRASPED_COLOR;
      }
      FVector w_threshold_extent = FVector(
          W_GR_THRESHOLD_EXTENT_CM,
          W_GR_THRESHOLD_EXTENT_CM,
          w_threshold_height);
      HxDebugDrawSystem::box(
          this,
          w_bar_location + w_threshold_extent * FVector::UpVector,
          w_threshold_extent,
          FQuat::Identity,
          GR_THRESHOLD_COLOR);

      // Draw the total score bar.
      FVector w_score_extent = FVector(W_GR_WIDTH_CM, W_GR_WIDTH_CM,
          result.score * grasp_visualization_parameters_.score_to_cm);
      HxDebugDrawSystem::box(
          this,
          w_bar_location + w_score_extent.Z * FVector::UpVector,
          w_score_extent,
          FQuat::Identity,
          score_color);

      // Draw lines between the bodies contributing to the grasp, and the object being grasped.
      for (int64_t body_id : result.body_ids) {
        // Verify that our body exists.
        FGraspBodyInfo *body = gd_body_id_to_component_and_bone_.Find(body_id);
        if (body && body->component) {
          FVector object_location = object->component->GetCenterOfMass(object->bone_name);
          FVector body_location = body->component->GetCenterOfMass(body->bone_name);
          HxDebugDrawSystem::line(this, object_location, body_location,
              score_color, W_GR_OBJECT_LINE_THICKNESS_CM);
        }
      }
    }
  }
}

bool AHxCoreActor::serverSetPhysicsAuthorityMode_Validate(EPhysicsAuthorityMode) {
  return true;
}

void AHxCoreActor::serverSetPhysicsAuthorityMode_Implementation(EPhysicsAuthorityMode mode) {
  physics_authority_mode_ = mode;
}

AHxCoreActor* AHxCoreActor::getAndMaintainPseudoSingleton(UWorld* world) {
  if (world == nullptr) {
    return nullptr;
  }
  // Return early if there is already a designated core.
  if (designated_core_actor_ != nullptr) {
    return designated_core_actor_->isHaptxSystemInitialized() ? designated_core_actor_ : nullptr;
  }

  // If a valid singleton exists, return it.
  if (IsValid(designated_core_actor_)) {
    return designated_core_actor_;
  } else {
    designated_core_actor_ = nullptr;
  }

  // Look for an existing core in the level.
  AHxCoreActor* core_actor = nullptr;
  TArray<AActor*> core_actors;
  UGameplayStatics::GetAllActorsOfClass(world, AHxCoreActor::StaticClass(), core_actors);
  if (core_actors.Num() > 1) {
    for (AActor* actor : core_actors) {
      if (core_actor == nullptr) {
        core_actor = Cast<AHxCoreActor>(actor);
      } else {
        UE_LOG(HaptX, Warning,
            TEXT("More than one AHxCoreActor detected in the level. Deleting %s!\nMake sure to only have one if you want to configure the HaptX system, otherwise the one you configure might be deleted due to the arbitrary order of BeginPlay() calls."), *actor->GetName());
        actor->Destroy();
      }
    }
  } else if (core_actors.Num() == 1) {
    core_actor = Cast<AHxCoreActor>(core_actors[0]);
  }

  // If we did not find a AHxCoreActor, spawn one.
  if (core_actor == nullptr) {
    AActor* actor = world->SpawnActor<AHxCoreActor>();
    UE_LOG(HaptX, Log, TEXT("A HaptXCoreActor has been added to the scene."));
    core_actor = Cast<AHxCoreActor>(actor);
  }

  // Open the core_actor
  core_actor->initializeHaptxSystem();

  // If openAllInterfaces() succeeded, designated_core_actor_ will have been assigned the value
  // of core_actor, otherwise designated_core_actor_ will be nullptr.
  return designated_core_actor_;
}

bool AHxCoreActor::tryRegisterObjectWithCi(UPrimitiveComponent* comp, FName bone,
    bool register_again, int64_t& object_id) {
  // The default contact tolerance distance to use on objects registered with the CI.
  static constexpr const float DEFAULT_CONTACT_TOLERANCE_M = 0.f;
  // The default object compliance to use on objects registered with the CI.
  static constexpr const float DEFAULT_OBJECT_COMPLIANCE_M_N = 0.f;

  if (comp == nullptr) {
    UE_LOG(HaptX, Error, TEXT("AHxCoreActor::registerObjectWithCi(): Null component provided."))
    return false;
  }

  FBodyInstance* obj_inst = comp->GetBodyInstance(bone, false);
  if (obj_inst == nullptr) {
    UE_LOG(HaptX, Error,
        TEXT("AHxCoreActor::registerObjectWithCi(): Component %s doesn't have an associated FBodyInstance."),
        *comp->GetName())
    return false;
  }

  object_id = getBodyInstanceId(obj_inst);
  if (object_id == INVALID_BODY_INSTANCE_ID) {
    UE_LOG(HaptX, Error, TEXT(
        "AHxCoreActor::registerObjectWithCi(): Failed to get body instance ID for component %s and bone %s."),
        *comp->GetName(), *bone.ToString())
    return false;
  }

  UHxPhysicalMaterial* hx_phys_mat = Cast<UHxPhysicalMaterial>(
      obj_inst->GetSimplePhysicalMaterial());

  // If we should register the object with the CI .
  if (!contact_interpreter_.isObjectRegistered(object_id) || register_again) {
    SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_registerObjectIfNotAlready_CI)

    const bool causes_tf = !allow_tactile_feedback_collision_type_whitelist_ ||
        tactile_feedback_collision_types_.Contains(comp->GetCollisionObjectType());
    const bool causes_ff = !allow_force_feedback_collision_type_whitelist_ ||
        force_feedback_collision_types_.Contains(comp->GetCollisionObjectType());

    HaptxApi::ContactInterpreter::ObjectParameters ci_properties = { causes_tf, causes_ff,
        DEFAULT_CONTACT_TOLERANCE_M, DEFAULT_OBJECT_COMPLIANCE_M_N };

    if (hx_phys_mat != nullptr) {
      ci_properties.triggers_tactile_feedback = causes_tf &&
          !hx_phys_mat->disable_tactile_feedback_;
      if (hx_phys_mat->override_force_feedback_enabled_) {
        ci_properties.triggers_force_feedback = hx_phys_mat->force_feedback_enabled_;
      }
      if (hx_phys_mat->override_base_contact_tolerance_) {
        ci_properties.base_contact_tolerance_m =
            hxFromUnrealLength(hx_phys_mat->base_contact_tolerance_cm_);
      }
      if (hx_phys_mat->override_compliance_) {
        ci_properties.compliance_m_n = hx_phys_mat->compliance_cm_cn_;
      }
    }

    std::shared_ptr<const HaptxApi::SimulationCallbacks>& callbacks =
        ci_object_id_to_callbacks_.Emplace(object_id,
        std::make_shared<const PrimitiveComponentCallbacks>(comp, bone));
    contact_interpreter_.registerObject(object_id, ci_properties, callbacks);
  }

  return true;
}

void AHxCoreActor::registerBodyWithCi(int64_t ci_body_id, UPrimitiveComponent* comp, FName bone,
    const FBodyParameters& parameters, HaptxApi::RigidBodyPart rigid_body_part) {
  if (comp == nullptr) {
    UE_LOG(HaptX, Error, TEXT("AHxCoreActor::registerBodyWithCi(): Null component provided."))
    return;
  }

  FBodyInstance* obj_inst = comp->GetBodyInstance(bone, false);
  if (obj_inst == nullptr) {
    UE_LOG(HaptX, Error,
        TEXT("AHxCoreActor::registerBodyWithCi(): Component %s doesn't have an associated FBodyInstance."),
        *comp->GetName())
    return;
  }

  std::shared_ptr<const HaptxApi::SimulationCallbacks>& callbacks =
      ci_object_id_to_callbacks_.Emplace(ci_body_id,
      std::make_shared<const PrimitiveComponentCallbacks>(comp, bone));
  contact_interpreter_.registerBody(ci_body_id, parameters.unwrap(), rigid_body_part, callbacks);
}

bool AHxCoreActor::tryRegisterObjectWithGd(UPrimitiveComponent* comp, FName bone,
    bool register_again, int64_t& object_id) {
  if (comp == nullptr) {
    UE_LOG(HaptX, Error, TEXT("AHxCoreActor::registerObjectWithGd(): Null component provided."))
    return false;
  }

  FBodyInstance* obj_inst = getBodyInstance(comp, bone);
  if (obj_inst == nullptr || !IsValid(obj_inst->OwnerComponent.Get())) {
    UE_LOG(HaptX, Error, TEXT(
        "AHxCoreActor::registerObjectWithGd(): Component %s doesn't have an associated FBodyInstance."),
        *comp->GetName())
    return false;
  }

  object_id = getBodyInstanceId(obj_inst);
  UHxPhysicalMaterial* hx_phys_mat = Cast<UHxPhysicalMaterial>(
      obj_inst->GetSimplePhysicalMaterial());
  ECollisionChannel collision_object_type = obj_inst->OwnerComponent->GetCollisionObjectType();

  // If we should register the object with the GD.
  if (!grasp_detector_.isObjectRegistered(object_id) || register_again) {
    SCOPE_CYCLE_COUNTER_IF_PROFILING(STAT_registerObjectIfNotAlready_GD)

    HaptxApi::GraspDetector::ObjectParameters gd_properties =
        HaptxApi::GraspDetector::DEFAULT_OBJECT_PARAMETERS;
    const bool causes_grasps = !allow_grasp_collision_type_whitelist_ ||
        grasp_collision_types_.Contains(collision_object_type);
    gd_properties.can_be_grasped = causes_grasps;

    if (hx_phys_mat != nullptr) {
      if (hx_phys_mat->override_grasping_enabled_) {
        gd_properties.can_be_grasped = hx_phys_mat->grasping_enabled_;
      }
      gd_properties.override_default_grasp_threshold = hx_phys_mat->override_grasp_threshold_;
      gd_properties.grasp_threshold = hx_phys_mat->grasp_threshold_;
      gd_properties.override_default_release_hysteresis =
          hx_phys_mat->override_release_hysteresis_;
      gd_properties.release_hysteresis = hx_phys_mat->release_hysteresis_;
    }

    grasp_detector_.registerObject(object_id, gd_properties);

    // Make sure that the HxCoreActor can find this object by its ID.
    USkeletalMeshComponent* skel_mesh_comp = Cast<USkeletalMeshComponent>(
        obj_inst->OwnerComponent.Get());
    gd_object_id_to_component_and_bone_.Emplace(object_id,
        FGraspObjectInfo(obj_inst->OwnerComponent.Get(), skel_mesh_comp != nullptr ?
        skel_mesh_comp->GetBoneName(obj_inst->InstanceBoneIndex) : NAME_None));
  }
  return true;
}

void AHxCoreActor::registerGdBody(int64_t gd_body_id, UPrimitiveComponent* comp, FName bone,
    bool is_anchor) {
  if (comp == nullptr) {
    UE_LOG(HaptX, Error, TEXT("AHxCoreActor::registerBodyWithGd(): Null component provided."))
    return;
  }

  gd_body_id_to_component_and_bone_.Add(gd_body_id,
      FGraspBodyInfo(gd_body_id, comp, bone, is_anchor));
}

void AHxCoreActor::printLogMessages() {
  // Take any messages logged from the API, and add them to the Unreal Engine log.
  while (haptx_log_messages_.size() > 0) {
    HaptxApi::LogMessage &log_message = haptx_log_messages_.front();

    FString message = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(log_message.caller.c_str()),
        UTF8_TO_TCHAR(log_message.message_data.c_str()));

    switch (log_message.severity) {
      case (HaptxApi::ELogSeverity::ELS_WARNING):
        AHxCoreActor::logWarning(message,
            log_message.access_level == HaptxApi::ELogAccess::ELA_USER);
        break;
      case (HaptxApi::ELogSeverity::ELS_ERROR):
        AHxCoreActor::logError(message,
            log_message.access_level == HaptxApi::ELogAccess::ELA_USER);
        break;
      default:
        AHxCoreActor::log(message);
    }

    // Remove the first message from the list.
    haptx_log_messages_.pop_front();
  }
}

void AHxCoreActor::customPhysics(float delta_time_s_, FBodyInstance* body_instance) {
  physics_delta_time_s_ += delta_time_s_;
}

int32 AHxCoreActor::log(const TCHAR* message, bool add_to_screen, int32 message_key,
    float duration) {
  return AHxCoreActor::log(FString(message), add_to_screen, duration, message_key);
}

int32 AHxCoreActor::logWarning(const TCHAR* message, bool add_to_screen, int32 message_key) {
  return AHxCoreActor::logWarning(FString(message), add_to_screen, message_key);
}

int32 AHxCoreActor::logError(const TCHAR* message, bool add_to_screen, int32 message_key) {
  return AHxCoreActor::logError(FString(message), add_to_screen, message_key);
}

int32 AHxCoreActor::log(const FString& message, bool add_to_screen, int32 message_key,
    float duration) {
  UE_LOG(HaptX, Log, TEXT("%s"), *message)
  if (add_to_screen) {
    return AHxOnScreenLog::logToScreenWithSeverity(message, EOnScreenMessageSeverity::INFO,
        duration, FColor::Silver, message_key);
  }
  return -1;
}

int32 AHxCoreActor::logWarning(const FString& message, bool add_to_screen, int32 message_key) {
  // The duration [s] to display on-screen warning messages.
  // TODO: Make this a configurable property.
  const float ON_SCREEN_WARNING_MESSAGE_DURATION = 10.0f;

  UE_LOG(HaptX, Warning, TEXT("%s"), *message)
  if (add_to_screen) {
    return AHxOnScreenLog::logToScreenWithSeverity(message, EOnScreenMessageSeverity::WARNING,
        ON_SCREEN_WARNING_MESSAGE_DURATION, FColor::Yellow, message_key);
  }
  return -1;
}

int32 AHxCoreActor::logError(const FString& message, bool add_to_screen, int32 message_key) {
  // The duration [s] to display on-screen error messages.
  // TODO: Make this a configurable property.
  const float ON_SCREEN_ERROR_MESSAGE_DURATION = 15.0f;

  UE_LOG(HaptX, Error, TEXT("%s"), *message)
  if (add_to_screen) {
    return AHxOnScreenLog::logToScreenWithSeverity(message, EOnScreenMessageSeverity::ERROR,
        ON_SCREEN_ERROR_MESSAGE_DURATION, FColor::Red, message_key);
  }
  return -1;
}

void AHxCoreActor::logRestartMessage() {
  if (!has_printed_restart_message_) {
    static const FString message = TEXT(
        "There were errors in the HaptX SDK. You must fix them, then restart your game before the HaptX SDK will function correctly again. Check the log for more information.");
    UE_LOG(HaptX, Error, TEXT("%s"), *message)
    AHxOnScreenLog::logToScreenWithSeverity(message, EOnScreenMessageSeverity::ERROR, MAX_FLT,
        FColor::Silver);
    has_printed_restart_message_ = true;
  }
}
