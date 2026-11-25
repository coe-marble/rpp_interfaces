#pragma once

#include <rpp_interfaces/field_layout.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace rpp_interfaces
{
// Prepare reference-major offsets during configuration. A current-only reference
// has one stored sample per scalar even though its declared horizon length is zero.
class ReferenceHorizon
{
public:
  static constexpr std::size_t max_samples = 4096;
  static constexpr std::size_t max_values = 1024 * 1024;

  ReferenceHorizon() = default;

  template<class Offsets>
  ReferenceHorizon(
    const FieldLayout & reference, const Offsets & offsets, std::uint32_t horizon_length)
  : horizon_length_(horizon_length), sample_count_(horizon_length == 0 ? 1 : horizon_length)
  {
    if (sample_count_ > max_samples || reference.size() > max_values / sample_count_) {
      throw std::invalid_argument("Reference horizon exceeds capacity");
    }
    const auto value_count = reference.size() * sample_count_;
    if (horizon_length == 0) {
      if (!offsets.empty()) {
        throw std::invalid_argument("A current-only reference requires empty horizon offsets");
      }
      offsets_.assign(value_count, 0);
      return;
    }
    if (reference.size() == 0 ||
      (offsets.size() != sample_count_ && offsets.size() != value_count))
    {
      throw std::invalid_argument("Horizon offsets must have H or R * H entries");
    }

    offsets_.resize(value_count);
    const bool shared = offsets.size() == sample_count_;
    for (std::size_t scalar = 0; scalar < reference.size(); ++scalar) {
      const auto start = scalar * sample_count_;
      std::uint64_t previous = 0;
      for (std::size_t sample = 0; sample < sample_count_; ++sample) {
        const auto offset = offsets[shared ? sample : start + sample];
        if ((sample == 0 && offset != 0) || (sample != 0 && offset <= previous)) {
          throw std::invalid_argument("Each reference horizon must start at zero and strictly increase");
        }
        offsets_[start + sample] = offset;
        previous = offset;
      }
      maximum_offset_ = std::max(maximum_offset_, previous);
    }

    // All four quaternion coefficients must describe the same sampling instant.
    for (std::size_t field = 0; field < reference.fields().size(); ++field) {
      if (reference.fields()[field].kind != contract::COORDINATE_QUATERNION) {continue;}
      const auto & range = reference.ranges()[field];
      for (std::size_t scalar = 1; scalar < range.size; ++scalar) {
        for (std::size_t sample = 0; sample < sample_count_; ++sample) {
          if (offset(range.offset + scalar, sample) != offset(range.offset, sample)) {
            throw std::invalid_argument("Quaternion scalars require identical horizon offsets");
          }
        }
      }
    }
  }

  std::uint32_t horizon_length() const noexcept {return horizon_length_;}
  std::size_t sample_count() const noexcept {return sample_count_;}
  std::size_t value_count() const noexcept {return offsets_.size();}
  std::uint64_t maximum_offset() const noexcept {return maximum_offset_;}
  std::uint64_t offset(std::size_t scalar, std::size_t sample) const noexcept
  {return offsets_[scalar * sample_count_ + sample];}

  template<class Values>
  bool valid_values(const FieldLayout & reference, const Values & values) const
  {
    if (values.size() != value_count() || reference.size() * sample_count_ != value_count()) {
      return false;
    }
    for (std::size_t index = 0; index < values.size(); ++index) {
      if (!std::isfinite(static_cast<double>(values[index]))) {return false;}
    }
    for (std::size_t field = 0; field < reference.fields().size(); ++field) {
      if (reference.fields()[field].kind != contract::COORDINATE_QUATERNION) {continue;}
      const auto first = reference.ranges()[field].offset;
      for (std::size_t sample = 0; sample < sample_count_; ++sample) {
        double norm = 0.0;
        for (std::size_t scalar = 0; scalar < 4; ++scalar) {
          const double value = values[(first + scalar) * sample_count_ + sample];
          norm += value * value;
        }
        if (!std::isfinite(norm) || std::abs(norm - 1.0) > 1e-3) {return false;}
      }
    }
    return true;
  }

private:
  std::uint32_t horizon_length_ = 0;
  std::size_t sample_count_ = 1;
  std::uint64_t maximum_offset_ = 0;
  std::vector<std::uint64_t> offsets_;
};
}  // namespace rpp_interfaces
