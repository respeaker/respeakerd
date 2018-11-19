message("== scripts/CMakeLists.txt")

install(PROGRAMS ${PROJECT_SOURCE_DIR}/scripts/respeakerd_safe
                 ${PROJECT_SOURCE_DIR}/scripts/respeakerd-pi-tools
        DESTINATION bin)

#install(FILES ${PROJECT_SOURCE_DIR}/scripts/respeakerd.service
#              #${PROJECT_SOURCE_DIR}/scripts/respeaker_python_client.service
#        DESTINATION /etc/systemd/system)

install(FILES ${PROJECT_SOURCE_DIR}/scripts/respeakerd.conf DESTINATION /etc/respeaker)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts/seeed-voicecard DESTINATION /usr/share/respeaker)


