cmake_minimum_required(VERSION 3.0)

message("== CMakeLists.txt")

project(respeakerd VERSION 1.2.0)

# you should only bump this up when librespeaker's ABI has been changed
set(MIN_LIBRESPEAKER_VERSION 2.1.1)

message(STATUS "building ${PROJECT_NAME} version ${PROJECT_VERSION} (${CMAKE_BUILD_TYPE}) ...")

file(WRITE ${PROJECT_BINARY_DIR}/VERSION "${PROJECT_VERSION}")
file(WRITE ${PROJECT_BINARY_DIR}/MIN_LIBRESPEAKER_VERSION "${MIN_LIBRESPEAKER_VERSION}")

set(CMAKE_SKIP_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_PREFIX /usr)

include("TargetArch.cmake")
target_architecture(arch)
message("the arch is ${arch}")
string(FIND ${arch} "86" is_x86)
string(FIND ${arch} "arm" is_arm)

add_subdirectory(src)
add_subdirectory(scripts)

