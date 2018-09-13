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

    def __init__( self, stations, aps, **kwargs ):
        thread = threading.Thread(name='vanet', target=self.configureApp,
                                  args=(stations, aps), kwargs=dict(kwargs,))
        thread.daemon = True
        thread.start()

    def configureApp(self, stations, aps, config_file='map.sumocfg',
                     clients=1, port=8813):
        try:
            mobility.stations = stations
            mobility.aps = aps
            mobility.mobileNodes = stations
            self.start(stations, config_file, clients, port)
        except:
            info("Something went wrong when trying to start sumo\n")

    def setWifiParameters(self):
        thread = threading.Thread(name='wifiParameters', target=mobility.parameters)
        thread.start()

    def start( self, stations, config_file, clients, port ):
        sumoBinary = checkBinary('sumo-gui')
        myfile = (os.path.join( os.path.dirname(__file__), "data/%s" % config_file))
        sumoConfig = myfile

        if not trace.isEmbedded():
            os.system(' %s -c %s --num-clients %s '
                      '--remote-port %s &' % (sumoBinary, sumoConfig,
                                              clients, port))
            trace.init(port)

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
            for vehID in trace.vehicle.getIDList():
                if not(vehID in veh_list):
                    init=initialisation(veh_list, travel_time_list,
                                        visited_list, visited, time,
                                        speed, vehID)
                    veh_list = init[0]
                    travel_time_list = init[1]
                    visited_list = init[2]
                    visited = init[3]
                    time = init[4]
                    speed = init[5]

                no_change = noChangeSaveTimeAndSpeed(
                    visited, veh_list, time, speed, vehID)
                visited = no_change[0]
                time = no_change[1]
                speed = no_change[2]

                change = changeSaveTimeAndSpeed(
                    visited, veh_list, time, speed, visited_list, vehID,
                    travel_time_list)
                visited = change[0]
                time = change[1]
                speed = change[2]
                visited_list = change[3]
                travel_time_list = change[4]

            for vehID2 in trace.vehicle.getIDList():
                for vehID1 in trace.vehicle.getIDList():
                    road1 = trace.vehicle.getRoadID(vehID1)
                    road2 = trace.vehicle.getRoadID(vehID2)
                    opposite_road1 = '-' + road1
                    opposite_road2 = '-' + road2
                    if not((vehID2,vehID1) in veh_interact_list):
                        x1 = trace.vehicle.getPosition(vehID1)[0]
                        y1 = trace.vehicle.getPosition(vehID1)[1]
                        x2 = trace.vehicle.getPosition(vehID2)[0]
                        y2 = trace.vehicle.getPosition(vehID2)[1]

                        if int(vehID1) < len(stations):
                            stations[int(vehID1)].params['position'] = x1, y1, 0
                            if 'carsta' in stations[int(vehID1)].params:
                                car = stations[int(vehID1)].params['carsta']
                                car.params['position'] = stations[int(vehID1)].params['position']
                                car.set_pos_wmediumd()
                            stations[int(vehID1)].set_pos_wmediumd()

                        if abs(x1-x2)>0 and abs(x1-x2)<20 \
                                and (road1 == opposite_road2 or road2 == opposite_road1):
                            veh_interact_list.append((vehID2,vehID1))
                            route2 = trace.vehicle.getRoute(vehID2)
                            route1 = trace.vehicle.getRoute(vehID1)
                            index2 = route2.index(trace.vehicle.getRoadID(vehID2))
                            index1 = route1.index(trace.vehicle.getRoadID(vehID1))
                            visited_edge_2 = visited_list[veh_list.index(vehID2)][0:len(
                                visited_list[veh_list.index(vehID2)])-1]
                            reroutage(visited_edge_2, travel_time_list,
                                      vehID1, vehID2, veh_list)
            step=step+1
        trace.close()
        sys.stdout.flush()
