#!/usr/bin/python
import uberbus.moodlamp
import uberbus.hid
import time
import sys
import gobject
import threading
import colorsys

#lamps = ['wipptischlampen.local', 'wipplampelampen.local', 'wipplampen.local']
#lamps = ['kuechenzeile.local', 'kuechelampen.local']


hidname = sys.argv[1]
defaultlamp = sys.argv[2]
#lamps = sys.argv[2:]
lamps = []

hid = uberbus.hid.HID(hidname)
hid.connect(True)

bwstate = 0
colormode = 0
s = 0.5
v = 0.5
h = 0.5

r = g = b = 0
lamp = 0
#time when to reset the selected lamp to the default
lampselectiontimeout = 0
#refreshlcdevent = threading.Event()
#hid.lcd(0,0,"Resolving...    ")
#hid.lcd(0,1,"                ")

class HIDCallback(uberbus.hid.HIDCallback):
    def newUnsolicited(self, node, data):
        global lampselectiontimeout
        lampselectiontimeout = time.time()+60
        uberbus.hid.HIDCallback.newUnsolicited(self, node, data)

    def onButtonPressed(self, node, button):
        global lamp,lamps,colormode,bwstate,refreshlcdevent
        if button == 0:
            hid.clear(7-lamp)
            lamp = (lamp+1) % len(lamps)
            refreshlcd()
#            if lamp < len(lamps):
#                hid.lcd(0,0,lamps[lamp])
        elif button == 1:
            colormode = (colormode + 1) % 3
            refreshlcd()
#            if mode == 1:
#                mode = 2
#                hid.lcd(0,1,'Off   Mode Group')
#            elif mode == 2:
#                mode = 1
#                hid.lcd(0,1,'White Mode Group')
        elif button == 6:
            a = uberbus.moodlamp.Moodlamp(lamps[lamp],True)
            print "connecting to", lamps[lamp]
            a.connect()
            if bwstate:
                #a.setcolor(255,255,255)
                a.timedfade(0xff,0xff,0xff,.5)
            else:
                #a.setcolor(0,0,0)
                a.timedfade(0x00,0x00,0x00,.5)
            bwstate = not bwstate
            refreshlcd()
                
    def onAnalogChanged(self, node, channel, value):
        global h,s,v,r,g,b,colormode,lamp,lamps
        #value = (ord(rc[2]) << 8) + ord(rc[3]);
        if channel == 4:
            h = value/1024.
            #h = value * (360./1024.)
        elif channel == 5:
            s = value/1024.
            #v = 1. - value / 1024.
        elif channel == 6:
            v = value/1024.
            #s = value / 1024.
        
        
        if colormode == 0:
            (r,g,b) = colorsys.hsv_to_rgb(h,s,v) 
        elif colormode == 1:
            (r,g,b) = colorsys.hls_to_rgb(h,v,s) #hsvToRGB(h,s,v)
        else: 
            (r,g,b) = (h,s,v)
#       print "h=",h,", s=",s,", v=",v,", r=",r,", g=",g,", b=",b
        a = uberbus.moodlamp.Moodlamp(lamps[lamp],True)
        print "connecting to", lamps[lamp]
        a.connect()
        a.timedfade(int(r*255),int(g*255),int(b*255),.5)

def refreshlcd():
    global lamp,lamps,colormode,bwstate,lampselectiontimeout,refreshlcdevent
	
    resolveprogress = ' '

    #while True:
		#print "refreshlcd()"
	#refreshlcdevent.clear()
    if len(lamps) > 0:
		if lampselectiontimeout < time.time():
			colormode = 0
			lamp = 0

	#    if mode == 2:
	#        hid.lcd(0,1,'Off   Mode Group')
	#    elif mode == 1:
	#        hid.lcd(0,1,'White Mode Group')
		
		hid.lcd(0,0,lamps[lamp])

		if bwstate:
			bwcaption = "White"
		else:
			bwcaption = "Off  "

		if colormode == 0:
			colorcaption = "HSV"
		elif colormode == 1:
			colorcaption = "HSL"
		else:
			colorcaption = "RGB"
		
		hid.lcd(0,1,"%s %s  Group"%(bwcaption,colorcaption))
		#refreshlcdevent.wait(10)
    else:
		hid.lcd(0,0,"Resolving..%s    "%resolveprogress)
		progress = {
			'.': ' ',
			' ': '.',
			'V': '<',
			'<': '^',
			'^': '>',
			'>': 'V'
		}
		resolveprogress = progress.get(resolveprogress)
		#refreshlcdevent.wait(0.5)
    return True

class Resolver(uberbus.moodlamp.MoodlampResolver):
    def newNode(self, node, address, multicast):
        global lamp,lamps,defaultlamp
        if multicast == True:
            if len(lamps) == 0 or node == defaultlamp:
                # insert the node at position one but keep current selection
                lamps.insert(0,node)
                lamp = 0#(lamp + 1) % len(lamps)
                hid.lcd(0,0,lamps[lamp])
            else:
                lamps.append(node)
    
    def removedNode(self, node):
        if node in lamps:
            lamps.remove(node)
        lamp %= len(lamps)


def idlerefresh():
    refreshlcd()
    gobject.timeout_add(10000, idlerefresh)

Resolver(udp=True)
hid.listen(HIDCallback()) 
idlerefresh()
#refreshlcdthread = threading.Thread(group=None, target=idlerefresh, name="idlerefresh", args=(), kwargs={})
#refreshlcdthread.daemon = True
#refreshlcdthread.start()
hid.checkForever()

