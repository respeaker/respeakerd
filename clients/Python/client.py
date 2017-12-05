# -*- coding: utf-8 -*-

import sys
import os
import logging
import socket
import json

logger = logging.getLogger(__file__)

SOCKET_FILE = '/tmp/respeakerd.sock'

class RespeakerdClient(object):
    def __init__(self, timeout=1):
        self.timeout = timeout
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.stop = False
        self.buff = ''

    def connect(self):
        logger.info('Start to connect to the socket: {} ...'.format(SOCKET_FILE))
        try:
            self.sock.settimeout(self.timeout)
            self.sock.connect(SOCKET_FILE)
        except socket.error, msg:
            logger.error("Error when connect to the socket: {}".format(msg))
            sys.exit(1)
        except socket.timeout:
            logger.error('Timeout when connect to the socket')
            sys.exit(1)

    def close(self):
        self.stop = True
        self.sock.close()
    
    def blocking_send(self, json_obj):
        try:
            self.sock.sendall(json.dump(json_obj))
        except Exception, e:
            logger.error('Error when sendall: {}'.format(str(e)))
            return False
        
        return True

    def blocking_recv(self):
        line = self._cut_line()
        if line:
            return line

        while not self.stop:
            chunk = ''
            try:
                # will block to timeout
                chunk = self.sock.recv(16)
            except socket.timeout:
                continue
            except Exception, e:
                logger.error('Error when recv: {}'.format(str(e)))
                break

            self.buff += chunk
            line = self._cut_line()
            if line:
                json_obj = None
                try:
                    json_obj = json.loads(line)
                except:
                    logger.warn('Can not decode json: {}'.format(line))
                
                return json_obj

        # break by exceptions
        return None

            

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
    client.connect()
    while True:
        try:
            print client.blocking_recv()
        except KeyboardInterrupt:
            break
    client.close()

    

