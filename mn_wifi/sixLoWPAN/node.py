"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import re
from time import sleep

from mininet.log import debug, error, info
from mininet.link import Intf
from mininet.node import OVSSwitch
from mininet.util import BaseString, getincrementaldecoder, Python3, \
    errRun, quietRun
from mininet.moduledeps import pathCheck
from mn_wifi.node import Node_wifi, UserAP
from mn_wifi.sixLoWPAN.link import IntfSixLoWPAN

from re import findall


class LowPANNode(Node_wifi):
    """A virtual network node is simply a shell in a network namespace.
       We communicate with it using pipes."""

    portBase = 0  # Nodes always start with eth0/port0, even in OF 1.0

    def __init__(self, name, inNamespace=True, **params):
        """name: name of node
           inNamespace: in network namespace?
           privateDirs: list of private directory strings or tuples
           params: Node parameters (see config() for details)"""

        # Make sure class actually works
        self.checkSetup()

        if 'position' in params:
            self.position = params.get('position')
        self.name = params.get('name', name)
        self.privateDirs = params.get('privateDirs', [])
        self.inNamespace = params.get('inNamespace', inNamespace)

        # Python 3 complains if we don't wait for shell exit
        self.waitExited = params.get('waitExited', Python3)

        # Stash configuration parameters for future reference
        self.params = params

        self.intfs = {}  # dict of port numbers to interfaces
        self.wintfs = {}  # dict of port numbers to interfaces
        self.ports = {}  # dict of interfaces to port numbers
        self.wports = {}  # dict of interfaces to port numbers
        self.nameToIntf = {}  # dict of interface names to Intfs

        # Make pylint happy
        (self.shell, self.execed, self.pid, self.stdin, self.stdout,
         self.lastPid, self.lastCmd, self.pollOut) = (
            None, None, None, None, None, None, None, None)
        self.waiting = False
        self.readbuf = ''

        # Incremental decoder for buffered reading
        self.decoder = getincrementaldecoder()

        # Start command interpreter shell
        self.master, self.slave = None, None  # pylint
        self.startShell()
        self.mountPrivateDirs()

    # File descriptor to node mapping support
    # Class variables and methods
    inToNode = {}  # mapping of input fds to nodes
    outToNode = {}  # mapping of output fds to nodes

    # Convenience and configuration methods
    def setIP6(self, ip, prefixLen=64, intf=None, **kwargs):
        """Set the IP address for an interface.
           intf: intf or intf name
           ip: IP address as a string
           kwargs: any additional arguments for intf.setIP"""
        return self.intf(intf).setIP6(ip, prefixLen, **kwargs)

    def IP6(self, intf=None):
        "Return IP address of a node or specific interface."
        return self.intf(intf).IP6()

    def configRPLD(self):
        pk = "A3A6197FE1496816A40689A300E3492A6683A1082A79E8AE9D42A17FFA8EF6AC6B785080C056C3D9536721F41113302B8D4328A188A2A302677D761AB6B67A73708481A2201FE50266D1746678786A4BC080187E0A352B2D693C51543CDE0C013B02134B643ECCF01B40B734FE630CE7E9008708CC0898118DCCC8FD9B648FA6589CE8516B46B909715E2263C731637ED367B75A600456927707B781F5D3B2E23168F3E17BB1C0685664B121098893E5250F1A949B110D69C44601902B4CD7A3B6A35FC10B78B397C7CC6A1EC77686EE12C6D88693FAB7132DF684D1947E8ED82818C06F3208BD2DDA339D2505B0BC5C10FC3F37450FD26A4B89F659378094E1C5313A7275D3E2AAB804B414B1C297E804577AB3AE024FE17872BB5AA1925873843679C4FB838AC035E3B3255A0CB7A0A16103A17E97F894C66732EEFC6DCC85CBD60B9642F5A2D2B1706D98721DA0B7F630373E01C78B35937F9C50AC2484F0840BA0D9AAB4861E00808150A616E783213A7A36A0C858A4A299B855C304988C36E10477CB92DB1AAAE9F766BCB101DD80AB84D7044B5904E6925D6E1616D0F37BB77B0641056D14DA5C2F4A16E0A25492C868BDCA56F0C73888C683719BB8B97A2AF5487866B4AD65DC3C82549D9D7861E1BC518DA3B83333B4E15C69128715A41944F688C44704B6AE93AD4C32A85EB862CA7832EB852DF9D53FEEB51B99AC73ADDB4DA94809ABD5B7EA295F1E96983CB61CD2B3AFC33CCA6990555B0352D50A957617CA859A1853E3725FD84711595C1FA423EEF04C7AC68A2EF7A9518C29E513A80AAC7641C34A1519297B0339E72A5F2D502691CA981E368CF77538A12C930B3AC48DAB60B9ECCED1A972891C7CE69ABE759077BDF18F8EC1B4B1DC8EFBB03001A517F4910F6F0711DD573D65F9537833C7EE7B6CDD849E66B016C0E437172A2AEDEB75D0082856131FE787580EF48282D752F6B45159634E00C017CFC539F8BA8EEC6A4CF5E4164C444C12D92ADC397A7189CD431B5FC0F64A320A648235A2BBF5C192BC12D5B54B34F42E1AE9709DB41637F80B6EF1C15E25A3281B3A19BC62C131B73D20A4536B5DD7E412AC726B644C8E606826DEF7A8A6D29EA3928FA9F7C7B4A28E19645C71A9A0D2D40561CA346EF7BC95077030E84DB47C7C7D4164F64822586731B4397672400C4A3AA5FA321248086A57F16B66594CCC452EDDB884B533000BF2AD5C6B363DC396B8B3502A15AC514654577301D4859E18647946B07BC5C48533C2604311A08ED4B7F883BBA4C056741BC1A83116C3280D54083585A4054CD9727B9940BD2BCAF7A2CA4552CC347890034C327069C9EEB222DB204D2EA7C4AF907BA23C62FA1113002574E2507BD5DB9106A130DD138B8827A7489235B2820D83B43F3C746B02D44E78370A1FD717FA1364F3A1912AE79A2DA5540622B686F0A5561C7EDA8739BA53AD38014CAF781261F1AF28F3C5A4DA06019C2969A73E56906A6CE22CFE30BE85EA4B591C1A44863D292445909568F7D3BC192014B0BACE4ECCC888203EFB9570FC4A6B47D7A4F1D30CC770ACA809BB105225C15C0CAD62309E505B73B0825BA85CF5DACFD099AAF8DC47D4A4B97C6CB46CE20DB15E5A2355C45677884F16BD7D69789D0A853D94ADC2659D329CF79519"

        sk="423710DE5B6D3D1046D32520C33045742A2E11682AAF840BCAA43ECF489E2F94BC9BE0512D6433F993A91AFB18B383C655F126EE4954708C7BD9603190F3CA37307E0FF345FB65800CD8BA4B2A2F5704406220A6F0216F0CA98B05089ECA16AC264A303F974431C20BEC6B1E5671603FD7050E6B9839700EF78B6BBA07A9A3B69F36CC52F2D76097F50CC5160562570BEC009B225039B9DAAD96375089E62CBFB18DB035BB12E08EB7633C5DAAB37725032E426DF9EBC8B2E93EC5AA45602B5B0C2BA4C6A56CE8093344981DDFE9BF84F175D15031317241069480E5E0B148F75F2C19503CF125EE660EB069553648502650CF416B5E661471353595D8FCCE99337DC0FCC440D59867EC56A7D70F3D1772179A9FA4603504551CF29CAAF1F11187097B10B7362A62602474BCBCF5C690756DF4F05B226801B025953C9A1E0C004DA017C7AA101946568F2FE05D1A74A828A81FA663435E8C76BAF6CD0F3CB9435481E5416236A3AC4262B431B87A8EC58D672A12F2C223477833074C08BBB9B1FEC1C40A28AD0F905E529A9C7041311060A80ABB0C3D0A774FC28614D24ED0909635D217B8440CC72B5A5A2B13755A8A85B05F45150E69F398CA796F3919748BA7119CF657307425E4FA88C3583ABBF5C92C5A83A63311B70C25D0E14B498B298C83C2176C44625205E179479D238151CC6C10E0444A211716DA5325679EE37067E75201C3789D5C62937679087E700D91708C845886E58BA5E67A3BEE908E68FA3DD7A56FCE32005DDA59610B7A855212758663680599A93608DB4506E2310AD89798D85377ACE80956EA3D074857B76060DAFA96043BC609843AF4A6BE570964CB249921FB54232C0E4A090CB66CAC3905CA3699B68227680ED4278BFB9B2996AC075B0B10CC003C512589733B1D6B61B34BAC8C875D20883AD06953EFB09DCA61AC0432205B581246891E0B880E77FC4A56B50D7AD09A70A4548BDB126B996A248484D885C07929C773D96471D703B8E3B158012D06F2B8DC3B3CC5246E396AB9C54B48DC71326D1227C0F7394F961F24B0403952BF7D32B54F6044F1FA50CCF85DA0A6204C49B89358307F73AF9786AFFF6A39C25407087A49AC87073F595BDA902D81EBC9E886C76C70BB59F279F639B80290C6F5509EA7301EDB632B2227762E7A337B0B9E3E9162CD41426693B0F9C279CBAAC8ACE2B5B4ECCECC21CD423470D2205B72C3AA95E4290FCA806A7B45031464D177839A909C2DCA63A308B06AC6A24616A3E7D23B9440B74621900E0A96B7C3A8EDD186BC718354C3709E462D5158106834CD2894B7E6383CDAACBE235193C21A93B5F74ED99222CF9CC95281C7D2B01A75253FBD138FE480AB10F11A098CB4D6FC4FF4FB6A42D6388C20945544A08C074A08A639CF13254268685CD9967F80A51E96BB1CBC9F2206844025345F9907DC1A70C37A9EC859367CCBA7960045B77A0C8BB8672CF42E9DD5C1A0D868C1053144366E295512DB9A3B60A931FA6835D777A0D0155CC6EABC42735C9373B2F688280F53BF8DF5628715BFC1F90EFB7544B95771A350CBC32525A5A332C9180E32722756B0B701C410BAB7B3C02B31DE01A109AB925A960C543142A3A6197FE1496816A40689A300E3492A6683A1082A79E8AE9D42A17FFA8EF6AC6B785080C056C3D9536721F41113302B8D4328A188A2A302677D761AB6B67A73708481A2201FE50266D1746678786A4BC080187E0A352B2D693C51543CDE0C013B02134B643ECCF01B40B734FE630CE7E9008708CC0898118DCCC8FD9B648FA6589CE8516B46B909715E2263C731637ED367B75A600456927707B781F5D3B2E23168F3E17BB1C0685664B121098893E5250F1A949B110D69C44601902B4CD7A3B6A35FC10B78B397C7CC6A1EC77686EE12C6D88693FAB7132DF684D1947E8ED82818C06F3208BD2DDA339D2505B0BC5C10FC3F37450FD26A4B89F659378094E1C5313A7275D3E2AAB804B414B1C297E804577AB3AE024FE17872BB5AA1925873843679C4FB838AC035E3B3255A0CB7A0A16103A17E97F894C66732EEFC6DCC85CBD60B9642F5A2D2B1706D98721DA0B7F630373E01C78B35937F9C50AC2484F0840BA0D9AAB4861E00808150A616E783213A7A36A0C858A4A299B855C304988C36E10477CB92DB1AAAE9F766BCB101DD80AB84D7044B5904E6925D6E1616D0F37BB77B0641056D14DA5C2F4A16E0A25492C868BDCA56F0C73888C683719BB8B97A2AF5487866B4AD65DC3C82549D9D7861E1BC518DA3B83333B4E15C69128715A41944F688C44704B6AE93AD4C32A85EB862CA7832EB852DF9D53FEEB51B99AC73ADDB4DA94809ABD5B7EA295F1E96983CB61CD2B3AFC33CCA6990555B0352D50A957617CA859A1853E3725FD84711595C1FA423EEF04C7AC68A2EF7A9518C29E513A80AAC7641C34A1519297B0339E72A5F2D502691CA981E368CF77538A12C930B3AC48DAB60B9ECCED1A972891C7CE69ABE759077BDF18F8EC1B4B1DC8EFBB03001A517F4910F6F0711DD573D65F9537833C7EE7B6CDD849E66B016C0E437172A2AEDEB75D0082856131FE787580EF48282D752F6B45159634E00C017CFC539F8BA8EEC6A4CF5E4164C444C12D92ADC397A7189CD431B5FC0F64A320A648235A2BBF5C192BC12D5B54B34F42E1AE9709DB41637F80B6EF1C15E25A3281B3A19BC62C131B73D20A4536B5DD7E412AC726B644C8E606826DEF7A8A6D29EA3928FA9F7C7B4A28E19645C71A9A0D2D40561CA346EF7BC95077030E84DB47C7C7D4164F64822586731B4397672400C4A3AA5FA321248086A57F16B66594CCC452EDDB884B533000BF2AD5C6B363DC396B8B3502A15AC514654577301D4859E18647946B07BC5C48533C2604311A08ED4B7F883BBA4C056741BC1A83116C3280D54083585A4054CD9727B9940BD2BCAF7A2CA4552CC347890034C327069C9EEB222DB204D2EA7C4AF907BA23C62FA1113002574E2507BD5DB9106A130DD138B8827A7489235B2820D83B43F3C746B02D44E78370A1FD717FA1364F3A1912AE79A2DA5540622B686F0A5561C7EDA8739BA53AD38014CAF781261F1AF28F3C5A4DA06019C2969A73E56906A6CE22CFE30BE85EA4B591C1A44863D292445909568F7D3BC192014B0BACE4ECCC888203EFB9570FC4A6B47D7A4F1D30CC770ACA809BB105225C15C0CAD62309E505B73B0825BA85CF5DACFD099AAF8DC47D4A4B97C6CB46CE20DB15E5A2355C45677884F16BD7D69789D0A853D94ADC2659D329CF795197A49DEA26F62961DEF8A3FCFC679A40BFAA5603E601B848F62C03A1DA67C8202EA58BFB97444D278E61D54AC063A0D5FB52DF9600AECA72579F5F529FCB8B792"

        cmd = 'ifaces = { {\n'
        cmd += '        ifname = \"{}-pan0\",\n'.format(self.name)
        if 'storing_mode' not in self.params:
            self.params['storing_mode'] = 2
        if 'trickle_t' not in self.params:
            self.params['trickle_t'] = 1
        if 'public_key' not in self.params:
            self.params['public_key'] = pk
            cmd += '        public_key = \"{}\",\n'.format(self.params['public_key'])
        if 'secret_key' not in self.params:
            self.params['secret_key'] = sk
            cmd += '        secret_key = \"{}\",\n'.format(self.params['secret_key'])

        if 'dodag_root' in self.params and self.params['dodag_root']:
            cmd += '        dodag_root = true,\n'
        else:
            cmd += '        dodag_root = false,\n'
        cmd += '        mode_of_operation = {},\n'.format(self.params['storing_mode'])
        cmd += '        trickle_t = {},\n'.format(self.params['trickle_t'])
        if 'dodag_root' in self.params and self.params['dodag_root']:
            cmd += '        rpls = { {\n'
            cmd += '               instance = 1,\n'
            cmd += '               dags = { {\n'
            cmd += '                       mode_of_operation = {},\n'.format(self.params['storing_mode'])
            cmd += '                       dest_prefix = \"fd3c:be8a:173f:8e80::/64\",\n'
            cmd += '                       dodagid = \"fd3c:be8a:173f:8e80::1\",\n'
            cmd += '               }, }\n'
            cmd += '        }, }\n'
        cmd += '}, }'
        
        self.pexec('echo \'{}\' > lowpan-{}.conf'.format(cmd, self.name), shell=True)
        self.runRPLD()

    def runRPLD(self):
        self.cmd('nohup rpld -C lowpan-{}.conf > /dev/null 2>&1 &'.format(self.name))

    def setDefault6Route(self, intf=None):
        """Set the default ipv6 route to go through intf.
           intf: Intf or {dev <intfname> via <gw-ip> ...}"""
        # Note setParam won't call us if intf is none
        if isinstance(intf, BaseString) and ' ' in intf:
            params = intf
        else:
            params = 'dev %s' % intf
        # Do this in one line in case we're messing with the root namespace
        self.cmd('ip -6 route del default; ip -6 route add default', params)

    def config(self, mac=None, ip=None, ip6=None,
               defaultRoute=None, lo='up', **_params):
        """Configure Node according to (optional) parameters:
           mac: MAC address for default interface
           ip: IP address for default interface
           ip addr: arbitrary interface configuration
           Subclasses should override this method and call
           the parent class's config(**params)"""
        # If we were overriding this method, we would call
        # the superclass config method here as follows:
        # r = Parent.config( **_params )
        r = {}

        self.setParam(r, 'setIP6', ip=ip6)
        self.setParam(r, 'setDefaultRoute', defaultRoute=defaultRoute)

        # This should be examined
        self.cmd('ip link set lo ' + lo)
        return r


class APSensor(LowPANNode):
    """A APSensor is a Node that is running (or has execed?)
       an OpenFlow switch."""
    portBase = 1  # Switches start with port 1 in OpenFlow
    dpidLen = 16  # digits in dpid passed to switch

    def __init__(self, name, dpid=None, opts='', listenPort=None, **params):
        """dpid: dpid hex string (or None to derive from name, e.g. s1 -> 1)
           opts: additional switch options
           listenPort: port to listen on for dpctl connections"""
        LowPANNode.__init__(self, name, **params)
        self.dpid = self.defaultDpid(dpid)
        self.opts = opts
        self.listenPort = listenPort
        if not self.inNamespace:
            self.controlIntf = Intf('lo', self, port=0)

    def defaultDpid(self, dpid=None):
        "Return correctly formatted dpid from dpid or switch name (s1 -> 1)"
        if dpid:
            # Remove any colons and make sure it's a good hex number
            dpid = dpid.replace(':', '')
            assert len(dpid) <= self.dpidLen and int(dpid, 16) >= 0
            return '0' * (self.dpidLen - len(dpid)) + dpid
        else:
            # Use hex of the first number in the switch name
            nums = re.findall(r'\d+', self.name)
            if nums:
                dpid = hex(int(nums[0]))[2:]
            else:
                raise Exception('Unable to derive default datapath ID - '
                                'please either specify a dpid or use a '
                                'canonical ap name such as ap23.')
            return '1' + '0' * (self.dpidLen - 1 - len(dpid)) + dpid

    def defaultIntf(self):
        "Return control interface"
        if self.controlIntf:
            return self.controlIntf
        else:
            return LowPANNode.defaultIntf(self)

    def sendCmd(self, *cmd, **kwargs):
        """Send command to Node.
           cmd: string"""
        kwargs.setdefault('printPid', False)
        if not self.execed:
            return LowPANNode.sendCmd(self, *cmd, **kwargs)
        else:
            error('*** Error: %s has execed and cannot accept commands' %
                  self.name)

    # Automatic class setup support
    isSetup = False

    def connected(self):
        "Is the switch connected to a controller? (override this method)"
        # Assume that we are connected by default to whatever we need to
        # be connected to. This should be overridden by any OpenFlow
        # switch, but not by a standalone bridge.
        debug('Assuming', repr(self), 'is connected to a controller\n')
        return True

    def stop(self, deleteIntfs=True):
        """Stop switch
           deleteIntfs: delete interfaces? (True)"""
        if deleteIntfs:
            self.deleteIntfs()

    def __repr__(self):
        "More informative string representation"
        intfs = (','.join(['%s:%s' % (i.name, i.IP())
                           for i in self.intfList()]))
        return '<%s %s: %s pid=%s> ' % (
            self.__class__.__name__, self.name, intfs, self.pid)


class UserSensor(APSensor):
    "User-space AP."

    dpidLen = 12

    def __init__(self, name, dpopts='--no-slicing', **kwargs):
        """Init.
           name: name for the switch
           dpopts: additional arguments to ofdatapath (--no-slicing)"""
        APSensor.__init__(self, name, **kwargs)
        super(UserAP, self).__init__(name, dpopts='--no-slicing', **kwargs)
        pathCheck('ofdatapath', 'ofprotocol',
                  moduleName='the OpenFlow reference user switch' +
                             '(openflow.org)')
        if self.listenPort:
            self.opts += ' --listen=ptcp:%i ' % self.listenPort
        else:
            self.opts += ' --listen=punix:/tmp/%s.listen' % self.name
        self.dpopts = dpopts

    def start(self, controllers):
        """Start OpenFlow reference user datapath.
           Log to /tmp/sN-{ofd,ofp}.log.
           controllers: list of controller objects"""
        # Add controllers
        clist = ','.join(['tcp:%s:%d' % (c.IP(), c.port)
                          for c in controllers])
        ofdlog = '/tmp/' + self.name + '-ofd.log'
        ofplog = '/tmp/' + self.name + '-ofp.log'
        intfs = [str(i) for i in self.intfList()
                 if (isinstance(i, IntfSixLoWPAN) and not i.IP6() or
                     (not isinstance(i, IntfSixLoWPAN) and not i.IP()))]

        self.cmd('ofdatapath -i ' + ','.join(intfs) +
                 ' punix:/tmp/' + self.name + ' -d %s ' % self.dpid +
                 self.dpopts +
                 ' 1> ' + ofdlog + ' 2> ' + ofdlog + ' &')
        self.cmd('ofprotocol unix:/tmp/' + self.name +
                 ' ' + clist +
                 ' --fail=closed ' + self.opts +
                 ' 1> ' + ofplog + ' 2>' + ofplog + ' &')
        if "no-slicing" not in self.dpopts:
            # Only TCReapply if slicing is enable
            sleep(1)  # Allow ofdatapath to start before re-arranging qdisc's
            for intf in self.intfList():
                if not intf.IP():
                    self.TCReapply(intf)

    def stop(self, deleteIntfs=True):
        """Stop OpenFlow reference user datapath.
           deleteIntfs: delete interfaces? (True)"""
        # self.cmd('kill %ofdatapath')
        # self.cmd('kill %ofprotocol')
        # super(UserAP, self).stop(deleteIntfs)


class OVSSensor(APSensor, OVSSwitch):
    "Open vSwitch Sensor. Depends on ovs-vsctl."

    def __init__(self, name, failMode='secure', datapath='kernel',
                 inband=False, protocols=None,
                 reconnectms=1000, stp=False, batch=False, **params):
        """name: name for switch
           failMode: controller loss behavior (secure|open)
           datapath: userspace or kernel mode (kernel|user)
           inband: use in-band control (False)
           protocols: use specific OpenFlow version(s) (e.g. OpenFlow13)
                      Unspecified (or old OVS version) uses OVS default
           reconnectms: max reconnect timeout in ms (0/None for default)
           stp: enable STP (False, requires failMode=standalone)
           batch: enable batch startup (False)"""
        APSensor.__init__(self, name, **params)
        self.failMode = failMode
        self.datapath = datapath
        self.inband = inband
        self.protocols = protocols
        self.reconnectms = reconnectms
        self.stp = stp
        self._uuids = []  # controller UUIDs
        self.batch = batch
        self.commands = []  # saved commands for batch startup

    @classmethod
    def setup(cls):
        "Make sure Open vSwitch is installed and working"
        pathCheck('ovs-vsctl',
                  moduleName='Open vSwitch (openvswitch.org)')
        # This should no longer be needed, and it breaks
        # with OVS 1.7 which has renamed the kernel module:
        #  moduleDeps( subtract=OF_KMOD, add=OVS_KMOD )
        out, err, exitcode = errRun('ovs-vsctl -t 1 show')
        if exitcode:
            error(out + err +
                  'ovs-vsctl exited with code %d\n' % exitcode +
                  '*** Error connecting to ovs-db with ovs-vsctl\n'
                  'Make sure that Open vSwitch is installed, '
                  'that ovsdb-server is running, and that\n'
                  '"ovs-vsctl show" works correctly.\n'
                  'You may wish to try '
                  '"service openvswitch-switch start".\n')
            exit(1)
        version = quietRun('ovs-vsctl --version')
        cls.OVSVersion = findall(r'\d+\.\d+', version)[0]

    def start(self, controllers):
        "Start up a new OVS OpenFlow switch using ovs-vsctl"
        if self.inNamespace:
            raise Exception(
                'OVS kernel AP does not work in a namespace')
        int(self.dpid, 16)  # DPID must be a hex string
        # Command to add interfaces
        intfs = ''.join(' -- add-port %s %s' % (self, intf) +
                        self.intfOpts(intf)
                        for intf in self.intfList()
                        if self.ports[intf] and
                        (isinstance(intf, IntfSixLoWPAN) and not intf.IP6() or
                         not isinstance(intf, IntfSixLoWPAN) and not intf.IP()))

        # Command to create controller entries
        clist = [(self.name + c.name, '%s:%s:%d' %
                  (c.protocol, c.IP(), c.port))
                 for c in controllers]
        if self.listenPort:
            clist.append((self.name + '-listen',
                          'ptcp:%s' % self.listenPort))
        ccmd = '-- --id=@%s create Controller target=\\"%s\\"'
        if self.reconnectms:
            ccmd += ' max_backoff=%d' % self.reconnectms
        cargs = ' '.join(ccmd % (name, target)
                         for name, target in clist)
        # Controller ID list
        cids = ','.join('@%s' % name for name, _target in clist)
        # Try to delete any existing bridges with the same name
        if not self.isOldOVS():
            cargs += ' -- --if-exists del-br %s' % self
        # One ovs-vsctl command to rule them all!
        self.vsctl(cargs +
                   ' -- add-br %s' % self +
                   ' -- set bridge %s controller=[%s]' % (self, cids) +
                   self.bridgeOpts() +
                   intfs)
        # If necessary, restore TC config overwritten by OVS
        if not self.batch:
            for intf in self.intfList():
                self.TCReapply(intf)

    @classmethod
    def batchStartup(cls, aps, run=errRun):
        """Batch startup for OVS
           switches: switches to start up
           run: function to run commands (errRun)"""
        info('...')
        cmds = 'ovs-vsctl'
        for ap in aps:
            if ap.isOldOVS():
                # Ideally we'd optimize this also
                run('ovs-vsctl del-br %s' % ap)
            for cmd in ap.commands:
                cmd = cmd.strip()
                # Don't exceed ARG_MAX
                if len(cmds) + len(cmd) >= cls.argmax:
                    run(cmds, shell=True)
                    cmds = 'ovs-vsctl'
                cmds += ' ' + cmd
                ap.cmds = []
                ap.batch = False
        if cmds:
            run(cmds, shell=True)
        # Reapply link config if necessary...
        for switch in aps:
            for intf in switch.intfs.values():
                if isinstance(intf, IntfSixLoWPAN):
                    intf.config(**intf.params)
        return ap

    # This should be ~ int( quietRun( 'getconf ARG_MAX' ) ),
    # but the real limit seems to be much lower
    argmax = 128000
