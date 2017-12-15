# -*- coding: utf-8 -*-

import sys
import time
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

ST_IDLE = 1
ST_CONN = 2

MESSAGES = {
    "ready": {"type": "status", "data": "ready"},
    "connecting": {"type": "status", "data": "connecting"},
    "stop_capture": {"type": "cmd", "data": "stop_capture"}
}

class RespeakerdSource(Element):
    def __init__(self):
        super(RespeakerdSource, self).__init__()

        self.client = respeakerd_client.RespeakerdClient()
        self.client_state = ST_IDLE
        self.done = False
        self.on_detected = None
        self.dir = 0
        self.event_queue = queue.Queue(maxsize=1000)
        self.cloud_state = MESSAGES['connecting']
        self.timer = threading.Timer(1, self.timer_proc)
        self.timer.start()

    def timer_proc(self):
        if self.client_state != ST_IDLE:
            self.event_queue.put(self.cloud_state)
        self.timer = threading.Timer(1, self.timer_proc)
        self.timer.start()


    def run(self):
        while not self.done:
            if self.client_state == ST_IDLE:
                if self.client.connect():
                    self.client_state = ST_CONN
                    while not self.event_queue.empty():
                        self.event_queue.get_nowait()
                    self.event_queue.put(MESSAGES['ready'])
                else:
                    time.sleep(1)
            else:
                # process downlink event queue first
                need_reconn = False
                while not self.event_queue.empty():
                    cmd = None
                    try:
                        cmd = self.event_queue.get_nowait()
                    except:
                        break
                    if cmd:
                        try:
                            self.client.send(cmd)
                        except respeakerd_client.DisconnectException:
                            self.client.close()
                            self.client_state = ST_IDLE
                            need_reconn = True
                            break
                if need_reconn:
                    continue

                # fetch uplink event and audio data
                msg = None
                try:
                    msg = self.client.try_get_json()
                    # heart beat
                    self.client.send([0])
                except respeakerd_client.DisconnectException:
                    self.client.close()
                    self.client_state = ST_IDLE
                    continue

                if not msg:
                    time.sleep(0.01)
                    continue
                if type(msg) == dict and 'type' in msg and 'data' in msg:
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
        self.client_state = ST_IDLE
        self.ready_state = False
        thread = threading.Thread(target=self.run)
        thread.daemon = True
        thread.start()


    def stop(self):
        self.done = True
        self.client.close()
        self.timer.cancel()

    def is_active(self):
        return not self.done


    def stop_capture(self):
        self.event_queue.put(MESSAGES['stop_capture'])

    def on_cloud_ready(self):
        self.cloud_state = MESSAGES['ready']

    def on_disconnected(self):
        self.cloud_state = MESSAGES['connecting']

    def set_callback(self, callback):
        self.on_detected = callback


