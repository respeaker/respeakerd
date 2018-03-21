#!/usr/bin/env python
#! -*- coding: utf-8 -*-

import os
import time
from pydbus import SystemBus
from gi.repository import GLib
from pixel_ring import pixel_ring
import mraa
import signal

def setup_signals(signals, handler):
    """
    This is a workaround to signal.signal(signal, handler)
    which does not work with a GLib.MainLoop() for some reason.
    Thanks to: http://stackoverflow.com/a/26457317/5433146
    args:
        signals (list of signal.SIG... signals): the signals to connect to
        handler (function): function to be executed on these signals
    """
    def install_glib_handler(sig): # add a unix signal handler
        GLib.unix_signal_add( GLib.PRIORITY_HIGH, 
            sig, # for the given signal
            handler, # on this signal, run this function
            sig # with this argument
            )

    for sig in signals: # loop over all signals
        GLib.idle_add( # 'execute'
            install_glib_handler, sig, # add a handler for this signal
            priority = GLib.PRIORITY_HIGH  )

class StateMachine(object):

    state = 'thinking'
    last_dir = 0
    avs_ready = False
    respeakerd_ready = False

    def on_ready(self):
        print("===== on_ready =====")
        self.state = 'off'
        pixel_ring.off()

    def on_avs_ready(self):
        print("avs ready...")
        self.avs_ready = True
        if self.avs_ready and self.respeakerd_ready:
            self.on_ready()
    
    def on_respeakerd_ready(self):
        print("respeakerd ready...")
        self.respeakerd_ready = True
        if self.avs_ready and self.respeakerd_ready:
            self.on_ready()

    def on_detected(self, dir):
        print('===== on_detected =====\r\n@ {}`'.format(dir))
        if self.state in ['listening', 'thinking']:
            print('invalid trigger, reason: listening or thinking can not be interrupted!')
            return
        self.state = 'detected'
        self.last_dir = (dir + 360 - 60)%360
        pixel_ring.wakeup(self.last_dir)

    def on_listening(self):
        print("===== on_listening =====")
        if self.state != 'detected':
            print('pointing to last dir...')
            pixel_ring.wakeup(self.last_dir)
        print("")
        self.state = 'listening'
        pixel_ring.listen()

    def on_speaking(self):
        print("===== on_speaking =====")
        self.state = 'speaking'
        pixel_ring.speak()

    def on_thinking(self):
        print("===== on_thinking =====")
        self.state = 'thinking'
        pixel_ring.think()

    def on_idle(self):
        print("===== on_off =====")
        self.state = 'off'
        pixel_ring.off()
        
def main():

    # enable led power
    en = mraa.Gpio(12)
    if os.geteuid() != 0 :
        time.sleep(1)
    en.dir(mraa.DIR_OUT)
    en.write(0)

    pixel_ring.set_brightness(100)
    pixel_ring.think()

    sm = StateMachine();

    def bus_handler(sender, object, iface, signal, params):
        # print(sender)
        # print(object)
        # print(signal)
        # print(params)

        if signal == 'trigger':
            try:
                dir = params[0]
            except:
                dir = 0
            sm.on_detected(dir)
        elif signal == 'on_listen':
            sm.on_listening()
        elif signal == 'on_think':
            sm.on_thinking()
        elif signal == 'on_speak':
            sm.on_speaking()
        elif signal == 'on_idle':
            sm.on_idle()
        elif signal == 'connecting':
            sm.on_thinking()
        elif signal == 'ready':
            sm.on_avs_ready()
        elif signal == 'respeakerd_ready':
            sm.on_respeakerd_ready()

    system_bus = SystemBus()
    loop = GLib.MainLoop()
    sub = system_bus.subscribe(iface='respeakerd.signal', signal_fired=bus_handler)

    def on_exit(sig):
        sub.unsubscribe()
        loop.quit()
    
    setup_signals(
        signals = [signal.SIGINT, signal.SIGTERM, signal.SIGHUP],
        handler = on_exit
    )

    print("Running...")
    
    try:
        loop.run()
    except KeyboardInterrupt:
        pass
    finally:
        pixel_ring.off()

    print("Quit...")

    en.write(1)

if __name__ == '__main__':
    main()

