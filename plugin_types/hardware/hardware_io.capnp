@0xd984ee289762313a;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";
using Rt = import "rpp_interfaces/common/execution.capnp";

struct HardwareInterfaceDesc {
  name @0 :Text;
  typeId @1 :UInt64;
}

struct HardwareDesc {
  interfaces @0 :List(HardwareInterfaceDesc);
}

struct HardwareIODesc {
  measurements @0 :D.FieldListDesc; # read() output, independent of state layouts.
  commands @1 :D.FieldListDesc; # write() input and accepted output ordering.
  limits @2 :List(D.LimitDesc); # Each limit selects fields from commands.
}

interface Hardware $Anot.plugin("Hardware") {
  # Declare exported typed RPP endpoint names and type IDs.
  getDesc @0 () -> (desc :HardwareDesc);
}

interface HardwareIO $Anot.plugin("HardwareIO") {
  # Declare measurement/command channels and static command limits.
  getDesc @0 () -> (desc :HardwareIODesc);
  # Read a coherent numeric measurement sample and acquisition time.
  read @1 () -> (values :List(Float64), timeNs :UInt64, status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Write a complete actuator command and report the values actually accepted.
  write @2 (command :List(Float64))
      -> (accepted :List(Float64), status :UInt8) $Rt.realtime; # common/constants.capnp: code*
  # Stop resources selected by indices into desc.commands.fields, across all their modes.
  stop @3 (resources :List(UInt32), mode :UInt8) # hardware/constants.capnp: stop*
      -> (status :UInt8) $Rt.realtime; # common/constants.capnp: code*
}
