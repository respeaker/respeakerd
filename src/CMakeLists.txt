message("== src/CMakeLists.txt")

INCLUDE(FindPkgConfig)

set(GFLAGS_PATH ${PROJECT_SOURCE_DIR}/libs/gflags/lib/ubuntu64)

if(${is_arm} GREATER -1)
    set(GFLAGS_PATH ${PROJECT_SOURCE_DIR}/libs/gflags/lib/rpi)
endif(${is_arm} GREATER -1)

# gflags
find_library(LIBGFLAGS_PATH libgflags_nothreads.a ${GFLAGS_PATH} NO_DEFAULT_PATH)
if(NOT LIBGFLAGS_PATH)
message(FATAL_ERROR "libgflags.a not found")
endif(NOT LIBGFLAGS_PATH)
message(STATUS "gflags is found at " ${LIBGFLAGS_PATH})

pkg_check_modules(SNDFILE  REQUIRED  sndfile)

pkg_check_modules(DBUS  REQUIRED  dbus-1)

pkg_check_modules(LIBRESPEAKER  REQUIRED  respeaker)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/cppcodec
    ${PROJECT_SOURCE_DIR}/libs/gflags/include
    ${LIBRESPEAKER_INCLUDE_DIRS}
    ${SNDFILE_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
)

link_directories(
    ${LIBRESPEAKER_LIBRARY_DIRS}
    ${SNDFILE_LIBRARY_DIRS}
    ${DBUS_LIBRARY_DIRS}
)

set(CMAKE_CXX_FLAGS "-fPIC -std=c++11 -fpermissive")

SET(SRC
    main.cc
)

add_executable(respeakerd ${SRC})

target_link_libraries(respeakerd
    ${LIBGFLAGS_PATH} -lpthread -lm
    ${LIBRESPEAKER_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${DBUS_LIBRARIES}
)

# strip the rpath for the executable
install(TARGETS respeakerd DESTINATION ${PROJECT_BINARY_DIR}/)
#install(PROGRAMS ${PROJECT_SOURCE_DIR}/scripts/respeaker_python_client DESTINATION bin)
#install(FILES ${PROJECT_SOURCE_DIR}/scripts/respeakerd.service
#              ${PROJECT_SOURCE_DIR}/scripts/respeaker_python_client.service
#        DESTINATION /etc/systemd/system/)

