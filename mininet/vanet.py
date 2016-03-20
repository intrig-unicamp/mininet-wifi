"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

from __future__ import division 
from pylab import math, cos, sin, np
from math import atan2
from pylab import ginput as ginp
from mininet.wifiPlot import plot
from random import randrange

import warnings
import matplotlib.cbook
from mininet.wifiEmulationEnvironment import emulationEnvironment

warnings.filterwarnings("ignore",category=matplotlib.cbook.mplDeprecation)

class vanet( object ):

    #variables
    COM_RANGE = 50 # communication range
    scatter = 0

    com_lines =[]  
    all_points = []
    road = []
    points = []
    totalRoads = []
    range = []
    
    bss = {}
    currentRoad = {}
    interX = {}
    interY = {}
    velo = {}
    initial = {}    
    cars ={}
    
    time_per_iteraiton = 100*math.pow(10,-3)
    
    def __init__( self, cars, baseStations, nroads, MAX_X, MAX_Y ):
        
        [self.road.append(x) for x in range(0, nroads )]
        [self.points.append(x) for x in range(0, nroads )]
        [self.totalRoads.append(x) for x in range(0, nroads )]
        [self.range.append(cars[x].range) for x in range(0, len(cars) )]
        
        plot.instantiateGraph(MAX_X, MAX_Y)    
        
        self.display_grid(baseStations, nroads)
        self.display_cars(cars)
        
        while emulationEnvironment.continue_:
            [self.scatter,self.com_lines] = self.simulate_car_movement(self.scatter,self.com_lines)

    def get_line(self, x1, y1, x2, y2):
        points = []
        issteep = abs(y2-y1) > abs(x2-x1)
        if issteep:
            x1, y1 = y1, x1
            x2, y2 = y2, x2
        rev = False
        if x1 > x2:
            x1, x2 = x2, x1
            y1, y2 = y2, y1
            rev = True
        deltax = x2 - x1
        deltay = abs(y2-y1)
        error = int(deltax / 2)
        y = y1
        ystep = None
        if y1 < y2:
            ystep = 1
        else:
            ystep = -1
        for x in range(x1, x2 + 1):
            if issteep:
                points.append((y, x))
            else:
                points.append((x, y))
            error -= deltay
            if error < 0:
                y += ystep
                error += deltax
        # Reverse the list if the coordinates were reversed
        if rev:
            points.reverse()
        return points    

    def display_grid(self, baseStations, nroads):
        
        for n in range(0, nroads):
            if n == 0:
                p = ginp(2)
                self.points[n] = p
                self.all_points = p
            else:
                p = ginp(1)
                self.points[n] = p
                self.all_points.append(p[0])
                
            x1 = [x[0] for x in self.points[n]]
            y1 = [x[1] for x in self.points[n]]
            if n == 0:
                self.points[n] = self.get_line(int(x1[0]),int(y1[0]),int(x1[1]),int(y1[1])) # Get all the points in the line
            else:
                self.points[n] = self.get_line(int(self.all_points[n][0]),int(self.all_points[n][1]),int(p[0][0]),int(p[0][1])) # Get all the points in the line
            
            x1 = [x[0] for x in self.points[n]]
            y1 = [x[1] for x in self.points[n]]
            
            self.interX[n] = x1
            self.interY[n] = y1            
            
            self.road[n] =  plot.plotLine2d(x1,y1, color='g') # Create a line object with the x y values of the points in a line
            plot.plotLine(self.road[n])
            #plot.plotDraw()
        
        for i in range(len(baseStations)):
            self.bss[baseStations[i]] = ginp(1)[0]
            bs_x = self.bss[baseStations[i]][0]
            bs_y = self.bss[baseStations[i]][1]
            self.scatter = plot.plotScatter(bs_x, bs_y) 
            baseStations[i].position = bs_x, bs_y, 0
            plot.instantiateAnnotate(baseStations[i])
            plot.instantiateCircle(baseStations[i])
            plot.drawTxt(baseStations[i])
            plot.drawCircle(baseStations[i])
            plot.plotDraw()
        
    
    def display_cars(self, cars):
    
        car_lines = []
        
        for n in range(0,len(cars)):
            car_lines.append(self.road[n])
        
        for n in range(0, len(self.totalRoads)):
            
            road = self.road[n]            
            line_data = road.get_data()
            
            x_min, x_max = self.lineX(line_data)
            y_min, y_max = self.lineY(line_data)

            locX = (x_max - x_min)/2 + x_min
            locY = (y_max - y_min)/2 + y_min
            
            plot.plotLineTxt(locX, locY, n+1)
            
        #temporal variable to hold values of cars
        points = [[],[]]
        
        #get X cars in the graph
        for n in range(0,len(cars)):
            
            random_index = randrange(0,len(car_lines))            
            self.currentRoad[cars[n]] = random_index            
            car_line = car_lines[random_index]
            point = car_line.get_xydata()[0] #first point in the graph
            
            #calculate the angle
            line_data = car_line.get_data()            
            ang = self.calculateAngle(line_data)
            
            self.cars[cars[n]] = self.carProperties(point, ang, x_min, x_max)
            
            #for the even cars shift angle to negative
            #so that it goes in opposite direction from car1
            i = self.cars.keys().index(cars[n])
            if i%2==0:
                ang = ang + math.pi
                point = car_line.get_xydata()[-1] #for this car get the last point as positions
    
            x_min, x_max = self.lineX(line_data)
            
            self.initial[cars[n]] = self.carPoint(point)
             
            #add scatter
            points[0].append(point[0])
            points[1].append(point[1])
                        
            self.speed(cars[n]) # Get Speed
            
            #Useful to Graph
            plot.instantiateCircle(cars[n])
            plot.instantiateAnnotate(cars[n])
            
        #plot the cars
        self.scatter = plot.plotScatter(points[0],points[1])
      
    def lineX(self, line_data):
        """ get the minimum and maximums of the line"""
        x_min = min(line_data[0])
        x_max = max(line_data[0])
        return x_min, x_max
    
    def lineY(self, line_data):
        """ get the minimum and maximums of the line"""
        y_min = min(line_data[1])
        y_max = max(line_data[1])
        return y_min, y_max
      
    def speed(self, car):
        i = self.cars.keys().index(car)
        if i%2==0:
            self.velo[car] = car.max_speed, car.min_speed
        else:
            self.velo[car] = car.max_speed, car.min_speed
      
    def calculateAngle(self, line_data):
        """Calculate Angle"""
        xdiff = line_data[0][-1]-line_data[0][0]
        ydiff = line_data[1][-1]-line_data[1][0]
        ang = atan2(ydiff,xdiff)
        return ang
    
    def carProperties(self, point, ang, x_min, x_max):
        temp = []
        temp.append(point[0])
        temp.append(point[1])
        temp.append(ang)
        temp.append(x_min)
        temp.append(x_max)
        return temp
    
    def carPoint(self, point):
        temp = []
        temp.append(point[0])
        temp.append(point[1])
        return temp
      
    def line_properties(self, line, car):
        
        i = self.cars.keys().index(car)
        line_data = line.get_data() # Get the x and y values of the points in the line
        ang = self.calculateAngle(line_data) #Get angle         
        point = list(line.get_xydata()[0]) #first point in the graph
        
        if i%2==0:
            ang = ang + math.pi
            point = list(line.get_xydata()[-1]) #for this car get the last point as positions
    
        x_min, x_max = self.lineX(line_data)
        
        self.cars[car] = self.carProperties(point, ang, x_min, x_max)
        self.initial[car] = self.carPoint(point)
                
    def repeat (self, car):
        #Check if it is the last mile
        lastRoad = True
        i = self.cars.keys().index(car)
        
        if i%2==0:
            for n in reversed(self.totalRoads):
                if n < self.currentRoad[car]:
                    self.line_properties(self.road[n], car) # get properties of each line in a path
                    self.currentRoad[car] = n
                    lastRoad = False
                    break
            if lastRoad:
                self.line_properties(self.road[len(self.totalRoads)-1], car) 
                self.currentRoad[car] = len(self.totalRoads)-1            
        else:
            for n in (self.totalRoads):
                if n > self.currentRoad[car]:
                    self.line_properties(self.road[n], car) # get properties of each line in a path
                    self.currentRoad[car] = n
                    lastRoad = False
                    break
            if lastRoad:
                self.line_properties(self.road[0], car) 
                self.currentRoad[car] = 0
    
    def findIntersection(self):
        #have to work on
        list1 = [list(a) for a in zip(self.interX[0], self.interY[0])]
        list2= [list(a) for a in zip(self.interX[2], self.interY[2])]
        
        first_tuple_list = [tuple(lst) for lst in list1]
        secnd_tuple_list = [tuple(lst) for lst in list2]
        
        first_set = set(first_tuple_list)
        secnd_set = set(secnd_tuple_list)
        (element,) = first_set.intersection(secnd_set) 
        print element[0]
    
    
    def simulate_car_movement(self,scatter,com_lines):
        #temporal variables
        points= [[],[]]
        scatter.remove()
        
        nodes = dict(self.cars.items() + self.bss.items())
        
        #print initial
        while com_lines:
            com_lines[0].remove()
            del(com_lines[0])
        
        #iterate over each car
        for car in self.cars:  
            #get all the properties of the car
            velocity = round(np.random.uniform(self.velo[car][0],self.velo[car][1]))
            position_x = self.cars[car][0]
            position_y = self.cars[car][1]
            
            car.position = position_x, position_y, 0
            angle = self.cars[car][2]
           
            #calculate new position of the car
            position_x = position_x + velocity*cos(angle)*self.time_per_iteraiton
            position_y = position_y + velocity*sin(angle)*self.time_per_iteraiton
           
            #check if car gets out of the line
            # no need to check for y coordinates as car follows the line
            if position_x < self.cars[car][3] or position_x> self.cars[car][4]:
                self.repeat(car)
                points[0].append(self.initial[car][0])
                points[1].append(self.initial[car][1])
            else:
                self.cars[car][0] = position_x
                self.cars[car][1] = position_y
                points[0].append(position_x)
                points[1].append(position_y)
               
                for node in nodes:
                    if nodes[node] == nodes[car]:
                        continue
                    else:
                        #compute to see if vehicle is in range
                        inside = math.pow((nodes[node][0]-position_x),2) + math.pow((nodes[node][1]-position_y),2)
                        if inside <= math.pow(node.range+30,2):
                            line = plot.plotLine2d([position_x,nodes[node][0]],[position_y,nodes[node][1]], color='r')
                            com_lines.append(line)
                            plot.plotLine(line)
  
            plot.drawTxt(car)
            plot.drawCircle(car)               
          
        scatter = plot.plotScatter(points[0],points[1])        
        plot.plotDraw()
    
        return [scatter,com_lines]