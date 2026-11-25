@0xf239d88271776c00;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";
using Rt = import "rpp_interfaces/common/execution.capnp";

struct EstimatorDesc {
  state @0 :D.FieldListDesc; # Owned estimate; covariance uses its tangent dimension.
  inputs @1 :D.FieldListDesc;
  measurements @2 :List(D.FieldListDesc); # Measurement input layouts.
}

interface Estimator $Anot.plugin("Estimator") {
  # Declare estimate output, known inputs and measurement layouts.
  getDesc @0 () -> (desc :EstimatorDesc);
  # Establish the initial estimate and reference time while inactive.
  initialize @1 (state :List(Float64), covariance :List(Float64), timeNs :UInt64)
      -> (status :UInt8); # common/constants.capnp: code*
  # Advance the owned estimate to timeNs using discrete process covariance.
  predict @2 (input :List(Float64), processCovariance :List(Float64),
              timeNs :UInt64, dtNs :UInt64, inputDtNs :UInt64)
      -> (status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Fuse desc.measurements[measurementIndex] at acquisition time under the late-data policy.
  correct @3 (measurementIndex :UInt32, measurement :List(Float64), covariance :List(Float64),
              timeNs :UInt64)
      -> (status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Copy one coherent estimate and its reference time into prepared outputs.
  estimate @4 () -> (state :List(Float64), covariance :List(Float64), timeNs :UInt64,
                    status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Replace the estimate and clear filter history while inactive.
  reset @5 (state :List(Float64), covariance :List(Float64), timeNs :UInt64)
      -> (status :UInt8); # common/constants.capnp: code*
}
