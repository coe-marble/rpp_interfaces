@0xb9d6a02f7c31e845;

using Anot = import "rpp_common/anot.capnp";
using D = import "rpp_interfaces/common/descriptions.capnp";

interface StateDescriptionProvider $Anot.plugin("StateDescriptionProvider") {
  # Declare the state layout; this component stores no runtime state values.
  getDesc @0 () -> (desc :D.FieldListDesc);
}
