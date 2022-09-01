from typing import List
from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.net import Mininet_wifi
from mn_wifi.node import Car
from mn_wifi.sumo.invoker import SumoInvoker
from mn_wifi.sumo.controller import SumoControlThread
from mn_wifi.link import wmediumd, mesh as Mesh
from mn_wifi.wmediumdConnector import interference

from vlc import CarController

import os

if __name__ == '__main__':

    setLogLevel('info')
    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)
    "Create a network."
    info("*** Creating nodes\n")

    for name in ['obs', 'car1', 'car2', 'car3']: 
        net.addCar(name, wlans=2, encrypt=[None, 'wpa2'])

    kwargs = {
        'ssid': 'vanet-ssid', 
        'mode': 'g', 
        'passwd': '123456789a',
        'encrypt': 'wpa2', 
        'failMode': 'standalone', 
        'datapath': 'user'
    }
    net.addAccessPoint(
        'e1', mac='00:00:00:11:00:01', channel='1',
        position='100,25,0', range=200, **kwargs
    )
    info("*** Configuring Propagation Model\n")
    net.setPropagationModel(model="logDistance", exp=4.5)

    info("*** Configuring wifi nodes\n")
    net.configureWifiNodes()

    controlled_cars: List[Car] = net.cars[1:3]
    for node in controlled_cars: 
        net.addLink(
            node, cls=Mesh, intf=node.intfNames()[0], 
            ssid='meshNet', channel=5, ht_cap='HT40+'
        )

    info("*** Starting network\n")
    net.build()

    for enb in net.aps:
        enb.start([])
    
    for i, mn_car in enumerate(controlled_cars, start=1): 
        mn_car.setIP(ip=f'192.168.0.{i}', intf=mn_car.intfNames()[0])
        mn_car.setIP(ip=f'192.168.1.{i}', intf=mn_car.intfNames()[1])

    nodes = controlled_cars + net.aps

    net.telemetry(
        nodes=nodes, data_type='position',
        min_x=-50, min_y=-75,
        max_x=250, max_y=150
    )
    info("***** Telemetry Initialised\n")

    example_root_path = os.path.dirname(os.path.abspath(__file__))
    cfg_file_path = os.path.join(example_root_path, 'conf' ,'map.sumocfg')
    net.useExternalProgram(
        program=SumoInvoker, config_file=cfg_file_path,
        port=8813, clients=2, exec_order=0,
        extra_params=['--delay', '1000', '--start', 'false']
    )

    sumo_ctl = SumoControlThread('SUMO_CAR_CONTROLLER', verbose=True)
    for car in controlled_cars: sumo_ctl.add(CarController(car))
    sumo_ctl.setDaemon(True)
    sumo_ctl.start()

    info("***** TraCI Initialised\n")
    CLI(net)

    info("***** CLI Initialised\n")

    info("*** Stopping network\n")
    net.stop()
    