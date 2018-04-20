#!/bin/bash

R=`grep "Image" /etc/issue.net | awk '{print $4}'`

if [ $(echo "$R < 20180107"|bc) = 1 ]; then
    echo "Please upgrade your system version to 20180107 or later"
    echo "Refer to the guide here: https://github.com/respeaker/get_started_with_respeaker/blob/master/docs/ReSpeaker_Core_V2/getting_started.md#image-installation"
    exit 1
fi

USER_ID=`id -u`

if [ ${USER_ID} != 1000 ]; then
    echo "Please run this script with user respeaker"
    exit 1
fi

## prepare PulseAudio
#rm -rf ~/.config/pulse/client.conf
#pulseaudio -k
#sleep 1

SEEED_8MIC_EXISTS=`pactl list sources|grep alsa_input.platform-sound_0.seeed-8ch -c`

if [ $SEEED_8MIC_EXISTS = 0 ]; then
    echo "PulseAudio device <alsa_input.platform-sound_0.seeed-8ch> not found"
    echo "Please make sure you are running this script on ReSpeaker Core v2"
    exit 1
fi

PULSE_SOURCE="alsa_input.platform-sound_0.seeed-8ch"

## Install deps
sudo apt update
sudo apt install -y --reinstall librespeaker
sudo apt install -y git cmake
sudo apt install -y python-mraa python-upm libmraa1 libupm1 mraa-tools
sudo pip install avs pixel_ring voice-engine pydbus

cd /home/respeaker
git clone https://github.com/respeaker/respeakerd.git

cd /home/respeaker/respeakerd

sudo cp -f build/respeakerd /usr/local/bin
sudo cp -f scripts/respeakerd_safe /usr/local/bin
sudo chmod a+x /usr/local/bin/respeakerd
sudo chmod a+x /usr/local/bin/respeakerd_safe
sudo mkdir -p /usr/local/etc/respeakerd
sudo cp -Rf build/resources /usr/local/etc/respeakerd/
sudo cp -f scripts/respeakerd.service /etc/systemd/system/

#enable system service
sudo systemctl enable respeakerd
#stop service in case user has run this script before
sudo systemctl stop respeakerd
#start the service
sudo systemctl start respeakerd

echo "The respeakerd services has been started."
echo "You can view it's log via:"
echo ""
echo "sudo journalctl -f -u respeakerd"
echo ""

IP_ETH=`ifconfig eth0|grep inet|grep -v inet6|awk '{print $2}'`
IP_WLAN=`ifconfig wlan0|grep inet|grep -v inet6|awk '{print $2}'`

echo "Before we can run the Alexa demo, we need you to do the authorization for the Alexa service."
echo "We need you to VNC connect to the board. If you haven't practiced on VNC operation, please refer to:"
echo "https://github.com/respeaker/get_started_with_respeaker/blob/master/docs/ReSpeaker_Core_V2/getting_started.md#2-vnc"
echo ""
echo "The IP addresses of your board are:"
if [ x${IP_ETH} != x ]; then
    echo "- eth0: ${IP_ETH}"
fi

if [ x${IP_WLAN} != x ]; then
    echo "- wlan0: ${IP_WLAN}"
fi
echo ""

echo "------"
echo "Open the browser inside the VNC desktop, and go to 'http://127.0.0.1:3000'"
echo "Login with your Amazon account and authorize Alexa service."
echo "When you finish that, the script will continue, or press Ctrl+C to continue"

alexa-auth 2>&1 > /dev/null

echo "Now run the Alexa demo via the following command, the trigger word is 'snowboy'"
echo ""
echo "python /home/respeaker/respeakerd/clients/Python/demo_respeaker_v2_vep_alexa_with_light.py"
echo ""


