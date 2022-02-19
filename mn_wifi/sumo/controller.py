from __future__ import annotations

from abc import ABCMeta, abstractmethod
from threading import Thread
import traceback
from typing import Callable, List, Optional

from . import traci
from traci.exceptions import TraCIException


class SumoStepListener(traci.StepListener, metaclass=ABCMeta):
    
    @classmethod
    def Substeps(cls, priority: int=0):
        def __inner(task): 
            def __wrapper(*args,**kwargs): 
                return task(*args,**kwargs)
            setattr(__wrapper, 'priority', priority)
            return __wrapper
        return __inner
    
    @property
    @abstractmethod
    def name(self) -> str: 
        return 'SumoStepListener'

    def __init__(self) -> None:
        super().__init__() 
        self.__cur_step = 0
        self.__tasks: List[Callable] = []
        for _, field in self.__class__.__dict__.items():
            f_priority = getattr(field, 'priority', -1)
            if f_priority >= 0: self.__tasks.append(field)
        self.__tasks.sort(key=lambda t: -t.priority)
        self.is_started = False
        self.is_alive = False

    @property
    def cur_time(self) -> float: 
        return traci.simulation.getTime()

    @property
    def cur_step(self) -> int: 
        return self.__cur_step

    def before_listening(self): 
        print(f'{self.name.capitalize()} START LISTENING')
        return None

    def step(self,t) -> bool:
        '''
        An implementation for the abstract method, traci.StepListener.step() 
        The parameter t is the number of steps executed. In this implementation, 
        our program will execute `t` times _step_core method. It will always 
        return True even when the _step_core throws an error, because the program 
        will stop if the step method return False.
        '''
        if self.is_started and not self.is_alive: 
            return True
        if t == 0: t = 1 
        for _ in range(t): 
            self.__cur_step += 1
            self._step_one()
        return True 

    def _step_one(self) -> bool:
        '''
        The method will be invoked exact once in each traci step
        '''
        try: 
            for task in self.__tasks: task(self)
            if not self.is_started: 
                self.is_started = True
                self.is_alive = True
        except TraCIException as traci_exception:
            if self.is_started: 
                self.is_alive = False
                raise traci_exception
        return True
    
    def after_listening(self): 
        print(f'{self.name.capitalize()} STOP LISTENING')
        return None


class SumoControlThread(Thread): 
    
    def __init__(self, name, port:int=8813, priority:int=1, verbose=False):
        Thread.__init__(self,name=name)   
        traci.init(port)
        traci.setOrder(priority)
        self.__is_verbose = verbose
        self.__listeners: List[SumoStepListener] = []

    def run(self) -> None: 
        for listener in self.__listeners: listener.before_listening()
        while traci.simulation.getMinExpectedNumber() > 0 and len(self.__listeners) > 0:
            try: 
                traci.simulationStep()
            except: 
                for listener in self.__listeners: 
                    if listener.is_alive: continue
                    if not listener.is_started: continue
                    listener.after_listening()
                    self.remove(listener)
                if self.__is_verbose: print(traceback.format_exc())
        for listener in self.__listeners: listener.after_listening()
        traci.close()
        return None
    
    def add(self, listener: SumoStepListener) -> Optional[int]:
        self.__listeners.append(listener)
        listener.id = traci.addStepListener(listener)
        return listener.id
    
    def remove(self, listener: SumoStepListener) -> bool:
        self.__listeners.remove(listener)
        return traci.removeStepListener(listener.id)