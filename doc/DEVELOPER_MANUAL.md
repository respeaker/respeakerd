# 开发者参考文档

## 1. 概述

## 2. 如何编译

### 2.1 依赖

- json: https://github.com/nlohmann/json, 仅一个头文件
- base64: https://github.com/tplgy/cppcodec, 仅一个头文件
- gflags: https://gflags.github.io/gflags/, 命令行参数库, 已经编译成静态库
- log4cplus: 日志库, 已经编译成静态库
- asio: 待删除???

## 3. 运行参数

## 4. 协作开发约定

### 4.1 git flow 约定

采用经典 git-flow 工作流, 即 dev 为开发分支, master 为生产分支, 每个开发者必须基于 dev 分支新建 feature 分支进行新功能开发. 达到 milestone 后由管理员进行 release 流程 - 建立 release 分支, 并发布测试, 测试无问题后由管理员将  release 分支合并进 master 分支, 并打版本标签.

参考: https://danielkummer.github.io/git-flow-cheatsheet/index.zh_CN.html

## 附 A. socket包协议

respeakerd 暴露 unix domain socket 于/tmp/respeakerd.sock 文件. 此 socket 为 stream 型 socket, 双工, 分输出和输入通道.

输出通道: 输出音频数据块及事件给 client 端;

输入通道: client 端向 server 端(respeakerd)传递信息, 例如: stop_capture 指令.

输出和输入通道上均采用 json 封装数据, 以 "\r\n" 分包, 即:

{json-packet}\r\n{json-packet}\r\n{json-packet}\r\n

### A.1 输出通道 json 格式

{"type": "audio", "data": "base64编码的x毫秒音频数据", "direction": 浮点数表示的方向角度}

{"type": "event", "data": "hotword", "direction": 浮点数表示的方向角度}

### A.2 输入通道 json 格式

目前仅支持一个指令 stop_capture.

{"cmd": "stop_capture", "cmd_data": ""}


