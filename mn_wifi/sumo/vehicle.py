from __future__ import annotations
from typing import Tuple
from math import sqrt
import traci

class SumoVehicle(object): 

    def __init__(self, sumo_v_id: str):
        self.__vlc = traci.vehicle
        self.__id = sumo_v_id
        self.__duration = 0

    @property
    def safe_gap(self) -> int: 
        return self.__vlc.getMinGap(self.__id)

    @property
    def position(self) -> Tuple: 
        x,y = self.__vlc.getPosition(self.__id)
        return (x, y, 0)
    
    @property
    def lane(self) -> int: 
        return self.__vlc.getLaneIndex(self.__id) 
    
    @lane.setter
    def lane(self, value: int) -> None: 
        if self.__duration == 0: self.__duration = int(traci.simulation.getTime()) + 1
        return self.__vlc.changeLane(self.__id, value, self.__duration)

    @property
    def speed(self) -> int:
        return self.__vlc.getSpeed(self.__id)
    
    @speed.setter
    def speed(self, ex_sp)-> None: 
        self.__vlc.setSpeed(self.__id, ex_sp)
    
    @property
    def road(self) -> str:
        return self.__vlc.getRoadID(self.__id)

    @property
    def distance(self) -> int: 
        return self.__vlc.getDistance(self.__id) 

    @property
    def heading(self) -> float:
        return self.__vlc.getAngle(self.__id)
    
    @property
    def name(self) -> int: 
        return self.__id

    '''
    The is a bug for the traci.vehicle.getLeader method,
    which will return None, if we change its lane during
    SUMO default lane changing behaviour. 
    '''
    def get_leader_with_distance(self) -> Tuple[str, float]: 
        out = self.__vlc.getLeader(self.__id)
        return out if out is not None else (None, None)

    def distance_to(self, other_pos: Tuple[int, int, int]) -> int: 
        x1, y1, z1 = self.position
        x2, y2, z2 = other_pos
        dis = sqrt(pow(x1-x2, 2) + pow(y1-y2, 2) + pow(z1-z2, 2))
        return dis if x1 > x2 else -dis

    