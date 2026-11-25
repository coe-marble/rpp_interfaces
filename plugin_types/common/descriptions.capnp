@0xfc4f407c833948ba;

# Descriptions are implemented by plugins and remain fixed while active.
# Callers validate routes and prepare numeric mappings during RPP configuration.
# Orders and indices are local to each instance, including instances in lists.
# Routes preserve kind, quantity, frames and resource meaning; conflicting writers fail.

using C = import "rpp_interfaces/common/constants.capnp";

struct FieldDesc {
  name @0 :Text;
  kind @1 :UInt8 = C.coordinateInvalid; # common/constants.capnp: coordinate*
  quantity @2 :UInt8 = C.quantityInvalid; # common/constants.capnp: quantity*
  size @3 :UInt32; # Euclidean/angle width; zero infers one angle or one quaternion.
  frameId @4 :Text;
  childFrameId @5 :Text;
  resource @6 :Text;
}

struct FieldListDesc {
  # Ordered semantic fields; stored/tangent offsets and totals are calculated.
  # Subsets select complete fields. Independently routed scalars need separate fields.
  fields @0 :List(FieldDesc); # Unique names within this layout; empty permits an unused input/output.
}

struct StateWithDescription {
  state @0 :List(Float64);
  desc @1 :FieldListDesc;
}

struct StateWithDescriptionStamped {
  state @0 :StateWithDescription;
  # Unsigned nanoseconds in the caller's clock domain, as for all interface timestamps.
  stampNs @1 :UInt64;
}

struct LimitDesc {
  fields @0 :FieldListDesc; # Subset of the parent descriptor's limited fields.
  # Each list is empty (unbounded) or matches the flattened fields width.
  lower @1 :List(Float64);
  upper @2 :List(Float64);
  maxRate @3 :List(Float64); # Maximum absolute rate in quantity units per second; nonnegative.
  # Lower -inf, upper +inf and rate +inf disable individual bounds; NaN is invalid.
  # Multiple limits on the same field intersect; contradictory limits fail configuration.
}
