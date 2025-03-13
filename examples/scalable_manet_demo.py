#!/usr/bin/env python

"""
This example builds on the adhoc.py example file to create a custom topology
for mobile ad hoc networks (MANET). The number of stations are prompted and
inserted in a 100x100 grid in random locations. Users will be able to specify
the MANET Protocol used to communicate between stations.

To run the demo use:
sudo python scalable_manet_demo.py

If stations are unable to ping one another, exit out of the CLI, then
please use the following commands:
sudo systemctl stop network-manager
sudo systemctl start network-manager

Then restart the demo and it should solve the unreachable host error.
Contributer: Dane Oshiro
"""

import sys
import random

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, adhoc
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference



def topology(args):
#    "Create a network."
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)

    info("*** Creating nodes\n")
    kwargs = {}
    if '-a' in args:
        kwargs['range'] = 100

    list_stations = []
    try:
        numStations = int(input('    How many stations would you like: '))
    except SyntaxError:
        info('Please enter a number from 1-100\n')
    except NameError:
        info('Please enter a number from 1-100\n')
    except ValueError:
        info('Please enter a number from 1-100\n')
    except IndexError:
        info('Please enter a number from 1-100\n')
    
    info('    Station location is randomized in a 100x100 grid')
    info('    num stations=', numStations, '\n')
    for station in range(1, numStations+1):
        s_name = 'sta'+ str(station)
        s_addr = 'fe80::' + str(station)
        pos_x = random.randint(0,10) * 10
        pos_y = random.randint(0,10) * 10
        pos_z = random.randint(0,10) * 10
        pos = str(pos_x) + ',' + str(pos_y) +',' + str(pos_z)
        station = net.addStation(s_name, ip6=s_addr,
                                 position=pos, **kwargs)
        list_stations.append(station)


    net.setPropagationModel(model="logDistance", exp=4)

    info("*** Configuring nodes\n")
    net.configureNodes()

    info("*** Creating links\n")
    # MANET routing protocols supported by proto:
    # babel, batman_adv, batmand and olsr
    # WARNING: we may need to stop Network Manager if you want
    # to work with babel
    protocols = ['babel', 'batman_adv', 'batmand', 'olsrd', 'olsrd2']
    kwargs = {}
    for proto in args:
        if proto in protocols:
            kwargs['proto'] = proto

    station_count = 0
    proto_set = 0
    sta_proto = 0
    for station in list_stations:

        proto_list = '    1. babel\n    2. batman_adv\n    3. batmand\n    4. olsrd\n    5. olsrd2\n'
        done = 0
        if proto_set == 0:
            while done == 0:
                try :
                    sta_proto = int(input('    Enter number for ' +  str(station) +
                                          ' protocol desired for link:\n' + proto_list))
                    sta_proto = sta_proto - 1 #since the proto list starts at 1
                    done = 1
                except SyntaxError:
                    info('Please enter a number from 1-6\n')
                except NameError:
                    info('Please enter a number from 1-6\n')
                except ValueError:
                    info('Please enter a number from 1-6\n')
                except IndexError:
                    info('Please enter a number from 1-6\n')

            info('*** Link establishing, please wait.\n')            
            net.addLink(station, cls=adhoc, intf=(str(station) + "-wlan0"),
                        ssid='adhocNet',  proto=protocols[sta_proto], mode='g', channel=5,
                        ht_cap='HT40+', **kwargs)
            info('*** Link ', station, ' established.\n')



    net.plotGraph(max_x=100, max_y=100)
    
    info("*** Starting network\n")
    net.build()

    info("\n*** Addressing...\n")
    if 'proto' not in kwargs:
        sta_counter = 1
        for station in list_stations:
            info('setting station', station, '\n')
            station.setIP6('2001::'+ str(station) + '/64', 
                           intf=(str(station) + "-wlan0"))


    info("*** Running CLI\n")
    CLI(net)

    info("*** Stopping network\n")
    net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    topology(sys.argv)
manet_demo.py
Displaying manet_demo.py.
