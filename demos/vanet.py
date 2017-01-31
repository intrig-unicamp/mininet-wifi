# !/usr/bin/python

"""
This code illustrates a single case about Vehicular Ad-Hoc Networks. 
More detail about VANET implementation can be found at the paper titled
From Theory to Experimental Evaluation: Resource Management in Software-Defined Vehicular Networks
url: ...
Video clip available at:
"""

import os
import time
import matplotlib.pyplot as plt

from mininet.net import Mininet
from mininet.node import Controller, OVSKernelSwitch, OVSKernelAP
from mininet.link import TCLink
from mininet.log import setLogLevel
from mininet.cli import CLI

switch_pkt = 'switch-pkt.vanetdata'
switch_throughput = 'switch-throughput.vanetdata'
c0_pkt = 'c0-pkt.vanetdata'
c0_throughput = 'c0-throughput.vanetdata'

def graphic():

    f1 = open('./' + switch_pkt, 'r')
    s_pkt = f1.readlines()
    f1.close()

    f11 = open('./' + switch_throughput, 'r')
    s_throughput = f11.readlines()
    f11.close()

    f2 = open('./' + c0_pkt, 'r')
    c_pkt = f2.readlines()
    f2.close()

    f21 = open('./' + c0_throughput, 'r')
    c_throughput = f21.readlines()
    f21.close()

    # initialize some variable to be lists:
    time_ = []
    
    l1 = []
    l2 = []
    t1 = []
    t2 = []
        
    ll1 = []
    ll2 = []
    tt1 = []
    tt2 = []
    
    # scan the rows of the file stored in lines, and put the values into some variables:
    i = 0
    for x in s_pkt:    
        p = x.split()
        l1.append(int(p[0]))
        if len(l1) > 1:
            ll1.append(l1[i] - l1[i - 1])
        i += 1
    
    i = 0
    for x in s_throughput:    
        p = x.split()
        t1.append(int(p[0]))
        if len(t1) > 1:
            tt1.append(t1[i] - t1[i - 1])
        i += 1
    
    i = 0
    for x in c_pkt:    
        p = x.split()
        l2.append(int(p[0]))
        if len(l2) > 1:
            ll2.append(l2[i] - l2[i - 1])
        i += 1
    
    i = 0
    for x in c_throughput:    
        p = x.split()
        t2.append(int(p[0]))
        if len(t2) > 1:
            tt2.append(t2[i] - t2[i - 1])
        i += 1
    
    i = 0
    for x in range(len(ll1)):    
        time_.append(i)
        i = i + 0.5
    
    fig, ax1 = plt.subplots()
    ax2 = ax1.twinx()
    
    ax1.plot(time_, ll1, color='red', label='Received Data (client)', ls="--", markevery=7, linewidth=1)
    ax1.plot(time_, ll2, color='black', label='Transmited Data (server)', markevery=7, linewidth=1)
    ax2.plot(time_, tt1, color='red', label='Throughput (client)', ls="-.", markevery=7, linewidth=1)
    ax2.plot(time_, tt2, color='black', label='Throughput (server)', ls=':', markevery=7, linewidth=1)
    ax1.legend(loc=2, borderaxespad=0., fontsize=12)
    ax2.legend(loc=1, borderaxespad=0., fontsize=12)

    ax2.set_yscale('log')

    ax1.set_ylabel("# Packets (unit)", fontsize=18)
    ax1.set_xlabel("Time (seconds)", fontsize=18)
    ax2.set_ylabel("Throughput (bytes/sec)", fontsize=18)

    plt.show()
    plt.savefig("graphic.eps")

def topology():

    taskTime = 20

    "Create a network."
    net = Mininet(controller=Controller, link=TCLink, switch=OVSKernelSwitch, accessPoint=OVSKernelAP)

    print "*** Creating nodes"
    car = []
    stas = []
    for x in range(0, 4):
        car.append(x)
        stas.append(x)
    for x in range(0, 4):
        car[x] = net.addCar('car%s' % (x), wlans=2, ip='10.0.0.%s/8' % (x + 1), \
        mac='00:00:00:00:00:0%s' % x, mode='b', position='%d,%d,0' % ((120 - (x * 20)), (100 - (x * 0))))

    eNodeB1 = net.addAccessPoint('eNodeB1', ssid='eNodeB1', dpid='1000000000000000', mode='ac', channel='1', position='80,75,0')
    eNodeB2 = net.addAccessPoint('eNodeB2', ssid='eNodeB2', dpid='2000000000000000', mode='ac', channel='6', position='180,75,0')
    rsu1 = net.addAccessPoint('rsu1', ssid='rsu1', dpid='3000000000000000', mode='g', channel='11', position='140,120,0')
    c1 = net.addController('c1', controller=Controller)
    client = net.addHost ('client')
    switch = net.addSwitch ('switch', dpid='4000000000000000')

    net.plotNode(client, position='125,230,0')
    net.plotNode(switch, position='125,200,0')

    print "*** Configuring wifi nodes"
    net.configureWifiNodes()  

    print "*** Creating links"
    net.addLink(eNodeB1, switch)
    net.addLink(eNodeB2, switch)
    net.addLink(rsu1, switch)
    net.addLink(switch, client)
    net.addLink(rsu1, car[0])
    net.addLink(eNodeB2, car[0])
    net.addLink(eNodeB1, car[3])

    print "*** Starting network"
    net.build()
    c1.start()
    eNodeB1.start([c1])
    eNodeB2.start([c1])
    rsu1.start([c1])
    switch.start([c1])

    for sw in net.vehicles:
        sw.start([c1])

    i = 1
    j = 2
    for c in car:
        c.cmd('ifconfig %s-wlan0 192.168.0.%s/24 up' % (c, i))
        c.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (c, i))
        c.cmd('ip route add 10.0.0.0/8 via 192.168.1.%s' % j)
        i += 2
        j += 2

    i = 1
    j = 2
    for v in net.vehiclesSTA:
        v.cmd('ifconfig %s-eth0 192.168.1.%s/24 up' % (v, j))
        v.cmd('ifconfig %s-mp0 10.0.0.%s/24 up' % (v, i))
        v.cmd('echo 1 > /proc/sys/net/ipv4/ip_forward')
        i += 1
        j += 2

    for v1 in net.vehiclesSTA:
        i = 1
        j = 1
        for v2 in net.vehiclesSTA:
            if v1 != v2:
                v1.cmd('route add -host 192.168.1.%s gw 10.0.0.%s' % (j, i))
            i += 1
            j += 2

    client.cmd('ifconfig client-eth0 200.0.10.2')
    net.vehiclesSTA[3].cmd('ifconfig car3STA-eth1 200.0.10.1')
    net.vehiclesSTA[0].cmd('ifconfig car0STA-eth0 200.0.10.50')

    car[0].cmd('modprobe bonding mode=3')
    car[0].cmd('ip link add bond0 type bond')
    car[0].cmd('ip link set bond0 address 02:01:02:03:04:08')
    car[0].cmd('ip link set car0-eth0 down')
    car[0].cmd('ip link set car0-eth0 address 00:00:00:00:00:11')
    car[0].cmd('ip link set car0-eth0 master bond0')
    car[0].cmd('ip link set car0-wlan0 down')
    car[0].cmd('ip link set car0-wlan0 address 00:00:00:00:00:15')
    car[0].cmd('ip link set car0-wlan0 master bond0')
    car[0].cmd('ip link set car0-wlan1 down')
    car[0].cmd('ip link set car0-wlan1 address 00:00:00:00:00:13')
    car[0].cmd('ip link set car0-wlan1 master bond0')
    car[0].cmd('ip addr add 200.0.10.100/24 dev bond0')
    car[0].cmd('ip link set bond0 up')

    car[3].cmd('ifconfig car3-wlan0 200.0.10.150')

    client.cmd('ip route add 192.168.1.8 via 200.0.10.150')
    client.cmd('ip route add 10.0.0.1 via 200.0.10.150')

    net.vehiclesSTA[3].cmd('ip route add 200.0.10.2 via 192.168.1.7')
    net.vehiclesSTA[3].cmd('ip route add 200.0.10.100 via 10.0.0.1')
    net.vehiclesSTA[0].cmd('ip route add 200.0.10.2 via 10.0.0.4')

    car[0].cmd('ip route add 10.0.0.4 via 200.0.10.50')
    car[0].cmd('ip route add 192.168.1.7 via 200.0.10.50')
    car[0].cmd('ip route add 200.0.10.2 via 200.0.10.50')
    car[3].cmd('ip route add 200.0.10.100 via 192.168.1.8')

    """uncomment to plot graph"""
    net.plotGraph(max_x=250, max_y=250)

    net.startGraph()

    os.system('rm *.vanetdata')

    #os.system('xterm -hold -title "car0" -e "util/m car0 ping 200.0.10.2" &')
    car[0].cmdPrint("cvlc -vvv v4l2:///dev/video0 --mtu 1000 --sout \'#transcode{vcodec=mp4v,vb=800,scale=1,\
                acodec=mpga,ab=128,channels=1}: duplicate{dst=display,dst=rtp{sdp=rtsp://200.0.10.100:8080/helmet.sdp}\' &")
    client.cmdPrint("cvlc rtsp://200.0.10.100:8080/helmet.sdp &")

    os.system('ovs-ofctl mod-flows switch in_port=1,actions=drop')
    os.system('ovs-ofctl mod-flows switch in_port=2,actions=drop')
    os.system('ovs-ofctl mod-flows switch in_port=3,actions=drop')

    time.sleep(2)

    print "applying first rule"
    os.system('ovs-ofctl mod-flows switch in_port=1,actions=output:4')
    os.system('ovs-ofctl mod-flows switch in_port=4,actions=output:1')
    os.system('ovs-ofctl mod-flows switch in_port=2,actions=drop')
    os.system('ovs-ofctl mod-flows switch in_port=3,actions=drop')
    os.system('ovs-ofctl del-flows eNodeB1')
    os.system('ovs-ofctl del-flows eNodeB2')
    os.system('ovs-ofctl del-flows rsu1')

    car[0].cmd('ip route add 200.0.10.2 via 200.0.10.50')
    client.cmd('ip route add 200.0.10.100 via 200.0.10.150')

    timeout = time.time() + taskTime
    currentTime = time.time()
    i = 0
    while True:
        if time.time() > timeout:
            break;
        if time.time() - currentTime >= i:
            car[0].cmd('ifconfig bond0 | grep \"TX packets\" | awk -F\' \' \'{print $3}\' >> %s' % c0_pkt)
            client.cmd('ifconfig client-eth0 | grep \"RX packets\" | awk -F\' \' \'{print $3}\' >> %s' % switch_pkt)
            car[0].cmd('ifconfig bond0 | grep \"bytes\" | awk -F\' \' \'NR==2{print $5}\' >> %s' % c0_throughput)
            client.cmd('ifconfig client-eth0 | grep \"bytes\" | awk -F\' \' \'NR==1{print $5}\' >> %s' % switch_throughput)
            i += 0.5

    print "Moving nodes"    
    car[0].moveNodeTo('150,100,0')
    car[1].moveNodeTo('120,100,0')
    car[2].moveNodeTo('90,100,0')
    car[3].moveNodeTo('70,100,0')

    #time.sleep(3)

    print "applying second rule"
    os.system('ovs-ofctl mod-flows switch in_port=1,actions=drop')
    os.system('ovs-ofctl mod-flows switch in_port=2,actions=output:4')
    os.system('ovs-ofctl mod-flows switch in_port=4,actions=output:2,3')
    os.system('ovs-ofctl mod-flows switch in_port=3,actions=output:4')
    os.system('ovs-ofctl del-flows eNodeB1')
    os.system('ovs-ofctl del-flows eNodeB2')
    os.system('ovs-ofctl del-flows rsu1')

    car[0].cmd('ip route del 200.0.10.2 via 200.0.10.50')
    client.cmd('ip route del 200.0.10.100 via 200.0.10.150')

    timeout = time.time() + taskTime
    currentTime = time.time()
    i = 0
    while True:
        if time.time() > timeout:
            break;
        if time.time() - currentTime >= i:
            car[0].cmd('ifconfig bond0 | grep \"TX packets\" | awk -F\' \' \'{print $3}\' >> %s' % c0_pkt)
            client.cmd('ifconfig client-eth0 | grep \"RX packets\" | awk -F\' \' \'{print $3}\' >> %s' % switch_pkt)
            car[0].cmd('ifconfig bond0 | grep \"bytes\" | awk -F\' \' \'NR==2{print $5}\' >> %s' % c0_throughput)
            client.cmd('ifconfig client-eth0 | grep \"bytes\" | awk -F\' \' \'NR==1{print $5}\' >> %s' % switch_throughput)
            i += 0.5

    print "Moving nodes"
    car[0].moveNodeTo('190,100,0')
    car[1].moveNodeTo('150,100,0')
    car[2].moveNodeTo('120,100,0')
    car[3].moveNodeTo('90,100,0')

    #time.sleep(2)

    print "applying third rule"
    os.system('ovs-ofctl mod-flows switch in_port=1,actions=drop')
    os.system('ovs-ofctl mod-flows switch in_port=3,actions=drop')
    os.system('ovs-ofctl mod-flows switch in_port=2,actions=output:4')
    os.system('ovs-ofctl mod-flows switch in_port=4,actions=output:2')
    os.system('ovs-ofctl del-flows eNodeB1')
    os.system('ovs-ofctl del-flows eNodeB2')
    os.system('ovs-ofctl del-flows rsu1')

    timeout = time.time() + taskTime
    currentTime = time.time()
    i = 0
    while True:
        if time.time() > timeout:
            break;
        if time.time() - currentTime >= i:
            car[0].cmd('ifconfig bond0 | grep \"TX packets\" | awk -F\' \' \'{print $3}\' >> %s' % c0_pkt)
            client.cmd('ifconfig client-eth0 | grep \"RX packets\" | awk -F\' \' \'{print $3}\' >> %s' % switch_pkt)
            car[0].cmd('ifconfig bond0 | grep \"bytes\" | awk -F\' \' \'NR==2{print $5}\' >> %s' % c0_throughput)
            client.cmd('ifconfig client-eth0 | grep \"bytes\" | awk -F\' \' \'NR==1{print $5}\' >> %s' % switch_throughput)
            i += 0.5

    print "*** Generating graphic"
    graphic()

    os.system('pkill xterm')

    print "*** Running CLI"
    CLI(net)

    #os.system('rm *.vanetdata')
    
    print "*** Stopping network"
    net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    topology()
