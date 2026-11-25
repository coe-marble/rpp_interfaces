@0x8c5c85bf1367f887;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";
using Rt = import "rpp_interfaces/common/execution.capnp";

struct ControlConstraintDesc {
  state @0 :D.FieldListDesc;
  commands @1 :D.FieldListDesc;
  values @2 :D.FieldListDesc; # Euclidean rows; bounds packs all lower then all upper values.
}

interface ControlConstraint $Anot.plugin("ControlConstraint") {
  # Declare candidate inputs and constraint output rows.
  getDesc @0 () -> (desc :ControlConstraintDesc);
  # Read lower/upper constraint bounds for a solver stage time.
  bounds @1 (timeNs :UInt64) -> (bounds :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Evaluate constraint rows at a candidate state/input inside the controller.
  evaluate @2 (state :List(Float64), input :List(Float64), timeNs :UInt64,
               inputDtNs :UInt64) -> (values :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Differentiate those rows with respect to state increments and model inputs.
  jacobian @3 (state :List(Float64), input :List(Float64), timeNs :UInt64,
               inputDtNs :UInt64)
      -> (stateJacobian :List(Float64), inputJacobian :List(Float64),
          status :UInt8) $Rt.realtime; # common/constants.capnp: code*
}
