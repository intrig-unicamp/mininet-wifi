#!/usr/bin/env python
"""@package topo

Wireless Network topology creation.

@author Ramon Fontes (ramonrf@dca.fee.unicamp.br)

This package includes code to represent network topologies.
"""

from mininet.util import irange
from mininet.topo import Topo


class Topo_WiFi(Topo):
    "Data center network representation for structured multi-trees."

    def addStation(self, name, **opts):
        """Convenience method: Add host to graph.
           name: host name
           opts: host options
           returns: host name"""
        if not opts and self.hopts:
            opts = self.hopts
        return self.addNode(name, **opts)

    def addAccessPoint(self, name, **opts):
        """Convenience method: Add switch to graph.
           name: switch name
           opts: switch options
           returns: switch name"""

        if not opts and self.sopts:
            opts = self.sopts
        result = self.addNode(name, isAP=True, **opts)
        return result

    # This legacy port management mechanism is clunky and will probably
    # be removed at some point.

    def addPort(self, src, dst, sport=None, dport=None):
        """Generate port mapping for new edge.
            src: source switch name
            dst: destination switch name"""
        # Initialize if necessary
        ports = self.ports
        ports.setdefault(src, {})
        ports.setdefault(dst, {})
        # New port: number of outlinks + base
        if sport is None:
            src_base = 1 if self.isAP(src) else 0
            if 'ap' in src:
                sport = None
            else:
                sport = len(ports[ src ]) + src_base
        if dport is None:
            dst_base = 1 if self.isAP(dst) else 0
            if 'ap' in dst:
                dport = None
            else:
                dport = len(ports[ dst ]) + dst_base
        ports[ src ][ sport ] = (dst, dport)
        ports[ dst ][ dport ] = (src, sport)
        return sport, dport

    def nodes( self, sort=True ):
        "Return nodes in graph"
        if sort:
            return self.sorted( self.g.nodes() )
        else:
            return self.g.nodes()

    def aps( self, sort=True ):
        """Return aps.
           sort: sort aps alphabetically
           returns: dpids list of dpids"""
        return [ n for n in self.nodes( sort ) if self.isAP( n ) ]

    def stations( self, sort=True ):
        """Return stations.
           sort: sort stations alphabetically
           returns: list of stations"""
        return [ n for n in self.nodes( sort ) if not self.isAP( n ) ]


    def isAP( self, n ):
        "Returns true if node is a switch."
        return self.g.node[ n ].get( 'isAP', False )


# Our idiom defines additional parameters in build(param...)
# pylint: disable=arguments-differ

class SingleAPTopo(Topo_WiFi):
    "Single ap connected to k stations."
    def build(self, k=2, **_opts):
        "k: number of stations"
        self.k = k
        ap = self.addAccessPoint('ap1')
        for h in irange(1, k):
            sta = self.addStation('sta%s' % h)
            self.addLink(sta, ap)


class MinimalWirelessTopo(SingleAPTopo):
    "Minimal wireless topology with two stations and one ap"
    def build(self):
        return SingleAPTopo.build(self, k=2)

                
class LinearWirelessTopo(Topo_WiFi):
    "Linear Wireless topology of k aps, with n stations per ap."

    def build(self, k=2, n=1, **_opts):
        """k: number of switches
           n: number of hosts per switch"""
        self.k = k
        self.n = n
        if n == 1:
            genStaName = lambda i, j: 'sta%s' % i
        else:
            genStaName = lambda i, j: 'sta%sap%d' % (j, i)

        lastAP = None
        for i in irange(1, k):
            # Add accessPoint
            ap = self.addAccessPoint('ap%s' % i, ssid='ssid_ap%s' % i)
            # Add hosts to accessPoint
            for j in irange(1, n):
                sta = self.addStation(genStaName(i, j))
                self.addLink(sta, ap)
            # Connect accessPoint to previous
            if lastAP:
                self.addLink(ap, lastAP)
            lastAP = ap

# pylint: enable=arguments-differ
