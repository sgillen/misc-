#pragma once

#include <memory>
#include <vector>

#include "drake/examples/compass_gait/gen/compass_gait_continuous_state.h"
#include "drake/examples/compass_gait/gen/compass_gait_params.h"
#include "drake/systems/framework/event.h"
#include "drake/systems/framework/leaf_system.h"
#include "drake/systems/framework/scalar_conversion_traits.h"
#include "drake/systems/framework/witness_function.h"

namespace drake {
namespace examples {
namespace three_link {


  //sgillen@20181223-11:12  TODO update the docs
  
/// Dynamical representation of the idealized hybrid dynamics of a "compass
/// gait", as described in
///   http://underactuated.mit.edu/underactuated.html?chapter=simple_legs
/// with a few small differences.  The implementation here has no torque input
/// (yet).  In addition, this implementation has two additional state
/// variables that are not required in the mathematical model:
///   - a discrete state for the position of the stance toe along the ramp
///   - a Boolean indicator for "left support" (true when the stance leg is
///     the left leg).
/// These are helpful for outputting the floating-base model coordinate, e.g.
/// for visualization.
///
/// Note: This model only supports walking downhill on the ramp, because that
/// restriction enables a clean / numerically robust implementation of the foot
/// collision witness function that avoids falls detections on the "foot
/// scuffing" collision.
///
/// Inputs: None.<br/>
/// Continuous States: stance, swing, stancedot, swingdot.<br/>
/// Discrete State: stance toe position.<br/>
/// Abstract State: left support indicator.<br/>
/// Outputs:
///   <ol start=0>
///   <li>continuous (minimal coordinates) state output</li>
///   <li>floating-base state output</li>
///   </ol>
///
/// @tparam T The vector element type, which must be a valid Eigen scalar.
///
/// Instantiated templates for the following scalar types @p T are provided:
/// - double
/// - AutoDiffXd
/// - symbolic::Expression
template <typename T>
class ThreeLink final : public systems::LeafSystem<T> {
 public:
  DRAKE_NO_COPY_NO_MOVE_NO_ASSIGN(ThreeLink);

  /// Constructs the plant.
  ThreeLink();

  /// Scalar-converting copy constructor.  See @ref system_scalar_conversion.
  template <typename U>
  explicit ThreeLink(const ThreeLink<U>&) : ThreeLink<T>() {}

  /// Returns reference to the output port that publishes only
  /// [theta_stance, theta_swing, thetatdot_stance, thetadot_swing].
  const systems::OutputPort<T>& get_minimal_state_output_port() const {
    return this->get_output_port(0);
  }

  /// Returns reference to the output port that provides the state required by
  /// a RigidBodyTree loaded from ThreeLink.urdf (as instantiated with
  /// FloatingBaseType::kRollPitchYaw).
  const systems::OutputPort<T>& get_floating_base_state_output_port() const {
    return this->get_output_port(1);
  }

  /// Returns the ThreeLinkContinuousState.
  static const ThreeLinkContinuousState<T>& get_continuous_state(
      const systems::Context<T>& context) {
    return get_continuous_state(context.get_continuous_state());
  }

  /// Returns the mutable ThreeLinkContinuousState.
  static ThreeLinkContinuousState<T>& get_mutable_continuous_state(
      systems::Context<T>* context) {
    return get_mutable_continuous_state(
        &context->get_mutable_continuous_state());
  }

  static const T& get_toe_position(const systems::Context<T>& context) {
    return context.get_discrete_state(0).GetAtIndex(0);
  }

  static void set_toe_position(const T& value, systems::State<T>* state) {
    state->get_mutable_discrete_state().get_mutable_vector(0).SetAtIndex(0,
                                                                         value);
  }

  static bool left_leg_is_stance(const systems::Context<T> &context) {
    return context.template get_abstract_state<bool>(0);
  }

  static void set_left_leg_is_stance(bool value, systems::State<T>* state) {
    state->template get_mutable_abstract_state<bool>(0) = value;
  }

  /// Access the ThreeLinkParams.
  const ThreeLinkParams<T>& get_parameters(
      const systems::Context<T>& context) const {
    return this->template GetNumericParameter<ThreeLinkParams>(context, 0);
  }

  ///@{
  /// Manipulator equation of ThreeLink: M(q)v̇ + bias(q,v) = 0.
  /// - M is the 2x2 mass matrix.
  /// - bias is a 2x1 vector that includes the Coriolis term and gravity term,
  ///     i.e. bias = C(q,v)*v - τ_g(q).
  Vector2<T> DynamicsBiasTerm(const systems::Context<T> &context) const;
  Matrix2<T> MassMatrix(const systems::Context<T> &context) const;
  ///@}

 private:
  static const ThreeLinkContinuousState<T>& get_continuous_state(
      const systems::ContinuousState<T>& cstate) {
    return dynamic_cast<const ThreeLinkContinuousState<T>&>(
        cstate.get_vector());
  }

  static ThreeLinkContinuousState<T>& get_mutable_continuous_state(
      systems::ContinuousState<T>* cstate) {
    return dynamic_cast<ThreeLinkContinuousState<T>&>(
        cstate->get_mutable_vector());
  }

  // Calculate the kinetic and potential energy (in the world frame attached to
  // the stance toe).
  T DoCalcKineticEnergy(const systems::Context<T>& context) const final;
  T DoCalcPotentialEnergy(const systems::Context<T>& context) const final;

  // WitnessFunction to check when the foot hits the ramp (with a sufficiently
  // large step length).
  T FootCollision(const systems::Context<T>& context) const;

  // Handles the impact dynamics, including resetting the stance and swing legs.
  void CollisionDynamics(const systems::Context<T> &context,
                         const systems::UnrestrictedUpdateEvent<T> &,
                         systems::State<T> *state) const;

  void MinimalStateOut(const systems::Context<T>& context,
                       ThreeLinkContinuousState<T>* output) const;

  void FloatingBaseStateOut(const systems::Context<T>& context,
                            systems::BasicVector<T>* output) const;

  // Implements the simple double pendulum dynamics during stance.
  void DoCalcTimeDerivatives(
      const systems::Context<T>& context,
      systems::ContinuousState<T>* derivatives) const final;

  void DoGetWitnessFunctions(const systems::Context<T>&,
                             std::vector<const systems::WitnessFunction<T>*>*
                                 witnesses) const final;

  // The system stores its witness function internally.
  std::unique_ptr<systems::WitnessFunction<T>> foot_collision_;
};

}  // namespace compass_gait
}  // namespace examples
}  // namespace drake
