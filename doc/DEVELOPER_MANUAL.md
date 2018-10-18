# Developer Manual

## 1. Overview

`respeakerd` is the server application for the microphone array solutions of SEEED, based on `librespeaker` which combines the audio front-end processing algorithms.

It's also a good example on how to utilize the `librespeaker`. Users can implement their own server application / daemon to invoke `librespeaker`.

This manual shows how to compile and run this project `respeakerd`, and then introduces the protocol used in the communication between `respeakerd` and a Python client implementation of AVS (https://github.com/respeaker/avs).

## 2. How to compile

### 2.1 Dependencies

- json: https://github.com/nlohmann/json, header only
- base64: https://github.com/tplgy/cppcodec, header only
- gflags: https://gflags.github.io/gflags/, precompiled for ARM platform
- inih: https://github.com/benhoyt/inih, source files are included
- libsndfile1-dev libasound2-dev: save PCM to wav file, installed by librespeaker
- libdbus-1-dev: required by main.cc
- cmake
- librespeaker

```shell
$ sudo apt install -y cmake libdbus-1-dev
$ sudo apt install -y --reinstall librespeaker
```

### 2.2 Compile

```shell
$ cd PROJECT-ROOT/build
$ cmake ..
$ make
$ cp src/respeakerd . && chmod a+x ./respeakerd
```

## 3. Parameters

### 3.1 Command line parameters

The command line parameters may change during the development of this project. To get the updated information of the command line paramenters, please inspect the options with

```shell
$ cd PROJECT-ROOT/build
$ ./respeakerd -help
```

```text
-agc_level (dBFS for AGC, the range is [-31, 0]) type: int32 default: -3
-config (the path of the configuration file) type: string default: "/etc/respeaker/respeakerd.conf"
-debug (print more message) type: bool default: false
-dynamic_doa (if enabled, the DoA direction will dynamically track the sound, otherwise it only changes when hotword detected) type: bool default: false
-enable_wav_log (enable logging audio streams into wav files for VEP and respeakerd) type: bool default: false
-fifo_file (the path of the fifo file when enable pulse mode) type: string default: "/tmp/music.input"
-hotword_engine (the hotword engine, can be snips, snowboy) type: string default: "snowboy"
-mic_type (the type of microphone array, can be CIRCULAR_6MIC, CIRCULAR_4MIC) type: string default: "CIRCULAR_6MIC"
-mode (the mode of respeakerd, can be standard, pulse) type: string default: "standard"
-ref_channel (the channel index of the AEC reference, 6 or 7) type: int32 default: 6
-snips_model_path (the path to snips-hotword's model file) type: string default: "/usr/share/respeaker/snips/model"
-snips_sensitivity (the sensitivity of snips-hotword) type: double default: 0.5
-snowboy_model_path (the path to snowboay's model file) type: string default: "/usr/share/respeaker/snowboy/resources/snowboy.umdl"
-snowboy_res_path (the path to snowboay's resource file) type: string default: "/usr/share/respeaker/snowboy/resources/common.res"
-snowboy_sensitivity (the sensitivity of snowboay) type: string default: "0.5"
-source (the source of pulseaudio) type: string default: "default"
-test (test the configuration file) type: bool default: false
```

respeakerd can work in multiple modes.

1. Standard mode (default, or `-mode=standard`)
    In this mode respeakerd will work as a socket server, and communicate with clients via the [socket protocol](#appendix-a-socket-protocol), audio stream and events like triggered will go through this socket, in JSON format. The socket is an UNIX Domain Socket at `/tmp/respeakerd.sock`. respeakerd will recreate this socket file every time it startup.

2. PulseAudio mode (`-mode=pulse`)
    respeakerd can stream its output into PulseAudio system in this mode. With the PulseAudio system, the processed audio stream out of respeakerd can then be dispatched to arbitrary consumer applications. To work with PulseAudio, configurations need to be done for PulseAudio, see [4. PulseAudio mode](#pulseaudio-mode). After those configurations, PulseAudio will create a fifo file `/tmp/music.input` to receive audio stream. So if you don't know how to configure PulseAudio to create the fifo file at another path, please don't change the `-fifo_file` parameter of respeakerd, just use the default.

<!--3. Manual DoA mode （`-mode=manual_with_kws` or `-mode=manual_without_kws`)

    These modes are pretty much like the standard mode, respeakerd will also work as a socket server, and communicate with clients via the socket protocol, but disable the DoA functionality. The beamforming will point to the direction specified manually by users. The direction can be set by a JSON command via the socket protocol, see below for more detail.-->

### 3.2 Configuration file

All the command line options (except --test and --config) will be reflected in the configuration file. The default location of the configuration file is `/etc/respeaker/respeakerd.conf`.

The configurations in the file have lower priority, that is, if you specify the same option both in command line and the configuration file, `respeakerd` will take the value from command line.

## 4. PulseAudio mode

### 4.1 PulseAudio configuratin

We need PulseAudio's `module-pipe-source` module to be loaded. This is handled in `scripts/respeakerd_safe`, it will detect if users have configured `respeakerd` to work as `pulse`mode, and will load the module automatically. When we're doing development, we might hope to load the module manually.

```shell
pactl load-module module-pipe-source source_name="respeakerd_output" rate=16000 channels=1
pactl set-default-source respeakerd_output
```

Or just put this into PulseAudio's configuration file. 

```shell
$ sudo vim /etc/pulse/default.pa
```

Add the following line to the end of the file:

```text
load-module module-pipe-source source_name="respeakerd_output" rate=16000 channels=1
set-default-source respeakerd_output
```

### 4.2 Start `respeakerd` in PulseAudio mode

When we're doing development, we might want to start `respeakerd` in `pulse` mode manually.

```
$ cd PROJECT-ROOT/build
$ ./respeakerd --mode=pulse --source="alsa_input.platform-sound_0.seeed-8ch" --debug
```

Add other options if you need. 
> Please note that if no application's consuming the audio stream from `respeakerd_output` source, respeakerd will seem like get stuck. This is normal because writing to a Linux pipe will be blocked if there's no consumer at the other end of this pipe. Everything will be working if you start to read the pipe, e.g. `parecord -d respeakerd_output dump.wav`.

<!--## 5. Manual DoA mode

Manual DoA mode is designed for the user who want to detect the speaker direction with other methods(e.g. with camera) and pick the voice audio in that direction.


And there are 2 manual DoA modes: one is `manual_with_kws` and another is `manual_without_kws`. Literally, the first mode will output a `hotword event` when the keyword is detected. And `manual_without_kws` mode only outputs audio data.

### 5.1 Start respeakerd in manual DoA mode

```
$ cd PROJECT-ROOT/build
$ ./respeakerd -debug -snowboy_model_path="./resources/snowboy.umdl" -snowboy_res_path="./resources/common.res" -snowboy_sensitivity="0.5" -mode=manual_with_kws
```

### 5.2 Start test_manual_doa python client

`test_manual_doa.py` is an example to show how to set direction to respeakerd. In this example, direction will be set to next 60 degree after a `hotword event`. Please refer to Appendix A for more details.

```
$ cd PROJECT-ROOT/clients/Python
$ python test_manual_doa.py
```
-->


## Appendix A. Socket protocol

`respeakerd` exposes unix domain socket at `/tmp/respeakerd.sock`, this socket is a duplex stream socket, including `input channel` and `output channel`.

Output channel: `respeakerd` outputs audio data and events to clients.

Input channel: clients report messages to `respeakerd`, e.g. cloud_ready status message.

Please note that for now the `respeakerd` only accepts one client connection.

The messages are wrapped in json format, splited by "\r\n", like:

```json
{json-packet}\r\n{json-packet}\r\n{json-packet}\r\n
```

### A.1 The json structure of output channel

```json
{"type": "audio", "data": "audio data encoded with base64", "direction": float number in degree unit}
```

```json
{"type": "event", "data": "hotword", "direction": float number in degree unit}
```

### A.2 The json structure of input channel

For now the following messages are supported:

```json
{"type": "status", "data": "ready"}
```

This is a status message which indicates that the client application has just connected to the cloud (here this client is both a client of respeakerd and a client of ASR cloud, e.g. Alexa Voice Service), respeakerd can now accept voice commands. In the following of this muanual, we illustrate all the mentions of cloud with Alexa.

```json
{"type": "status", "data": "connecting"}
```

This is a status message which indicates that Alexa client has just lost connection to the cloud, respeakerd can't accept voice commands until `ready` state.

```json
{"type": "cmd", "data": "stop_capture"}
```

This is a command message issued from the client. Generally the client gets this message from Alexa cloud, as Alexa has detected the end of a sentence. `respeakerd` hasn't utilize this message for now, it just keeps posting data to the client, becuase the base library the client is using - `voice-engine` - does drop packets when Alexa isn't available to receive inputs.

```json
{"type": "status", "data": "on_speak"}
```

This is a status message which indicates that the client has just received the speech synthesis from Alexa and will begin to play. `respeakerd` utilizes this status to enhence the algorithms. It's recommended that the client should capture this event and pass it down to `respeakerd` if you're doing your own client application.

<!--```json
{"type": "cmd", "data": "set_direction", "direction": int number of degree[0, 359]}
```

This is a command message issued from the client.
It only works at `manual doa mode` in respeakerd. Generally, respeakerd produces 6 audio beams from 6 microphones by the Beamforming algorithm, and one of the beams will be selected as the output beam by the DOA (Direction Of Arrial) algorithm. At `manual doa mode`, respeakerd will not do DOA algorithm as `standard mode ` or `pulse mode`, and the output beam is fixed to the one you specified (default: beam0).
The client should send this message, when it needs to pick audio data from a desired direction. The following table shows the range of degree of each beam and microphone:

beam: | beam1 | beam2 | beam3 | beam4 | beam5 | beam6
----|----|----|----|----|----|----
microphone: | mic1 | mic2 | mic3 | mic4 | mic5  | mic6
degree: | 0-29 / 330-359 | 30-89  | 90-149 | 150-209 | 210-269 | 270-329
-->

## Appendix B. D-Bus protocol

respeakerd uses System Bus to deliver signals. This is especially usefull when it's working with the [C++ version AVS client](https://github.com/respeaker/avs-device-sdk), as the C++ version AVS client doesn't communicate with respeakerd via the socket protocol but PulseAudio instead, so it can no longer receive the critical `hotword` event from respeakerd via json through the socket protocol. It receives the events via D-Bus.

D-Bus object name: "/io/respeaker/respeakerd"
Interface: "respeakerd.signal"

respeakerd outputs:
- `trigger` signal
- `respeakerd_ready` signal

respeakerd listens to:
- client `ready` signal
- client `connecting` signal
- client `on_speak` signal

And the following signals will be listened by the pixel_ring_server (scripts/pixel_ring_server, which is a Python script to drive the RBG led ring on the board)
- `on_idle` signal
- `on_listen` signal
- `on_think` signal
- `on_speak` signal

Except `trigger` and `respeakerd_ready`, all other signals are generated by the C++ AVS client.


```