#pragma once

#include <rpp_interfaces/field_layout.hpp>
#include <rpp_cpp/list_definitions.hpp>

#include <optional>

namespace rpp_interfaces
{
template<class Values, class Value>
inline void gather_fields(
  const Values & source, const std::vector<std::size_t> & route, rpp::List<Value> & destination,
  std::size_t source_offset = 0, std::size_t destination_offset = 0)
{
  for (std::size_t index = 0; index < route.size(); ++index) {
    destination[destination_offset + index] = static_cast<Value>(source[source_offset + route[index]]);
  }
}

template<class Values, class Value>
inline void scatter_fields(
  const Values & source, const std::vector<std::size_t> & route, rpp::List<Value> & destination,
  std::size_t source_offset = 0, std::size_t destination_offset = 0)
{
  for (std::size_t index = 0; index < route.size(); ++index) {
    destination[destination_offset + route[index]] = static_cast<Value>(source[source_offset + index]);
  }
}

inline std::optional<std::size_t> find_field(const FieldLayout & layout, const std::string & name)
{
  for (std::size_t i = 0; i < layout.fields().size(); ++i) {
    if (layout.fields()[i].name == name) {return i;}
  }
  return std::nullopt;
}

inline bool same_field(
  const FieldLayout & a, std::size_t ai, const FieldLayout & b, std::size_t bi)
{
  const auto & x = a.fields()[ai];
  const auto & y = b.fields()[bi];
  return x.name == y.name && x.kind == y.kind && x.quantity == y.quantity &&
         x.frameId == y.frameId && x.childFrameId == y.childFrameId &&
         x.resource == y.resource && a.ranges()[ai].size == b.ranges()[bi].size;
}

// The available layout may contain extra fields and use a different order.
[[nodiscard]] inline bool validate_required_fields(
  const FieldLayout & available, const FieldLayout & required) noexcept
{
  for (std::size_t field = 0; field < required.fields().size(); ++field) {
    const auto found = find_field(available, required.fields()[field].name);
    if (!found || !same_field(available, *found, required, field)) {return false;}
  }
  return true;
}

inline std::vector<std::size_t> field_route(const FieldLayout & source, const FieldLayout & destination)
{
  std::vector<std::size_t> route;
  route.reserve(destination.size());
  for (std::size_t i = 0; i < destination.fields().size(); ++i) {
    const auto found = find_field(source, destination.fields()[i].name);
    if (!found || !same_field(source, *found, destination, i)) {
      throw std::invalid_argument("Missing or incompatible field: " + destination.fields()[i].name);
    }
    const auto & range = source.ranges()[*found];
    for (std::size_t j = 0; j < range.size; ++j) {route.push_back(range.offset + j);}
  }
  return route;
}

inline FieldLayout merge_fields(const FieldLayout & first, const FieldLayout & second)
{
  auto fields = first.fields();
  for (std::size_t i = 0; i < second.fields().size(); ++i) {
    const auto found = find_field(first, second.fields()[i].name);
    if (found && !same_field(first, *found, second, i)) {
      throw std::invalid_argument("Conflicting field: " + second.fields()[i].name);
    }
    if (!found) {fields.push_back(second.fields()[i]);}
  }
  return FieldLayout(std::move(fields));
}

}  // namespace rpp_interfaces
