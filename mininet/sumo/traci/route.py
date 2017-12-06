# -*- coding: utf-8 -*-
"""
@file    route.py
@author  Michael Behrisch
@author  Lena Kalleske
@date    2008-10-09
@version $Id: route.py 13811 2013-05-01 20:31:43Z behrisch $

Python implementation of the TraCI interface.

SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
Copyright (C) 2008-2013 DLR (http://www.dlr.de/) and contributors
All rights reserved
"""
import struct

subscriptionResults = ''
_RETURN_VALUE_FUNC = ''

def return_value_func(self):
    from . import trace
    from . import constants as tc

    self._RETURN_VALUE_FUNC = {tc.ID_LIST:   trace.Storage.readStringList,
                          tc.VAR_EDGES: trace.Storage.readStringList}
    self.subscriptionResults = trace.SubscriptionResults(self._RETURN_VALUE_FUNC)

def _getUniversal(self, varID, routeID):
    from . import trace
    from . import constants as tc
    self.return_value_func()
    result = trace._sendReadOneStringCmd(tc.CMD_GET_ROUTE_VARIABLE, varID, routeID)
    return self._RETURN_VALUE_FUNC[varID](result)

def getIDList():
    """getIDList() -> list(string)
    
    Returns a list of all routes in the network.
    """
    from . import constants as tc
    return _getUniversal(tc.ID_LIST, "")

def getEdges(routeID):
    """getEdges(string) -> list(string)
    
    Returns a list of all edges in the route.
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_EDGES, routeID)


def subscribe(self, routeID, varIDs=None, begin=0, end=2**31-1):
    """subscribe(string, list(integer), double, double) -> None
    
    Subscribe to one or more route values for the given interval.
    A call to this method clears all previous subscription results.
    """
    from . import trace
    from . import constants as tc
    varIDs = (tc.ID_LIST,)
    self.subscriptionResults.reset()
    trace._subscribe(tc.CMD_SUBSCRIBE_ROUTE_VARIABLE, begin, end, routeID, varIDs)

def getSubscriptionResults(self, routeID=None):
    """getSubscriptionResults(string) -> dict(integer: <value_type>)
    
    Returns the subscription results for the last time step and the given route.
    If no route id is given, all subscription results are returned in a dict.
    If the route id is unknown or the subscription did for any reason return no data,
    'None' is returned.
    It is not possible to retrieve older subscription results than the ones
    from the last time step.
    """
    return self.subscriptionResults.get(routeID)

def subscribeContext(self, routeID, domain, dist, varIDs=None, begin=0, end=2**31-1):
    from . import trace
    from . import constants as tc
    varIDs = (tc.ID_LIST,)
    self.subscriptionResults.reset()
    trace._subscribeContext(tc.CMD_SUBSCRIBE_ROUTE_CONTEXT, begin, end, routeID, domain, dist, varIDs)

def getContextSubscriptionResults(self, routeID=None):
    return self.subscriptionResults.getContext(routeID)


def add(routeID, edges):
    from . import trace
    from . import constants as tc
    trace._beginMessage(tc.CMD_SET_ROUTE_VARIABLE, tc.ADD, routeID,
                        1 + 4 + sum(map(len, edges)) + 4 * len(edges))
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRINGLIST, len(edges))
    for e in edges:
        trace._message.string += struct.pack("!i", len(e)) + e
    trace._sendExact()
