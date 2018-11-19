#!/bin/bash

cmake ..
rm -f CMakeCache.txt

sed -i "1c \respeakerd ($(cat VERSION)-build$(date +%y%m%d)) testing; urgency=low" debian/changelog

echo "librespeaker 2 librespeaker (>= $(cat MIN_LIBRESPEAKER_VERSION))" > debian/shlibs.local

dpkg-buildpackage -b -rfakeroot -us -uc
