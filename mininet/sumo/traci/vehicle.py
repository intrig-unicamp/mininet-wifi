# -*- coding: utf-8 -*-
"""
@file    vehicle.py
@author  Michael Behrisch
@author  Lena Kalleske
@date    2011-03-09
@version $Id: vehicle.py 13752 2013-04-27 06:06:24Z behrisch $

Python implementation of the TraCI interface.

SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
Copyright (C) 2011 DLR (http://www.dlr.de/) and contributors
All rights reserved
"""
import struct
import trace
import constants as tc

DEPART_TRIGGERED = -1
DEPART_NOW = -2

def _readBestLanes(result):
    result.read("!iB")
    nbLanes = result.read("!i")[0] # Length
    lanes = []
    for i in range(nbLanes):
        result.read("!B")
        laneID = result.readString()
        length, occupation, offset = result.read("!BdBdBb")[1::2]
        allowsContinuation = result.read("!BB")[1]
        nextLanesNo = result.read("!Bi")[1]
        nextLanes = []
        for j in range(nextLanesNo):
            nextLanes.append(result.readString())
        lanes.append( [laneID, length, occupation, offset, allowsContinuation, nextLanes ] )
    return lanes


_RETURN_VALUE_FUNC = {tc.ID_LIST:             trace.Storage.readStringList,
                      tc.VAR_SPEED:           trace.Storage.readDouble,
                      tc.VAR_SPEED_WITHOUT_TRACI: trace.Storage.readDouble,
                      tc.VAR_POSITION:        lambda result: result.read("!dd"),
                      tc.VAR_ANGLE:           trace.Storage.readDouble,
                      tc.VAR_ROAD_ID:         trace.Storage.readString,
                      tc.VAR_LANE_ID:         trace.Storage.readString,
                      tc.VAR_LANE_INDEX:      trace.Storage.readInt,
                      tc.VAR_TYPE:            trace.Storage.readString,
                      tc.VAR_ROUTE_ID:        trace.Storage.readString,
                      tc.VAR_COLOR:           lambda result: result.read("!BBBB"),
                      tc.VAR_LANEPOSITION:    trace.Storage.readDouble,
                      tc.VAR_CO2EMISSION:     trace.Storage.readDouble,
                      tc.VAR_COEMISSION:      trace.Storage.readDouble,
                      tc.VAR_HCEMISSION:      trace.Storage.readDouble,
                      tc.VAR_PMXEMISSION:     trace.Storage.readDouble,
                      tc.VAR_NOXEMISSION:     trace.Storage.readDouble,
                      tc.VAR_FUELCONSUMPTION: trace.Storage.readDouble,
                      tc.VAR_NOISEEMISSION:   trace.Storage.readDouble,
                      tc.VAR_EDGE_TRAVELTIME: trace.Storage.readDouble,
                      tc.VAR_EDGE_EFFORT:     trace.Storage.readDouble,
                      tc.VAR_ROUTE_VALID:     lambda result: bool(result.read("!B")[0]),
                      tc.VAR_EDGES:           trace.Storage.readStringList,
                      tc.VAR_SIGNALS:         trace.Storage.readInt,
                      tc.VAR_LENGTH:          trace.Storage.readDouble,
                      tc.VAR_MAXSPEED:        trace.Storage.readDouble,
                      tc.VAR_VEHICLECLASS:    trace.Storage.readString,
                      tc.VAR_SPEED_FACTOR:    trace.Storage.readDouble,
                      tc.VAR_SPEED_DEVIATION: trace.Storage.readDouble,
                      tc.VAR_EMISSIONCLASS:   trace.Storage.readString,
                      tc.VAR_WIDTH:           trace.Storage.readDouble,
                      tc.VAR_MINGAP:          trace.Storage.readDouble,
                      tc.VAR_SHAPECLASS:      trace.Storage.readString,
                      tc.VAR_ACCEL:           trace.Storage.readDouble,
                      tc.VAR_DECEL:           trace.Storage.readDouble,
                      tc.VAR_IMPERFECTION:    trace.Storage.readDouble,
                      tc.VAR_TAU:             trace.Storage.readDouble,
                      tc.VAR_BEST_LANES:      _readBestLanes,
                      tc.DISTANCE_REQUEST:    trace.Storage.readDouble}
subscriptionResults = trace.SubscriptionResults(_RETURN_VALUE_FUNC)

def _getUniversal(varID, vehID):
    result = trace._sendReadOneStringCmd(tc.CMD_GET_VEHICLE_VARIABLE, varID, vehID)
    return _RETURN_VALUE_FUNC[varID](result)

def getIDList():
    """getIDList() -> list(string)
    
    Returns a list of all known vehicles.
    """
    return _getUniversal(tc.ID_LIST, "")

def getSpeed(vehID):
    """getSpeed(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_SPEED, vehID)

def getSpeedWithoutTraCI(vehID):
    """getSpeedWithoutTraCI(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_SPEED_WITHOUT_TRACI, vehID)

def getPosition(vehID):
    """getPosition(string) -> (double, double)
    
    Returns the position of the named vehicle within the last step [m,m].
    """
    return _getUniversal(tc.VAR_POSITION, vehID)

def getAngle(vehID):
    """getAngle(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_ANGLE, vehID)

def getRoadID(vehID):
    """getRoadID(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_ROAD_ID, vehID)

def getLaneID(vehID):
    """getLaneID(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_LANE_ID, vehID)

def getLaneIndex(vehID):
    """getLaneIndex(string) -> integer
    
    .
    """
    return _getUniversal(tc.VAR_LANE_INDEX, vehID)

def getTypeID(vehID):
    """getTypeID(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_TYPE, vehID)

def getRouteID(vehID):
    """getRouteID(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_ROUTE_ID, vehID)

def getRoute(vehID):
    """getRoute(string) -> list(string)
    
    .
    """
    return _getUniversal(tc.VAR_EDGES, vehID)

def getLanePosition(vehID):
    """getLanePosition(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_LANEPOSITION, vehID)

def getColor(vehID):
    """getColor(string) -> (integer, integer, integer, integer)
    
    .
    """
    return _getUniversal(tc.VAR_COLOR, vehID)

def getCO2Emission(vehID):
    """getCO2Emission(string) -> double
    
    Returns the CO2 emission in mg for the last time step.
    """
    return _getUniversal(tc.VAR_CO2EMISSION, vehID)

def getCOEmission(vehID):
    """getCOEmission(string) -> double
    
    Returns the CO emission in mg for the last time step.
    """
    return _getUniversal(tc.VAR_COEMISSION, vehID)

def getHCEmission(vehID):
    """getHCEmission(string) -> double
    
    Returns the HC emission in mg for the last time step.
    """
    return _getUniversal(tc.VAR_HCEMISSION, vehID)

def getPMxEmission(vehID):
    """getPMxEmission(string) -> double
    
    Returns the particular matter emission in mg for the last time step.
    """
    return _getUniversal(tc.VAR_PMXEMISSION, vehID)

def getNOxEmission(vehID):
    """getNOxEmission(string) -> double
    
    Returns the NOx emission in mg for the last time step.
    """
    return _getUniversal(tc.VAR_NOXEMISSION, vehID)

def getFuelConsumption(vehID):
    """getFuelConsumption(string) -> double
    
    Returns the fuel consumption in ml for the last time step.
    """
    return _getUniversal(tc.VAR_FUELCONSUMPTION, vehID)

def getNoiseEmission(vehID):
    """getNoiseEmission(string) -> double
    
    Returns the noise emission in db for the last time step.
    """
    return _getUniversal(tc.VAR_NOISEEMISSION, vehID)

def getAdaptedTraveltime(vehID, time, edgeID):
    """getAdaptedTraveltime(string, double, string) -> double
    
    .
    """
    trace._beginMessage(tc.CMD_GET_VEHICLE_VARIABLE, tc.VAR_EDGE_TRAVELTIME, vehID, 1+4+1+4+1+4+len(edgeID))
    trace._message.string += struct.pack("!BiBiBi", tc.TYPE_COMPOUND, 2, tc.TYPE_INTEGER, time,
                                         tc.TYPE_STRING, len(edgeID)) + edgeID
    return trace._checkResult(tc.CMD_GET_VEHICLE_VARIABLE, tc.VAR_EDGE_TRAVELTIME, vehID).readDouble()

def getEffort(vehID, time, edgeID):
    """getEffort(string, double, string) -> double
    
    .
    """
    trace._beginMessage(tc.CMD_GET_VEHICLE_VARIABLE, tc.VAR_EDGE_EFFORT, vehID, 1+4+1+4+1+4+len(edgeID))
    trace._message.string += struct.pack("!BiBiBi", tc.TYPE_COMPOUND, 2, tc.TYPE_INTEGER, time,
                                         tc.TYPE_STRING, len(edgeID)) + edgeID
    return trace._checkResult(tc.CMD_GET_VEHICLE_VARIABLE, tc.VAR_EDGE_EFFORT, vehID).readDouble()

def isRouteValid(vehID):
    return _getUniversal(tc.VAR_ROUTE_VALID, vehID)

def getSignals(vehID):
    """getSignals(string) -> integer
    
    .
    """
    return _getUniversal(tc.VAR_SIGNALS, vehID)

def getLength(vehID):
    """getLength(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_LENGTH, vehID)

def getMaxSpeed(vehID):
    """getMaxSpeed(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_MAXSPEED, vehID)

def getVehicleClass(vehID):
    """getVehicleClass(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_VEHICLECLASS, vehID)

def getSpeedFactor(vehID):
    """getSpeedFactor(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_SPEED_FACTOR, vehID)

def getSpeedDeviation(vehID):
    """getSpeedDeviation(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_SPEED_DEVIATION, vehID)

def getEmissionClass(vehID):
    """getEmissionClass(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_EMISSIONCLASS, vehID)

def getWidth(vehID):
    """getWidth(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_WIDTH, vehID)

def getMinGap(vehID):
    """getMinGap(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_MINGAP, vehID)

def getShapeClass(vehID):
    """getShapeClass(string) -> string
    
    .
    """
    return _getUniversal(tc.VAR_SHAPECLASS, vehID)

def getAccel(vehID):
    """getAccel(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_ACCEL, vehID)

def getDecel(vehID):
    """getDecel(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_DECEL, vehID)

def getImperfection(vehID):
    """getImperfection(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_IMPERFECTION, vehID)

def getTau(vehID):
    """getTau(string) -> double
    
    .
    """
    return _getUniversal(tc.VAR_TAU, vehID)

def getBestLanes(vehID):
    """getBestLanes(string) -> 
    
    .
    """
    return _getUniversal(tc.VAR_BEST_LANES, vehID)

def getDrivingDistance(vehID, edgeID, pos, laneID=0):
    """getDrivingDistance(string, string, double, integer) -> double
    
    .
    """
    trace._beginMessage(tc.CMD_GET_VEHICLE_VARIABLE, tc.DISTANCE_REQUEST, vehID, 1+4+1+4+len(edgeID)+4+1+1)
    trace._message.string += struct.pack("!BiBi", tc.TYPE_COMPOUND, 2,
                                         tc.POSITION_ROADMAP, len(edgeID)) + edgeID
    trace._message.string += struct.pack("!dBB", pos, laneID, REQUEST_DRIVINGDIST)
    return trace._checkResult(tc.CMD_GET_VEHICLE_VARIABLE, tc.DISTANCE_REQUEST, vehID).readDouble()

def getDrivingDistance2D(vehID, x, y):
    """getDrivingDistance2D(string, double, double) -> integer
    
    .
    """
    trace._beginMessage(tc.CMD_GET_VEHICLE_VARIABLE, tc.DISTANCE_REQUEST, vehID, 1+4+1+4+4+1)
    trace._message.string += struct.pack("!BiBddB", tc.TYPE_COMPOUND, 2,
                                         tc.POSITION_2D, x, y, REQUEST_DRIVINGDIST)
    return trace._checkResult(tc.CMD_GET_VEHICLE_VARIABLE, tc.DISTANCE_REQUEST, vehID).readDouble()


def subscribe(vehID, varIDs=(tc.VAR_ROAD_ID, tc.VAR_LANEPOSITION), begin=0, end=2**31-1):
    """subscribe(string, list(integer), double, double) -> None
    
    Subscribe to one or more vehicle values for the given interval.
    A call to this method clears all previous subscription results.
    """
    subscriptionResults.reset()
    trace._subscribe(tc.CMD_SUBSCRIBE_VEHICLE_VARIABLE, begin, end, vehID, varIDs)

def getSubscriptionResults(vehID=None):
    """getSubscriptionResults(string) -> dict(integer: <value_type>)
    
    Returns the subscription results for the last time step and the given vehicle.
    If no vehicle id is given, all subscription results are returned in a dict.
    If the vehicle id is unknown or the subscription did for any reason return no data,
    'None' is returned.
    It is not possible to retrieve older subscription results than the ones
    from the last time step.
    """
    return subscriptionResults.get(vehID)

def subscribeContext(vehID, domain, dist, varIDs=(tc.VAR_ROAD_ID, tc.VAR_LANEPOSITION), begin=0, end=2**31-1):
    subscriptionResults.reset()
    trace._subscribeContext(tc.CMD_SUBSCRIBE_VEHICLE_CONTEXT, begin, end, vehID, domain, dist, varIDs)

def getContextSubscriptionResults(vehID=None):
    return subscriptionResults.getContext(vehID)


def setMaxSpeed(vehID, speed):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_MAXSPEED, vehID, speed)

def setStop(vehID, edgeID, pos=1., laneIndex=0, duration=2**31-1):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.CMD_STOP, vehID, 1+4+1+4+len(edgeID)+1+8+1+1+1+4)
    trace._message.string += struct.pack("!Bi", tc.TYPE_COMPOUND, 4)
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRING, len(edgeID)) + edgeID
    trace._message.string += struct.pack("!BdBBBi", tc.TYPE_DOUBLE, pos, tc.TYPE_BYTE, laneIndex, tc.TYPE_INTEGER, duration)
    trace._sendExact()

def changeLane(vehID, laneIndex, duration):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.CMD_CHANGELANE, vehID, 1+4+1+1+1+4)
    trace._message.string += struct.pack("!BiBBBi", tc.TYPE_COMPOUND, 2, tc.TYPE_BYTE, laneIndex, tc.TYPE_INTEGER, duration)
    trace._sendExact()

def slowDown(vehID, speed, duration):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.CMD_SLOWDOWN, vehID, 1+4+1+8+1+4)
    trace._message.string += struct.pack("!BiBdBi", tc.TYPE_COMPOUND, 2, tc.TYPE_DOUBLE, speed, tc.TYPE_INTEGER, duration)
    trace._sendExact()

def changeTarget(vehID, edgeID):
    trace._sendStringCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.CMD_CHANGETARGET, vehID, edgeID)

def setRouteID(vehID, routeID):
    trace._sendStringCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_ROUTE_ID, vehID, routeID)

def setRoute(vehID, edgeList):
    """
    changes the vehicle route to given edges list.
    The first edge in the list has to be the one that the vehicle is at at the moment.
    
    example usage:
    setRoute('1', ['1', '2', '4', '6', '7'])
    
    this changes route for vehicle id 1 to edges 1-2-4-6-7
    """
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_ROUTE, vehID,
                        1+4+sum(map(len, edgeList))+4*len(edgeList))
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRINGLIST, len(edgeList))
    for edge in edgeList:
        trace._message.string += struct.pack("!i", len(edge)) + edge
    trace._sendExact()

def setAdaptedTraveltime(vehID, begTime, endTime, edgeID, time):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_EDGE_TRAVELTIME, vehID, 1+4+1+4+1+4+1+4+len(edgeID)+1+8)
    trace._message.string += struct.pack("!BiBiBiBi", tc.TYPE_COMPOUND, 4, tc.TYPE_INTEGER, begTime,
                                         tc.TYPE_INTEGER, endTime, tc.TYPE_STRING, len(edgeID)) + edgeID
    trace._message.string += struct.pack("!Bd", tc.TYPE_DOUBLE, time)
    trace._sendExact()

def setEffort(vehID, begTime, endTime, edgeID, effort):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_EDGE_EFFORT, vehID, 1+4+1+4+1+4+1+4+len(edgeID)+1+4)
    trace._message.string += struct.pack("!BiBiBiBi", tc.TYPE_COMPOUND, 4, tc.TYPE_INTEGER, begTime,
                                         tc.TYPE_INTEGER, endTime, tc.TYPE_STRING, len(edgeID)) + edgeID
    trace._message.string += struct.pack("!Bd", tc.TYPE_DOUBLE, effort)
    trace._sendExact()

def rerouteTraveltime(vehID):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.CMD_REROUTE_TRAVELTIME, vehID, 1+4)
    trace._message.string += struct.pack("!Bi", tc.TYPE_COMPOUND, 0)
    trace._sendExact()

def rerouteEffort(vehID):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.CMD_REROUTE_EFFORT, vehID, 1+4)
    trace._message.string += struct.pack("!Bi", tc.TYPE_COMPOUND, 0)
    trace._sendExact()

def setSignals(vehID, signals):
    trace._sendIntCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_SIGNALS, vehID, signals)

def moveTo(vehID, laneID, pos):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_MOVE_TO, vehID, 1+4+1+4+len(laneID)+1+8)
    trace._message.string += struct.pack("!Bi", tc.TYPE_COMPOUND, 2)
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRING, len(laneID)) + laneID
    trace._message.string += struct.pack("!Bd", tc.TYPE_DOUBLE, pos)
    trace._sendExact()

def setSpeed(vehID, speed):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_SPEED, vehID, speed)

def setColor(vehID, color):
    """setColor(string, (integer, integer, integer, integer))
    sets color for vehicle with the given ID.
    i.e. (255,0,0,0) for the color red. 
    The fourth integer (alpha) is currently ignored
    """
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_COLOR, vehID, 1+1+1+1+1)
    trace._message.string += struct.pack("!BBBBB", tc.TYPE_COLOR, int(color[0]), int(color[1]), int(color[2]), int(color[3]))
    trace._sendExact()

def setLength(vehID, length):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_LENGTH, vehID, length)

def setVehicleClass(vehID, clazz):
    trace._sendStringCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_VEHICLECLASS, vehID, clazz)

def setSpeedFactor(vehID, factor):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_SPEED_FACTOR, vehID, factor)

def setSpeedDeviation(vehID, deviation):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_SPEED_DEVIATION, vehID, deviation)

def setEmissionClass(vehID, clazz):
    trace._sendStringCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_EMISSIONCLASS, vehID, clazz)

def setWidth(vehID, width):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_WIDTH, vehID, width)

def setMinGap(vehID, minGap):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_MINGAP, vehID, minGap)

def setShapeClass(vehID, clazz):
    trace._sendStringCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_SHAPECLASS, vehID, clazz)

def setAccel(vehID, accel):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_ACCEL, vehID, accel)

def setDecel(vehID, decel):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_DECEL, vehID, decel)

def setImperfection(vehID, imperfection):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_IMPERFECTION, vehID, imperfection)

def setTau(vehID, tau):
    trace._sendDoubleCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_TAU, vehID, tau)

def add(vehID, routeID, depart=DEPART_NOW, pos=0, speed=0, lane=0, typeID="DEFAULT_VEHTYPE"):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.ADD, vehID,
                        1+4 + 1+4+len(typeID) + 1+4+len(routeID) + 1+4 + 1+8 + 1+8 + 1+1)
    if depart > 0:
        depart *= 1000
    trace._message.string += struct.pack("!Bi", tc.TYPE_COMPOUND, 6)
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRING, len(typeID)) + typeID
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRING, len(routeID)) + routeID
    trace._message.string += struct.pack("!Bi", tc.TYPE_INTEGER, depart)
    trace._message.string += struct.pack("!BdBd", tc.TYPE_DOUBLE, pos, tc.TYPE_DOUBLE, speed)
    trace._message.string += struct.pack("!BB", tc.TYPE_BYTE, lane)
    trace._sendExact()

def remove(vehID, reason=tc.REMOVE_VAPORIZED):
    '''Remove vehicle with the given ID for the give reason. 
       Reasons are defined in module constants and start with REMOVE_'''
    trace._sendByteCmd(tc.CMD_SET_VEHICLE_VARIABLE, tc.REMOVE, vehID, reason)

def moveToVTD(vehID, edgeID, lane, x, y):
    trace._beginMessage(tc.CMD_SET_VEHICLE_VARIABLE, tc.VAR_MOVE_TO_VTD, vehID, 1+4+1+4+len(edgeID)+1+4+1+8+1+8)
    trace._message.string += struct.pack("!Bi", tc.TYPE_COMPOUND, 4)
    trace._message.string += struct.pack("!Bi", tc.TYPE_STRING, len(edgeID)) + edgeID
    trace._message.string += struct.pack("!Bi", tc.TYPE_INTEGER, lane)    
    trace._message.string += struct.pack("!Bd", tc.TYPE_DOUBLE, x)
    trace._message.string += struct.pack("!Bd", tc.TYPE_DOUBLE, y)
    trace._sendExact()

