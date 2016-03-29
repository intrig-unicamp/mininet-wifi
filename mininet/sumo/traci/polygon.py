# -*- coding: utf-8 -*-
"""
@file    polygon.py
@author  Michael Behrisch
@date    2011-03-16
@version $Id: polygon.py 13752 2013-04-27 06:06:24Z behrisch $

Python implementation of the TraCI interface.

SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
Copyright (C) 2011 DLR (http://www.dlr.de/) and contributors
All rights reserved
"""
import struct
import trace
import constants as tc

_RETURN_VALUE_FUNC = {tc.ID_LIST:   trace.Storage.readStringList,
                      tc.VAR_TYPE:  trace.Storage.readString,
                      tc.VAR_SHAPE: trace.Storage.readShape,
                      tc.VAR_COLOR: lambda result: result.read("!BBBB")}
subscriptionResults = trace.SubscriptionResults(_RETURN_VALUE_FUNC)

def _getUniversal(varID, polygonID):
    result = trace._sendReadOneStringCmd(tc.CMD_GET_POLYGON_VARIABLE, varID, polygonID)
    return _RETURN_VALUE_FUNC[varID](result)

def getIDList():
    """getIDList() -> list(string)
    
    Returns a list of all polygons in the network.
    """
    return _getUniversal(tc.ID_LIST, "")

def getType(polygonID):
    """getType(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_TYPE, polygonID)

def getShape(polygonID):
    """getShape(string) -> list((double, double))
    
    .
    """
    return _getUniversal(tc.VAR_SHAPE, polygonID)

def getColor(polygonID):
    """getColor(string) -> (integer, integer, integer, integer)
    
    .
    """
    return _getUniversal(tc.VAR_COLOR, polygonID)


def subscribe(polygonID, varIDs=(tc.VAR_SHAPE,), begin=0, end=2**31-1):
    """subscribe(string, list(integer), double, double) -> None
    
    Subscribe to one or more polygon values for the given interval.
    A call to this method clears all previous subscription results.
    """
    subscriptionResults.reset()
    trace._subscribe(tc.CMD_SUBSCRIBE_POLYGON_VARIABLE, begin, end, polygonID, varIDs)

def getSubscriptionResults(polygonID=None):
    """getSubscriptionResults(string) -> dict(integer: <value_type>)
    
    Returns the subscription results for the last time step and the given poi.
    If no polygon id is given, all subscription results are returned in a dict.
    If the polygon id is unknown or the subscription did for any reason return no data,
    'None' is returned.
    It is not possible to retrieve older subscription results than the ones
    from the last time step.
    """
    return subscriptionResults.get(polygonID)

def subscribeContext(polygonID, domain, dist, varIDs=(tc.VAR_SHAPE,), begin=0, end=2**31-1):
    subscriptionResults.reset()
    trace._subscribeContext(tc.CMD_SUBSCRIBE_POLYGON_CONTEXT, begin, end, polygonID, domain, dist, varIDs)

def getContextSubscriptionResults(polygonID=None):
    return subscriptionResults.getContext(polygonID)


def setType(polygonID, polygonType):
    trace._beginMessage(tc.CMD_SET_POLYGON_VARIABLE, tc.VAR_TYPE, polygonID, 1+4+len(polygonType))
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRING, len(polygonType)) + polygonType
    trace._sendExact()

def setShape(polygonID, shape):
    trace._beginMessage(tc.CMD_SET_POLYGON_VARIABLE, tc.VAR_SHAPE, polygonID, 1+1+len(shape)*(8+8))
    trace._message.string += struct.pack("!BB", tc.TYPE_POLYGON, len(shape))
    for p in shape:
        trace._message.string += struct.pack("!dd", p)
    trace._sendExact()

def setColor(polygonID, color):
    trace._beginMessage(tc.CMD_SET_POLYGON_VARIABLE, tc.VAR_COLOR, polygonID, 1+1+1+1+1)
    trace._message.string += struct.pack("!BBBBB", tc.TYPE_COLOR, int(color[0]), int(color[1]), int(color[2]), int(color[3]))
    trace._sendExact()

def add(polygonID, shape, color, fill=False, polygonType="", layer=0):
    trace._beginMessage(tc.CMD_SET_POLYGON_VARIABLE, tc.ADD, polygonID, 1+4 + 1+4+len(polygonType) + 1+1+1+1+1 + 1+1 + 1+4 + 1+1+len(shape)*(8+8))
    trace._message.string += struct.pack("!Bi", tc.TYPE_COMPOUND, 5)
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRING, len(polygonType)) + polygonType
    trace._message.string += struct.pack("!BBBBB", tc.TYPE_COLOR, int(color[0]), int(color[1]), int(color[2]), int(color[3]))
    trace._message.string += struct.pack("!BB", tc.TYPE_UBYTE, int(fill))
    trace._message.string += struct.pack("!Bi", tc.TYPE_INTEGER, layer)
    trace._message.string += struct.pack("!BB", tc.TYPE_POLYGON, len(shape))
    for p in shape:
        trace._message.string += struct.pack("!dd", *p)
    trace._sendExact()

def remove(polygonID, layer=0):
    trace._beginMessage(tc.CMD_SET_POLYGON_VARIABLE, tc.REMOVE, polygonID, 1+4)
    trace._message.string += struct.pack("!Bi", tc.TYPE_INTEGER, layer)
    trace._sendExact()
