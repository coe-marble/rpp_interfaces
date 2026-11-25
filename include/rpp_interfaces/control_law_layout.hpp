#pragma once

#include <rpp_interfaces/reference_horizon.hpp>
#include <rpp_schema/rpp_interfaces/ControlLawDesc.hpp>

#include <cstddef>
#include <stdexcept>

namespace rpp_interfaces
{
// Compile each plugin's flat descriptions and horizon before numeric execution.
class ControlLawLayout
{
public:
  ControlLawLayout() = default;

  explicit ControlLawLayout(const rpp_schema::rpp_interfaces::ControlLawDesc_Native & description)
  : state_(description.state.fields),
    reference_(description.reference.fields),
    control_(description.control.fields),
    horizon_(reference_, description.horizonOffsetsNs, description.horizonLength)
  {
    if (control_.size() == 0) {
      throw std::invalid_argument("A controller needs control output fields");
    }
  }

  const FieldLayout & state() const noexcept {return state_;}
  const FieldLayout & reference() const noexcept {return reference_;}
  const FieldLayout & control() const noexcept {return control_;}
  const ReferenceHorizon & horizon() const noexcept {return horizon_;}
  std::size_t reference_size() const noexcept {return horizon_.value_count();}

private:
  FieldLayout state_, reference_, control_;
  ReferenceHorizon horizon_;
};
}  // namespace rpp_interfaces
