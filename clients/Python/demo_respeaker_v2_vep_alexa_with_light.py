# -*- coding: utf-8 -*-

"""
# How to run?

## Better to run in python virtualenv
```
pip install virtualenv                                     # install virtualenv
python -m virtualenv --system-site-packages ~/env          # create python virtual environment
source ~/env/bin/activate                                  # activate python venv
```

## Voice-engine setup
https://github.com/respeaker/get_started_with_respeaker/blob/master/docs/ReSpeaker_Core_V2/getting_started.md#voice-engine-setting
```
sudo apt update
sudo apt install gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gir1.2-gstreamer-1.0 python-gi python-gst-1.0
sudo apt install libatlas-base-dev python-pyaudio

cd ~
git clone https://github.com/respeaker/respeaker_v2_eval.git
cd ~/respeaker_v2_eval

pip install ./webrtc*.whl
pip install ./snowboy*.whl
pip install avs
pip install voice-engine
```

## Install dependency - pixel_ring
```
git clone --depth 1 https://github.com/respeaker/pixel_ring.git
cd pixel_ring
pip install .
sudo apt install python-numpy
```

## Setup MRAA
https://github.com/respeaker/get_started_with_respeaker/blob/master/docs/ReSpeaker_Core_V2/mraa_upm.md#update--mraa-and-upm-libraries-to-latest-version
```

```

## Run
#run the script with sudo (to drive the pixel_ring)
sudo mraa-gpio set 12 0
python demo_respeaker_v2_vep_alexa_with_light.py

"""

import os
import time
import logging
from respeakerd_source import RespeakerdSource
from avs.alexa import Alexa
from pixel_ring import pixel_ring
import mraa


def main():
    logging.basicConfig(level=logging.DEBUG)
    #logging.getLogger('avs.alexa').setLevel(logging.INFO)
    logging.getLogger('hpack.hpack').setLevel(logging.INFO)

    en = mraa.Gpio(12)
    if os.geteuid() != 0 :
        time.sleep(1)
    en.dir(mraa.DIR_OUT)
    en.write(0)

    src = RespeakerdSource()
    alexa = Alexa()

    src.link(alexa)

    pixel_ring.think()

    def on_ready():
        print("===== on_ready =====\r\n")
        pixel_ring.off()
        src.on_cloud_ready()

    def on_listening():
        print("===== on_listening =====\r\n")
        pixel_ring.listen()

    def on_speaking():
        print("===== on_speaking =====\r\n")
        pixel_ring.speak()

    def on_thinking():
        print("===== on_thinking =====\r\n")
        src.stop_capture()
        pixel_ring.think()

    def on_off():
        print("===== on_off =====\r\n")
        pixel_ring.off()

    def on_detected(dir):
        logging.info('detected at {}`'.format(dir))
        pixel_ring.wakeup((dir + 30)%360)
        alexa.listen()

    alexa.state_listener.on_listening = on_listening
    alexa.state_listener.on_thinking = on_thinking
    alexa.state_listener.on_speaking = on_speaking
    alexa.state_listener.on_finished = on_off
    alexa.state_listener.on_ready = on_ready

    src.set_callback(on_detected)

    src.recursive_start()

    while True:
        try:
            time.sleep(1)
        except KeyboardInterrupt:
            break

    src.recursive_stop()

    en.write(1)


if __name__ == '__main__':
    main()






