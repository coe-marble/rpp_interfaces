@0xa556a9146d36f414;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";
using Rt = import "rpp_interfaces/common/execution.capnp";

struct ReferenceGeneratorDesc @0xf302b193db5e4af7 {
  state @0 :D.FieldListDesc; # Measured inputs; independent of the desired reference layout.
  targetMask @1 :List(Bool); # One flag per reference.fields entry; selected fields are goals.
  reference @2 :D.FieldListDesc; # Produced reference fields for one sample.
  features @3 :UInt8; # reference/constants.capnp: generator* bitmask
  # The manager connects these fields to consumers using configured field routes.
}

struct ReferenceMonitorDesc @0xd127887789b20fd5 {
  state @0 :D.FieldListDesc;
  reference @1 :D.FieldListDesc; # Required reference fields for one sample.
}

interface ReferenceGenerator $Anot.plugin("ReferenceGenerator") {
  # Declare measured inputs, reference outputs and which output fields accept goals.
  getDesc @0 () -> (desc :ReferenceGeneratorDesc);
  # R is the flattened desc.reference width; H is the bound horizonLength.
  # With H = 0, return R current values at timeNs.
  # Otherwise return R * H values in reference-major order: index r * H + h.
  sample @1 (state :List(Float64), timeNs :UInt64)
      -> (reference :List(Float64), finished :Bool, status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Clear the target at the next sample boundary; this does not stop hardware.
  cancel @2 () -> (status :UInt8); # common/constants.capnp: code*
  # Bind on the management thread with sample calls paused. Commit the complete
  # horizon atomically and retain goals. H = 0 means current references with empty offsets.
  # For H > 0, length H shares offsets across all reference scalars; length R * H supplies
  # per-scalar offsets in reference-major order. Each horizon starts at zero and
  # strictly increases. Quaternion scalars share identical offsets.
  # Reject other offsets lengths with an invalid status.
  # The manager prepares the field and time routes for each consumer.
  bind @3 (horizonOffsetsNs :List(UInt64), horizonLength :UInt32 = 0)
      -> (status :UInt8); # common/constants.capnp: code*
  # Stage one desired reference in desc.reference order; targetMask selects goal fields.
  # Empty updateMask updates every target scalar. Otherwise it follows desc.reference.
  # False entries retain their existing point/path/trajectory and sampling origin.
  setPoint @4 (state :List(Float64), updateMask :List(Bool)) -> (status :UInt8); # common/constants.capnp: code*
  # Stage waypoints flattened row-major; each row has desc.reference's stored width.
  setPath @5 (states :List(Float64), updateMask :List(Bool)) -> (status :UInt8); # common/constants.capnp: code*
  # Stage one desired-reference row per relative arrival time, starting at zero.
  setTrajectory @6 (states :List(Float64), timesNs :List(UInt64), updateMask :List(Bool))
      -> (status :UInt8); # common/constants.capnp: code*
}

interface PlanarReferenceGenerator
    extends(ReferenceGenerator)
    $Anot.plugin("PlanarReferenceGenerator") {}
interface SpatialReferenceGenerator
    extends(ReferenceGenerator)
    $Anot.plugin("SpatialReferenceGenerator") {}
interface GenericReferenceGenerator
    extends(ReferenceGenerator)
    $Anot.plugin("GenericReferenceGenerator") {}

interface ReferenceMonitor $Anot.plugin("ReferenceMonitor") {
  # Declare measured-state and reference inputs; connections belong to the manager.
  getDesc @0 () -> (desc :ReferenceMonitorDesc);
  # Read current reference values in desc.reference order.
  # finished means all contributing generators finished.
  check @1 (state :List(Float64), reference :List(Float64),
            finished :Bool, timeNs :UInt64)
      -> (progress :UInt8, # reference/constants.capnp: progress*
          status :UInt8) $Rt.realtime; # common/constants.capnp: code*
}
