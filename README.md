# respeakerd

respeakerd is the server application for the microphone array solutions of SEEED, based on librespeaker which combines the audio front-end processing algorithms.

## 1. Installation

### 1.1 Prerequisites

**(1) ReSpeaker Core v2**

The system running on ReSpeaker v2 should be upgraded to version `20180107` or later, as from that version on, many fixes for PulseAudio configuration has been applied.

OneDrive download link: <a href="https://bfaceafsieduau-my.sharepoint.com/personal/miaojg22_off365_cn/_layouts/15/guestaccess.aspx?folderid=0bb3c4f3f122d4c2bb0f65eee2b5938f8&authkey=AfLSkcE8QeeUHTQ8GGfrrsU"><img src="https://github.com/respeaker/get_started_with_respeaker/blob/master/img/onedrive.png?raw=true" height="25"></img></a>

You can backup your workspace to the onboard eMMC. If your onboard eMMC isn't formated, format it via `fdisk` and mount it.

**(2) Raspberry Pi**

raspbian `stretch` is recommended. You need to install the driver for ReSpeaker Pi Hats, please refer [here](https://github.com/respeaker/seeed-voicecard).

Now we support the following Pi Hats:

- ReSpeaker 6 Mic Array for Raspberry Pi
<!-- - ReSpeaker 4 Mic Array for Raspberry Pi -->

Secondly you need to add the apt repository of Seeed.

```shell
$ echo "deb https://seeed-studio.github.io/pi_repo/ stretch main" | sudo tee /etc/apt/sources.list.d/seeed.list
$ curl https://seeed-studio.github.io/pi_repo/public.key | sudo apt-key add -
$ sudo apt update
```

After the first time you installted the Hat's driver, please do the  following configuration.

```shell
$ sudo apt install respeakerd-pi-tools
$ sudo respeakerd-pi-tools setup-pulse
```

Reboot the Pi to apply the configurations ( for PulseAudio ) before you move to the next step.

### 1.2 Installation

ssh to the board, then execute

```shell
curl https://raw.githubusercontent.com/respeaker/respeakerd/master/scripts/install_all.sh|bash
```

This script will install all the dependencies, and write the microphone array type in `/etc/respeaker/respeakerd.conf` as your selection. The Alexa authorization is needed by the Python client of respeakerd.



## 2. Run

### 2.1 Run `respeakerd`

In the above `step 1.2`, a systemd service `respeakerd` will be installed and started. If everything's right, the `respeakerd` should be running now. You can inspect the status of the respeakerd service with

```shell
sudo journalctl -f -u respeakerd
```

### 2.2 Run clients

We have implemented a Python client for `respeakerd`, this client is also an AVS client. Since all the Python dependencies are already installed by the script in step 1.2, you can simply run the client with

```shell
python ~/respeakerd/clients/Python/demo_respeaker_v2_vep_alexa_with_light.py
```

And speak `snowboy` to trigger the conversation with Alexa.

We have also modified the official AVS Device SDK (C++) to work with `respeakerd` - https://github.com/respeaker/avs-device-sdk. We will have a separated [guide](doc/AVS_DEVICE_SDK.md) on this.



## 3. Troubleshooting Tips

### 3.1 Under the hood

The following image shows the software stack, and the audio flow. Understanding this will be helpfull to your debugging.

![](https://user-images.githubusercontent.com/5130185/46943198-baf91c00-d0a1-11e8-8958-285771fa15fa.png)


### 3.2 ASLA configurations

**(a)** If you ever touched `/etc/asound.conf` and did some mofidications there, you're recommended to restore this file to its default.

For ReSpeaker Core v2, there's no `/etc/asound.conf` by default.

For Raspberry Pi, the [seeed-voicecard](https://github.com/respeaker/seeed-voicecard) installation script will install a systemd service which restores `/etc/asound.conf` to its default every boot up. Please make sure you've not disabled the `seeed-voicecard` service.

```shell
$ sudo systemctl list-unit-files | grep seeed
seeed-voicecard.service                enabled
```



**(b)** Check the volume settings for the playback and cpature devices

For ReSpeaker Core v2, if you want to restore the ALSA volume to its default, do as the following

```shell
$ sudo alsamixer
```

Tune it like this

![](https://user-images.githubusercontent.com/5130185/47064878-a8065900-d213-11e8-9bc8-ba2d9a59f91d.png)

Then save the configuration permanetly.

```shell
$ sudo alsactl store
```

For Raspberry Pi, the same thing as `/etc/asound.conf` will happen. The `seeed-voicecard` service will restore the mixer configuration every boot, with the configuration file at the following path as the original.

- /etc/voicecard/ac108_asound.state - ReSpeaker 4 Mic Array for Raspberry Pi
- /etc/voicecard/ac108_6mic.state - ReSpeaker Linear 4 Mic Array for Raspberry Pi, ReSpeaker 6 Mic Array for Raspberry Pi

> Please note that, if you want to change our default volume configuration, any `alsamixer` `alsactl` operation will be overwritten when the system boots up next time. You need to do as the following.
>
> Tune the volume with alsamixer -> Save the mixer configuration to state file via `alsactl store` -> `cp /var/lib/alsa/asound.state /etc/voicecard/ac108_asound.state` if you're using ReSpeaker 4 Mic Array for Raspberry Pi, `cp /var/lib/alsa/asound.state /etc/voicecard/ac108_6mic.state` if you're using ReSpeaker Linear 4 Mic Array for Raspberry Pi, ReSpeaker 6 Mic Array for Raspberry Pi, and then reboot the Pi.

### 3.3 PulseAudio configuration

`respeakerd` depends on PulseAudio system. For ReSpeaker Core v2, PulseAudio is included by default in the system image. For Raspberry Pi, PulseAudio will be installed as a dependence of `respeakerd` when you install `respeakerd` with `apt-get`. PulseAudio will detect the microhpne array codec with the `udev` mechanism. So if you ever touched the configuration of PulseAudio and disabled the `module-udev-detect` module, please remember to enable it. You can check if `udev` is enabled in your PulseAudio configuration with

```shell
$ pactl list modules|grep -n3 udev
```

If you can find the following text in the output, `udev` is enabled.

```shell
44-Module #5
45:    	Name: module-udev-detect
46-    	Argument:
47-    	Usage counter: n/a
48-    	Properties:
```

If you run

```shell
$ pactl list sources
```

You will be able to find a source with name

- alsa_input.platform-sound_0.seeed-8ch - ReSpeaker Core v2
- alsa_input.platform-soc_sound.seeed-source - ReSpeaker 4 Mic Array for Raspberry Pi
- alsa_input.platform-soc_sound.seeed-8ch - ReSpeaker Linear 4 Mic Array for Raspberry Pi, ReSpeaker 6 Mic Array for Raspberry Pi



## Other resources

- [Developer Manual](doc/DEVELOPER_MANUAL.md) - More technical details

