# -*- coding: utf-8 -*-

import sys
import threading
import logging
import base64

try:
    import Queue as queue
except ImportError:
    import queue

from voice_engine.element import Element
import respeakerd_client

logger = logging.getLogger(__name__)

class RespeakerdSource(Element):
    def __init__(self):
        super(RespeakerdSource, self).__init__()

        self.client = respeakerd_client.RespeakerdClient()
        self.done = False
        self.on_detected = None
        self.dir = 0
        self.event_queue = queue.Queue(maxsize=1000)


    def run(self):
        while not self.done:
            # process downlink event queue first
            while True:
                cmd = None
                try:
                    cmd = self.event_queue.get_nowait()
                except:
                    break
                if cmd:
                    self.client.blocking_send(cmd)

            # fetch uplink event and audio data
            msg = self.client.blocking_recv()
            if msg and type(msg) == dict and 'type' in msg and 'data' in msg:
                msg_type = msg['type']
                msg_data = msg['data']
                msg_dir = msg['direction'] if 'direction' in msg else 0
                if msg_type == 'event' and msg_data == 'hotword':
                    self.dir = msg_dir
                    if callable(self.on_detected):
                        self.on_detected(self.dir)
                elif msg_type == 'audio':
                    # this is a chunk of audio
                    decoded_data = None
                    try:
                        decoded_data = base64.b64decode(msg_data)
                    except:
                        logger.warn('base64 can not decode')

                    if decoded_data:
                        super(RespeakerdSource, self).put(decoded_data)


    def start(self):
        self.done = False
        self.client.connect()
        thread = threading.Thread(target=self.run)
        thread.daemon = True
        thread.start()


    def stop(self):
        self.done = True
        self.client.close()


    def is_active(self):
        return not self.done


    def stop_capture(self):
        command = {"cmd": "stop_capture", "cmd_data": ""}
        self.event_queue.put(command)


    def set_callback(self, callback):
        self.on_detected = callback
