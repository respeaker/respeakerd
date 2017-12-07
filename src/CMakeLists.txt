message("== src/CMakeLists.txt")

INCLUDE(FindPkgConfig)

set(GFLAGS_PATH ${PROJECT_SOURCE_DIR}/libs/gflags/lib/ubuntu64)
set(LOG4CPLUS_PATH ${PROJECT_SOURCE_DIR}/libs/log4cplus/lib/ubuntu64)

if(${is_arm} GREATER -1)
    set(GFLAGS_PATH ${PROJECT_SOURCE_DIR}/libs/gflags/lib/rpi)
    set(LOG4CPLUS_PATH ${PROJECT_SOURCE_DIR}/libs/log4cplus/lib/rpi)
    set(WEBRTC_RESPEAKER_PATH ${PROJECT_SOURCE_DIR}/libs/webrtc-respeaker/lib/rpi)
endif(${is_arm} GREATER -1)

# gflags
find_library(LIBGFLAGS_PATH libgflags.a ${GFLAGS_PATH} NO_DEFAULT_PATH)
if(NOT LIBGFLAGS_PATH)
message(FATAL_ERROR "libgflags.a not found")
endif(NOT LIBGFLAGS_PATH)
message(STATUS "gflags is found at " ${LIBGFLAGS_PATH})

# log4cplus
find_library(LIBLOG4CPLUS_PATH liblog4cplus.a ${LOG4CPLUS_PATH} NO_DEFAULT_PATH)
if(NOT LIBLOG4CPLUS_PATH)
message(FATAL_ERROR "liblog4cplus.a not found")
endif(NOT LIBLOG4CPLUS_PATH)
message(STATUS "liblog4cplus is found at " ${LIBLOG4CPLUS_PATH})

pkg_check_modules(LIBRESPEAKER  REQUIRED  respeaker)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/cppcodec
    ${PROJECT_SOURCE_DIR}/libs/asio/include
    ${PROJECT_SOURCE_DIR}/libs/log4cplus/include
    ${PROJECT_SOURCE_DIR}/libs/gflags/include
    ${LIBRESPEAKER_INCLUDE_DIRS}
)

link_directories(
    ${LIBRESPEAKER_LIBRARY_DIRS}
)

add_definitions(-DASIO_STANDALONE)

set(CMAKE_CXX_FLAGS "-fPIC -std=c++11 -fpermissive")

SET(SRC
    main.cc
)

add_executable(respeakerd ${SRC})

target_link_libraries(respeakerd
    ${LIBGFLAGS_PATH}
    ${LIBLOG4CPLUS_PATH}
    ${LIBRESPEAKER_LIBRARIES}
)

install(PROGRAMS respeakerd DESTINATION bin)

