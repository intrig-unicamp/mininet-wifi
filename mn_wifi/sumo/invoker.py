from typing import List
from mn_wifi.node import AP, Car
from mn_wifi.sumo.sumolib.sumolib import checkBinary
from mn_wifi.sumo.traci import main as traci, _vehicle
from mn_wifi.sumo.runner import sumo
from subprocess import Popen, PIPE


class SumoInvoker(sumo):

    def __init__(self, cars: List[Car], aps: List[AP], **kwargs) -> None:
        for car in cars: car.pos = car.position
        return super().__init__(cars, aps, **kwargs)

    def start(self, 
        mnwf_vlc: List[Car], 
        cfg_path:str, cli_num: int, port_num: int, exe_order: int, 
        ex_params: List[str]
    ):
        cmd_list = [
            checkBinary('sumo-gui'), 
            '-c', cfg_path, 
            '--num-clients', str(cli_num), 
            '--remote-port', str(port_num), 
            '--time-to-teleport', '-1'
        ]
        Popen(cmd_list + ex_params, stdin=PIPE, stdout=PIPE, stderr=PIPE)
        traci.init(port_num)
        traci.setOrder(exe_order)

        self.setWifiParameters()
        sumo_vlc_ctl = _vehicle.VehicleDomain()
        sumo_vlc_ctl._connection = traci.getConnection(label="default")
        mnwf_vlc_dict = {vlc.name:vlc for vlc in mnwf_vlc}
        while traci.simulation.getMinExpectedNumber() > 0:
            traci.simulationStep()
            for vlc_id in sumo_vlc_ctl.getIDList():
                x, y = sumo_vlc_ctl.getPosition(vlc_id)
                if vlc_id not in mnwf_vlc_dict: continue
                mnwf_vlc = mnwf_vlc_dict[vlc_id]
                mnwf_vlc.position = x, y, 0
                mnwf_vlc.set_pos_wmediumd(mnwf_vlc.position)

                if hasattr(mnwf_vlc, 'sumo') and mnwf_vlc.sumo:
                    args = list(mnwf_vlc.sumo.sumoargs)
                    mnwf_vlc.sumo(vlc_id, sumo_vlc_ctl, *args)
                    del mnwf_vlc.sumo
