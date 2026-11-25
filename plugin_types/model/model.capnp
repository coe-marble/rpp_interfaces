@0xbe0920075bdbcf34;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";
using Rt = import "rpp_interfaces/common/execution.capnp";

struct ModelDesc {
  state @0 :D.FieldListDesc; # Input coordinate space for state, retract and difference.
  inputs @1 :D.FieldListDesc;
  prediction @2 :D.FieldListDesc; # Fields written by predict(); may be a subset of state.
  observations @3 :D.FieldListDesc; # Fields written by observe().
  accelerations @4 :D.FieldListDesc; # inverseDynamics input; empty when unsupported.
  efforts @5 :D.FieldListDesc; # inverseDynamics output; empty when unsupported.
  features @6 :UInt8; # model/constants.capnp: model* bitmask
}

interface Model $Anot.plugin("Model") {
  # Declare numerical inputs/outputs and optional operations; no live state is owned.
  getDesc @0 () -> (desc :ModelDesc);
  # Predict declared output fields at timeNs + dtNs from supplied state at timeNs.
  predict @1 (state :List(Float64), input :List(Float64), timeNs :UInt64,
              dtNs :UInt64, inputDtNs :UInt64)
      -> (next :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Return A[prediction tangent x state tangent], B[prediction tangent x input values].
  linearize @2 (state :List(Float64), input :List(Float64), timeNs :UInt64,
                dtNs :UInt64, inputDtNs :UInt64)
      -> (a :List(Float64), b :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Apply an increment in desc.state's coordinate space.
  retract @3 (state :List(Float64), delta :List(Float64))
      -> (next :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Compute an increment between two values in desc.state's coordinate space.
  difference @4 (origin :List(Float64), target :List(Float64))
      -> (delta :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Predict one flat vector in desc.observations order, without target subtraction.
  observe @5 (state :List(Float64), input :List(Float64), timeNs :UInt64,
              inputDtNs :UInt64)
      -> (values :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Return Jacobians with observation-tangent rows and state-tangent/input-value columns.
  observationJacobian @6 (state :List(Float64), input :List(Float64),
                          timeNs :UInt64, inputDtNs :UInt64)
      -> (stateJacobian :List(Float64), inputJacobian :List(Float64),
          status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Map desc.accelerations to desc.efforts using the supplied state.
  inverseDynamics @7 (state :List(Float64), acceleration :List(Float64))
      -> (effort :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Compute an increment in desc.observations' coordinate space.
  observationDifference @8 (origin :List(Float64), target :List(Float64))
      -> (delta :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
}
