# -*- coding: utf-8 -*-
"""
@file    multientryexit.py
@author  Michael Behrisch
@date    2011-03-16
@version $Id: multientryexit.py 12906 2012-10-30 11:02:27Z behrisch $

Python implementation of the TraCI interface.

SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
Copyright (C) 2011 DLR (http://www.dlr.de/) and contributors
All rights reserved
"""

class multientryexit(object):

    subscriptionResults = ''
    _RETURN_VALUE_FUNC = ''

    def return_value_func(self):
        from . import trace
        from . import constants as tc
        self._RETURN_VALUE_FUNC = {tc.ID_LIST:                          trace.Storage.readStringList,
                              tc.LAST_STEP_VEHICLE_NUMBER:         trace.Storage.readInt,
                              tc.LAST_STEP_MEAN_SPEED:             trace.Storage.readDouble,
                              tc.LAST_STEP_VEHICLE_ID_LIST:        trace.Storage.readStringList,
                              tc.LAST_STEP_VEHICLE_HALTING_NUMBER: trace.Storage.readInt}
        self.subscriptionResults = trace.SubscriptionResults(self._RETURN_VALUE_FUNC)

    def _getUniversal(self, varID, detID):
        from . import trace
        from . import constants as tc
        self. return_value_func()
        result = trace._sendReadOneStringCmd(tc.CMD_GET_MULTI_ENTRY_EXIT_DETECTOR_VARIABLE, varID, detID)
        return self._RETURN_VALUE_FUNC[varID](result)

    def getIDList(self):
        """getIDList() -> list(string)

        Returns a list of all e3 detectors in the network.
        """
        from . import constants as tc
        return self._getUniversal(tc.ID_LIST, "")

    def getLastStepVehicleNumber(self, detID):
        """getLastStepVehicleNumber(string) -> integer

        .
        """
        from . import constants as tc
        return self._getUniversal(tc.LAST_STEP_VEHICLE_NUMBER, detID)

    def getLastStepMeanSpeed(self, detID):
        """getLastStepMeanSpeed(string) -> double

        .
        """
        from . import constants as tc
        return self._getUniversal(tc.LAST_STEP_MEAN_SPEED, detID)

    def getLastStepVehicleIDs(self, detID):
        """getLastStepVehicleIDs(string) -> list(string)

        .
        """
        from . import constants as tc
        return self._getUniversal(tc.LAST_STEP_VEHICLE_ID_LIST, detID)

    def getLastStepHaltingNumber(self, detID):
        """getLastStepHaltingNumber(string) -> integer

        .

        """
        from . import constants as tc
        return self._getUniversal(tc.LAST_STEP_VEHICLE_HALTING_NUMBER, detID)


    def subscribe(self, detID, varIDs=None, begin=0, end=2**31-1):
        """subscribe(string, list(integer), double, double) -> None

        Subscribe to one or more detector values for the given interval.
        A call to this method clears all previous subscription results.
        """
        from . import trace
        from . import constants as tc

        varIDs = (tc.LAST_STEP_VEHICLE_NUMBER,)
        self.subscriptionResults.reset()
        trace._subscribe(tc.CMD_SUBSCRIBE_MULTI_ENTRY_EXIT_DETECTOR_VARIABLE, begin, end, detID, varIDs)

    def getSubscriptionResults(self, detID=None):
        """getSubscriptionResults(string) -> dict(integer: <value_type>)

        Returns the subscription results for the last time step and the given detector.
        If no detector id is given, all subscription results are returned in a dict.
        If the detector id is unknown or the subscription did for any reason return no data,
        'None' is returned.
        It is not possible to retrieve older subscription results than the ones
        from the last time step.
        """
        return self.subscriptionResults.get(detID)

    def subscribeContext(self, detID, domain, dist, varIDs=None, begin=0, end=2**31-1):
        from . import trace
        from . import constants as tc

        varIDs = (tc.LAST_STEP_VEHICLE_NUMBER,)
        self.subscriptionResults.reset()
        trace._subscribeContext(tc.CMD_SUBSCRIBE_MULTI_ENTRY_EXIT_DETECTOR_CONTEXT, begin, end, detID, domain, dist, varIDs)

    def getContextSubscriptionResults(self, detID=None):
        return self.subscriptionResults.getContext(detID)
