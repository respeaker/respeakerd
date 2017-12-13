# -*- coding: utf-8 -*-

import sys
import os
import errno
import logging
import socket
import json
import threading
import time

logger = logging.getLogger(__name__)

SOCKET_FILE = '/tmp/respeakerd.sock'
SOCKET_TIMEOUT = 1    # seconds

class DisconnectException(Exception):
    pass

class RespeakerdClient(object):
    def __init__(self, timeout=None):
        if timeout:
            self.timeout = timeout
        else:
            self.timeout = SOCKET_TIMEOUT
        
        self.sock = None
        self.lock = threading.Lock()    # protect self.sock object
        self.stop = False
        self.buff = ''

    def connect(self):
        logger.info('Start to connect to the socket: {} ...'.format(SOCKET_FILE))
        try:
            with self.lock:
                self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                self.sock.setblocking(0)
                self.sock.connect(SOCKET_FILE)
        except socket.error, msg:
            logger.error("Error when connect to the socket: {}".format(msg))
            self.stop = True
            return False
        except socket.timeout:
            logger.error('Timeout when connect to the socket')
            self.stop = True
            return False

        logger.info('Connected to socket')
        self.stop = False
        return True

    def close(self):
        self.stop = True
        with self.lock:
            logger.info("going to close the socket...")
            self.sock.close()
    
    def send(self, json_obj):
        try:
            with self.lock:
                self.sock.sendall(json.dumps(json_obj) + "\r\n")
        except socket.error as e:
            if e.errno == 32:
                raise DisconnectException
            if e.errno == 11:
                return
            else:
                logger.error('Other socket error when send data: #{} {}'.format(e.errno, e.strerror))
        except Exception as e:
            logger.error('Uncatched error when send data: {}'.format(str(e)))


    def recv_all(self):
        while not self.stop:
            chunk = ''
            try:
                with self.lock:
                    # will block to timeout
                    chunk = self.sock.recv(16)
            except socket.timeout:
                logger.debug('Recv timeout')
                break
            except socket.error as e:
                if e.errno == 32:
                    raise DisconnectException
                elif e.errno == 11:
                    # Resource temporarily unavailable
                    break
                else:
                    logger.error('Other socket error when recv data: #{} {}'.format(e.errno, e.strerror))
                    break
            except Exception as e:
                logger.error('Uncatched error when recv: {}'.format(str(e)))
                break

            self.buff += chunk


    def try_get_json(self):
        
        self.recv_all()

        if len(self.buff) < 2:
            return None

        json_obj = None
        line = self._cut_line()
        if line:
            try:
                json_obj = json.loads(line)
            except:
                logger.warn('Can not decode json: {}'.format(line))
            
        return json_obj

    def _cut_line(self):
        line = ''
        index = self.buff.find('\r\n')
        if index > -1:
            line = self.buff[:index+2]
            self.buff = self.buff[index+2:]
            line = line.strip('\r\n')

        return line


if __name__ == "__main__":

    logging.basicConfig(level=logging.DEBUG)
    
    client = RespeakerdClient()
    if not client.connect():
        sys.exit()
    
    while True:
        try:
            print client.try_get_json()
            client.send([0])
            time.sleep(0.1)
        except KeyboardInterrupt:
            break
    client.close()

    

