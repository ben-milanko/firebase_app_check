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
if(NOT TARGET firebase_app)
    # In the Trax build environment, the SDK is added by the root CMakeLists.txt.
    # add_subdirectory("${FIREBASE_CPP_SDK_DIR}" "${CMAKE_BINARY_DIR}/firebase_sdk_build" EXCLUDE_FROM_ALL)
endif()

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${FIREBASE_CPP_SDK_DIR}/include")

set_property(TARGET ${PLUGIN_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# Link Firebase C++ SDK libs directly with config-specific paths to avoid
# Debug/Release CRT mismatch. The SDK CMake targets default to Debug libs
# which cause LNK2019 errors (__imp__CrtDbgReport) in Release builds.
set(FB_LIB_BASE "${FIREBASE_CPP_SDK_DIR}/libs/windows/VS2019/MD/x64")
target_link_libraries(${PLUGIN_NAME} PUBLIC
  "$<$<CONFIG:Debug>:${FB_LIB_BASE}/Debug/firebase_app_check.lib>"
  "$<$<NOT:$<CONFIG:Debug>>:${FB_LIB_BASE}/Release/firebase_app_check.lib>"
)

# Include Firebase SDK headers
target_include_directories(${PLUGIN_NAME} PUBLIC
  "${FIREBASE_CPP_SDK_DIR}/include")

# WinRT + system libs
set(ADDITIONAL_LIBS advapi32 ws2_32 crypt32 rpcrt4 ole32 windowsapp RuntimeObject)
target_link_libraries(${PLUGIN_NAME} PUBLIC "${ADDITIONAL_LIBS}")

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PLUGIN_NAME} PUBLIC flutter flutter_wrapper_plugin)
