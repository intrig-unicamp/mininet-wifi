# -*- coding: utf-8 -*-
"""
@file    junction.py
@author  Michael Behrisch
@date    2011-03-17
@version $Id: junction.py 13752 2013-04-27 06:06:24Z behrisch $

Python implementation of the TraCI interface.

SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
Copyright (C) 2011 DLR (http://www.dlr.de/) and contributors
All rights reserved
"""
_RETURN_VALUE_FUNC = ''
subscriptionResults = ''

def return_value_func(self):
    from . import trace
    from . import constants as tc

    self._RETURN_VALUE_FUNC = {tc.ID_LIST:      trace.Storage.readStringList,
                         tc.VAR_POSITION: lambda result: result.read("!dd")}
    self.subscriptionResults = trace.SubscriptionResults(self._RETURN_VALUE_FUNC)

def _getUniversal(self, varID, junctionID):
    from . import trace
    from . import constants as tc
    self.return_value_func()
    result = trace._sendReadOneStringCmd(tc.CMD_GET_JUNCTION_VARIABLE, varID, junctionID)
    return _RETURN_VALUE_FUNC[varID](result)

def getIDList():
    """getIDList() -> list(string)
    
    Returns a list of all junctions in the network.
    """
    from . import constants as tc
    return _getUniversal(tc.ID_LIST, "")

def getPosition(junctionID):
    """getPosition(string) -> (double, double)
    
    Returns the coordinates of the center of the junction.
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_POSITION, junctionID)


def subscribe(junctionID, varIDs=None, begin=0, end=2**31-1):
    """subscribe(string, list(integer), double, double) -> None
    
    Subscribe to one or more junction values for the given interval.
    A call to this method clears all previous subscription results.
    """
    from . import constants as tc
    from . import trace
    varIDs = (tc.VAR_POSITION,)
    subscriptionResults.reset()
    trace._subscribe(tc.CMD_SUBSCRIBE_JUNCTION_VARIABLE, begin, end, junctionID, varIDs)

def getSubscriptionResults(junctionID=None):
    """getSubscriptionResults(string) -> dict(integer: <value_type>)
    
    Returns the subscription results for the last time step and the given junction.
    If no junction id is given, all subscription results are returned in a dict.
    If the junction id is unknown or the subscription did for any reason return no data,
    'None' is returned.
    It is not possible to retrieve older subscription results than the ones
    from the last time step.
    """
    return subscriptionResults.get(junctionID)

def subscribeContext(junctionID, domain, dist, varIDs=None, begin=0, end=2**31-1):
    from . import constants as tc
    from . import trace

    varIDs = (tc.VAR_POSITION,)
    subscriptionResults.reset()
    trace._subscribeContext(tc.CMD_SUBSCRIBE_JUNCTION_CONTEXT, begin, end, junctionID, domain, dist, varIDs)

def getContextSubscriptionResults(junctionID=None):
    return subscriptionResults.getContext(junctionID)
