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
import trace
import constants as tc

_RETURN_VALUE_FUNC = {tc.ID_LIST:             trace.Storage.readStringList,
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
subscriptionResults = trace.SubscriptionResults(_RETURN_VALUE_FUNC)

def _getUniversal(varID, typeID):
    result = trace._sendReadOneStringCmd(tc.CMD_GET_VEHICLETYPE_VARIABLE, varID, typeID)
    return _RETURN_VALUE_FUNC[varID](result)

def getIDList():
    """getIDList() -> list(string)
    
    Returns a list of all known vehicle types.
    """
    return _getUniversal(tc.ID_LIST, "")

def getLength(typeID):
    """getLength(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_LENGTH, typeID)

def getMaxSpeed(typeID):
    """getMaxSpeed(string) -> double
    
    .
    """
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
    return _getUniversal(tc.VAR_SPEED_DEVIATION, typeID)

def getAccel(typeID):
    """getAccel(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_ACCEL, typeID)

def getDecel(typeID):
    """getDecel(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_DECEL, typeID)

def getImperfection(typeID):
    """getImperfection(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_IMPERFECTION, typeID)

def getTau(typeID):
    """getTau(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_TAU, typeID)

def getVehicleClass(typeID):
    """getVehicleClass(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_VEHICLECLASS, typeID)

def getEmissionClass(typeID):
    """getEmissionClass(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_EMISSIONCLASS, typeID)

def getShapeClass(typeID):
    """getShapeClass(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_SHAPECLASS, typeID)

def getMinGap(typeID):
    """getMinGap(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_MINGAP, typeID)

def getWidth(typeID):
    """getWidth(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_WIDTH, typeID)

def getColor(typeID):
    """getColor(string) -> (integer, integer, integer, integer)
    
    .
    """
    return _getUniversal(tc.VAR_COLOR, typeID)


def subscribe(typeID, varIDs=(tc.VAR_MAXSPEED,), begin=0, end=2**31-1):
    """subscribe(string, list(integer), double, double) -> None
    
    Subscribe to one or more vehicle type values for the given interval.
    A call to this method clears all previous subscription results.
    """
    subscriptionResults.reset()
    trace._subscribe(tc.CMD_SUBSCRIBE_VEHICLETYPE_VARIABLE, begin, end, typeID, varIDs)

def getSubscriptionResults(typeID=None):
    """getSubscriptionResults(string) -> dict(integer: <value_type>)
    
    Returns the subscription results for the last time step and the given vehicle type.
    If no vehicle type id is given, all subscription results are returned in a dict.
    If the vehicle type id is unknown or the subscription did for any reason return no data,
    'None' is returned.
    It is not possible to retrieve older subscription results than the ones
    from the last time step.
    """
    return subscriptionResults.get(typeID)

def subscribeContext(typeID, domain, dist, varIDs=(tc.VAR_MAXSPEED,), begin=0, end=2**31-1):
    subscriptionResults.reset()
    trace._subscribeContext(tc.CMD_SUBSCRIBE_VEHICLETYPE_CONTEXT, begin, end, typeID, domain, dist, varIDs)

def getContextSubscriptionResults(typeID=None):
    return subscriptionResults.getContext(typeID)


def setLength(typeID, length):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_LENGTH, typeID, length)

def setMaxSpeed(typeID, speed):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_MAXSPEED, typeID, speed)

def setVehicleClass(typeID, clazz):
    trace._sendStringCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_VEHICLECLASS, typeID, clazz)

def setSpeedFactor(typeID, factor):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_SPEED_FACTOR, typeID, factor)

def setSpeedDeviation(typeID, deviation):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_SPEED_DEVIATION, typeID, deviation)

def setEmissionClass(typeID, clazz):
    trace._sendStringCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_EMISSIONCLASS, typeID, clazz)

def setWidth(typeID, width):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_WIDTH, typeID, width)

def setMinGap(typeID, minGap):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_MINGAP, typeID, minGap)

def setShapeClass(typeID, clazz):
    trace._sendStringCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_SHAPECLASS, typeID, clazz)

def setAccel(typeID, accel):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_ACCEL, typeID, accel)

def setDecel(typeID, decel):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_DECEL, typeID, decel)

def setImperfection(typeID, imperfection):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_IMPERFECTION, typeID, imperfection)

def setTau(typeID, tau):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_TAU, typeID, tau)

def setColor(typeID, color):
    trace._beginMessage(tc.CMD_SET_VEHICLETYPE_VARIABLE, tc.VAR_COLOR, typeID, 1+1+1+1+1)
    trace._message.string += struct.pack("!BBBBB", tc.TYPE_COLOR, int(color[0]), int(color[1]), int(color[2]), int(color[3]))
    trace._sendExact()
