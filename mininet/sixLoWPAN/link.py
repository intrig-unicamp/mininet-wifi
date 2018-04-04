"author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)"

import re
from sys import version_info as py_version_info


class IntfSixLoWPAN( object ):

    "Basic interface object that can configure itself."

    def __init__( self, name, node=None, port=None, link=None,
                  mac=None, **params ):
        """name: interface name (e.g. h1-eth0)
           node: owning node (where this intf most likely lives)
           link: parent link if we're part of a link
           other arguments are passed to config()"""
        self.node = node
        self.name = name
        self.link = link
        self.mac = mac
        self.ip, self.prefixLen = None, None

        # if interface is lo, we know the ip is 127.0.0.1.
        # This saves an ipaddr command per node
        if self.name == 'lo':
            self.ip = '127.0.0.1'
            self.prefixLen = 8
        # Add to node (and move ourselves if necessary )
        if node:
            moveIntfFn = params.pop( 'moveIntfFn', None )
            if moveIntfFn:
                node.addIntf( self, port=port, moveIntfFn=moveIntfFn )
            else:
                node.addIntf( self, port=port )
        # Save params for future reference
        self.params = params
        self.config( **params )

    def cmd( self, *args, **kwargs ):
        "Run a command in our owning node"
        return self.node.cmd( *args, **kwargs )

    def ipAddr(self, *args):
        "Configure ourselves using ip addr"
        if len(args) == 0:
            return self.cmd('ip addr show', self.name)
        else:
            self.cmd('ip addr flush ', self.name)
            return self.cmd('ip -6 addr add ', args[0], 'dev', self.name)

    def ipLink(self, *args):
        "Configure ourselves using ip link"
        return self.cmd('ip link set', self.name, *args)

    def setIP( self, ipstr, prefixLen=None ):
        """Set our IP address"""
        # This is a sign that we should perhaps rethink our prefix
        # mechanism and/or the way we specify IP addresses
        if '/' in ipstr:
            self.ip, self.prefixLen = ipstr.split( '/' )
            return self.ipAddr( ipstr )
        else:
            if prefixLen is None:
                raise Exception( 'No prefix length set for IP address %s'
                                 % ( ipstr, ) )
            self.ip, self.prefixLen = ipstr, prefixLen
            return self.ipAddr('%s/%s' % (ipstr, prefixLen))

    def setMAC(self, macstr):
        """Set the MAC address for an interface.
           macstr: MAC address as string"""
        self.mac = macstr
        return (self.ipLink('down') +
                self.ipLink('address', macstr) +
                self.ipLink('up'))

    _ipMatchRegex = re.compile(r'\d+\.\d+\.\d+\.\d+')
    _macMatchRegex = re.compile(r'..:..:..:..:..:..')

    def updateIP(self):
        "Return updated IP address based on ip addr"
        # use pexec instead of node.cmd so that we dont read
        # backgrounded output from the cli.
        ipAddr, _err, _exitCode = self.node.pexec(
            'ip addr show %s' % self.name)
        if py_version_info < (3, 0):
            ips = self._ipMatchRegex.findall(ipAddr)
        else:
            ips = self._ipMatchRegex.findall(ipAddr.decode('utf-8'))
        self.ip = ips[0] if ips else None
        return self.ip

    def updateMAC(self):
        "Return updated MAC address based on ip addr"
        ipAddr = self.ipAddr()
        if py_version_info < (3, 0):
            macs = self._macMatchRegex.findall(ipAddr)
        else:
            macs = self._macMatchRegex.findall(ipAddr.decode('utf-8'))
        self.mac = macs[0] if macs else None
        return self.mac

    # Instead of updating ip and mac separately,
    # use one ipAddr call to do it simultaneously.
    # This saves an ipAddr command, which improves performance.

    def updateAddr(self):
        "Return IP address and MAC address based on ipAddr."
        ipAddr = self.ipAddr()
        if py_version_info < (3, 0):
            ips = self._ipMatchRegex.findall(ipAddr)
            macs = self._macMatchRegex.findall(ipAddr)
        else:
            ips = self._ipMatchRegex.findall(ipAddr)
            macs = self._macMatchRegex.findall(ipAddr.decode('utf-8'))
        self.ip = ips[0] if ips else None
        self.mac = macs[0] if macs else None
        return self.ip, self.mac

    def IP( self ):
        "Return IP address"
        return self.ip

    def MAC( self ):
        "Return MAC address"
        return self.mac

    def isUp(self, setUp=False):
        "Return whether interface is up"
        if setUp:
            cmdOutput = self.ipLink('up')
            # no output indicates success
            if cmdOutput:
                # error( "Error setting %s up: %s " % ( self.name, cmdOutput ) )
                return False
            else:
                return True
        else:
            return "UP" in self.ipAddr()

    def rename(self, newname):
        "Rename interface"
        self.ipLink('down')
        result = self.cmd('ip link set', self.name, 'name', newname)
        self.name = newname
        self.ipLink('up')
        return result

    # The reason why we configure things in this way is so
    # That the parameters can be listed and documented in
    # the config method.
    # Dealing with subclasses and superclasses is slightly
    # annoying, but at least the information is there!

    def setParam( self, results, method, **param ):
        """Internal method: configure a *single* parameter
           results: dict of results to update
           method: config method name
           param: arg=value (ignore if value=None)
           value may also be list or dict"""
        name, value = list(param.items())[ 0 ]
        f = getattr( self, method, None )
        if not f or value is None:
            return
        if isinstance( value, list ):
            result = f( *value )
        elif isinstance( value, dict ):
            result = f( **value )
        else:
            result = f( value )
        results[ name ] = result
        return result

    def config( self, mac=None, ip=None, ipAddr=None,
                up=True, **_params ):
        """Configure Node according to (optional) parameters:
           mac: MAC address
           ip: IP address
           ipAddr: arbitrary interface configuration
           Subclasses should override this method and call
           the parent class's config(**params)"""
        # If we were overriding this method, we would call
        # the superclass config method here as follows:
        # r = Parent.config( **params )
        r = {}
        self.setParam( r, 'setMAC', mac=mac )
        self.setParam( r, 'setIP', ip=ip )
        self.setParam( r, 'isUp', up=up )
        self.setParam(r, 'ipAddr', ipAddr=ipAddr)
        return r

    def delete( self ):
        "Delete interface"
        self.cmd( 'iwpan dev ' + self.node.params['wpan'][0] + ' del' )
        # We used to do this, but it slows us down:
        # if self.node.inNamespace:
        # Link may have been dumped into root NS
        # quietRun( 'ip link del ' + self.name )
        self.node.delIntf( self )
        self.link = None

    def status( self ):
        "Return intf status as a string"
        links, _err, _result = self.node.pexec( 'ip link show' )
        if self.name in links:
            return "OK"
        else:
            return "MISSING"

    def __repr__( self ):
        return '<%s %s>' % ( self.__class__.__name__, self.name )

    def __str__( self ):
        return self.name


class sixLoWPANLink(object):

    def __init__(self, node, port=None, intfName=None, addr=None,
                 cls=None, **params):
        """Create 6LoWPAN link to another node.
           node: node
           intf: default interface class/constructor"""

        node.cmd('ip link set lo up')
        node.cmd('ip link set %s down' % node.params['wpan'][0])
        node.cmd('iwpan dev %s set pan_id "%s"' % (node.params['wpan'][0], params['panid']))
        node.cmd('ip link add link %s name %s-lowpan type lowpan'
                 % (node.params['wpan'][0], node.name))
        node.cmd('ip link set %s up' % node.params['wpan'][0])
        node.cmd('ip link set %s-lowpan up' % node.name)

        if params is None:
            params = {}
        if port is not None:
            params[ 'port' ] = port
        if 'port' not in params:
            params[ 'port' ] = node.newPort()
        if not intfName:
            ifacename = 'lowpan'
            intfName1 = self.wpanName(node, ifacename, node.newWpanPort())
        if not cls:
            cls = IntfSixLoWPAN
        params['ip'] = node.params['ip']
        params['name'] = intfName1

        intf1 = cls(node=node, mac=addr, link=self, **params)
        intf2 = '6LoWPAN'
        # All we are is dust in the wind, and our two interfaces
        self.intf1, self.intf2 = intf1, intf2

    @staticmethod
    def _ignore(*args, **kwargs):
        "Ignore any arguments"
        pass

    def wpanName(self, node, ifacename, n):
        "Construct a canonical interface name node-ethN for interface n."
        # Leave this as an instance method for now
        assert self
        return node.name + '-' + ifacename # + repr(n)

    def delete(self):
        "Delete this link"
        self.intf1.delete()
        self.intf1 = None
        self.intf2 = None

    def stop(self):
        "Override to stop and clean up link as needed"
        self.delete()

    def status(self):
        "Return link status as a string"
        return "(%s %s)" % (self.intf1.status(), self.intf2)

    def __str__(self):
        return '%s<->%s' % (self.intf1, self.intf2)
