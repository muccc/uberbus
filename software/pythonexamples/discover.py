#!/usr/bin/python
import uberbus.moodlamp
import gobject

class Resolver(uberbus.moodlamp.MoodlampResolver):
    def newNode(self, nodeinfo):
        print 'resolved: ', nodeinfo.node, nodeinfo.address, nodeinfo.port, nodeinfo.multicast
        if nodeinfo.multicast == True:
            m = uberbus.moodlamp.Moodlamp(nodeinfo.address, udp=True)
            m.connect()
            m.setcolor(0,0,255)

Resolver(udp=True)
gobject.MainLoop().run()
