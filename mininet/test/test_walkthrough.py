#!/usr/bin/env python

"""
Tests for the Mininet Walkthrough
TODO: missing xterm test
"""

import unittest
import pexpect
import os
import re
from time import sleep
from mininet.util import quietRun
from distutils.version import StrictVersion

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

    # def testWireshark( self ):
    #    "Use tshark to test the of dissector"
        # Satisfy pylint
    #    assert self
    #    if StrictVersion( tsharkVersion() ) < StrictVersion( '1.12.0' ):
    #        tshark = pexpect.spawn( 'tshark -i lo -R of' )
    #    else:
    #        tshark = pexpect.spawn( 'tshark -i lo -Y openflow_v1' )
    #    tshark.expect( [ 'Capturing on lo', "Capturing on 'Loopback'" ] )
    #    mn = pexpect.spawn( 'mn --wifi --test pingall' )
    #    sleep(3)
    #    mn.expect( '0% dropped' )
    #    tshark.expect( [ '74 Hello', '74 of_hello', '74 Type: OFPT_HELLO' ] )
    #    tshark.sendintr()

    # def testBasic( self ):
    #    "Test basic CLI commands (help, nodes, net, dump)"
    #    p = pexpect.spawn( 'mn --wifi' )
    #    sleep(3)
    #    p.expect( self.prompt )
        # help command
    #    p.sendline( 'help' )
    #    index = p.expect( [ 'commands', self.prompt ] )
    #    self.assertEqual( index, 0, 'No output for "help" command')
        # nodes command
    #    p.sendline( 'nodes' )
    #    p.expect( r'([chs]\d ?){4}' )
    #    nodes = p.match.group( 0 ).split()
    #    self.assertEqual( len( nodes ), 4, 'No nodes in "nodes" command')
    #    p.expect( self.prompt )
        # net command
    #    p.sendline( 'net' )
    #    expected = [ x for x in nodes ]
    #    while len( expected ) > 0:
    #        index = p.expect( expected )
    #        node = p.match.group( 0 )
    #        expected.remove( node )
    #        p.expect( '\n' )
    #    self.assertEqual( len( expected ), 0, '"nodes" and "net" differ')
    #    p.expect( self.prompt )
        # dump command
    #    p.sendline( 'dump' )
    #    expected = [ r'<\w+ (%s)' % n for n in nodes ]
    #    actual = []
    #    for _ in nodes:
    #        index = p.expect( expected )
    #        node = p.match.group( 1 )
    #        actual.append( node )
    #        p.expect( '\n' )
    #    self.assertEqual( actual.sort(), nodes.sort(),
    #                      '"nodes" and "dump" differ' )
    #    p.expect( self.prompt )
    #    p.sendline( 'exit' )
    #    p.wait()

    # def testHostCommands( self ):
    #    "Test ifconfig and ps on sta1 and ap1"
    #    p = pexpect.spawn( 'mn --wifi' )
    #    sleep(3)
    #    p.expect( self.prompt )
    #    interfaces = [ 'sta1-wlan0', 'ap1-wlan1', '[^-]wlan0', 'lo', self.prompt ]
        # sta1 ifconfig
    #    p.sendline( 'sta1 ifconfig -a' )
    #    ifcount = 0
    #    while True:
    #        index = p.expect( interfaces )
    #        if index == 0 or index == 3:
    #            ifcount += 1
    #        elif index == 1:
    #            self.fail( 'ap1 interface displayed in "sta1 ifconfig"' )
    #        elif index == 2:
    #            self.fail( 'wlan0 displayed in "sta1 ifconfig"' )
    #        else:
    #            break
    #    self.assertEqual( ifcount, 2, 'Missing interfaces on sta1')
        # ap1 ifconfig
    #    p.sendline( 'ap1 ifconfig -a' )
    #    ifcount = 0
    #    while True:
    #        index = p.expect( interfaces )
    #        if index == 0:
    #            self.fail( 'sta1 interface displayed in "ap1 ifconfig"' )
    #        elif index == 1 or index == 2 or index == 3:
    #            ifcount += 1
    #        else:
    #            break
    #    self.assertEqual( ifcount, 3, 'Missing interfaces on ap1')
        # sta1 ps
    #    p.sendline( "sta1 ps -a | egrep -v 'ps|grep'" )
    #    p.expect( self.prompt )
    #    h1Output = p.before
        # ap1 ps
    #    p.sendline( "ap1 ps -a | egrep -v 'ps|grep'" )
    #    p.expect( self.prompt )
   #     s1Output = p.before
        # strip command from ps output
    #    h1Output = h1Output.split( '\n', 1 )[ 1 ]
    #    s1Output = s1Output.split( '\n', 1 )[ 1 ]
    #    self.assertEqual( h1Output, s1Output, 'sta1 and ap1 "ps" output differs')
    #    p.sendline( 'exit' )
    #    p.wait()

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
        p.sendline('sta1 python -m SimpleHTTPServer 80 &')
        p.expect(self.prompt)
        p.sendline(' sta2 wget -O - sta1')
        p.expect('200 OK')
        p.expect(self.prompt)
        p.sendline('sta1 kill %python')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    # PART 2
    # def testRegressionRun( self ):
    #    "Test pingpair (0% drop) and iperf (bw > 0) regression tests"
        # test pingpair
    #    p = pexpect.spawn( 'mn --wifi --test pingpair' )
    #    sleep(3)
    #    p.expect( '0% dropped' )
    #    p.expect( pexpect.EOF )
        # test iperf
    #    p = pexpect.spawn( 'mn --wifi --test iperf' )
    #    sleep(3)
    #    p.expect( r"Results: \['([\d\.]+) .bits/sec'," )
    #    bw = float( p.match.group( 1 ) )
    #    self.assertTrue( bw > 0 )
    #    p.expect( pexpect.EOF )

    # def testTopoChange( self ):
    #    "Test pingall on single,3 and linear,4 topos"
        # testing single,3
    #    p = pexpect.spawn( 'mn --wifi --test pingall --topo single,3' )
    #    sleep(3)
    #    p.expect( r'(\d+)/(\d+) received')
    #    received = int( p.match.group( 1 ) )
    #    sent = int( p.match.group( 2 ) )
    #    self.assertEqual( sent, 6, 'Wrong number of pings sent in single,3' )
    #    self.assertEqual( sent, received, 'Dropped packets in single,3')
    #    p.expect( pexpect.EOF )
        # testing linear,4
    #    p = pexpect.spawn( 'mn --wifi --test pingall --topo linear,4' )
    #    sleep(3)
    #    p.expect( r'(\d+)/(\d+) received')
    #    received = int( p.match.group( 1 ) )
    #    sent = int( p.match.group( 2 ) )
    #    self.assertEqual( sent, 12, 'Wrong number of pings sent in linear,4' )
    #    self.assertEqual( sent, received, 'Dropped packets in linear,4')
    #    p.expect( pexpect.EOF )

    # def testLinkChange( self ):
    #    "Test TCLink bw and delay"
    #    p = pexpect.spawn( 'mn --wifi --link tc,bw=10,delay=10ms' )
        # test bw
    #    sleep(3)
    #    p.expect( self.prompt )
    #    p.sendline( 'iperf' )
    #    p.expect( r"Results: \['([\d\.]+) Mbits/sec'," )
    #    bw = float( p.match.group( 1 ) )
    #    self.assertTrue( bw < 10.1, 'Bandwidth > 10 Mb/s')
    #    self.assertTrue( bw > 9.0, 'Bandwidth < 9 Mb/s')
    #    p.expect( self.prompt )
        # test delay
    #    p.sendline( 'sta1 ping -c 4 sta2' )
    #    p.expect( r'rtt min/avg/max/mdev = '
    #              r'([\d\.]+)/([\d\.]+)/([\d\.]+)/([\d\.]+) ms' )
    #    delay = float( p.match.group( 2 ) )
    #    self.assertTrue( delay > 40, 'Delay < 40ms' )
    #    self.assertTrue( delay < 45, 'Delay > 40ms' )
    #    p.expect( self.prompt )
    #    p.sendline( 'exit' )
    #    p.wait()

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
        custom = os.path.join(custom, '../../custom/topo-2sw-2host.py')
        custom = os.path.normpath(custom)
        p = pexpect.spawn(
            'mn --wifi --custom %s --topo mytopo --test pingall' % custom)
        sleep(3)
        p.expect('0% dropped')
        p.expect(pexpect.EOF)
        
    def testMobility(self):
        "Start Mininet-WiFi using mobility, then test ping"
        p = pexpect.spawn(
            'python examples/wifiMobility.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        
    def testPropagationModel(self):
        "Start Mininet-WiFi using a propagation model, then test ping and rssi"
        p = pexpect.spawn(
            'python examples/wifiPropagationModel.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('py sta1.params[\'rssi\'][0]')
        p.expect(self.prompt)
        a = p.before
        sleep(1)
        p.sendline('py sta1.params[\'rssi\'][0]')
        p.expect(self.prompt)
        b = p.before
        if a != b:
            p.sendline('exit')
            p.wait()
        
    def testMesh(self):
        "Start Mininet-WiFi with wireless mesh, then test ping"
        p = pexpect.spawn(
            'python examples/wifimesh.py')
        sleep(2)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        
    def testAdhoc(self):
        "Start Mininet-WiFi with wireless adhoc, then test ping"
        p = pexpect.spawn(
            'python examples/adhoc.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        
    def testAuthentication(self):
        "Start Mininet-WiFi using WPA, then test ping"
        p = pexpect.spawn(
            'python examples/wifiAuthentication.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        
    def testHandover(self):
        "Start Mininet-WiFi with handover, then test handover"
        p = pexpect.spawn(
            'python examples/handover.py')
        sleep(3)
        p.sendline('sta1 iwconfig')
        p.expect('ssid-ap1')
        p.expect(self.prompt)
        sleep(30)
        p.sendline('sta1 iwconfig')
        p.expect('ssid-ap2')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        
    def testmultipleWlan(self):
        "Start Mininet-WiFi with multiple WLAN"
        p = pexpect.spawn(
            'python examples/multipleWlan.py')
        sleep(3)
        p.sendline('sta1 ifconfig sta1-wlan0')
        p.expect('sta1-wlan0')
        p.expect(self.prompt)
        p.sendline('sta1 ifconfig sta1-wlan1')
        p.expect('sta1-wlan1')
        p.expect(self.prompt)
        p.sendline('sta1 ifconfig sta1-wlan2')
        p.expect('sta1-wlan2')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        
    def testPosition(self):
        "Start Mininet-WiFi when the position is statically defined, then test ping"
        p = pexpect.spawn(
            'python examples/wifiPosition.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()
        
    def testWmediumdStatic(self):
        "Start Mininet-WiFi with wmediumd, then test ping"
        p = pexpect.spawn(
            'python examples/wmediumd_ibss_static.py')
        sleep(3)
        p.sendline('sta1 ping -c 1 sta2')
        p.expect('1 packets transmitted, 1 received')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    def testMultipleInstances(self):
        """Start Mininet-WiFi with multiple instances, then test ping"""
        p1 = pexpect.spawn(
            'python examples/wmediumd_multiple_instances.py')
        sleep(3)
        p1.sendline('sta1 ping -c 1 sta2')
        p1.expect('1 packets transmitted, 1 received')
        p1.expect(self.prompt)
        p1.sendline('sta1 ifconfig')
        p1.expect('02:00:00:00:00:00')
        p1.expect(self.prompt)
        p2 = pexpect.spawn(
            'python examples/wmediumd_multiple_instances.py')
        sleep(3)
        p2.sendline('sta1 ping -c 1 sta2')
        p2.expect('1 packets transmitted, 1 received')
        p2.expect(self.prompt)
        p2.sendline('sta1 ifconfig')
        p2.expect('02:00:00:00:03:00')
        p2.expect(self.prompt)
        p1.sendline('exit')
        p1.wait()
        p2.sendline('exit')
        p2.wait()
        os.system('rmmod mac80211_hwsim')
        
    def testDynamicMAC(self):
        "Verify that MACs are set correctly"
        p = pexpect.spawn('mn --wifi')
        sleep(3)
        p.expect(self.prompt)
        for i in range(1, 3):
            p.sendline('sta%d ifconfig' % i)
            p.expect('02:00:00:00:0%d:00' % (i - 1))
            p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

    # def testStaticMAC( self ):
    #    "Verify that MACs are set to easy to read numbers"
    #    p = pexpect.spawn( 'mn --wifi --mac' )
    #    sleep(3)
    #    p.expect( self.prompt )
    #    sleep(3)
    #    for i in range( 1, 3 ):
    #        p.sendline( 'sta%d ifconfig' % i )
    #        p.expect( 'HWaddr 02:00:00:00:00:0%d' % i )
    #        p.expect( self.prompt )

    def testAPs(self):
        "Run iperf test using user and ovsk aps"
        aps = [ 'user', 'ovsk' ]
        for ap in aps:
            p = pexpect.spawn('mn --wifi --switch %s --test iperf' % ap)
            p.expect(r"Results: \['([\d\.]+) .bits/sec',")
            bw = float(p.match.group(1))
            self.assertTrue(bw > 0)
            p.expect(pexpect.EOF)

    def testBenchmark(self):
        "Run benchmark and verify that it takes less than 2 seconds"
        p = pexpect.spawn('mn --wifi --test none')
        sleep(3)
        p.expect(r'completed in ([\d\.]+) seconds')
        time = float(p.match.group(1))
        self.assertTrue(time < 2, 'Benchmark takes more than 2 seconds')

    def testOwnNamespace( self ):
        "Test running user ap in its own namespace"
        p = pexpect.spawn( 'mn --wifi --innamespace --ap user' )
        sleep(3)
        p.expect( self.prompt )
        interfaces = [ 'sta1-wlan0', 'ap1-wlan1', '[^-]eth0', 'lo', self.prompt ]
        p.sendline( 'ap1 ifconfig -a' )
        ifcount = 0
        while True:
            index = p.expect( interfaces )
            if index == 1 or index == 3:
                ifcount += 1
            elif index == 0:
                self.fail( 'sta1 interface displayed in "ap1 ifconfig"' )
            elif index == 2:
                self.fail( 'wlan0 displayed in "ap1 ifconfig"' )
            else:
                break
        #self.assertEqual( ifcount, 2, 'Missing interfaces on ap1' )
        # verify that all stations a reachable
        p.sendline( 'pingall' )
        p.expect( r'(\d+)% dropped' )
        dropped = int( p.match.group( 1 ) )
        self.assertEqual( dropped, 0, 'pingall failed')
        p.expect( self.prompt )
        p.sendline( 'exit' )
        p.wait()

    # PART 3
    def testPythonInterpreter(self):
        "Test py and px by checking IP for sta1 and adding sta3"
        p = pexpect.spawn('mn --wifi')
        sleep(3)
        p.expect(self.prompt)
        # test host IP
        p.sendline('py sta1.IP()')
        p.expect('10.0.0.1')
        p.expect(self.prompt)
        # test adding host
        p.sendline("px net.addHost('sta3')")
        p.expect(self.prompt)
        p.sendline("px net.addLink(ap1, sta3)")
        p.expect(self.prompt)
        p.sendline('net')
        p.expect('sta3')
        p.expect(self.prompt)
        p.sendline('py sta3.MAC()')
        p.expect('([a-f0-9]{2}:?){6}')
        p.expect(self.prompt)
        p.sendline('exit')
        p.wait()

   # def testLink( self ):
   #     "Test link CLI command using ping"
   #     p = pexpect.spawn( 'mn --wifi' )
   #     sleep(3)
   #     p.expect( self.prompt )
   #     p.sendline( 'link ap1 sta1 down' )
   #     p.expect( self.prompt )
   #     p.sendline( 'sta1 ping -c 1 sta2' )
   #     p.expect( 'unreachable' )
   #     p.expect( self.prompt )
   #     p.sendline( 'link ap1 sta1 up' )
   #     p.expect( self.prompt )
   #     p.sendline( 'sta1 ping -c 1 sta2' )
   #     p.expect( '0% packet loss' )
   #     p.expect( self.prompt )
   #     p.sendline( 'exit' )
   #     p.wait()

    # @unittest.skipUnless( os.path.exists( '/tmp/pox' ) or
    #                      '1 received' in quietRun( 'ping -c 1 github.com' ),
    #                      'Github is not reachable; cannot download Pox' )
    # def testRemoteController( self ):
    #    "Test Mininet using Pox controller"
        # Satisfy pylint
    #    assert self
    #    if not os.path.exists( '/tmp/pox' ):
    #        p = pexpect.spawn(
    #            'git clone https://github.com/noxrepo/pox.git /tmp/pox' )
    #        p.expect( pexpect.EOF )
    #    pox = pexpect.spawn( '/tmp/pox/pox.py forwarding.l2_learning' )
    #    net = pexpect.spawn(
    #        'mn --wifi --controller=remote,ip=127.0.0.1,port=6633 --test pingall' )
    #    sleep(3)
    #    net.expect( '0% dropped' )
    #    net.expect( pexpect.EOF )
    #    pox.sendintr()
    #    pox.wait()


if __name__ == '__main__':
    unittest.main()
