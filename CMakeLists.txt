cmake_minimum_required(VERSION 3.15)
project(rpp_interfaces)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rpp_cpp REQUIRED)

add_library(rpp_interfaces INTERFACE)
add_library(rpp_interfaces::rpp_interfaces ALIAS rpp_interfaces)
target_compile_features(rpp_interfaces INTERFACE cxx_std_17)
target_link_libraries(rpp_interfaces INTERFACE rpp_cpp::rpp_cpp)
target_include_directories(rpp_interfaces INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
include(cmake/generated-headers.cmake)

install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})
install(DIRECTORY plugin_types DESTINATION share/${PROJECT_NAME})
install(FILES plugins.json DESTINATION share/${PROJECT_NAME})
install(TARGETS rpp_interfaces EXPORT rpp_interfacesTargets)
ament_python_install_package(rpp_interfaces)
ament_export_targets(rpp_interfacesTargets)
ament_export_dependencies(rpp_cpp)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_pytest REQUIRED)
  ament_add_gtest(test_signal_router tests/signal_router_tests.cpp)
  target_link_libraries(test_signal_router rpp_interfaces)
  ament_add_gtest(test_control_law_layout tests/control_law_layout_tests.cpp)
  target_link_libraries(test_control_law_layout rpp_interfaces)
  ament_add_pytest_test(test_fields tests/test_fields.py)
endif()

ament_package(CONFIG_EXTRAS_POST cmake/generated-headers.cmake)
