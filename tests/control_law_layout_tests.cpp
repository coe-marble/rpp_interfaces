#include <gtest/gtest.h>
#include <limits>
#include <rpp_interfaces/control_law_layout.hpp>

namespace
{
rpp_schema::rpp_interfaces::ControlLawDesc_Native make_description()
{
  using namespace rpp_interfaces::contract;
  rpp_schema::rpp_interfaces::ControlLawDesc_Native description;
  description.state.fields = {
    {"base.position", COORDINATE_EUCLIDEAN, QUANTITY_LINEAR_POSITION, 2,
      "map", "base_link", "base"}};
  description.reference = description.state;
  description.control.fields = {
    {"base.force", COORDINATE_EUCLIDEAN, QUANTITY_FORCE, 2,
      "base_link", "base_link", "base"}};
  description.horizonLength = 0;
  return description;
}
}  // namespace

TEST(ControlLawLayout, CurrentOnlyReferencesHaveOneValuePerScalar)
{
  const auto description = make_description();
  const rpp_interfaces::ControlLawLayout layout(description);
  EXPECT_EQ(layout.state().size(), 2u);
  EXPECT_EQ(layout.reference().size(), 2u);
  EXPECT_EQ(layout.control().size(), 2u);
  EXPECT_EQ(layout.reference_size(), 2u);
  EXPECT_EQ(layout.horizon().horizon_length(), 0u);
  EXPECT_EQ(layout.horizon().sample_count(), 1u);
  EXPECT_EQ(layout.horizon().offset(0, 0), 0u);
  EXPECT_EQ(layout.horizon().offset(1, 0), 0u);
}

TEST(ControlLawLayout, SharedHorizonAppliesToEveryReferenceScalar)
{
  auto description = make_description();
  description.horizonLength = 3;
  description.horizonOffsetsNs = {0, 100000000, 200000000};
  const rpp_interfaces::ControlLawLayout layout(description);
  EXPECT_EQ(layout.reference_size(), 6u);
  EXPECT_EQ(layout.horizon().horizon_length(), 3u);
  for (std::size_t scalar = 0; scalar < 2; ++scalar) {
    EXPECT_EQ(layout.horizon().offset(scalar, 0), 0u);
    EXPECT_EQ(layout.horizon().offset(scalar, 1), 100000000u);
    EXPECT_EQ(layout.horizon().offset(scalar, 2), 200000000u);
  }
}

TEST(ControlLawLayout, ExplicitSingleSampleAndStateOnlyControllersAreSupported)
{
  auto description = make_description();
  description.horizonLength = 1;
  description.horizonOffsetsNs = {0};
  const rpp_interfaces::ControlLawLayout single_sample(description);
  EXPECT_EQ(single_sample.reference_size(), 2u);
  description.horizonLength = 0;
  description.horizonOffsetsNs.clear();
  description.reference.fields.clear();
  const rpp_interfaces::ControlLawLayout state_only(description);
  EXPECT_EQ(state_only.reference_size(), 0u);
  EXPECT_EQ(state_only.control().size(), 2u);
}

TEST(ControlLawLayout, PerScalarHorizonsUseReferenceMajorOrder)
{
  auto description = make_description();
  description.horizonLength = 3;
  description.horizonOffsetsNs = {0, 100, 400, 0, 50, 200};
  const rpp_interfaces::ControlLawLayout layout(description);
  EXPECT_EQ(layout.reference_size(), 6u);
  EXPECT_EQ(layout.horizon().offset(0, 1), 100u);
  EXPECT_EQ(layout.horizon().offset(0, 2), 400u);
  EXPECT_EQ(layout.horizon().offset(1, 1), 50u);
  EXPECT_EQ(layout.horizon().offset(1, 2), 200u);
  EXPECT_EQ(layout.horizon().maximum_offset(), 400u);
}

TEST(ControlLawLayout, RejectsMalformedHorizonShapesAndTimes)
{
  auto description = make_description();
  description.horizonOffsetsNs = {0};
  EXPECT_THROW((rpp_interfaces::ControlLawLayout(description)), std::invalid_argument);
  description.horizonLength = 3;
  for (const auto & offsets : std::vector<std::vector<std::uint64_t>>{
      {}, {0, 10}, {1, 10, 20}, {0, 0, 10}, {0, 20, 10},
      {0, 10, 20, 1, 10, 20}, {0, 10, 20, 0, 10, 10}})
  {
    description.horizonOffsetsNs = offsets;
    EXPECT_THROW((rpp_interfaces::ControlLawLayout(description)), std::invalid_argument);
  }
}

TEST(ControlLawLayout, QuaternionScalarsRequireMatchingHorizons)
{
  using namespace rpp_interfaces::contract;
  auto description = make_description();
  description.reference.fields = {
    {"base.orientation", COORDINATE_QUATERNION, QUANTITY_ORIENTATION, 4,
      "map", "base_link", "base"}};
  description.horizonLength = 2;
  description.horizonOffsetsNs = {0, 10, 0, 10, 0, 10, 0, 10};
  const rpp_interfaces::ControlLawLayout layout(description);
  EXPECT_EQ(layout.reference_size(), 8u);
  description.horizonOffsetsNs[5] = 20;
  EXPECT_THROW((rpp_interfaces::ControlLawLayout(description)), std::invalid_argument);
}

TEST(ControlLawLayout, ValidatesQuaternionSamplesInReferenceMajorOrder)
{
  using namespace rpp_interfaces::contract;
  auto description = make_description();
  description.reference.fields = {
    {"base.orientation", COORDINATE_QUATERNION, QUANTITY_ORIENTATION, 4,
      "map", "base_link", "base"}};
  description.horizonLength = 2;
  description.horizonOffsetsNs = {0, 10};
  const rpp_interfaces::ControlLawLayout layout(description);
  std::vector<double> values{0, 0, 0, 0, 0, 0, 1, 1};
  EXPECT_TRUE(layout.horizon().valid_values(layout.reference(), values));
  values[7] = 0;
  EXPECT_FALSE(layout.horizon().valid_values(layout.reference(), values));
}

TEST(ControlLawLayout, RejectsDuplicateOrMissingControlFields)
{
  auto description = make_description();
  description.control.fields.push_back(description.control.fields.front());
  EXPECT_THROW((rpp_interfaces::ControlLawLayout(description)), std::invalid_argument);
  description.control.fields.clear();
  EXPECT_THROW((rpp_interfaces::ControlLawLayout(description)), std::invalid_argument);
}

TEST(ControlLawLayout, BoundsHorizonStorageBeforeAllocating)
{
  auto description = make_description();
  description.horizonLength = std::numeric_limits<std::uint32_t>::max();
  EXPECT_THROW((rpp_interfaces::ControlLawLayout(description)), std::invalid_argument);
  description.reference.fields.front().size = static_cast<std::uint32_t>(rpp_interfaces::FieldLayout::max_width);
  description.horizonLength = static_cast<std::uint32_t>(rpp_interfaces::ReferenceHorizon::max_samples);
  description.horizonOffsetsNs.resize(description.horizonLength);
  for (std::size_t sample = 0; sample < description.horizonLength; ++sample) {
    description.horizonOffsetsNs[sample] = sample;
  }
  EXPECT_THROW((rpp_interfaces::ControlLawLayout(description)), std::invalid_argument);
}

TEST(ControlLawLayout, RetainsUnsignedNanosecondRange)
{
  auto description = make_description();
  description.horizonLength = 2;
  description.horizonOffsetsNs = {0, std::numeric_limits<std::uint64_t>::max()};
  const rpp_interfaces::ControlLawLayout layout(description);
  EXPECT_EQ(layout.horizon().maximum_offset(), std::numeric_limits<std::uint64_t>::max());
}

TEST(ControlFieldLayout, MaskedUpdatesIgnoreUnusedValuesAndKeepQuaternionsAtomic)
{
  using namespace rpp_interfaces::contract;
  const rpp_interfaces::FieldLayout layout({
    {"position", COORDINATE_EUCLIDEAN, QUANTITY_LINEAR_POSITION, 1, "map", "body", "base"},
    {"orientation", COORDINATE_QUATERNION, QUANTITY_ORIENTATION, 4, "map", "body", "base"}});
  std::vector<double> values(5, std::numeric_limits<double>::quiet_NaN());
  values[0] = 1.0;
  std::vector<bool> mask{true, false, false, false, false};
  EXPECT_TRUE(layout.valid_update(values, mask));
  EXPECT_FALSE(layout.valid_row(values));
  mask[1] = true;
  EXPECT_FALSE(layout.valid_update(values, mask));
  values = {1.0, 0.0, 0.0, 0.0, 1.0};
  mask.assign(5, true);
  EXPECT_TRUE(layout.valid_update(values, mask));
  EXPECT_TRUE(layout.valid_row(values));
}
