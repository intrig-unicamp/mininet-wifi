# -*- coding: utf-8 -*-
"""
@file    vehicletype.py
@author  Michael Behrisch
@author  Lena Kalleske
@date    2008-10-09
@version $Id: vehicletype.py 13811 2013-05-01 20:31:43Z behrisch $

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

    self._RETURN_VALUE_FUNC = {tc.ID_LIST:             trace.Storage.readStringList,
                          tc.VAR_LENGTH:          trace.Storage.readDouble,
                          tc.VAR_MAXSPEED:        trace.Storage.readDouble,
                          tc.VAR_SPEED_FACTOR:    trace.Storage.readDouble,
                          tc.VAR_SPEED_DEVIATION: trace.Storage.readDouble,
                          tc.VAR_ACCEL:           trace.Storage.readDouble,
                          tc.VAR_DECEL:           trace.Storage.readDouble,
                          tc.VAR_IMPERFECTION:    trace.Storage.readDouble,
                          tc.VAR_TAU:             trace.Storage.readDouble,
                          tc.VAR_VEHICLECLASS:    trace.Storage.readString,
                          tc.VAR_EMISSIONCLASS:   trace.Storage.readString,
                          tc.VAR_SHAPECLASS:      trace.Storage.readString,
                          tc.VAR_MINGAP:          trace.Storage.readDouble,
                          tc.VAR_WIDTH:           trace.Storage.readDouble,
                          tc.VAR_COLOR:           lambda result: result.read("!BBBB")}
    self.subscriptionResults = trace.SubscriptionResults(self._RETURN_VALUE_FUNC)

def _getUniversal(self, varID, typeID):
    from . import trace
    from . import constants as tc
    self.return_value_func()
    result = trace._sendReadOneStringCmd(tc.CMD_GET_VEHICLETYPE_VARIABLE, varID, typeID)
    return self._RETURN_VALUE_FUNC[varID](result)

def getIDList():
    """getIDList() -> list(string)
    
    Returns a list of all known vehicle types.
    """
    from . import constants as tc
    return _getUniversal(tc.ID_LIST, "")

def getLength(typeID):
    """getLength(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_LENGTH, typeID)

def getMaxSpeed(typeID):
    """getMaxSpeed(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_MAXSPEED, typeID)

def getSpeedFactor(typeID):
    """getSpeedFactor(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_SPEED_FACTOR, typeID)

def getSpeedDeviation(typeID):
    """getSpeedDeviation(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_SPEED_DEVIATION, typeID)

def getAccel(typeID):
    """getAccel(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_ACCEL, typeID)

def getDecel(typeID):
    """getDecel(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_DECEL, typeID)

def getImperfection(typeID):
    """getImperfection(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_IMPERFECTION, typeID)

def getTau(typeID):
    """getTau(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_TAU, typeID)

def getVehicleClass(typeID):
    """getVehicleClass(string) -> string
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_VEHICLECLASS, typeID)

def getEmissionClass(typeID):
    """getEmissionClass(string) -> string
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_EMISSIONCLASS, typeID)

def getShapeClass(typeID):
    """getShapeClass(string) -> string
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_SHAPECLASS, typeID)

def getMinGap(typeID):
    """getMinGap(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_MINGAP, typeID)

def getWidth(typeID):
    """getWidth(string) -> double
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_WIDTH, typeID)

def getColor(typeID):
    """getColor(string) -> (integer, integer, integer, integer)
    
    .
    """
    from . import constants as tc
    return _getUniversal(tc.VAR_COLOR, typeID)


def subscribe(typeID, varIDs=None, begin=0, end=2**31-1):
    """subscribe(string, list(integer), double, double) -> None
    
    Subscribe to one or more vehicle type values for the given interval.
    A call to this method clears all previous subscription results.
    """
    from . import trace
    from . import constants as tc
    varIDs = (tc.VAR_MAXSPEED,)
    subscriptionResults.reset()
    trace._subscribe(tc.CMD_SUBSCRIBE_VEHICLETYPE_VARIABLE, begin, end, typeID, varIDs)

def getSubscriptionResults(self, typeID=None):
    """getSubscriptionResults(string) -> dict(integer: <value_type>)
    
    Returns the subscription results for the last time step and the given vehicle type.
    If no vehicle type id is given, all subscription results are returned in a dict.
    If the vehicle type id is unknown or the subscription did for any reason return no data,
    'None' is returned.
    It is not possible to retrieve older subscription results than the ones
    from the last time step.
    """
    return self.subscriptionResults.get(typeID)

def subscribeContext(self, typeID, domain, dist, varIDs=None, begin=0, end=2**31-1):
    from . import trace
    from . import constants as tc
    varIDs = (tc.VAR_MAXSPEED,)
    self.subscriptionResults.reset()
    trace._subscribeContext(tc.CMD_SUBSCRIBE_VEHICLETYPE_CONTEXT, begin, end, typeID, domain, dist, varIDs)

def getContextSubscriptionResults(self, typeID=None):
    return self.subscriptionResults.getContext(typeID)


def setLength(typeID, length):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_LENGTH, typeID, length)

def setMaxSpeed(typeID, speed):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_MAXSPEED, typeID, speed)

def setVehicleClass(typeID, clazz):
    from . import trace
    from . import constants as tc
    trace._sendStringCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_VEHICLECLASS, typeID, clazz)

def setSpeedFactor(typeID, factor):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_SPEED_FACTOR, typeID, factor)

def setSpeedDeviation(typeID, deviation):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_SPEED_DEVIATION, typeID, deviation)

def setEmissionClass(typeID, clazz):
    from . import trace
    from . import constants as tc
    trace._sendStringCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_EMISSIONCLASS, typeID, clazz)

def setWidth(typeID, width):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_WIDTH, typeID, width)

def setMinGap(typeID, minGap):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_MINGAP, typeID, minGap)

def setShapeClass(typeID, clazz):
    from . import trace
    from . import constants as tc
    trace._sendStringCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_SHAPECLASS, typeID, clazz)

def setAccel(typeID, accel):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_ACCEL, typeID, accel)

def setDecel(typeID, decel):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_DECEL, typeID, decel)

def setImperfection(typeID, imperfection):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_IMPERFECTION, typeID, imperfection)

def setTau(typeID, tau):
    from . import trace
    from . import constants as tc
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_TAU, typeID, tau)

def setColor(typeID, color):
    from . import trace
    from . import constants as tc
    trace._beginMessage(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_COLOR, typeID, 1 + 1 + 1 + 1 + 1)
    trace._message.string += struct.pack("!BBBBB", tc.TYPE_COLOR, int(color[0]), int(color[1]), int(color[2]), int(color[3]))
    trace._sendExact()
