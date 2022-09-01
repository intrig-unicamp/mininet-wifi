from mn_wifi.node import Car
from mn_wifi.sumo.vehicle import SumoVehicle
from mn_wifi.sumo.controller import SumoStepListener

class CarController(SumoStepListener): 

    def __init__(self, mnwf_car: Car):
        super().__init__()
        self.__car_name = mnwf_car.name
        self.__sumo_car = SumoVehicle(mnwf_car.name)
        self.__cur_lane = 0        
    
    @SumoStepListener.Substeps(priority=1)
    def change_cur_lane(self) -> None:
        if self.__sumo_car.distance > 75: self.__cur_lane = 1
        self.__sumo_car.lane = self.__cur_lane
    
    @property
    def name(self) -> str:
        return f'{self.__class__.__name__}::{self.__car_name}'