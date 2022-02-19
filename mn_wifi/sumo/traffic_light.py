from traci import trafficlight
from typing import Optional, List, Tuple

'''
THE CLASS CURRENTLY DID NOT SUPPORT THE TRACIT IN MININET WIFI, 
PLEASE UPDATE THE TRACIT IN THERE.
'''

class SumoTrafficLight(object): 

    def __init__(self, tfl_id: str, states: List[str]):
        self.__tfl_id = tfl_id
        self.__states = states.copy()
        self.distance = 0

    @property
    def name(self) -> str:
        return self.__tfl_id

    @property
    def phase(self) -> str: 
        index = trafficlight.getPhase(self.__tfl_id)
        return self.__states[index]

    @phase.setter
    def phase(self, val: str): 
        state_index = self.__states.index(val)
        return trafficlight.setPhase(self.__tfl_id, state_index)

    def set_state_with_duration(self, state: str, duration: Optional[int]=None) -> bool:
        try: state_index = self.__states.index(state)
        except: return False
        trafficlight.setPhaseDuration(self.__tfl_id, state_index, duration)
    
    def get_leader_with_distance(self) -> Tuple: 
        return (None, None)
