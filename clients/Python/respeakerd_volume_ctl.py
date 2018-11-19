from pulsectl import Pulse, PulseVolumeInfo

class VolumeCtl(object):
    def __init__(self):
        self.pulse = Pulse('volume-controller')
        self.sinks = self.pulse.sink_list()
        self.seeed_sink = None
        for i in self.sinks:
            if "seeed" in i.name:
                self.seeed_sink = i
                break

    def setVolume(self, vol):
        if vol > 100:
            vol = 100
        elif vol < 0:
            vol = 0
        vol = vol / 100.0
        if self.seeed_sink:
            try:
                self.pulse.volume_set_all_chans(self.seeed_sink, vol)
            except Exception as e:
                print("Fail to set volume, Error:{}".format(e))
        else:
            print("Fail to find Seeed voicecard")

    def getVolume(self):
        if self.seeed_sink:
            vol = 100.0 * self.pulse.volume_get_all_chans(self.seeed_sink)
            return vol
        else:
            print("Fail to find Seeed voicecard") 
            return (0.0)

    def setMute(self, muted):
        if self.seeed_sink:
            try:
                self.pulse.mute(self.seeed_sink, muted)
            except Exception as e:
                print("Fail to set mute, Error:{}".format(e))
        else:
            print("Fail to find Seeed voicecard")

