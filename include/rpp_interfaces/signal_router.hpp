#pragma once

#include <rpp_cpp/list_definitions.hpp>
#include <rpp_interfaces/field_routing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rpp_interfaces
{
// Describe sources before connecting their readers and writers. Configuration
// may allocate; call prepare_buffers() before reading or writing prepared routes.
// Those operations only copy numeric values.
// The owner serializes changes to the router with readers of its buffers.
class SignalRouter
{
public:
  using SignalId = std::size_t;
  using RouteId = std::size_t;
  static constexpr std::size_t max_values = 1024 * 1024;

  SignalId add_signal(std::string name)
  {
    for (const auto & signal : signals_) {
      if (signal.name == name) {throw std::invalid_argument("Repeated signal name: " + name);}
    }
    signals_.emplace_back(std::move(name));
    return signals_.size() - 1;
  }

  // Signal IDs survive a reset. Existing routes and their buffers do not.
  void reset()
  {
    routes_.clear();
    for (auto & signal : signals_) {
      signal.fields = {};
      signal.required_fields = {};
      signal.sources.clear();
      signal.rows = {0};
      signal.connected = false;
    }
  }

  void set_layout(SignalId id, FieldLayout fields)
  {
    auto & signal = editable_signal(id);
    if (!signal.sources.empty()) {throw std::logic_error("Declare the signal layout before its sources");}
    signal.fields = std::move(fields);
  }

  void add_source(SignalId id, const std::string & source, const FieldLayout & fields)
  {
    auto & signal = editable_signal(id);
    for (const auto & field : fields.fields()) {
      const auto previous = signal.sources.find(field.name);
      if (previous != signal.sources.end()) {
        throw std::invalid_argument("Signal '" + signal.name + "' field '" + field.name +
                "' has conflicting writers '" + previous->second + "' and '" + source + "'");
      }
    }
    auto combined = merge_fields(signal.fields, fields);
    for (const auto & field : fields.fields()) {signal.sources.emplace(field.name, source);}
    signal.fields = std::move(combined);
  }

  // Row labels are opaque to the router. A caller may use sample offsets,
  // timestamps, or ordinary row numbers; no clock or sampling policy is assumed.
  void set_rows(SignalId id, std::vector<std::uint64_t> rows)
  {
    if (rows.empty()) {throw std::invalid_argument("A signal needs at least one row");}
    auto sorted = rows;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
      throw std::invalid_argument("Signal row labels must be unique");
    }
    editable_signal(id).rows = std::move(rows);
  }

  RouteId connect_input(
    SignalId id, const FieldLayout & fields,
    const std::vector<std::uint64_t> & rows = {0})
  {
    auto required = merge_fields(signals_.at(id).required_fields, fields);
    const auto route = connect(id, fields, rows);
    signals_[id].required_fields = std::move(required);
    return route;
  }

  RouteId connect_output(
    SignalId id, const FieldLayout & fields,
    const std::vector<std::uint64_t> & rows = {0})
  {return connect(id, fields, rows);}

  void prepare_buffers()
  {
    for (auto & signal : signals_) {
      check_buffer_size(signal.fields.size(), signal.rows.size());
      rpp::List<double> prepared;
      prepared.resize(signal.fields.size() * signal.rows.size());
      signal.values = std::move(prepared);
    }
  }

  const FieldLayout & layout(SignalId id) const {return signals_.at(id).fields;}
  const FieldLayout & required_layout(SignalId id) const {return signals_.at(id).required_fields;}
  rpp::List<double> & signal_values(SignalId id) {return signals_[id].values;}
  const rpp::List<double> & signal_values(SignalId id) const {return signals_[id].values;}
  const std::vector<std::size_t> & indices(RouteId id) const {return routes_[id].indices;}
  const rpp::List<double> & route_values(RouteId id) const {return routes_[id].values;}

  rpp::ListConst<double> read(RouteId id)
  {
    auto & route = routes_[id];
    copy_to(id, signals_[route.signal].values, route.values);
    return route.values;
  }

  template<class Values>
  rpp::ListConst<double> read_from(RouteId id, const Values & values)
  {
    auto & route = routes_[id];
    copy_to(id, values, route.values);
    return route.values;
  }

  // External buffers use the prepared signal layout and route width.
  template<class Values, class Value>
  void copy_to(
    RouteId id, const Values & source, rpp::List<Value> & destination,
    std::size_t source_offset = 0, std::size_t destination_offset = 0) const
  {
    gather_fields(source, routes_[id].indices, destination, source_offset, destination_offset);
  }

  // Reject an invalid result before copying any of it into the shared signal.
  [[nodiscard]] bool write(RouteId id, rpp::ListConst<double> values)
  {
    const auto & route = routes_[id];
    if (values.size() != route.indices.size()) {return false;}
    for (std::size_t row = 0; row < route.row_count; ++row) {
      if (!route.fields.valid_row(values, row * route.fields.size())) {return false;}
    }
    scatter_fields(values, route.indices, signals_[route.signal].values);
    return true;
  }

private:
  struct Signal
  {
    explicit Signal(std::string signal_name) : name(std::move(signal_name)) {}
    std::string name;
    FieldLayout fields, required_fields;
    std::unordered_map<std::string, std::string> sources;
    std::vector<std::uint64_t> rows{0};
    rpp::List<double> values;
    bool connected = false;
  };

  struct Route
  {
    SignalId signal;
    FieldLayout fields;
    std::size_t row_count;
    std::vector<std::size_t> indices;
    rpp::List<double> values;
  };

  std::vector<Signal> signals_;
  std::vector<Route> routes_;

  Signal & editable_signal(SignalId id)
  {
    auto & signal = signals_.at(id);
    if (signal.connected) {
      throw std::logic_error("Describe signal '" + signal.name + "' before connecting its routes");
    }
    return signal;
  }

  static void check_buffer_size(std::size_t width, std::size_t rows)
  {
    if (rows > max_values || (rows != 0 && width > max_values / rows)) {
      throw std::invalid_argument("Signal buffer exceeds capacity");
    }
  }

  RouteId connect(
    SignalId id, const FieldLayout & fields, const std::vector<std::uint64_t> & rows)
  {
    auto & signal = signals_.at(id);
    if (rows.empty()) {throw std::invalid_argument("A route needs at least one row");}
    check_buffer_size(fields.size(), rows.size());
    check_buffer_size(signal.fields.size(), signal.rows.size());
    const auto selected_fields = field_route(signal.fields, fields);
    Route prepared{id, fields, rows.size(), {}, {}};
    prepared.indices.reserve(selected_fields.size() * rows.size());
    for (const auto label : rows) {
      const auto source_row = std::find(signal.rows.begin(), signal.rows.end(), label);
      if (source_row == signal.rows.end()) {
        throw std::invalid_argument("Requested row is absent from signal '" + signal.name + "'");
      }
      const auto row_index = static_cast<std::size_t>(source_row - signal.rows.begin());
      for (const auto field_index : selected_fields) {
        prepared.indices.push_back(row_index * signal.fields.size() + field_index);
      }
    }
    prepared.values.resize(prepared.indices.size());
    routes_.push_back(std::move(prepared));
    signal.connected = true;
    return routes_.size() - 1;
  }
};
}  // namespace rpp_interfaces
