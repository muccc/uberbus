import ubnode

class DigitalOutput(ubnode.UBNode):
    udptype = '_digitaloutput._udp'
    tcptype = '_digitaloutput._tcp'

    def __init__(self, address):
        ubnode.UBNode.__init__(self,address,2311)

    def set(self, pin):
        cmd = "s %s 1"%pin
        return self.sendCommand(cmd)

    def clear(self, pin):
        cmd = "s %s 0"%pin
        return self.sendCommand(cmd)

    def get(self, pin):
        cmd = "g %s"%pin
        if self.sendCommand(cmd):
            return self.getMessage()
        else:
            return False

class DigitalOutputResolver(ubresolver.UBResolver):
    def __init__(self, udp=False):
        ubresolver.UBResolver.__init__(self, DigitalOutput, udp)

