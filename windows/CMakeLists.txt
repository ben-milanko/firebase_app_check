cmake_minimum_required(VERSION 3.14)

set(FIREBASE_SDK_VERSION "12.7.0")

# Reuse logic from firebase_core to find or download SDK
if (EXISTS "$ENV{FIREBASE_CPP_SDK_DIR}/include/firebase/version.h")
    set(FIREBASE_CPP_SDK_DIR "$ENV{FIREBASE_CPP_SDK_DIR}")
else()
    set(extracted_path "${CMAKE_BINARY_DIR}/extracted")
    set(FIREBASE_CPP_SDK_DIR "${extracted_path}/firebase_cpp_sdk_windows")
endif()

message(STATUS "Using Firebase SDK from: ${FIREBASE_CPP_SDK_DIR}")

project(firebase_app_check LANGUAGES CXX)

set(PLUGIN_NAME "firebase_app_check_plugin")

list(APPEND PLUGIN_SOURCES
  "firebase_app_check_plugin.cpp"
  "firebase_app_check_plugin.h"
)

add_library(${PLUGIN_NAME} STATIC
  "include/firebase_app_check/firebase_app_check_plugin.h"
  "firebase_app_check_plugin_c_api.cpp"
  ${PLUGIN_SOURCES}
)

apply_standard_settings(${PLUGIN_NAME})

set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PUBLIC FLUTTER_PLUGIN_IMPL)

# Add the Firebase SDK subdirectory if not already added
# This creates the firebase_app, firebase_app_check, etc. targets
if(NOT TARGET firebase_app)
    # Note: In the Trax build environment, the SDK is added by the root CMakeLists.txt.
    # We skip adding it here to avoid "target already exists" errors caused by 
    # different source paths (env var vs fallback) resolving to the same targets.
    # add_subdirectory("${FIREBASE_CPP_SDK_DIR}" "${CMAKE_BINARY_DIR}/firebase_sdk_build" EXCLUDE_FROM_ALL)
endif()

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${FIREBASE_CPP_SDK_DIR}/include")

# Fix Debug/Release mismatch: firebase SDK ships with both configurations
# The default IMPORTED_LOCATION points to Debug libs, but Release builds need Release libs
foreach(fb_target firebase_app firebase_app_check)
    if(TARGET ${fb_target})
        get_target_property(firebase_lib_path ${fb_target} IMPORTED_LOCATION)
        if(firebase_lib_path AND "${firebase_lib_path}" MATCHES "Debug")
            string(REPLACE "Debug" "Release" firebase_lib_release_path "${firebase_lib_path}")
            if(EXISTS "${firebase_lib_release_path}")
                message(STATUS "Fixing ${fb_target} Release lib path to: ${firebase_lib_release_path}")
                set_target_properties(${fb_target} PROPERTIES
                  IMPORTED_LOCATION_RELEASE "${firebase_lib_release_path}"
                  IMPORTED_LOCATION_PROFILE "${firebase_lib_release_path}"
                )
            endif()
        endif()
    endif()
endforeach()

# Ensure we are using the correct runtime library (MultiThreadedDLL /MD) to match Flutter and Firebase SDK
# This is critical for avoiding LNK2019/LNK2001 errors with CRT symbols like invalid_parameter
set_property(TARGET ${PLUGIN_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# Link against firebase_app and firebase_app_check
set(FIREBASE_LIBS firebase_app firebase_app_check)
set(ADDITIONAL_LIBS advapi32 ws2_32 crypt32 rpcrt4 ole32 icu)

target_link_libraries(${PLUGIN_NAME} PUBLIC "${FIREBASE_LIBS}" "${ADDITIONAL_LIBS}")

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PLUGIN_NAME} PUBLIC flutter flutter_wrapper_plugin)
