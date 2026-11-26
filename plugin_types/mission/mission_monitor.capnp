@0xd142cbdc09ea673d;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";
using Rt = import "rpp_interfaces/common/execution.capnp";

struct MissionMonitorDesc @0xdc9e1f4dc96aa5fe {
  state @0 :D.FieldListDesc; # Required observations: motion, battery, temperature, etc.
  reference @1 :D.FieldListDesc; # Required current reference fields; may be empty.
  goal @2 :D.FieldListDesc; # Final-target inputs; empty means this is a guard-only monitor.
}

interface MissionMonitor $Anot.plugin("MissionMonitor") {
  # Layouts remain fixed while configured; the owner prepares flat numeric routes.
  getDesc @0 () -> (desc :MissionMonitorDesc);
  # Clear monitoring history when a new mission starts.
  # Replanning within the current mission does not reset its watchdogs.
  resetHistory @1 () -> (status :UInt8); # common/constants.capnp: code*
  # Observe state and current references in desc.state and desc.reference order.
  # timeNs uses the caller's clock domain; the first valid call sets the baseline.
  # Policies may check progress, tracking error, timeouts, or other mission limits.
  # With codeOk, ok=false is a valid decision that the mission must stop.
  # Continue only with codeOk and ok=true; an evaluation failure also stops it.
  # Every monitor participates in isOk, including the selected goal monitor.
  # The owner applies the stop policy; any false/error vetoes mission continuation.
  # message explains the decision/error and may be empty. Text handling belongs
  # to the supervision thread, outside the realtime numeric worker.
  isOk @2 (state :List(Float64), reference :List(Float64), timeNs :UInt64)
      -> (ok :Bool, status :UInt8, message :Text); # common/constants.capnp: code*
  # The task selects exactly one monitor with nonempty desc.goal for completion.
  # Call only that instance, using the same measured snapshot and the final target
  # in desc.goal order. All isOk checks must permit continuation before success.
  # A changed goal clears target-specific history without resetting mission watchdogs.
  # Guard-only instances return codeUnsupported if asked for goal completion.
  isGoalReached @3 (state :List(Float64), goal :List(Float64), timeNs :UInt64)
      -> (reached :Bool, status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # The owner serializes numeric calls/resetHistory with lifecycle operations.
  # isGoalReached does not allocate. Neither method logs, accesses ROS, or commands hardware.
}
