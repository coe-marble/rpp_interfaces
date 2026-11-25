if(NOT DEFINED RPP_INTERFACES_CPP_DIR)
  if(DEFINED ENV{RPP_HOME} AND NOT "$ENV{RPP_HOME}" STREQUAL "")
    set(RPP_INTERFACES_CPP_DIR "$ENV{RPP_HOME}/interfaces/cpp")
  else()
    set(RPP_INTERFACES_CPP_DIR "$ENV{HOME}/.rpp/interfaces/cpp")
  endif()
endif()

if(TARGET rpp_interfaces)
  target_include_directories(rpp_interfaces INTERFACE
    "$<BUILD_INTERFACE:${RPP_INTERFACES_CPP_DIR}>")
else()
  set_property(TARGET rpp_interfaces::rpp_interfaces APPEND PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES "${RPP_INTERFACES_CPP_DIR}")
endif()
