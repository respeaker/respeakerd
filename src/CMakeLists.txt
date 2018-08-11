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

pkg_check_modules(RESPEAKER REQUIRED respeaker>=${RESPEAKER_SDK_MINOR_VERSION})
# find_package(respeaker)
message(STATUS "using librespeaker " ${RESPEAKER_VERSION} " at " ${RESPEAKER_LIBRARY_DIRS})

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/cppcodec
    ${PROJECT_SOURCE_DIR}/libs/gflags/include
    ${RESPEAKER_INCLUDE_DIRS}
    ${SNDFILE_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
)

link_directories(
    ${RESPEAKER_LIBRARY_DIRS}
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
    ${RESPEAKER_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${DBUS_LIBRARIES}
)

# strip the rpath for the executable
install(TARGETS respeakerd DESTINATION ${PROJECT_BINARY_DIR}/)
install(TARGETS respeakerd DESTINATION bin)
install(FILES ${PROJECT_SOURCE_DIR}/build/resources/alexa.umdl
              ${PROJECT_SOURCE_DIR}/build/resources/common.res
              ${PROJECT_SOURCE_DIR}/build/resources/snowboy.umdl
        DESTINATION /etc/respeakerd/resources)
install(FILES ${PROJECT_SOURCE_DIR}/build/resources/model/thresholds_curve.json
              ${PROJECT_SOURCE_DIR}/build/resources/model/model.pb
              ${PROJECT_SOURCE_DIR}/build/resources/model/config.json
        DESTINATION /etc/respeakerd/resources/model)

#install(PROGRAMS ${PROJECT_SOURCE_DIR}/scripts/respeaker_python_client DESTINATION bin)
#install(FILES ${PROJECT_SOURCE_DIR}/scripts/respeakerd.service
#              ${PROJECT_SOURCE_DIR}/scripts/respeaker_python_client.service
#        DESTINATION /etc/systemd/system/)

