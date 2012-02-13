import dbus, gobject, avahi
from dbus import DBusException
from dbus.mainloop.glib import DBusGMainLoop
import os

TYPE = '_moodlamp._udp'
HOST = '2001::1'
IIF = 'eth0.2064'
OIF = 'eth0'
forwardlist = []

def service_resolved(*args):
    print 'service resolved'
    print 'name:', args[2]
    print 'address:', args[7]
    address = args[7]

    if address[0:2] == 'ff' and ':' in address and not address in forwardlist:
        print "preparing", address, "to be forwarded"
        forwardlist.append(address)
        os.system('smcrouter -a ' + IIF + ' ' + HOST + ' ' + address + ' ' + OIF)

def print_error(*args):
    print 'error_handler'
    print args[0]
    
def newhandler(interface, protocol, name, stype, domain, flags):
    print "Found service '%s' type '%s' domain '%s' " % (name, stype, domain)
    #print interface, protocol, name, stype, domain, flags

    server.ResolveService(interface, protocol, name, stype, 
        domain, avahi.PROTO_UNSPEC, dbus.UInt32(0), 
        reply_handler=service_resolved, error_handler=print_error)

def removehandler(interface, protocol, name, stype, domain, flags):
    print "Removed service '%s' type '%s' domain '%s' " % (name, stype, domain)
    print interface, protocol, name, stype, domain, flags


loop = DBusGMainLoop()

bus = dbus.SystemBus(mainloop=loop)

server = dbus.Interface( bus.get_object(avahi.DBUS_NAME, '/'),
        'org.freedesktop.Avahi.Server')

sbrowser = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
        server.ServiceBrowserNew(avahi.IF_UNSPEC,
            avahi.PROTO_UNSPEC, TYPE, 'local', dbus.UInt32(0))),
        avahi.DBUS_INTERFACE_SERVICE_BROWSER)

sbrowser.connect_to_signal("ItemNew", newhandler)
#sbrowser.connect_to_signal("ItemRemove", removehandler)

# add dummy ipv4 address on interface?
os.system('ifconfig eth0 10.0.0.1')
# start smcrouter daemon?
os.system('smcrouter -r')
os.system('smcrouter -D')
gobject.MainLoop().run()
