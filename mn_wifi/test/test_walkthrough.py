#!/usr/bin/env python

"""
Tests for the Mininet Walkthrough
TODO: missing xterm test
"""

import unittest
import os
import re
from time import sleep
from mininet.util import quietRun
import pexpect
from sys import version_info as py_version_info


def tsharkVersion():
    "Return tshark version"
    versionStr = quietRun('tshark -v')
    versionMatch = re.findall(r'TShark \d+.\d+.\d+', versionStr)[0]
    return versionMatch.split()[ 1 ]

# pylint doesn't understand pexpect.match, unfortunately!
# pylint:disable=maybe-no-member

class testWalkthrough(unittest.TestCase):
    "Test Mininet walkthrough"

    prompt = 'mininet-wifi>'

    # PART 1
    def testHelp(self):
        "Check the usage message"
        p = pexpect.spawn('mn --wifi -h')
        sleep(3)
        index = p.expect([ 'Usage: mn', pexpect.EOF ])
        self.assertEqual(index, 0)

    def testBasic(self):
        "Test basic CLI commands (help, nodes, net, dump)"
        p = pexpect.spawn('mn --wifi')
        sleep(3)
        p.expect(self.prompt)
        # help command
        p.sendline('help')
        index = p.expect([ 'commands', self.prompt ])
        self.assertEqual(index, 0, 'No output for "help" command')
        # nodes command
        p.sendline('nodes')
        p.expect(r'(ap\B\d ?){1}([c]\d ?){1}(sta\B\d ?){2}')
        nodes = p.match.group(0).split()
        self.assertEqual(len(nodes), 4, 'No nodes in "nodes" command')
        p.expect(self.prompt)
        # net command
        p.sendline('net')
        expected = [ x for x in nodes ]
        while len(expected) > 0:
            index = p.expect(expected)
            node = p.match.group(0)
            expected.remove(node)
            p.expect('\n')
        self.assertEqual(len(expected), 0, '"nodes" and "net" differ')
        p.expect(self.prompt)
        # dump command
        p.sendline('dump')
        expected = [ r'<\w+ (%s)' % n for n in nodes ]
        actual = []
        for _ in nodes:
            index = p.expect(expected)
            node = p.match.group(1)
            actual.append(node)
            p.expect('\n')
        self.assertEqual(actual.sort(), nodes.sort(),
                         '"nodes" and "dump" differ')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        p = pexpect.spawn('mn --wifi --position')
        sleep(3)
        p.expect(self.prompt)
        p.sendline('py sta1.params[\'position\']')
        p.expect('[1.0, 0.0, 0.0]')
        p.expect(self.prompt)
        p.sendline('py sta2.params[\'position\']')
        p.expect('[2.0, 0.0, 0.0]')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testConnectivity(self):
        "Test ping and pingall"
        p = pexpect.spawn('mn --wifi')
        p.expect(self.prompt)
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('pingall')
        p.expect('0% dropped')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testSimpleHTTP(self):
        "Start an HTTP server on sta1 and wget from sta2"
        p = pexpect.spawn('mn --wifi')
        p.expect(self.prompt)
        sleep(3)
        if py_version_info < (3, 0):
            p.sendline('sta1 python -m SimpleHTTPServer 80 &')
        else:
            p.sendline('sta1 python -m http.server 80 &')
        p.expect(self.prompt)
        p.sendline(' sta2 wget -O - sta1')
        p.expect('200 OK')
        p.expect(self.prompt)
        p.sendline('sta1 kill %python')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    # PART 2
    def testRegressionRun(self):
        "Test pingpair (0% drop) and iperf (bw > 0) regression tests"
        # test pingpair
        p = pexpect.spawn('mn --wifi --test pingpair')
        p.expect('0% dropped')
        p.expect(pexpect.EOF)
        # test iperf
        p = pexpect.spawn('mn --wifi --test iperf')
        p.expect(r"Results: \['([\d\.]+) .bits/sec',")
        bw = float(p.match.group(1))
        self.assertTrue(bw > 0)
        p.expect(pexpect.EOF)

    def testTopoChange(self):
        "Test pingall on single,3 and linear,4 topos"
        # testing single,3
        p = pexpect.spawn('mn --wifi --test pingall --topo single,3')
        p.expect(r'(\d+)/(\d+) received')
        received = int(p.match.group(1))
        sent = int(p.match.group(2))
        self.assertEqual(sent, 6, 'Wrong number of pings sent in single,3')
        self.assertEqual(sent, received, 'Dropped packets in single,3')
        p.expect(pexpect.EOF)
        # testing linear,4
        p = pexpect.spawn('mn --wifi --test pingall --topo linear,4')
        p.expect(r'(\d+)/(\d+) received')
        received = int(p.match.group(1))
        sent = int(p.match.group(2))
        self.assertTrue(sent > 10)  # it should be 12, but there is a delay
        # for association
        p.expect(pexpect.EOF)

    def testLinkChange(self):
        "Test TCLink bw and delay"
        p = pexpect.spawn('mn --wifi --link wtc,bw=10,delay=10ms')
        # test bw
        sleep(3)
        p.expect(self.prompt)
        p.sendline('iperf')
        p.expect(r"Results: \['([\d\.]+) Mbits/sec',")
        bw = float(p.match.group(1))
        self.assertTrue(bw < 10.1, 'Bandwidth > 10 Mb/s')
        self.assertTrue(bw > 9.0, 'Bandwidth < 9 Mb/s')
        p.expect(self.prompt)
        # test delay
        p.sendline('sta1 ping -c 4 sta2')
        p.expect(r'rtt min/avg/max/mdev = '
                 r'([\d\.]+)/([\d\.]+)/([\d\.]+)/([\d\.]+) ms')
        delay = float(p.match.group(2))
        self.assertTrue(delay > 20, 'Delay < 20ms')
        self.assertTrue(delay < 25, 'Delay > 20ms')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testVerbosity(self):
        "Test debug and output verbosity"
        # test output
        p = pexpect.spawn('mn --wifi -v output')
        p.expect(self.prompt)
        sleep(3)
        self.assertEqual(len(p.before), 0, 'Too much output for "output"')
        p.sendline('exit')
        p.wait()
        # test debug
        p = pexpect.spawn('mn --wifi -v debug --test none')
        sleep(3)
        p.expect(pexpect.EOF)
        lines = p.before.split('\n')
        self.assertTrue(len(lines) > 70, "Debug output is too short")

    def testCustomTopo(self):
        "Start Mininet using a custom topo, then run pingall"
        # Satisfy pylint
        assert self
        custom = os.path.dirname(os.path.realpath(__file__))
        custom = os.path.join(custom, '../../custom/topo-2ap-2sta.py')
        custom = os.path.normpath(custom)
        p = pexpect.spawn(
            'mn --wifi --custom %s --topo mytopo --test pingall' % custom)
        sleep(3)
        p.expect('0% dropped')
        p.expect(pexpect.EOF)

    def testMobility(self):
        "Start Mininet-WiFi using mobility, then test ping"
        p = pexpect.spawn(
            'python examples/mobility.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testVirtualIface(self):
        "Start Mininet-WiFi using simplewifitopology, then test vif"
        p = pexpect.spawn(
            'python examples/simplewifitopology.py -v')
        sleep(3)
        wlans = [ 'sta1-wlan0', 'sta1-wlan01', 'sta1-wlan02', self.prompt ]
        p.sendline('py sta1.params[\'wlan\']')
        p.expect(wlans)
        p.sendline('exit')
        p.wait()

    def testVanet(self):
        "Start Mininet-WiFi using vanet example"
        p = pexpect.spawn(
            'python examples/vanet.py')
        sleep(8)
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testSixLoWPan(self):
        "Start Mininet-WiFi using sixlowpan, then test pingall"
        p = pexpect.spawn(
            'python examples/6LoWPan.py')
        sleep(4)
        p.sendline('pingall')
        p.expect('0% dropped')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testPropagationModel(self):
        "Start Mininet-WiFi using a propagation model, then test ping and rssi"
        p = pexpect.spawn(
            'python examples/propagationModel.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testWiFiDirect(self):
        "Start Mininet-WiFi using wifi direct, then test ping"
        p = pexpect.spawn(
            'python examples/wifiDirect.py')
        sleep(15)
        p.sendline('pingall')
        p.expect('packets transmitted')
        p.expect(self.prompt)
        p.sendline('sta1 ping -c1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testMesh(self):
        "Start Mininet-WiFi with wireless mesh, then test ping"
        p = pexpect.spawn(
            'python examples/mesh.py')
        sleep(2)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('pingall')
        p.expect('0% dropped')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testAdhoc(self):
        "Start Mininet-WiFi with wireless adhoc, then test ping"
        p = pexpect.spawn(
            'python examples/adhoc.py')
        sleep(12)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('sta3 ping -c 1 sta3')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('sta1 ping -c 1 sta3')
        p.expect('1 packets transmitted, 0 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        p = pexpect.spawn(
            'python examples/adhoc.py -a')
        sleep(3)
        p.sendline('py sta1.params[\'range\']')
        p.expect('100')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testAuthentication(self):
        "Start Mininet-WiFi using WPA, then test ping"
        p = pexpect.spawn(
            'python examples/authentication.py')
        sleep(3)
        p.sendline('sta1 ping -c1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testReplayingMobility(self):
        "Start Mininet-WiFi using Replaying Mobility, then test ping"
        p = pexpect.spawn(
            'python examples/replaying/replayingMobility.py')
        sleep(15)
        p.sendline('sta1 ping -c1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('sta3 ping -c1 sta4')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testHandover(self):
        "Start Mininet-WiFi with handover, then test handover"
        p = pexpect.spawn(
            'python examples/handover.py')
        sleep(2)
        p.sendline('sta1 iw dev sta1-wlan0 info | grep ssid')
        p.expect('ssid-ap1')
        p.expect(self.prompt)
        sleep(8)
        p.sendline('sta1 iw dev sta1-wlan0 info | grep ssid')
        p.expect('ssid-ap2')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testmultipleWlan(self):
        "Start Mininet-WiFi with multiple WLAN"
        p = pexpect.spawn(
            'python examples/multipleWlan.py')
        sleep(3)
        p.sendline('sta1 ip addr show sta1-wlan0')
        p.expect('sta1-wlan0')
        p.expect(self.prompt)
        p.sendline('sta1 ip addr show sta1-wlan1')
        p.expect('sta1-wlan1')
        p.expect(self.prompt)
        p.sendline('sta1 ip addr show sta1-wlan2')
        p.expect('sta1-wlan2')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testPosition(self):
        """Start Mininet-WiFi when the position is statically defined,
        then test ping"""
        p = pexpect.spawn(
            'python examples/position.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testBgscan(self):
        "Start Mininet-WiFi, then test bgscan"
        p = pexpect.spawn(
            'python examples/handover_bgscan.py')
        sleep(5)
        p.sendline('sta1 iw dev sta1-wlan0 link | grep Connected')
        p.expect('00:00:00:00:00:01')
        p.expect(self.prompt)
        p.sendline('py sta1.setPosition(\'80,40,0\')')
        sleep(15)
        p.sendline('sta1 iw dev sta1-wlan0 link | grep Connected')
        p.expect('00:00:00:00:00:02')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def test4addr(self):
        "Start Mininet-WiFi with 4addr, then test connectivity"
        pexpect.spawn(
            'service network-manager stop')
        p = pexpect.spawn(
            'python examples/4address.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testMutipleSSID(self):
        "Start Mininet-WiFi with multiple SSIDs, then test connectivity"
        pexpect.spawn(
            'service network-manager stop')
        p = pexpect.spawn(
            'python examples/forwardingBySSID.py')
        sleep(3)
        p.sendline('sta1 iw dev sta1-wlan0 info | grep ssid')
        p.expect('ssid1')
        p.expect(self.prompt)
        p.sendline('sta2 iw dev sta2-wlan0 info | grep ssid')
        p.expect('ssid2')
        p.expect(self.prompt)
        p.sendline('sta3 iw dev sta3-wlan0 info | grep ssid')
        p.expect('ssid2')
        p.expect(self.prompt)
        p.sendline('sta4 iw dev sta4-wlan0 info | grep ssid')
        p.expect('ssid3')
        p.expect(self.prompt)
        p.sendline('sta5 iw dev sta5-wlan0 info | grep ssid')
        p.expect('ssid4')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testMeshAP(self):
        "Start Mininet-WiFi, then test wifiMeshAP.py"
        p = pexpect.spawn(
            'python examples/meshAP.py')
        sleep(3)
        p.sendline('sta1 ping -c1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testErrorProb(self):
        "Start Mininet-WiFi, then test wmediumd_error_prob.py"
        p = pexpect.spawn(
            'python examples/wmediumd_error_prob.py')
        sleep(4)
        p.sendline('sta1 ping -c1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('sta1 ping -c1 sta3')
        p.expect('1 packets transmitted, 0 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testWirelessParams(self):
        """Start Mininet-WiFi with sta in ap mode,
        then do an extensive test"""
        p = pexpect.spawn(
            'python examples/sta_ap_mode.py')
        sleep(5)
        p.sendline('sta1 iw dev sta1-wlan0 link | grep Connected')
        p.expect('02:00:00:00:01:00')
        p.expect(self.prompt)
        p.sendline('sta2 iw dev sta2-wlan0 link | grep Connected')
        p.expect('02:00:00:00:02:00')
        p.expect(self.prompt)
        p.sendline('py sta1.params[\'associatedTo\']')
        p.expect('Station ap1: ap1-wlan0:192.168.0.10')
        p.expect(self.prompt)
        p.sendline('py sta1.params[\'apsInRange\']')
        p.expect('Station ap1: ap1-wlan0:192.168.0.10')
        p.expect(self.prompt)
        p.sendline('py sta2.params[\'associatedTo\']')
        p.expect('Station ap2: ap2-wlan0:192.168.1.10')
        p.expect(self.prompt)
        p.sendline('py sta2.params[\'apsInRange\']')
        p.expect('Station ap2: ap2-wlan0:192.168.1.10')
        p.expect(self.prompt)
        p.sendline('net')
        p.expect('ap1 ap1-wlan0:wifi ap1-eth1:ap2-eth1')
        p.expect(self.prompt)
        stations = [ 'Station sta1: sta1-wlan0:10.0.0.1', self.prompt ]
        p.sendline('py ap1.params[\'associatedStations\']')
        p.expect(stations)
        p.sendline('py ap1.params[\'stationsInRange\']')
        p.expect(stations)
        stations = [ 'Station sta2: sta2-wlan0:10.0.0.2', self.prompt ]
        p.sendline('py ap2.params[\'associatedStations\']')
        p.expect(stations)
        p.sendline('py ap2.params[\'stationsInRange\']')
        p.expect(stations)
        p.sendline('py sta1.setPosition(\'100,40,0\')')
        sleep(4)
        p.sendline('py sta1.params[\'associatedTo\']')
        p.expect('Station ap2: ap2-wlan0:192.168.1.10')
        p.expect(self.prompt)
        p.sendline('py sta1.params[\'apsInRange\']')
        p.expect('Station ap2: ap2-wlan0:192.168.1.10')
        p.expect(self.prompt)
        stations = [ 'Station sta1: sta1-wlan0:10.0.0.1',
                     'Station sta2: sta2-wlan0:10.0.0.2', self.prompt ]
        p.sendline('py ap2.params[\'associatedStations\']')
        p.expect(stations)
        p.sendline('py ap2.params[\'stationsInRange\']')
        p.expect(stations)
        stations = [ self.prompt ]
        p.sendline('py ap1.params[\'associatedStations\']')
        p.expect(stations)
        p.sendline('py ap1.params[\'stationsInRange\']')
        p.expect(stations)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('0% packet loss')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        p = pexpect.spawn(
            'python examples/sta_ap_mode.py -m')
        sleep(10)
        p.sendline('py sta1.params[\'associatedTo\']')
        p.expect('Station ap2: ap2-wlan0:192.168.1.10')
        p.expect(self.prompt)
        accessPoints = [ 'Station ap2: ap2-wlan0:192.168.1.10', self.prompt ]
        p.sendline('py sta1.params[\'apsInRange\']')
        p.expect(accessPoints)
        stations = [ 'Station sta1: sta1-wlan0:10.0.0.1',
                     'Station sta2: sta2-wlan0:10.0.0.2', self.prompt ]
        p.sendline('py ap2.params[\'stationsInRange\']')
        p.expect(stations)
        p.sendline('py ap2.params[\'associatedStations\']')
        p.expect(stations)
        p.sendline('exit')
        p.wait()

    def testDynamicMAC(self):
        "Verify that MACs are set correctly"
        p = pexpect.spawn('mn --wifi')
        sleep(3)
        p.expect(self.prompt)
        for i in range(1, 3):
            p.sendline('sta%d ip addr show' % i)
            p.expect('02:00:00:00:0%d:00' % (i - 1))
            p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testStaticMAC(self):
        "Verify that MACs are set to easy to read numbers"
        p = pexpect.spawn('mn --wifi --mac')
        sleep(3)
        p.expect(self.prompt)
        for i in range(1, 3):
            p.sendline('sta%d ip addr show' % i)
            p.expect('00:00:00:00:00:0%d' % i)
            p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testAPs(self):
        "Run iperf test using user and ovsk aps"
        aps = [ 'user', 'ovsk' ]
        for ap in aps:
            p = pexpect.spawn('mn --wifi --ap %s --test iperf' % ap)
            p.expect(r"Results: \['([\d\.]+) .bits/sec',")
            bw = float(p.match.group(1))
            self.assertTrue(bw > 0)
            p.expect(pexpect.EOF)

    def testBenchmark(self):
        "Run benchmark and verify that it takes less than 4 seconds"
        p = pexpect.spawn('mn --wifi --test none')
        sleep(3)
        p.expect(r'completed in ([\d\.]+) seconds')
        time = float(p.match.group(1))
        self.assertTrue(time < 4, 'Benchmark takes more than 4 seconds')

    def testOwnNamespace(self):
        "Test running user ap in its own namespace"
        pexpect.spawn('mn -c')
        p = pexpect.spawn('mn --wifi --innamespace --ap user')
        sleep(3)
        p.expect(self.prompt)
        interfaces = [ 'sta1-wlan0', 'ap1-wlan1', '[^-]eth0', 'lo',
                       self.prompt ]
        p.sendline('ap1 ip addr show')
        ifcount = 0
        while True:
            index = p.expect(interfaces)
            if index == 1 or index == 3:
                ifcount += 1
            elif index == 0:
                self.fail('sta1 interface displayed in "ap1 ip addr show"')
            elif index == 2:
                self.fail('wlan0 displayed in "ap1 ip addr show"')
            else:
                break
        # self.assertEqual( ifcount, 2, 'Missing interfaces on ap1' )
        # verify that all stations a reachable
        p.sendline('pingall')
        p.expect(r'(\d+)% dropped')
        dropped = int(p.match.group(1))
        self.assertEqual(dropped, 0, 'pingall failed')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    # PART 3
    # def testPythonInterpreter(self):
    #    "Test py and px by checking IP for sta1 and adding sta3"
    #    p = pexpect.spawn('mn --wifi')
    #    sleep(3)
    #    p.expect(self.prompt)
    #    # test host IP
    #    p.sendline('py sta1.IP()')
    #    p.expect('10.0.0.1')
    #    p.expect(self.prompt)
        # test adding host
    #    p.sendline("px net.addStation('sta3')")
    #    p.expect(self.prompt)
    #    p.sendline("px net.addLink(ap1, sta3)")
    #    p.expect(self.prompt)
    #    p.sendline('net')
    #    p.expect('sta3')
    #    p.expect(self.prompt)
    #    p.sendline('py sta3.MAC()')
    #    p.expect('([a-f0-9]{2}:?){6}')
    #    p.expect(self.prompt)
    #    p.sendline('exit')
    #    p.wait()

    def testLink(self):
        "Test link CLI command using ping"
        p = pexpect.spawn('mn --wifi')
        sleep(3)
        p.expect(self.prompt)
        p.sendline('link ap1 sta1 down')
        p.expect(self.prompt)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('100% packet loss')
        p.expect(self.prompt)
        p.sendline('link ap1 sta1 up')
        p.expect(self.prompt)
        sleep(2)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('0% packet loss')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()


if __name__ == '__main__':
    unittest.main()
