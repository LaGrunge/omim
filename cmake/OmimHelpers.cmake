# Function for setting target platform:
function(omim_set_platform_var PLATFORM_VAR pattern)
  set(${PLATFORM_VAR} FALSE PARENT_SCOPE)

  if (NOT PLATFORM)
    if (${ARGN})
      list(GET ARGN 0 default_case)
      if (${default_case})
        set(${PLATFORM_VAR} TRUE PARENT_SCOPE)
        message("Setting ${PLATFORM_VAR} to true")
      endif()
    endif()
  else()
    message("Platform: ${PLATFORM}")
    if (${PLATFORM} MATCHES ${pattern})
      set(${PLATFORM_VAR} TRUE PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Functions for using in subdirectories
function(omim_add_executable executable)
  add_executable(${executable} ${ARGN})
  add_dependencies(${executable} BuildVersion)
  if (USE_ASAN)
    target_link_libraries(
      ${executable}
      "-fsanitize=address"
      "-fno-omit-frame-pointer"
    )
  endif()
  if (USE_TSAN)
    target_link_libraries(
      ${executable}
      "-fsanitize=thread"
      "-fno-omit-frame-pointer"
    )
  endif()
  if (USE_PPROF)
    target_link_libraries(${executable} "-lprofiler")
  endif()
  if (USE_PCH)
    add_precompiled_headers_to_target(${executable} ${OMIM_PCH_TARGET_NAME})
  endif()
endfunction()

function(omim_add_library library)
  add_library(${library} ${ARGN})
  add_dependencies(${library} BuildVersion)
  if (USE_PCH)
    add_precompiled_headers_to_target(${library} ${OMIM_PCH_TARGET_NAME})
  endif()
endfunction()

function(omim_add_test executable)
  if (NOT SKIP_TESTS)
    omim_add_executable(
      ${executable}
      ${ARGN}
      ${OMIM_ROOT}/testing/testingmain.cpp
     )
  endif()
endfunction()

function(omim_add_test_subdirectory subdir)
  if (NOT SKIP_TESTS)
    add_subdirectory(${subdir})
  else()
    message("SKIP_TESTS: Skipping subdirectory ${subdir}")
  endif()
endfunction()

function(omim_add_pybindings_subdirectory subdir)
  if (PYBINDINGS)
    add_subdirectory(${subdir})
  else()
    message("Skipping pybindings subdirectory ${subdir}")
  endif()
endfunction()

function(omim_link_platform_deps target)
  if ("${ARGN}" MATCHES "platform")
    if (PLATFORM_MAC)
      target_link_libraries(
        ${target}
        "-framework CFNetwork"
        "-framework Foundation"
        "-framework IOKit"
        "-framework SystemConfiguration"
        "-framework Security"
      )
    endif()
  endif()
endfunction()

function(omim_link_libraries target)
  if (TARGET ${target})
    target_link_libraries(${target} ${ARGN} ${CMAKE_THREAD_LIBS_INIT})
    omim_link_platform_deps(${target} ${ARGN})
  else()
    message("~> Skipping linking the libraries to the target ${target} as it"
            " does not exist")
  endif()
endfunction()

function(append VAR)
  set(${VAR} ${${VAR}} ${ARGN} PARENT_SCOPE)
endfunction()

function(add_clang_compile_options)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(${ARGV})
  endif()
endfunction()

function(add_gcc_compile_options)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(${ARGV})
  endif()
endfunction()

function(add_clang_cpp_compile_options)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${ARGV}>")
  endif()
endfunction()

function(add_gcc_cpp_compile_options)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${ARGV}>")
  endif()
endfunction()

function(export_directory_flags filename)
  get_directory_property(include_directories INCLUDE_DIRECTORIES)
  get_directory_property(definitions COMPILE_DEFINITIONS)
  get_directory_property(flags COMPILE_FLAGS)
  get_directory_property(options COMPILE_OPTIONS)

  if (PLATFORM_ANDROID)
    set(
      include_directories
      ${include_directories}
      ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}
    )
    set(
      platform_flags
      "${ANDROID_COMPILER_FLAGS} ${ANDROID_COMPILER_FLAGS_CXX}"
    )
    set(
      flags
      "--target=${CMAKE_C_COMPILER_TARGET}"
      "--sysroot=${CMAKE_SYSROOT}" ${flags}
    )
  endif()

  # Append Release/Debug flags:
  string(TOUPPER "${CMAKE_BUILD_TYPE}" upper_build_type)
  set(flags ${flags} ${CMAKE_CXX_FLAGS_${upper_build_type}})

  set(
    include_directories
    "$<$<BOOL:${include_directories}>\:-I$<JOIN:${include_directories},\n-I>\n>"
  )
  set(definitions "$<$<BOOL:${definitions}>:-D$<JOIN:${definitions},\n-D>\n>")
  set(flags "$<$<BOOL:${flags}>:$<JOIN:${flags},\n>\n>")
  set(options "$<$<BOOL:${options}>:$<JOIN:${options},\n>\n>")
  file(
    GENERATE OUTPUT
    ${filename}
    CONTENT
    "${definitions}${include_directories}${platform_flags}\n${flags}${options}\n"
  )
endfunction()

function(add_pic_pch_target header pch_target_name
         pch_file_name suffix pic_flag)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/pch_${suffix}")
  file(COPY "${header}" DESTINATION "${CMAKE_BINARY_DIR}/pch_${suffix}")
  set(_header "${CMAKE_BINARY_DIR}/pch_${suffix}/${pch_file_name}")
  set(
    _compiled_header
    "${CMAKE_BINARY_DIR}/pch_${suffix}/${pch_file_name}.${PCH_EXTENSION}"
  )
  add_custom_target(
    "${pch_target_name}_${suffix}"
    COMMAND
    "${CMAKE_CXX_COMPILER}" ${compiler_flags} ${c_standard_flags} ${pic_flag}
                            -x c++-header
                            -c "${_header}" -o "${_compiled_header}"
    COMMENT "Building precompiled omim CXX ${suffix} header"
  )
endfunction()

