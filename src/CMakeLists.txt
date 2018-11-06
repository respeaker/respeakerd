message("== src/CMakeLists.txt")

include(FindPkgConfig)

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

pkg_check_modules(RESPEAKER REQUIRED respeaker>=${MIN_LIBRESPEAKER_VERSION})
message(STATUS "found librespeaker " ${RESPEAKER_VERSION} " at " ${RESPEAKER_LIBDIR})

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

add_definitions(-DRESPEAKERD_VERSION="${PROJECT_VERSION}")

set(CMAKE_CXX_FLAGS "-fPIC -std=c++11 -fpermissive")

set(SRC
    ini/ini.c
    ini/INIReader.cpp
    main.cc
)

add_executable(respeakerd ${SRC})

target_link_libraries(respeakerd
    ${LIBGFLAGS_PATH} -lpthread -lm
    ${RESPEAKER_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${DBUS_LIBRARIES}
)

install(TARGETS respeakerd DESTINATION bin)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/scripts/respeakerd_safe DESTINATION bin)
install(FILES ${PROJECT_SOURCE_DIR}/scripts/respeakerd.service
              #${PROJECT_SOURCE_DIR}/scripts/respeaker_python_client.service
        DESTINATION /etc/systemd/system)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts/seeed-voicecard DESTINATION /usr/share/respeaker)
install(FILES ${PROJECT_SOURCE_DIR}/scripts/respeakerd.conf DESTINATION /etc/respeaker)

