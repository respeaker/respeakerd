# AVS Device SDK (C++)

This guide will show you how to run the Amazon official AVS Device SDK (C++) with `respeakerd`.

## 1. Prerequisites

Please finish [README - 1. Installation](README.md#1-installation) if you haven't.

## 2. Configure respeakerd

```shell
$ sudo vim /etc/respeaker/respeakerd.conf
```

Change the `mode` to pulse

```
# mode
# the mode of respeakerd, can be: standard, pulse
mode = pulse
```

Restart the service

```shell
$ sudo systemctl start respeakerd
```

## 3. Compile and Run AVS Device SDK

> This chapter pretty much refers to [Raspberry Pi Quick Start Guide](https://github.com/alexa/avs-device-sdk/wiki/Raspberry-Pi-Quick-Start-Guide).

You will notice that, the differences aginst the official guide are:
- skip chapter 1.3 since we don't need a wake word engine, wake word engine is included in respeakerd
- replace `git://github.com/alexa/avs-device-sdk.git` with `git://github.com/respeaker/avs-device-sdk.git`
- new build option `-DRESPEAKERD_KEY_WORD_DETECTOR=ON`
- skip chapter 3.4 since  the default configuration file of ALSA works well for us

```shell
$ cd ~ && mkdir sdk-folder && cd sdk-folder && mkdir sdk-build sdk-source third-party application-necessities && cd application-necessities && mkdir sound-files
$ sudo apt-get -y install git gcc cmake build-essential libsqlite3-dev libcurl4-openssl-dev libfaad-dev libsoup2.4-dev libgcrypt20-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-good libasound2-dev doxygen
$ cd ~/sdk-folder/third-party && wget -c http://www.portaudio.com/archives/pa_stable_v190600_20161030.tgz && tar zxf pa_stable_v190600_20161030.tgz && cd portaudio && ./configure --without-jack && make
$ sudo pip install commentjson
$ sudo pip install flask
$ cd ~/sdk-folder/sdk-source && git clone git://github.com/respeaker/avs-device-sdk.git
$ cd ~/sdk-folder/sdk-build && cmake ~/sdk-folder/sdk-source/avs-device-sdk -DCMAKE_BUILD_TYPE=DEBUG -DRESPEAKERD_KEY_WORD_DETECTOR=ON -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON -DPORTAUDIO_LIB_PATH=~/sdk-folder/third-party/portaudio/lib/.libs/libportaudio.a -DPORTAUDIO_INCLUDE_DIR=~/sdk-folder/third-party/portaudio/include
$ make SampleApp -j2

# obtain credentials of AVS
# https://github.com/alexa/avs-device-sdk/wiki/Raspberry-Pi-Quick-Start-Guide#3-obtain-credentials-and-set-up-your-local-auth-server
# back here when you finish the credentials
# strongly recommend to backup the credentials
$ cp ~/sdk-folder/sdk-build/Integration/AlexaClientSDKConfig.json ~

$ ~/sdk-folder/sdk-build/SampleApp/src/SampleApp ~/sdk-folder/sdk-build/Integration/AlexaClientSDKConfig.json

```

Now you are able to make conversations with Alexa, but all user experiences are done through the command line messages.

## 4. LED Ring Light Effect

Here we need some scripts hosted in the Git repo of `respeakerd`, let's just checkout the repo.

```shell
$ cd ~
$ git clone https://github.com/respeaker/respeakerd.git
```

And then tap the commands below.

```shell
$ sudo cp -f ~/respeakerd/scripts/pixel_ring_server /usr/local/bin/
$ sudo chmod a+x /usr/local/bin/pixel_ring_server
$ pixel_ring_server
```

## 5. Startup

```shell
$ sudo cp -f ~/respeakerd/scripts/avs_cpp_sdk_safe /usr/local/bin
$ sudo chmod a+x /usr/local/bin/avs_cpp_sdk_safe
$ sudo cp -f ~/respeakerd/scripts/pixel_ring_server.service /etc/systemd/system/
$ sudo sed -i -e "s/User=.*/User=$(id -n -u)/" /etc/systemd/system/pixel_ring_server.service
$ sudo sed -i -e "s/Group=.*/Group=$(id -n -g)/" /etc/systemd/system/pixel_ring_server.service
$ sudo cp -f ~/respeakerd/scripts/avs_cpp_sdk.service /etc/systemd/system/
$ sudo sed -i -e "s/User=.*/User=$(id -n -u)/" /etc/systemd/system/avs_cpp_sdk.service
$ sudo sed -i -e "s/Group=.*/Group=$(id -n -g)/" /etc/systemd/system/avs_cpp_sdk.service
$ sudo systemctl enable pixel_ring_server
$ sudo systemctl enable avs_cpp_sdk
$ sudo systemctl start pixel_ring_server 
$ sudo systemctl start avs_cpp_sdk
```
