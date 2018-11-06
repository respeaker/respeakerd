#!/bin/bash

PLATFORM=pi
DEFAULT_USER=pi
if [[ $(grep -c RK3229 /proc/device-tree/model) == 1 ]] ; then
    PLATFORM=axol
    DEFAULT_USER=respeaker
fi

echo "The platform is $PLATFORM"

if [[ $PLATFORM == axol ]] ; then
    R=`grep "Image" /etc/issue.net | awk '{print $4}'`

    if [[ $(echo "$R < 20180107"|bc) = 1 ]] ; then
        echo "Please upgrade your system version to 20180107 or later"
        echo "Refer to the guide here: https://github.com/respeaker/get_started_with_respeaker/blob/master/docs/ReSpeaker_Core_V2/getting_started.md#image-installation"
        exit 1
    fi
fi

USER_ID=`id -u`

if [[ ${USER_ID} != 1000 ]] ; then
    echo "Please run this script with user ${DEFAULT_USER}"
    exit 1
fi

## prepare PulseAudio
#rm -rf ~/.config/pulse/client.conf
#pulseaudio -k
#sleep 1

# SEEED_8MIC_EXISTS=`pactl list sources|grep alsa_input.platform-sound_0.seeed-8ch -c`

# if [ $SEEED_8MIC_EXISTS = 0 ]; then
#     echo "PulseAudio device <alsa_input.platform-sound_0.seeed-8ch> not found"
#     echo "Please make sure you are running this script on ReSpeaker Core v2"
#     exit 1
# fi

# PULSE_SOURCE="alsa_input.platform-sound_0.seeed-8ch"

## Install deps
# python-mraa,python-upm,libmraa1,libupm1,mraa-tools,libdbus-1-3,pulseaudio,mpg123,mpv,gstreamer1.0-plugins-good,gstreamer1.0-plugins-bad,gstreamer1.0-plugins-ugly,gir1.2-gstreamer-1.0,python-gi,python-gst-1.0,python-pyaudio,librespeaker
sudo apt update
sudo apt install -y git pulseaudio python-mraa python-upm libmraa1 libupm1 mraa-tools libdbus-1-3 mpg123 mpv gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gir1.2-gstreamer-1.0 python-gi python-gst-1.0 python-pyaudio
#sudo apt install -y --reinstall librespeaker
sudo pip install avs pixel_ring voice-engine pydbus

sudo apt install -y --reinstall respeakerd

## Check if the udev rule applies
NEED_REBOOT=0
if [[ $(pactl list sources | grep -c alsa_input) == 0 ]] ; then
    echo "udev rules need reboot to be applied"
    NEED_REBOOT=1
fi

MICTYPE=CIRCULAR_6MIC
## Select Array Type for RPi
if [[ $PLATFORM == pi ]] ; then
    PS3='Please select the type of your microphone array: '
    options=("ReSpeaker 6 Mic Array for Raspberry Pi" "ReSpeaker 4 Mic Array for Raspberry Pi" "Others - not supported now")
    select opt in "${options[@]}" ; do
        case "$REPLY" in
            1)
                MICTYPE=CIRCULAR_6MIC
                break
                ;;
            2)
                MICTYPE=CIRCULAR_4MIC
                break
                ;;
            3)
                exit 1
                ;;
            *) echo "invalid option $REPLY";;
        esac
    done
fi

echo "Your microphone array type is: ${MICTYPE}"

sudo sed -i -e "s/mic_type = \(.*\)/mic_type = ${MICTYPE}/" /etc/respeaker/respeakerd.conf

sudo systemctl restart respeakerd

echo "The respeakerd services has been started."
echo "You can view it's log via:"
echo ""
echo "sudo journalctl -f -u respeakerd"
echo ""

H="/home/${DEFAULT_USER}"

cd $H

if [[ -e $H/respeakerd ]] ; then
    cd $H/respeakerd
    git pull
else
    git clone https://github.com/respeaker/respeakerd.git
fi


IP_ETH=`ip -f inet -br address|grep -v 'lo'|grep -v 'wlan'|awk '{print $3}'|sed -e 's/\/24//'`
IP_WLAN=`ip -f inet -br address|grep -v 'lo'|grep 'wlan'|awk '{print $3}'|sed -e 's/\/24//'`

echo "Before we can run the Alexa demo, we need you to do the authorization for the Alexa service."
echo "We need you to VNC connect to the board. If you haven't practiced on VNC operation, please refer to:"
if [[ $PLATFORM == axol ]] ; then
    echo "https://github.com/respeaker/get_started_with_respeaker/blob/master/docs/ReSpeaker_Core_V2/getting_started.md#2-vnc"
else
    echo "https://www.raspberrypi.org/documentation/remote-access/vnc/"
fi
echo ""
echo "The IP addresses of your board are:"
if [ x${IP_ETH} != x ]; then
    echo "- eth: ${IP_ETH}"
fi

if [ x${IP_WLAN} != x ]; then
    echo "- wlan: ${IP_WLAN}"
fi
echo ""

echo "------"
echo "Open the browser inside the VNC desktop, and go to 'http://127.0.0.1:3000'"
echo "Login with your Amazon account and authorize Alexa service."
echo "If you enabled 2FA, you need to login amazon.com first and then 'http://127.0.0.1:3000'"
echo "When you finish that, the script will continue, or press Ctrl+C if you've done this before"

alexa-auth 2>&1 > /dev/null

if [[ $PLATFORM == pi && ${NEED_REBOOT} == 1 ]] ; then
    echo "Please reboot first to apply the udev configurations for PulseAudio."
fi

echo "Run the Alexa demo via the following command, the trigger word is 'snowboy'"
echo ""
echo "python ${H}/respeakerd/clients/Python/demo_respeaker_v2_vep_alexa_with_light.py"
echo ""


