#pragma once

#include <rpp_schema/rpp_interfaces/FieldListDesc.hpp>
#include <capnp_gen/rpp_interfaces/common/constants.capnp.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rpp_interfaces
{
namespace contract = ::schema::rpp_interfaces;
using Field = rpp_schema::rpp_interfaces::FieldDesc_Native;
using FieldList = rpp_schema::rpp_interfaces::FieldListDesc;

struct FieldRange
{
  std::size_t offset;
  std::size_t size;
  std::size_t tangent_offset;
  std::size_t tangent_size;
};

class FieldLayout
{
public:
  static constexpr std::size_t max_width = 4096;

  FieldLayout() = default;
  explicit FieldLayout(std::vector<Field> fields) : fields_(std::move(fields))
  {
    std::unordered_set<std::string> names;
    for (const auto & field : fields_) {
      if (field.name.empty() || !names.insert(field.name).second || field.quantity == contract::QUANTITY_INVALID) {
        throw std::invalid_argument("Fields require unique nonempty names and a quantity");
      }
      std::size_t width = field.size;
      std::size_t tangent = width;
      switch (field.kind) {
        case contract::COORDINATE_EUCLIDEAN:
          if (width == 0) {throw std::invalid_argument("Euclidean field size must be positive");}
          break;
        case contract::COORDINATE_CONTINUOUS_ANGLE:
          if (field.quantity != contract::QUANTITY_ANGULAR_POSITION) {
            throw std::invalid_argument("Continuous angles require angular position quantities");
          }
          width = width == 0 ? 1 : width;
          tangent = width;
          break;
        case contract::COORDINATE_QUATERNION:
          if (field.quantity != contract::QUANTITY_ORIENTATION) {
            throw std::invalid_argument("Quaternions require orientation quantities");
          }
          if (width != 0 && width != 4) {throw std::invalid_argument("Quaternion size must be 0 or 4");}
          width = 4;
          tangent = 3;
          break;
        default:
          throw std::invalid_argument("Unsupported coordinate kind");
      }
      if (width > max_width - size_) {throw std::invalid_argument("State layout is too large");}
      ranges_.push_back({size_, width, tangent_size_, tangent});
      size_ += width;
      tangent_size_ += tangent;
    }
  }

  const std::vector<Field> & fields() const noexcept {return fields_;}
  const std::vector<FieldRange> & ranges() const noexcept {return ranges_;}
  std::size_t size() const noexcept {return size_;}
  std::size_t tangent_size() const noexcept {return tangent_size_;}

  void write(FieldList & desc) const
  {
    auto destination = desc.fields();
    destination.resize(fields_.size());
    for (std::size_t i = 0; i < fields_.size(); ++i) {
      auto field = destination[i];
      const auto & source = fields_[i];
      field.name() = source.name;
      field.kind() = source.kind;
      field.quantity() = source.quantity;
      field.size() = source.size;
      field.frameId() = source.frameId;
      field.childFrameId() = source.childFrameId;
      field.resource() = source.resource;
    }
  }

  FieldList::Const description() const
  {
    FieldList desc;
    write(desc);
    return FieldList::Const(std::move(desc));
  }

  template<typename Values>
  bool valid_row(const Values & values, std::size_t start = 0) const
  {return valid_update(values, std::array<bool, 0>{}, start);}

  // Unselected values are not applied and need not contain valid coordinates.
  // Quaternion entries form one coordinate and therefore share an update flag.
  template<typename Values, typename Mask>
  bool valid_update(const Values & values, const Mask & mask, std::size_t start = 0) const
  {
    if (start > values.size() || size_ > values.size() - start ||
      (!mask.empty() && mask.size() != size_)) {return false;}
    for (std::size_t i = 0; i < size_; ++i) {
      if ((mask.empty() || mask[i]) && !std::isfinite(static_cast<double>(values[start + i]))) {return false;}
    }
    for (std::size_t i = 0; i < fields_.size(); ++i) {
      if (fields_[i].kind != contract::COORDINATE_QUATERNION) {continue;}
      const auto offset = start + ranges_[i].offset;
      const bool selected = mask.empty() || mask[ranges_[i].offset];
      double norm = 0.0;
      for (std::size_t j = 0; j < 4; ++j) {
        if (!mask.empty() && static_cast<bool>(mask[ranges_[i].offset + j]) != selected) {return false;}
        if (!selected) {continue;}
        const double value = values[offset + j];
        norm += value * value;
      }
      if (selected && (!std::isfinite(norm) || std::abs(norm - 1.0) > 1e-3)) {return false;}
    }
    return true;
  }

private:
  std::vector<Field> fields_;
  std::vector<FieldRange> ranges_;
  std::size_t size_ = 0;
  std::size_t tangent_size_ = 0;
};
}  // namespace rpp_interfaces
