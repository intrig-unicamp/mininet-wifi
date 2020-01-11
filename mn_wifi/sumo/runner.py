import sys
import os
import threading
from threading import Thread as thread

from mininet.log import info
from mn_wifi.mobility import mobility
from mn_wifi.sumo.sumolib.sumulib import checkBinary
from mn_wifi.sumo.traci import trace
from mn_wifi.sumo.function import initialisation, noChangeSaveTimeAndSpeed,\
    changeSaveTimeAndSpeed, reroutage


class sumo(object):

    vehCmds = None

    def __init__(self, cars, aps, **kwargs):
        mobility.thread_ = thread(name='vanet', target=self.configureApp,
                                  args=(cars, aps), kwargs=dict(kwargs,))
        mobility.thread_.daemon = True
        mobility.thread_._keep_alive = True
        mobility.thread_.start()

    @classmethod
    def getVehCmd(cls):
        return cls.vehCmds

    def configureApp(self, cars, aps, config_file='map.sumocfg',
                     clients=1, port=8813):
        try:
            mobility.cars = cars
            mobility.aps = aps
            mobility.mobileNodes = cars
            self.start(cars, config_file, clients, port)
        except:
            info("*** Connection with SUMO has been closed\n")

    def setWifiParameters(self):
        thread = threading.Thread(name='wifiParameters', target=mobility.parameters)
        thread.start()

    def start(self, cars, config_file, clients, port):
        sumoBinary = checkBinary('sumo-gui')
        sumoConfig = (os.path.join( os.path.dirname(__file__), "data/%s" % config_file))

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
        travel_time_list = []
        self.setWifiParameters()

        vehCmds = trace._vehicle.VehicleDomain()
        vehCmds._connection = trace.getConnection(label="default")
        sumo.vehCmds = vehCmds

        while True:
            trace.simulationStep()

            for vehID in vehCmds.getIDList():
                if not(vehID in veh_list):
                    init = initialisation(veh_list, travel_time_list,
                                          visited_list, visited, time,
                                          speed, vehID, vehCmds)
                    veh_list = init[0]
                    travel_time_list = init[1]
                    visited_list = init[2]
                    visited = init[3]
                    time = init[4]
                    speed = init[5]

                no_change = noChangeSaveTimeAndSpeed(
                    visited, veh_list, time, speed, vehID, vehCmds)
                visited = no_change[0]
                time = no_change[1]
                speed = no_change[2]

                change = changeSaveTimeAndSpeed(
                    visited, veh_list, time, speed, visited_list, vehID,
                    travel_time_list, vehCmds)
                visited = change[0]
                time = change[1]
                speed = change[2]
                visited_list = change[3]
                travel_time_list = change[4]

            for vehID1 in vehCmds.getIDList():
                x1 = vehCmds.getPosition(vehID1)[0]
                y1 = vehCmds.getPosition(vehID1)[1]

                if int(vehID1) < len(cars):
                    cars[int(vehID1)].position = x1, y1, 0
                    cars[int(vehID1)].set_pos_wmediumd(cars[int(vehID1)].position)

                    if hasattr(cars[int(vehID1)], 'sumo'):
                        if cars[int(vehID1)].sumo:
                            cars[int(vehID1)].sumo(vehID1, vehCmds)
                            del cars[int(vehID1)].sumo
            step += 1
        trace.close()
        sys.stdout.flush()
