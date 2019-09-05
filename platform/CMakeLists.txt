project(platform)

include_directories(${OMIM_ROOT}/platform)

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
  http_client.cpp
  http_client.hpp
  http_request.cpp
  http_request.hpp
  http_thread_callback.hpp
  http_uploader.hpp
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
  remote_file.cpp
  remote_file.hpp
  secure_storage.hpp
  servers_list.cpp
  servers_list.hpp
  settings.cpp
  settings.hpp
  socket.hpp
  string_storage_base.cpp
  string_storage_base.hpp
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
    http_client_apple.mm
    http_thread_apple.h
    http_thread_apple.mm
    http_uploader_apple.mm
    http_user_agent_dummy.cpp
    platform_mac.mm
    platform_unix_impl.cpp
    platform_unix_impl.hpp
    socket_apple.mm
    http_session_manager.mm
  )
elseif(${PLATFORM_LINUX})
  append(
    SRC
    http_client_curl.cpp
    http_uploader_dummy.cpp
    http_user_agent_dummy.cpp
    platform_linux.cpp
    platform_unix_impl.cpp
    platform_unix_impl.hpp
  )
endif()

add_subdirectory(platform_tests_support)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME} base coding jansson)


if (APPLE)
  target_compile_options(${PROJECT_NAME} PUBLIC "-fobjc-arc")
endif()

omim_add_test_subdirectory(platform_tests)
