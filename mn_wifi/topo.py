#!/usr/bin/env python
"""@package topo

Wireless Network topology creation.

@author Ramon Fontes (ramonrf@dca.fee.unicamp.br)

This package includes code to represent network topologies.
"""

from mininet.util import irange
from mininet.topo import Topo


class MultiGraph( object ):
    "Utility class to track nodes and edges - replaces networkx.MultiGraph"

    def __init__( self ):
        self.node = {}
        self.edge = {}

    def add_node( self, node, attr_dict=None, **attrs):
        """Add node to graph
           attr_dict: attribute dict (optional)
           attrs: more attributes (optional)
           warning: updates attr_dict with attrs"""
        attr_dict = {} if attr_dict is None else attr_dict
        attr_dict.update( attrs )
        self.node[ node ] = attr_dict

    def add_edge( self, src, dst, key=None, attr_dict=None, **attrs ):
        """Add edge to graph
           key: optional key
           attr_dict: optional attribute dict
           attrs: more attributes
           warning: udpates attr_dict with attrs"""
        attr_dict = {} if attr_dict is None else attr_dict
        attr_dict.update( attrs )
        self.node.setdefault( src, {} )
        self.node.setdefault( dst, {} )
        self.edge.setdefault( src, {} )
        self.edge.setdefault( dst, {} )
        self.edge[ src ].setdefault( dst, {} )
        entry = self.edge[ dst ][ src ] = self.edge[ src ][ dst ]
        # If no key, pick next ordinal number
        if key is None:
            keys = [ k for k in entry.keys() if isinstance( k, int ) ]
            key = max( [ 0 ] + keys ) + 1
        entry[ key ] = attr_dict
        return key

    def nodes( self, data=False):
        """Return list of graph nodes
           data: return list of ( node, attrs)"""
        return self.node.items() if data else self.node.keys()

    def edges_iter( self, data=False, keys=False ):
        "Iterator: return graph edges, optionally with data and keys"
        for src, entry in self.edge.items():
            for dst, entrykeys in entry.items():
                if src > dst:
                    # Skip duplicate edges
                    continue
                for k, attrs in entrykeys.items():
                    if data:
                        if keys:
                            yield( src, dst, k, attrs )
                        else:
                            yield( src, dst, attrs )
                    else:
                        if keys:
                            yield( src, dst, k )
                        else:
                            yield( src, dst )

    def edges( self, data=False, keys=False ):
        "Return list of graph edges"
        return list( self.edges_iter( data=data, keys=keys ) )

    def __getitem__( self, node ):
        "Return link dict for given src node"
        return self.edge[ node ]

    def __len__( self ):
        "Return the number of nodes"
        return len( self.node )

    def convertTo( self, cls, data=False, keys=False ):
        """Convert to a new object of networkx.MultiGraph-like class cls
           data: include node and edge data
           keys: include edge keys as well as edge data"""
        g = cls()
        g.add_nodes_from( self.nodes( data=data ) )
        g.add_edges_from( self.edges( data=( data or keys ), keys=keys ) )
        return g


class Topo_WiFi(Topo):
    "Data center network representation for structured multi-trees."

    def __init__(self, *args, **params):
        """Topo object.
           Optional named parameters:
           hinfo: default host options
           sopts: default switch options
           lopts: default link options
           calls build()"""
        self.g = MultiGraph()
        self.hopts = params.pop('hopts', {})
        self.sopts = params.pop('sopts', {})
        self.lopts = params.pop('lopts', {})
        # ports[src][dst][sport] is port on dst that connects to src
        self.ports = {}
        self.build(*args, **params)

    def build(self, *args, **params):
        "Override this method to build your topology."
        pass

    def addNode(self, name, **opts):
        """Add Node to graph.
           name: name
           opts: node options
           returns: node name"""
        self.g.add_node(name, **opts)
        return name

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
