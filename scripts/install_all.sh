#!/bin/bash

PLATFORM=axol
DEFAULT_USER=respeaker
if grep -q 'Raspberry Pi' /proc/device-tree/model; then
    PLATFORM=pi
    DEFAULT_USER=pi
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

if [[ $(whoami) != ${DEFAULT_USER} ]] ; then
    echo "Please run this script with user ${DEFAULT_USER}"
    exit 1
fi

## remove old installations
if [[ -e /usr/local/bin/respeakerd ]]; then
    sudo rm -rf /usr/local/bin/respeakerd*
fi

sudo systemctl is-active -q respeakerd && sudo systemctl stop respeakerd


## Install deps
# python-mraa,python-upm,libmraa1,libupm1,mraa-tools,libdbus-1-3,pulseaudio,mpg123,mpv,gstreamer1.0-plugins-good,gstreamer1.0-plugins-bad,gstreamer1.0-plugins-ugly,gir1.2-gstreamer-1.0,python-gi,python-gst-1.0,python-pyaudio,librespeaker
sudo apt-get update
sudo apt-get install -y git pulseaudio python-mraa python-upm libmraa1 libupm1 mraa-tools libdbus-1-3 mpg123 mpv gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gir1.2-gstreamer-1.0 python-gi python-gst-1.0 python-pyaudio
#sudo apt install -y --reinstall librespeaker
sudo pip install avs pixel_ring voice-engine pydbus

sudo apt-get install -y --reinstall respeakerd

H="/home/${DEFAULT_USER}"

if [[ -e $H/.config/pulse/client.conf ]]; then
    rm -rf $H/.config/pulse/client.conf
fi

if [[ $PLATFORM == axol && `grep -c "default-sample-format = float32le"` == 0 ]] ; then
    DAEMON_CONF=/etc/pulse/daemon.conf
    sudo sed -i '/default-sample-format/c\default-sample-format = float32le' ${DAEMON_CONF}
    sudo sed -i '/default-sample-rate/c\default-sample-rate = 48000' ${DAEMON_CONF}
    pulseaudio -k
    pactl info
fi

if [[ $PLATFORM == pi ]] ; then
    ## Check if PulseAudio has been configured right
    FOUND=`pactl list sources | grep -c -E "Name:.*seeed-(8ch|source)"`
    if [[ $FOUND == 0 ]]; then
        echo "Please use \"sudo respeakerd-pi-tools setup-pulse\"  to configure PulseAudio first."
        exit 1
    fi

    ## Select array type
    respeakerd-pi-tools select-array
fi

sudo systemctl start respeakerd

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

echo "Run the Alexa demo via the following command, the trigger word is 'snowboy'"
echo ""
echo "python ${H}/respeakerd/clients/Python/demo_respeaker_v2_vep_alexa_with_light.py"
echo ""


