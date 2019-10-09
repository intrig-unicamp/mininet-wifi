"""

    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)


"""

from __future__ import division
from math import atan2
from random import randint
from time import sleep
import warnings
from threading import Thread as thread
from random import randrange
import matplotlib.cbook
from pylab import ginput as ginp
from pylab import math, cos, sin, np

from mn_wifi.plot import plot2d
from mn_wifi.node import AP
from mininet.log import info


try:
    warnings.filterwarnings("ignore", category=matplotlib.cbook.mplDeprecation)
except:
    pass


class vanet(object):

    # variables
    scatter = 0
    com_lines = []
    all_points = []
    roads = []
    points = []
    interX = {}
    interY = {}
    time_per_iteration = 100 * math.pow(10, -3)

    def __init__(self, **params):
        from mn_wifi.mobility import mobility
        mobility.thread_ = thread(name='vanet', target=self.start,
                                  kwargs=dict(params,))
        mobility.thread_.daemon = True
        mobility.thread_._keep_alive = True
        mobility.thread_.start()

    def start(self, **params):
        'start topology'
        from mn_wifi.mobility import mobility

        cars = params['cars']
        mobility.stations = cars
        mobility.aps = params['aps']
        mobility.mobileNodes = cars
        [self.roads.append(x) for x in range(params['nroads'])]
        [self.points.append(x) for x in range(params['nroads'])]
        plot2d.instantiateGraph(params['min_x'], params['min_y'],
                                params['max_x'], params['max_y'])

        self.display_grid(params['aps'], params['conn'], params['nroads'])
        self.display_cars(cars)
        plot2d.plotGraph(cars, [])
        self.setWifiParameters(mobility)
        while mobility.thread_._keep_alive:
            [self.scatter, self.com_lines] = \
                self.simulate_car_movement(cars, params['aps'], self.scatter,
                                           self.com_lines, mobility)
            sleep(0.0001)

    @classmethod
    def setWifiParameters(cls, mobility):
        from threading import Thread as thread
        thread = thread(name='wifiParameters', target=mobility.parameters)
        thread.start()

    @classmethod
    def get_line(cls, x1, y1, x2, y2):
        points = []
        issteep = abs(y2 - y1) > abs(x2 - x1)
        if issteep:
            x1, y1 = y1, x1
            x2, y2 = y2, x2
        rev = False
        if x1 > x2:
            x1, x2 = x2, x1
            y1, y2 = y2, y1
            rev = True
        deltax = x2 - x1
        deltay = abs(y2 - y1)
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

    def display_grid(self, aps, conn, nroads):

        for n in range(nroads):
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
                # Get all the points in the line
                self.points[n] = self.get_line(int(x1[0]), int(y1[0]),
                                               int(x1[1]), int(y1[1]))
            else:
                self.points[n] = self.get_line(int(self.all_points[n][0]),
                                               int(self.all_points[n][1]),
                                               int(p[0][0]), int(p[0][1]))
            x1 = [x[0] for x in self.points[n]]
            y1 = [x[1] for x in self.points[n]]

            self.interX[n] = x1
            self.interY[n] = y1

            # Create a line object with the x y values of the points in a line
            self.roads[n] = plot2d.line2d(x1, y1, color='g')
            plot2d.line(self.roads[n])

        for bs in aps:
            bs.prop = ginp(1)[0]
            bs_x = round(bs.prop[0], 2)
            bs_y = round(bs.prop[1], 2)
            self.scatter = plot2d.scatter(float(bs_x), float(bs_y))
            bs.params['position'] = bs_x, bs_y, 0
            bs.set_pos_wmediumd(bs.params['position'])
            plot2d.instantiateNode(bs)
            plot2d.instantiateAnnotate(bs)
            plot2d.instantiateCircle(bs)
            plot2d.text(bs, float(bs_x), float(bs_y))
            plot2d.circle(bs, float(bs_x), float(bs_y))
            plot2d.draw()

        sleep(1)
        if 'src' in conn:
            for c in range(len(conn['src'])):
                line = plot2d.line2d([conn['src'][c].params['position'][0],
                                      conn['dst'][c].params['position'][0]],
                                     [conn['src'][c].params['position'][1],
                                      conn['dst'][c].params['position'][1]],
                                     'b', ls='dashed')
                plot2d.line(line)

    def display_cars(self, cars):

        car_lines = []

        for _ in range(len(cars)):
            n = randint(0, len(self.roads)-1)
            car_lines.append(self.roads[n])

        for n in range(len(self.roads)-1):
            road = self.roads[n]
            line_data = road.get_data()

            x_min, x_max = self.lineX(line_data)
            y_min, y_max = self.lineY(line_data)

            locX = (x_max - x_min) / 2 + x_min
            locY = (y_max - y_min) / 2 + y_min

            plot2d.lineTxt(locX, locY, n + 1)

        # temporal variable to hold values of cars
        points = [[], []]

        # get X cars in the graph
        i = 0
        for car in cars:
            i += 1
            random_index = randrange(len(car_lines))
            car.currentRoad = int(random_index)
            car_line = car_lines[random_index]
            point = car_line.get_xydata()[0]  # first point in the graph

            # calculate the angle
            line_data = car_line.get_data()
            ang = self.calculateAngle(line_data)

            car.prop = self.carProp(point, ang, x_min, x_max, y_min, y_max)

            # for the even cars shift angle to negative
            # so that it goes in opposite direction from car1
            car.i = i
            if i % 2 == 0:
                ang = ang + math.pi
                # for this car get the last point as positions
                point = car_line.get_xydata()[-1]

            x_min, x_max = self.lineX(line_data)
            y_min, y_max = self.lineY(line_data)

            car.initial = self.carPoint(point)

            # add scatter
            points[0].append(point[0])
            points[1].append(point[1])

            self.speed(car)  # Get Speed

        # plot cars
        self.scatter = plot2d.scatter(points[0], points[1])

    @classmethod
    def lineX(cls, line_data):
        """ get the minimum and maximums of the line"""
        x_min = min(line_data[0])
        x_max = max(line_data[0])
        return x_min, x_max

    @classmethod
    def lineY(cls, line_data):
        """ get the minimum and maximums of the line"""
        y_min = min(line_data[1])
        y_max = max(line_data[1])
        return y_min, y_max

    @classmethod
    def speed(cls, car):
        car.speed = car.max_speed, car.min_speed

    @classmethod
    def calculateAngle(cls, line_data):
        """Calculate Angle"""
        xdiff = line_data[0][-1] - line_data[0][0]
        ydiff = line_data[1][-1] - line_data[1][0]
        ang = atan2(ydiff, xdiff)
        return ang

    @classmethod
    def carProp(cls, point, ang, x_min, x_max, y_min, y_max):
        temp = []
        temp.append(point[0])
        temp.append(point[1])
        temp.append(ang)
        temp.append(x_min)
        temp.append(x_max)
        temp.append(y_min)
        temp.append(y_max)
        return temp

    @classmethod
    def carPoint(cls, point):
        temp = []
        temp.append(point[0])
        temp.append(point[1])
        return temp

    def line_prop(self, line, car):

        line_data = line.get_data()  # Get the x and y values of the points in the line
        ang = self.calculateAngle(line_data)  # Get angle
        point = list(line.get_xydata()[0])  # first point in the graph

        if car.i % 2 == 0:
            ang = ang + math.pi
            point = list(line.get_xydata()[-1])  # for this car get the last point as positions

        x_min, x_max = self.lineX(line_data)
        y_min, y_max = self.lineY(line_data)

        car.prop = self.carProp(point, ang, x_min, x_max, y_min, y_max)
        car.initial = self.carPoint(point)

    def repeat (self, car):
        # Check if it is the last mile
        lastRoad = True

        if car.i % 2 == 0:
            for n in range(len(self.roads)-1, 0, -1):
                if n < car.currentRoad:
                    car.currentRoad = n
                    # get properties of each line in a path
                    self.line_prop(self.roads[car.currentRoad], car)
                    lastRoad = False
                    break
            if lastRoad:
                car.currentRoad = len(self.roads) - 1
                self.line_prop(self.roads[car.currentRoad], car)
        else:
            for n in range(len(self.roads)-1):
                if n > car.currentRoad:
                    car.currentRoad = n
                    # get properties of each line in a path
                    self.line_prop(self.roads[car.currentRoad], car)
                    lastRoad = False
                    break
            if lastRoad:
                car.currentRoad = 0
                self.line_prop(self.roads[car.currentRoad], car)

    def findIntersection(self):
        # have to work on
        list1 = [list(a) for a in zip(self.interX[0], self.interY[0])]
        list2 = [list(a) for a in zip(self.interX[2], self.interY[2])]

        first_tuple_list = [tuple(lst) for lst in list1]
        secnd_tuple_list = [tuple(lst) for lst in list2]

        first_set = set(first_tuple_list)
        secnd_set = set(secnd_tuple_list)
        (element,) = first_set.intersection(secnd_set)
        info(element[0])

    def simulate_car_movement(self, cars, aps, scatter,
                              com_lines, mobility):
        # temporal variables
        points = [[], []]
        scatter.remove()

        nodes = cars + aps

        while com_lines:
            com_lines[0].remove()
            del com_lines[0]

        # iterate over each car
        for car in cars:
            # get all the properties of the car
            vel = round(np.random.uniform(car.speed[0], car.speed[1]))
            pos_x = car.prop[0]
            pos_y = car.prop[1]

            car.params['position'] = pos_x, pos_y, 0
            car.set_pos_wmediumd(car.params['position'])
            angle = car.prop[2]

            # calculate new position of the car
            pos_x = pos_x + vel * cos(angle) * self.time_per_iteration
            pos_y = pos_y + vel * sin(angle) * self.time_per_iteration

            if (pos_x < car.prop[3] or pos_x > car.prop[4]) \
                or (pos_y < car.prop[5] or pos_y > car.prop[6]):
                self.repeat(car)
                points[0].append(car.initial[0])
                points[1].append(car.initial[1])
            else:
                car.prop[0] = pos_x
                car.prop[1] = pos_y
                points[0].append(pos_x)
                points[1].append(pos_y)

                for node in nodes:
                    if nodes == car:
                        continue
                    else:
                        # compute to see if vehicle is in range
                        inside = math.pow((node.prop[0] - pos_x), 2) + \
                                 math.pow((node.prop[1] - pos_y), 2)
                        if inside <= math.pow(node.params['range'][0], 2):
                            if isinstance(node, AP):
                                color = 'black'
                            else:
                                color = 'r'
                            line = plot2d.line2d([pos_x, node.prop[0]],
                                                 [pos_y, node.prop[1]],
                                                 color=color)
                            com_lines.append(line)
                            plot2d.line(line)

            plot2d.update(car)

        plot2d.pause()
        if not mobility.thread_._keep_alive:
            exit()

        scatter = plot2d.scatter(points[0], points[1])
        plot2d.draw()

        return [scatter, com_lines]
