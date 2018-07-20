# -*- coding: utf-8 -*-

import os
import time
import logging
import signal
import threading
from respeakerd_source import RespeakerdSource
from avs.alexa import Alexa
from pixel_ring import pixel_ring
import mraa
import sys


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

    state = 'thinking'
    last_dir = 0

    def on_ready():
        global state
        print("===== on_ready =====\r\n")
        state = 'off'
        pixel_ring.off()
        src.on_cloud_ready()

    def on_listening():
        global state
        global last_dir
        print("===== on_listening =====\r\n")
        if state != 'detected':
            print('The last dir is {}'.format(last_dir))
            pixel_ring.wakeup(last_dir)
        state = 'listening'
        pixel_ring.listen()

    def on_speaking():
        global state
        print("===== on_speaking =====\r\n")
        state = 'speaking'
        src.on_speak()
        pixel_ring.speak()

    def on_thinking():
        global state
        print("===== on_thinking =====\r\n")
        state = 'thinking'
        src.stop_capture()
        pixel_ring.think()

    def on_off():
        global state
        print("===== on_off =====\r\n")
        state = 'off'
        pixel_ring.off()

    def on_detected(dir, index):
        global state
        global last_dir
        logging.info('detected hotword:{} at {}`'.format(index, dir))
        state = 'detected'
        last_dir = (dir + 360 - 60)%360
        pixel_ring.wakeup(last_dir)
        alexa.listen()

    def on_vad():
        # when someone is talking
        print("."),
        sys.stdout.flush()

    def on_silence():
        # when it is silent 
        pass

    alexa.state_listener.on_listening = on_listening
    alexa.state_listener.on_thinking = on_thinking
    alexa.state_listener.on_speaking = on_speaking
    alexa.state_listener.on_finished = on_off
    alexa.state_listener.on_ready = on_ready

    src.set_callback(on_detected)
    src.set_vad_callback(on_vad)
    src.set_silence_callback(on_silence)

    src.recursive_start()

    is_quit = threading.Event()
    def signal_handler(signal, frame):
        print('Quit')
        is_quit.set()

    signal.signal(signal.SIGINT, signal_handler)

    while not is_quit.is_set():
        try:
            time.sleep(1)
        except SyntaxError:
            pass
        except NameError:
            pass

    src.recursive_stop()

    en.write(1)


if __name__ == '__main__':
    main()






