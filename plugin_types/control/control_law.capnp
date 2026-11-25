@0xa3ef832f3aebfdaa;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";
using Rt = import "rpp_interfaces/common/execution.capnp";

# Optional cascade selection values carried in the flat reference array.
const referenceDisabled :UInt8 = 0;
const referenceExternal :UInt8 = 1;
const referenceInternal :UInt8 = 2;
const referenceDontCare :UInt8 = 3;

struct ControllerHandoverState {
  state @0 :D.StateWithDescription;
  previousControl @1 :D.StateWithDescription;
  auxiliaryState @2 :D.StateWithDescription; # Controller-owned error history or solver warm start.
}

struct AllocatorHandoverState {
  state @0 :D.StateWithDescription;
  previousControl @1 :D.StateWithDescription;
  auxiliaryState @2 :D.StateWithDescription;
}

# Each plugin declares its own flat layouts, fixed while configured.
# The manager connects compatible fields and prepares their numeric routes.
struct ControlLawDesc @0xdbf16d008f914139 {
  state @0 :D.FieldListDesc; # Measured inputs; shared read access.
  reference @1 :D.FieldListDesc; # Desired inputs; one sample's layout.
  control @2 :D.FieldListDesc; # Produced controls; exclusive destination claims.
  horizonOffsetsNs @3 :List(UInt64);
  horizonLength @4 :UInt32 = 0;
  # R is the flattened reference width; H is horizonLength.
  # H = 0 means current references only, with empty offsets.
  # For H > 0:
  # Offsets length H shares one horizon across all reference scalars.
  # Offsets length R * H gives each scalar its own horizon, at index r * H + h.
  # Other offsets lengths are configuration errors.
  # Every explicit horizon starts at zero and strictly increases.
  # Quaternion scalars share identical offsets to preserve a complete orientation sample.
  # The manager exposes reference fields under this controller's instance name.
  # Configuration selects their sources. Activation claims the control destinations;
  # reference expiry preserves those claims until deactivation.
}

struct ActuationMapperDesc {
  state @0 :D.FieldListDesc;
  control @1 :D.FieldListDesc; # Requested input and realized/realizedEstimate output ordering.
  actuators @2 :D.FieldListDesc; # commandMap output and feedbackMap input ordering.
  limits @3 :List(D.LimitDesc); # Each limit selects fields from actuators.
}

interface ControlLaw $Anot.plugin("ControlLaw")
    $Anot.lifecycleState("ControllerHandoverState") {
  # Declare the state, reference, and control fields of this configured instance.
  getDesc @0 () -> (desc :ControlLawDesc);
  # State and control follow their declared flattened field order.
  # With horizonLength = 0, reference contains R current values at timeNs.
  # Otherwise it contains R * H values in reference-major order: index r * H + h.
  # Each value uses offset h for a shared horizon, or offset r * H + h otherwise.
  # previousControl follows control order and contains the previous applied control
  # estimate, including actuator limits, or the selected reference at a cascade boundary.
  # enabledAxes is empty (all enabled) or has one flag per control scalar.
  # inputValid is empty (all valid) or has one flag per flattened reference value.
  # Invalid inputs have expired or are unavailable; their numeric values remain present.
  # The plugin handles unavailable references with safe control values or a failure status.
  # Disabled axes output zero and clear their controller history; inputs stay present.
  compute @1 (state :List(Float64), reference :List(Float64),
              enabledAxes :List(Bool),
              inputValid :List(Bool),
              previousControl :List(Float64), previousControlValid :Bool,
              timeNs :UInt64, dtNs :UInt64)
      -> (control :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Read controller-owned history or solver warm start with numeric updates paused.
  getAuxiliaryState @2 () -> (state :D.StateWithDescriptionStamped);
}

interface ActuationMapper $Anot.plugin("ActuationMapper")
    $Anot.lifecycleState("AllocatorHandoverState") {
  # Declare state, control/actuator spaces and static actuator limits.
  getDesc @0 () -> (desc :ActuationMapperDesc);
  # Map requested control without writing hardware or committing history.
  commandMap @1 (state :List(Float64), control :List(Float64), dtNs :UInt64)
      -> (actuators :List(Float64), realizedEstimate :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Map state and actuator feedback into realized controller-space input. The caller can
  # supply limited actuator commands or actual values reported by hardware.
  feedbackMap @2 (state :List(Float64), actuators :List(Float64))
      -> (realized :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Read with numeric updates paused. Stateless mappers return empty values and description.
  getAuxiliaryState @3 () -> (state :D.StateWithDescriptionStamped);
}
