# respeakerd
respeakerd is the server application for the microphone array solutions of SEEED, based on librespeaker which combines the audio front-end processing algorithms.

respeakerd and librespeaker will be delivered to SEEED's ODM customers, under NDA. Without the permission of SEEED, any party is not allowed to distribute the copy of the softwares to 3rd parties.

## How to run

### 1. Preparations

#### Backup

Backup your current workspace and upgrade the system of ReSpeaker v2 to `2018-1-xx`. You can backup your workspace to the onboard eMMC. If your onboard eMMC isn't formated, format it via `fdisk` and mount it.

#### librespeaker

You should request `librespeaker` from SEEED if you haven't it on hand.

Install `librespeaker` with:

```shell
$ sudo apt update
$ sudo dpkg -i librespeaker_*.deb
```

It may print error messages about missing dependencies, install them with:

```shell
$ sudo apt --fix-broken install
```

#### Audio configurations

You'd better double check the ALSA configuration of your system.

a. Make sure there's no self-defined asound.conf in `/etc`.

b. Check the volume settings for the playback and cpature devices:

```shell
$ sudo alsamixer
```

Tune it like this

![](https://gitlab.com/seeedstudio/respeakerd/uploads/e9ea93446962f8e524ebc54408a83f96/image.png)

Then save the configuration permanetly.

```shell
$ sudo alsactl store
```

After this, check the PulseAudio configuration of your system.

c. Make sure there's no self-defined client.conf in `~/.config/pulse/`. You may simply delete the directory:

```shell
$ rm -rf ~/.config/pulse
```

d. Make sure that `udev-detect` module is enabled:

```shell
$ pactl list modules|grep -n3 udev
```

If you see the following printings, it is loaded.

```shell
42-
43-
44-Module #5
45:    	Name: module-udev-detect
46-    	Argument:
47-    	Usage counter: n/a
48-    	Properties:
```

e. Make sure there's no manually added ALSA devices in `/etc/pulse/default.pa`. All the sound devices should be discovered by the udev-detect module.

And then restart PulseAudio with:

```shell
$ pulseaudio -k
```

This command kills the PulseAudio daemon, but PulseAudio will auto-spawn itself. Now connect to the GUI of the system via VNC, try recording with `Audacity`.


### 2. Run respeakerd

```shell
$ cd ~
$ git clone https://gitlab.com/seeedstudio/respeakerd.git
$ cd respeakerd/build
$ chmod a+x respeakerd
$ ./respeakerd -debug -snowboy_model_path="./resources/snowboy.umdl" -snowboy_res_path="./resources/common.res" -snowboy_sensitivity="0.4"
```

The program will pause on the printings like:

```shell
From        vep_amix_init() for    (complex_t*)vobj->amix->out[i] allocated  1024 bytes, flags VCP_MEM_AMIX        , reg:  0, total: 2048
From        vep_amix_init() for    (complex_t*)vobj->amix->out[i] allocated  1024 bytes, flags VCP_MEM_AMIX        , reg:  0, total: 1024
From        vep_amix_init() for    (complex_t*)vobj->amix->out[i] allocated  1024 bytes, flags VCP_MEM_AMIX        , reg:  0, total: 0
```

That's OK, let's go ahead to the setup of the Python client.

### 3. Run Python client

Open another terminal.

```shell
$ cd ~/respeakerd/clients/Python
$ sudo pip install avs pixel_ring voice-engine
$ sudo apt install  python-mraa python-upm libmraa1 libupm1 mraa-tools
$ python demo_respeaker_v2_vep_alexa_with_light.py

```


