#include <gtest/gtest.h>
#include <rpp_interfaces/signal_router.hpp>

#include <limits>
#include <vector>

namespace
{
using namespace rpp_interfaces::contract;
using rpp_interfaces::FieldLayout;
using rpp_interfaces::SignalRouter;

const FieldLayout velocity({
  {"imu.angularVelocity", COORDINATE_EUCLIDEAN, QUANTITY_ANGULAR_VELOCITY, 3,
    "imu", "imu", "vehicle"}});
const FieldLayout depth({
  {"pressure.depth", COORDINATE_EUCLIDEAN, QUANTITY_LINEAR_POSITION, 1,
    "map", "base", "vehicle"}});
}  // namespace

TEST(SignalRouter, CombinesIndependentSourcesAndSelectsFieldsAndRows)
{
  SignalRouter router;
  const auto measurements = router.add_signal("measurements");
  router.add_source(measurements, "imu", velocity);
  router.add_source(measurements, "pressure", depth);
  router.set_rows(measurements, {200, 100});
  const auto imu_output = router.connect_output(measurements, velocity, {100, 200});
  const auto depth_output = router.connect_output(measurements, depth, {100, 200});
  const FieldLayout estimator_fields({depth.fields()[0], velocity.fields()[0]});
  const auto estimator_input = router.connect_input(measurements, estimator_fields, {200});
  router.prepare_buffers();

  rpp::List<double> imu_values, depth_values;
  imu_values.resize(6);
  for (std::size_t index = 0; index < 6; ++index) {imu_values[index] = index + 1.0;}
  depth_values.resize(2);
  depth_values[0] = 7.0;
  depth_values[1] = 8.0;
  ASSERT_TRUE(router.write(imu_output, imu_values));
  ASSERT_TRUE(router.write(depth_output, depth_values));

  const auto estimate_input = router.read(estimator_input);
  ASSERT_EQ(estimate_input.size(), 4u);
  EXPECT_DOUBLE_EQ(estimate_input[0], 8.0);
  EXPECT_DOUBLE_EQ(estimate_input[1], 4.0);
  EXPECT_DOUBLE_EQ(estimate_input[2], 5.0);
  EXPECT_DOUBLE_EQ(estimate_input[3], 6.0);
  EXPECT_EQ(router.required_layout(measurements).fields()[0].name, "pressure.depth");

  // A bad later row must leave every existing measurement unchanged.
  depth_values[0] = 9.0;
  depth_values[1] = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(router.write(depth_output, depth_values));
  const auto pressure_input = router.connect_input(measurements, depth, {100, 200});
  const auto retained = router.read(pressure_input);
  EXPECT_DOUBLE_EQ(retained[0], 7.0);
  EXPECT_DOUBLE_EQ(retained[1], 8.0);
}

TEST(SignalRouter, RejectsConflictingWritersAndMissingOrIncompatibleInputs)
{
  SignalRouter router;
  const auto measurements = router.add_signal("measurements");
  router.add_source(measurements, "first_imu", velocity);
  EXPECT_THROW(router.add_source(measurements, "second_imu", velocity), std::invalid_argument);
  EXPECT_THROW(router.connect_input(measurements, depth), std::invalid_argument);

  auto incompatible = velocity.fields();
  incompatible[0].frameId = "other_imu";
  EXPECT_THROW(router.connect_input(measurements, FieldLayout(incompatible)), std::invalid_argument);
  EXPECT_THROW(router.connect_input(measurements, velocity, {1}), std::invalid_argument);

  const auto input = router.connect_input(measurements, velocity);
  router.prepare_buffers();
  router.signal_values(measurements)[0] = 3.0;
  EXPECT_DOUBLE_EQ(router.read(input)[0], 3.0);
}

TEST(SignalRouter, CopiesExternalRowsAndKeepsSignalIdsAcrossReconfiguration)
{
  SignalRouter router;
  const auto measurements = router.add_signal("measurements");
  const FieldLayout all_fields({depth.fields()[0], velocity.fields()[0]});
  router.set_layout(measurements, all_fields);
  const auto input = router.connect_input(measurements, velocity);
  router.prepare_buffers();

  rpp::List<double> external_rows, selected;
  external_rows.resize(8);
  for (std::size_t index = 0; index < 8; ++index) {external_rows[index] = index;}
  selected.resize(3);
  router.copy_to(input, external_rows, selected, 4);
  EXPECT_DOUBLE_EQ(selected[0], 5.0);
  EXPECT_DOUBLE_EQ(selected[1], 6.0);
  EXPECT_DOUBLE_EQ(selected[2], 7.0);

  router.reset();
  router.set_layout(measurements, velocity);
  const auto new_input = router.connect_input(measurements, velocity);
  router.prepare_buffers();
  router.signal_values(measurements)[0] = 12.0;
  EXPECT_DOUBLE_EQ(router.read(new_input)[0], 12.0);
}
