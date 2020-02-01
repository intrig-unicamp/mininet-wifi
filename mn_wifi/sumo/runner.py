import sys
import os
import threading
from threading import Thread as thread

from mininet.log import info
from mn_wifi.mobility import mobility
from mn_wifi.sumo.sumolib.sumolib import checkBinary
from mn_wifi.sumo.traci import trace, _vehicle


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
        sumoConfig = os.path.join(os.path.dirname(__file__), "data/%s" % config_file)

        if not trace.isEmbedded():
            os.system(' %s -c %s --num-clients %s '
                      '--remote-port %s --time-to-teleport -1 &'
                      % (sumoBinary, sumoConfig, clients, port))
            trace.init(port)
            trace.setOrder(0)

        step = 0
        self.setWifiParameters()

        vehCmds = _vehicle.VehicleDomain()
        vehCmds._connection = trace.getConnection(label="default")

        while True:
            trace.simulationStep()

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
