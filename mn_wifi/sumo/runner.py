import sys
import os
import threading

from mn_wifi.mobility import mobility
from sys import version_info as py_version_info
if py_version_info < (3, 0):
    from sumolib.sumulib import checkBinary
    from traci import trace
    from function import initialisation, noChangeSaveTimeAndSpeed,\
        changeSaveTimeAndSpeed, reroutage


class sumo(object):

    def __init__( self, **params ):
        thread = threading.Thread(name='vanet', target=self.configureApp,
                                  kwargs=dict(params,))
        thread.daemon = True
        thread.start()

    def configureApp(self, stations, aps, program, config_file, **params):
        try:
            mobility.stations = stations
            mobility.aps = aps
            mobility.mobileNodes = stations
            self.start(stations, program, config_file)
        except:
            pass

    def setWifiParameters(self):
        thread = threading.Thread(name='wifiParameters', target=mobility.parameters)
        thread.start()

    def start( self, stations, program, config_file ):
        PORT = 8813
        sumoBinary = checkBinary(program)
        myfile = (os.path.join( os.path.dirname(__file__), "data/%s" % config_file))
        sumoConfig = myfile

        if not trace.isEmbedded():
            os.system(' %s -c %s &' % (sumoBinary, sumoConfig))
            #subprocess.Popen(' %s -c  %s' % (sumoBinary,sumoConfig), shell=True, stdout=sys.stdout)
            trace.init(PORT)

        step=0
        ListVeh=[]
        ListTravelTime=[]
        Visited=[]
        ListVisited=[]
        time=[]
        speed=[]
        ListVehInteract=[]
        self.setWifiParameters()

        #while len(ListVeh) < len(nodes):
        while True:
            trace.simulationStep()
            for vehID in trace.vehicle.getIDList():
                if not(vehID in ListVeh):
                    Initialisation=initialisation(ListVeh, ListTravelTime,
                                                  ListVisited, Visited, time,
                                                  speed, vehID)
                    ListVeh=Initialisation[0]
                    ListTravelTime=Initialisation[1]
                    ListVisited=Initialisation[2]
                    Visited=Initialisation[3]
                    time=Initialisation[4]
                    speed=Initialisation[5]

                NoChangeSaveTimeAndSpeed=noChangeSaveTimeAndSpeed(
                    Visited, ListVeh, time, speed, vehID)
                Visited=NoChangeSaveTimeAndSpeed[0]
                time=NoChangeSaveTimeAndSpeed[1]
                speed=NoChangeSaveTimeAndSpeed[2]

                ChangeSaveTimeAndSpeed=changeSaveTimeAndSpeed(
                    Visited, ListVeh, time, speed, ListVisited, vehID,
                    ListTravelTime)
                Visited=ChangeSaveTimeAndSpeed[0]
                time=ChangeSaveTimeAndSpeed[1]
                speed=ChangeSaveTimeAndSpeed[2]
                ListVisited=ChangeSaveTimeAndSpeed[3]
                ListTravelTime=ChangeSaveTimeAndSpeed[4]

            for vehID2 in trace.vehicle.getIDList():
                for vehID1 in trace.vehicle.getIDList():
                    Road1=trace.vehicle.getRoadID(vehID1)
                    Road2=trace.vehicle.getRoadID(vehID2)
                    OppositeRoad1='-'+Road1
                    OppositeRoad2='-'+Road2
                    if not((vehID2,vehID1) in ListVehInteract):
                        x1=trace.vehicle.getPosition(vehID1)[0]
                        y1=trace.vehicle.getPosition(vehID1)[1]
                        x2=trace.vehicle.getPosition(vehID2)[0]
                        y2=trace.vehicle.getPosition(vehID2)[1]

                        if int(vehID1) < len(stations):
                            stations[int(vehID1)].params['position'] = x1, y1, 0
                            car = stations[int(vehID1)].params['carsta']
                            car.params['position'] = stations[int(vehID1)].params['position']
                            stations[int(vehID1)].params['carsta'].set_pos_wmediumd()
                            car.set_pos_wmediumd()
                            #stations[int(vehID1)].params['range'] = 130

                        if abs(x1-x2)>0 and abs(x1-x2)<20 and (
                                        Road1==OppositeRoad2 or Road2==OppositeRoad1):
                            ListVehInteract.append((vehID2,vehID1))
                            Route2=trace.vehicle.getRoute(vehID2)
                            Route1=trace.vehicle.getRoute(vehID1)
                            Index2=Route2.index(trace.vehicle.getRoadID(vehID2))
                            Index1=Route1.index(trace.vehicle.getRoadID(vehID1))
                            VisitedEdge2=ListVisited[ListVeh.index(vehID2)][0:len(
                                ListVisited[ListVeh.index(vehID2)])-1]
                            reroutage(VisitedEdge2,ListTravelTime,vehID1,vehID2,ListVeh)
            step=step+1
        trace.close()
        sys.stdout.flush()
