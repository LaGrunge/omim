project(platform)

include_directories(${GEOCORE_ROOT}/platform)

set(
  SRC
  chunks_download_strategy.cpp
  chunks_download_strategy.hpp
  constants.hpp
  country_defines.cpp
  country_defines.hpp
  country_file.cpp
  country_file.hpp
  get_text_by_id.cpp
  get_text_by_id.hpp
  local_country_file.cpp
  local_country_file.hpp
  local_country_file_utils.cpp
  local_country_file_utils.hpp
  localization.hpp
  measurement_utils.cpp
  measurement_utils.hpp
  mwm_version.cpp
  mwm_version.hpp
  platform.cpp
  platform.hpp
  preferred_languages.cpp
  preferred_languages.hpp
  settings.cpp
  settings.hpp
  socket.hpp
  string_storage_base.cpp
  string_storage_base.hpp
  target_os.hpp
)

append(
  SRC
  localization_dummy.cpp
  platform_std.cpp
  wifi_info.hpp
)

if(${PLATFORM_MAC})
  append(
    SRC
    platform_mac.mm
    platform_unix_impl.cpp
    platform_unix_impl.hpp
    socket_apple.mm
    http_session_manager.mm
  )
elseif(${PLATFORM_LINUX})
  append(
    SRC
    platform_linux.cpp
    platform_unix_impl.cpp
    platform_unix_impl.hpp
  )
endif()

geocore_add_test_subdirectory(platform_tests_support)

geocore_add_library(${PROJECT_NAME} ${SRC})
geocore_link_libraries(${PROJECT_NAME} base coding jansson ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})


if (APPLE)
  target_compile_options(${PROJECT_NAME} PUBLIC "-fobjc-arc")
endif()

geocore_add_test_subdirectory(platform_tests)
