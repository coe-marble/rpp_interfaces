@0xb9d6a02f7c31e845;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";
using Rt = import "rpp_interfaces/common/execution.capnp";

interface StateProvider $Anot.plugin("StateProvider") {
  # Fixed layout of the buffered state. Set/get use its flattened field order.
  getDesc @0 () -> (desc :D.FieldListDesc);
  # Read one coherent sample. Return codeNotReady before the first accepted write.
  # Retaining the returned view must not allow a later write to change its values.
  getState @1 () -> (state :List(Float64), timeNs :UInt64, status :UInt8) $Rt.realtime;
  # Replace the complete sample after validating its width and coordinate values.
  # Rejection leaves the previous sample unchanged. timeNs is acquisition time,
  # in the consumer's clock domain; the consumer owns freshness policy.
  setState @2 (state :List(Float64), timeNs :UInt64) -> (status :UInt8) $Rt.realtime;
  # The owner serializes reads/writes and lifecycle operations. ROS ingress stays
  # outside this interface; numeric calls do not allocate or access ROS.
}
