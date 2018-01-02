# Developer Manual

## 1. Overview

respeakerd is the server application for the microphone array solutions of SEEED, based on `librespeaker` which combines the audio front-end processing algorithms.

It's also a good example on how to utilize the `librespeaker`. Users can implement their own server application / daemon to invoke `librespeaker`.

This manual shows how to compile and run this project `respeakerd`, and then introduces the protocol used in the communication between `respeakerd` and a Python client implementation of AVS (https://github.com/respeaker/avs).

## 2. How to compile

### 2.1 Dependencies

- json: https://github.com/nlohmann/json, header only
- base64: https://github.com/tplgy/cppcodec, header only
- gflags: https://gflags.github.io/gflags/, precompiled for ARM platform
- log4cplus: logging library, not used at now.
- libsndfile1-dev libasound2-dev: save PCM to wav file
- cmake
- librespeaker

### 2.2 Compile

```shell
$ cd PROJECT-ROOT/build
$ cmake ..
$ make
```

## 3. Command line parameters

The command line parameters may change during the development of this project. To get the updated information of the command line paramenters, please

```shell
$ cd PROJECT-ROOT/build
$ ./respeakerd -help
```

## 4. Co-work conventions

### 4.1 git flow

采用经典 git-flow 工作流, 即 dev 为开发分支, master 为生产分支, 每个开发者必须基于 dev 分支新建 feature 分支进行新功能开发. 达到 milestone 后由管理员进行 release 流程 - 建立 release 分支, 并发布测试, 测试无问题后由管理员将  release 分支合并进 master 分支, 并打版本标签.

参考: https://danielkummer.github.io/git-flow-cheatsheet/index.zh_CN.html

Project developers please follow the classical git-flow workflow, that is, `dev` is the devlopment branch, `master` is the production branch, create `feature` branches based on `dev` branch. The `master` branch is protected, project administrator will merge `dev` into `master` after milestone.


## 附 A. Socket protocol

respeakerd 暴露 unix domain socket 于/tmp/respeakerd.sock 文件. 此 socket 为 stream 型 socket, 双工, 分输出和输入通道.
respeakerd exposes unix domain socket at `/tmp/respeakerd.sock`, this socket is a duplex stream socket, including input channel and output channel.

输出通道: 输出音频数据块及事件给 client 端;
Output channel: respeakerd outputs audio data and events to clients.

输入通道: client 端向 server 端(respeakerd)传递信息, 例如: stop_capture 指令.
Input channel: clients report messages to respeakerd, e.g. stop_capture command.

输出和输入通道上均采用 json 封装数据, 以 "\r\n" 分包, 即:
The messages are wrapped in json format, splited by "\r\n",

{json-packet}\r\n{json-packet}\r\n{json-packet}\r\n

### A.1 输出通道 json 格式 | The json structure of input channel

{"type": "audio", "data": "base64编码的x毫秒音频数据", "direction": 浮点数表示的方向角度}
{"type": "audio", "data": "audio data encoded with base64", "direction": float number in degree unit}

{"type": "event", "data": "hotword", "direction": 浮点数表示的方向角度}
{"type": "event", "data": "hotword", "direction": float number in degree unit}

### A.2 输入通道 json 格式 | The json structure of output channel

目前支持以下指令:
Now the following commands are supported:

{"type": "status", "data": "ready"}    # alexa 连接上云, 可以开始接受指令
{"type": "status", "data": "ready"}    # alexa connected to the Cloud, respeakerd can now accept voice commands

{"type": "status", "data": "connecting"}    # alexa 与云失去连接, 此时不接受指令, client 端应该负责用ui 提供用户此状态.
{"type": "status", "data": "connecting"}    # alexa lost connection to the Cloud, respeakerd can't accept voice commands until `ready` state.

{"type": "cmd", "data": "stop_capture"}    # alexa 检测到语音指令结束, 需要底层停止录音
{"type": "cmd", "data": "stop_capture"}    # alexa detected the end of a sentence, respeakerd can handle this event selectively.

{"type": "cmd", "data": "on_speak"}    # alexa 正在播放语音, 在刚开始播的前几秒需要禁止 hotword detection, 因为在此几秒内, AEC 无法应对突兀的大音量
{"type": "cmd", "data": "on_speak"}    # alexa begin to play speech synthesis, respeakerd should stop hotword detection at the very beginning few seconds, because the AEC algorithm can't bear the sudden increase of volume.


