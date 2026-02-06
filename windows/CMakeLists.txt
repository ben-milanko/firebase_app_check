cmake_minimum_required(VERSION 3.14)

set(FIREBASE_SDK_VERSION "12.7.0")

# Reuse logic from firebase_core to find or download SDK
if (EXISTS $ENV{FIREBASE_CPP_SDK_DIR}/include/firebase/version.h)
    set(FIREBASE_CPP_SDK_DIR $ENV{FIREBASE_CPP_SDK_DIR})
else()
    set(extracted_path "${CMAKE_BINARY_DIR}/extracted")
    set(FIREBASE_CPP_SDK_DIR "${extracted_path}/firebase_cpp_sdk_windows")
endif()

project(firebase_app_check LANGUAGES CXX)

set(PLUGIN_NAME "firebase_app_check_plugin")

list(APPEND PLUGIN_SOURCES
  "firebase_app_check_plugin.cpp"
  "firebase_app_check_plugin.h"
)

add_library(${PLUGIN_NAME} STATIC
  "include/firebase_app_check/firebase_app_check_plugin_c_api.h"
  "firebase_app_check_plugin_c_api.cpp"
  ${PLUGIN_SOURCES}
)

apply_standard_settings(${PLUGIN_NAME})

set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PUBLIC FLUTTER_PLUGIN_IMPL)

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${FIREBASE_CPP_SDK_DIR}/include")

# Link against firebase_app and firebase_app_check
set(FIREBASE_LIBS firebase_app firebase_app_check)
set(ADDITIONAL_LIBS advapi32 ws2_32 crypt32 rpcrt4 ole32 icu)

target_link_libraries(${PLUGIN_NAME} PUBLIC "${FIREBASE_LIBS}" "${ADDITIONAL_LIBS}")

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PLUGIN_NAME} PUBLIC flutter flutter_wrapper_plugin)
