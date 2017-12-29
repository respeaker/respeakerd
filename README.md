# respeakerd
respeakerd is the server application for the microphone array solutions of SEEED, based on librespeaker which combines the audio front-end processing algorithms.

respeakerd and librespeaker will be delivered to SEEED's ODM customers, under NDA. Without the permission of SEEED, any party is not allowed to distribute the copy of the softwares to 3rd parties.

## How to run

### 1. Preparations

- Backup your current workspace and upgrade the system of ReSpeaker v2 to `2018-1-xx`
    - You can backup your workspace to the onboard eMMC.
- Request `librespeaker` from SEEED
- Install `librespeaker` with: `sudo dpkg -i librespeaker_*.deb`

### 2. Run respeakerd

```shell
$ cd ~
$ git clone https://gitlab.com/seeedstudio/respeakerd.git
$ cd respeakerd/build
$ ./respeakerd -debug -snowboy_model_path="./resources/snowboy.umdl" -snowboy_res_path="./resources/common.res" -snowboy_sensitivity="0.4"
```

### 3. Run Python client

```shell


```


