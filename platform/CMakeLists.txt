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
  file_logging.cpp
  file_logging.hpp
  get_text_by_id.cpp
  get_text_by_id.hpp
  http_request.cpp
  http_request.hpp
  http_thread_callback.hpp
  http_user_agent.cpp
  http_user_agent.hpp
  local_country_file.cpp
  local_country_file.hpp
  local_country_file_utils.cpp
  local_country_file_utils.hpp
  localization.hpp
  measurement_utils.cpp
  measurement_utils.hpp
  mwm_traits.cpp
  mwm_traits.hpp
  mwm_version.cpp
  mwm_version.hpp
  platform.cpp
  platform.hpp
  preferred_languages.cpp
  preferred_languages.hpp
  secure_storage.hpp
  settings.cpp
  settings.hpp
  socket.hpp
  string_storage_base.cpp
  string_storage_base.hpp
  target_os.hpp
)

append(
  SRC
  http_user_agent_dummy.cpp
  localization_dummy.cpp
  network_policy_dummy.cpp
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
