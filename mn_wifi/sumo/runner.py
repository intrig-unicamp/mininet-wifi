import sys
import os
import threading

from mininet.log import info
from mn_wifi.mobility import mobility
from sys import version_info as py_version_info


if py_version_info < (3, 0):
    from sumolib.sumulib import checkBinary
    from traci import trace
    from function import initialisation, noChangeSaveTimeAndSpeed,\
        changeSaveTimeAndSpeed, reroutage


class sumo(object):

    def __init__( self, cars, aps, **kwargs ):
        thread = threading.Thread(name='vanet', target=self.configureApp,
                                  args=(cars, aps), kwargs=dict(kwargs,))
        thread.daemon = True
        thread.start()

    def configureApp(self, cars, aps, config_file='map.sumocfg',
                     clients=1, port=8813):
        #try:
        mobility.cars = cars
        mobility.aps = aps
        mobility.mobileNodes = cars
        self.start(cars, config_file, clients, port)
        #except:
        #info("Connection with SUMO closed.\n")

    def setWifiParameters(self):
        thread = threading.Thread(name='wifiParameters', target=mobility.parameters)
        thread.start()

    def start( self, cars, config_file, clients, port ):
        sumoBinary = checkBinary('sumo-gui')
        myfile = (os.path.join( os.path.dirname(__file__), "data/%s" % config_file))
        sumoConfig = myfile

        if not trace.isEmbedded():
            os.system(' %s -c %s --num-clients %s '
                      '--remote-port %s &' % (sumoBinary, sumoConfig,
                                              clients, port))
            trace.init(port)
            trace.setOrder(0)

        step = 0
        speed = []
        time = []
        visited = []
        veh_list = []
        visited_list = []
        veh_interact_list = []
        travel_time_list = []
        self.setWifiParameters()

        while True:
            trace.simulationStep()
            vehicleCommands = trace._vehicle.VehicleDomain()
            vehicleCommands._connection = trace.getConnection(label="default")
            for vehID in vehicleCommands.getIDList():
                if not(vehID in veh_list):
                    init=initialisation(veh_list, travel_time_list,
                                        visited_list, visited, time,
                                        speed, vehID, vehicleCommands)
                    veh_list = init[0]
                    travel_time_list = init[1]
                    visited_list = init[2]
                    visited = init[3]
                    time = init[4]
                    speed = init[5]

                no_change = noChangeSaveTimeAndSpeed(
                    visited, veh_list, time, speed, vehID, vehicleCommands)
                visited = no_change[0]
                time = no_change[1]
                speed = no_change[2]

                change = changeSaveTimeAndSpeed(
                    visited, veh_list, time, speed, visited_list, vehID,
                    travel_time_list, vehicleCommands)
                visited = change[0]
                time = change[1]
                speed = change[2]
                visited_list = change[3]
                travel_time_list = change[4]

            for vehID2 in vehicleCommands.getIDList():
                for vehID1 in vehicleCommands.getIDList():
                    road1 = vehicleCommands.getRoadID(vehID1)
                    road2 = vehicleCommands.getRoadID(vehID2)
                    opposite_road1 = '-' + road1
                    opposite_road2 = '-' + road2
                    if not((vehID2,vehID1) in veh_interact_list):
                        x1 = vehicleCommands.getPosition(vehID1)[0]
                        y1 = vehicleCommands.getPosition(vehID1)[1]
                        x2 = vehicleCommands.getPosition(vehID2)[0]
                        y2 = vehicleCommands.getPosition(vehID2)[1]

                        if int(vehID1) < len(cars):
                            cars[int(vehID1)].params['position'] = x1, y1, 0
                            cars[int(vehID1)].set_pos_wmediumd()

                        if abs(x1-x2)>0 and abs(x1-x2)<20 \
                                and (road1 == opposite_road2 or road2 == opposite_road1):
                            veh_interact_list.append((vehID2,vehID1))
                            route2 = vehicleCommands.getRoute(vehID2)
                            route1 = vehicleCommands.getRoute(vehID1)
                            index2 = route2.index(vehicleCommands.getRoadID(vehID2))
                            index1 = route1.index(vehicleCommands.getRoadID(vehID1))
                            visited_edge_2 = visited_list[veh_list.index(vehID2)][0:len(
                                visited_list[veh_list.index(vehID2)])-1]
                            reroutage(visited_edge_2, travel_time_list,
                                      vehID1, vehID2, veh_list)
            step=step+1
        trace.close()
        sys.stdout.flush()