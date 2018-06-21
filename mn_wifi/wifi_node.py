import mininet.node

class PlotClass():

    def plot(self, position):
        self.params['position'] = position.split(',')
        self.params['range'] = [0]
        self.plotted = True

class Node(mininet.node.Node, PlotClass):

    def __init__(self, name, inNamespace=True, **params):
        mininet.node.Node.__init__(self, name, inNamespace=True, **params)

class Host(Node, PlotClass):
    pass

class CPULimitedHost(mininet.node.CPULimitedHost, PlotClass):

    def __init__(self, name, sched='cfs', **kwargs):
        mininet.node.CPULimitedHost.__init__(self, name, sched='cfs', **kwargs)

class Switch(mininet.node.Switch, PlotClass):

    def __init__(self, name, dpid=None, opts='', listenPort=None, **params):
        mininet.node.Switch.__init__(self, name, dpid=None, opts='', listenPort=None, **params)

class UserSwitch(mininet.node.Switch, PlotClass):

    def __init__(self, name, dpopts='--no-slicing', **kwargs):
        mininet.node.Switch.__init__(self, name, dpopts='--no-slicing', **kwargs)

class OVSSwitch(mininet.node.OVSSwitch, PlotClass):

    def __init__(self, name, failMode='secure', datapath='kernel',
                 inband=False, protocols=None,
                 reconnectms=1000, stp=False, batch=False, **params):
        mininet.node.OVSSwitch.__init__(self, name, failMode='secure', datapath='kernel',
                                        inband=False, protocols=None,
                                        reconnectms=1000, stp=False, batch=False, **params)

OVSKernelSwitch = OVSSwitch

class OVSBridge(mininet.node.OVSBridge, PlotClass):

    def __init__(self, *args, **kwargs):
        mininet.node.OVSBridge(self, *args, **kwargs)

class IVSSwitch(mininet.node.IVSSwitch, PlotClass):

    def __init__(self, name, verbose=False, **kwargs):
        mininet.node.IVSSwitch(self, name, verbose=False, **kwargs)

class Controller(mininet.node.Controller, PlotClass):

    def __init__(self, name, inNamespace=False, command='controller',
                 cargs='-v ptcp:%d', cdir=None, ip="127.0.0.1",
                 port=6653, protocol='tcp', **params):
        mininet.node.Controller.__init__(self, name, inNamespace=False, command='controller',
                                         cargs='-v ptcp:%d', cdir=None, ip="127.0.0.1",
                                         port=6653, protocol='tcp', **params)

class OVSController(mininet.node.OVSController, PlotClass):

    def __init__(self, name, **kwargs):
        mininet.node.OVSController.__init__(self, name, **kwargs)

class NOX(mininet.node.NOX, PlotClass):

    def __init__(self, name, *noxArgs, **kwargs):
        mininet.node.NOX.__init__(self, name, *noxArgs, **kwargs)

class Ryu(mininet.node.Ryu, PlotClass):

    def __init__(self, name, *ryuArgs, **kwargs):
        mininet.node.Ryu.__init__(self, name, *ryuArgs, **kwargs)

class RemoteController(mininet.node.RemoteController, PlotClass):

    def __init__(self, name, ip='127.0.0.1',
                 port=None, **kwargs):
        mininet.node.RemoteController.__init__(self, name, ip='127.0.0.1',
                                               port=None, **kwargs)

DefaultControllers = ( Controller, OVSController )

def findController( controllers=DefaultControllers ):
    "Return first available controller from list, if any"
    for controller in controllers:
        if controller.isAvailable():
            return controller

def DefaultController( name, controllers=DefaultControllers, **kwargs ):
    "Find a controller that is available and instantiate it"
    controller = findController( controllers )
    if not controller:
        raise Exception( 'Could not find a default OpenFlow controller' )
    return controller( name, **kwargs )

def NullController( *_args, **_kwargs ):
    "Nonexistent controller - simply returns None"
    return None