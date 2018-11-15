message("== src/CMakeLists.txt")

include(FindPkgConfig)

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

