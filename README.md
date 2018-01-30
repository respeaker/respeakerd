# respeakerd

respeakerd is the server application for the microphone array solutions of SEEED, based on librespeaker which combines the audio front-end processing algorithms.

respeakerd and librespeaker will be delivered to SEEED's ODM customers, under NDA. Without the permission of SEEED, any party is not allowed to distribute the copy of the softwares to 3rd parties.

## How to run

### 1. Preparations

#### System upgrade

The system running on ReSpeaker v2 should be upgraded to version `20180107` or later, as from that version on, many fixes for PulseAudio configuration has been applied.

OneDrive download link: https://bfaceafsieduau-my.sharepoint.com/personal/miaojg22_off365_cn/_layouts/15/guestaccess.aspx?folderid=0bb3c4f3f122d4c2bb0f65eee2b5938f8&authkey=AfLSkcE8QeeUHTQ8GGfrrsU

You can backup your workspace to the onboard eMMC. If your onboard eMMC isn't formated, format it via `fdisk` and mount it.

#### librespeaker

librespeaker is distributed via Debian apt (stored in Seeed's repo), installation of librespeaker is as easy as the following:

```shell
$ sudo apt install librespeaker
```

To know more about librespeaker, please go to the documentation: http://respeaker.io/librespeaker_doc

#### Audio configurations

Though all the configurations are properly done within the system image, you're recommended to double check the configurations of your system, especially when you've changed something from the default.

First, ALSA configurations:

a. Make sure there's no self-defined asound.conf in `/etc/asound.conf`.

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

After this, check the PulseAudio configurations of your system.

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

e. Make sure there's no manually added ALSA devices in `/etc/pulse/default.pa`. All the sound devices should be discovered by the udev-detect module. If you have never touched this file, ignore this step.

Now restart PulseAudio with:

```shell
$ pulseaudio -k
```

This command kills the PulseAudio daemon, but PulseAudio will auto-spawn itself. Now connect to the GUI of the system via VNC, try recording with `Audacity`. If you have no problem in recording and playing, the preparations are done.


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
```

Authorize Alexa with your Amazon account:
```shell
$ alexa-auth
```

Now connect to the GUI desktop of the ReSpeaker's system via VNC, open the builtin browser, visit http://127.0.0.1:3000, do the OAuth.

Run the demo now:

```shell
$ python demo_respeaker_v2_vep_alexa_with_light.py
```

The hotword is `snowboy`.


