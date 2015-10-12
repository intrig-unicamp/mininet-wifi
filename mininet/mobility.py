# coding: utf-8
#
#  Copyright (C) 2008-2010 Istituto per l'Interscambio Scientifico I.S.I.
#  You can contact us by email (isi@isi.it) or write to:
#  ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 
#
#  This program was written by André Panisson <panisson@gmail.com>
#
'''
Created on Jan 24, 2012

@author: André Panisson
@contact: panisson@gmail.com
@organization: ISI Foundation, Torino, Italy
@source: https://github.com/panisson/pymobility
@copyright: http://dx.doi.org/10.5281/zenodo.9873
'''
import numpy as np
from numpy.random import rand

# define a Uniform Distribution
U = lambda MIN, MAX, SAMPLES: rand(*SAMPLES.shape) * (MAX - MIN) + MIN

# define a Truncated Power Law Distribution
P = lambda ALPHA, MIN, MAX, SAMPLES: ((MAX ** (ALPHA+1.) - 1.) * rand(*SAMPLES.shape) + 1.) ** (1./(ALPHA+1.))

# define an Exponential Distribution
E = lambda SCALE, SAMPLES: -SCALE*np.log(rand(*SAMPLES.shape))

# *************** Palm state probability **********************
def pause_probability_init(pause_low, pause_high, speed_low, speed_high, max_x, max_y):
    alpha1 = ((pause_high+pause_low)*(speed_high-speed_low))/(2*np.log(speed_high/speed_low))
    delta1 = np.sqrt((max_x*max_x) +(max_y*max_y))
    return alpha1/(alpha1+delta1)

# *************** Palm residual ******************************
def residual_time(mean, delta, shape=(1,)):
    t1 = mean - delta;
    t2 = mean + delta;
    u = rand(*shape);
    residual = np.zeros(shape)
    if delta != 0.0:
        case_1_u = u < (2.*t1/(t1+t2))
        residual[case_1_u] = u[case_1_u]*(t1+t2)/2.
        residual[np.logical_not(case_1_u)] = t2-np.sqrt((1.-u[np.logical_not(case_1_u)])*(t2*t2 - t1*t1))
    else:
        residual=u*mean  
    return residual

# *********** Initial speed ***************************
def initial_speed(speed_mean, speed_delta, shape=(1,)):
    v0 = speed_mean - speed_delta
    v1 = speed_mean + speed_delta
    u = rand(*shape)
    return pow(v1, u) / pow(v0, u - 1)

def init_random_waypoint(nr_nodes, max_x, max_y,
                         speed_low, speed_high, pause_low, pause_high):

    x = np.empty(nr_nodes)
    y = np.empty(nr_nodes)
    x_waypoint = np.empty(nr_nodes)
    y_waypoint = np.empty(nr_nodes)
    speed = np.empty(nr_nodes)
    pause_time = np.empty(nr_nodes)

    speed_low = float(speed_low)
    speed_high = float(speed_high)

    moving = np.ones(nr_nodes)
    speed_mean, speed_delta = (speed_low+speed_high)/2., (speed_high-speed_low)/2.
    pause_mean, pause_delta = (pause_low+pause_high)/2., (pause_high-pause_low)/2.

    # steady-state pause probability for Random Waypoint
    q0 = pause_probability_init(pause_low, pause_high, speed_low, speed_high, max_x, max_y)
    
    for i in range(nr_nodes):
        
        while True:
            if rand() < q0:
                moving[i] = 0.
                # M_0
                x1 = rand()*max_x
                x2 = rand()*max_x
                # M_1
                y1 = rand()*max_y
                y2 = rand()*max_y
                break
            else:
                # M_0
                x1 = rand()*max_x
                x2 = rand()*max_x
                # M_1
                y1 = rand()*max_y
                y2 = rand()*max_y
                
                #r is a ratio of the length of the randomly chosen path over
                # the length of a diagonal across the simulation area
                # ||M_1 - M_0||
                r = np.sqrt(((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1))/(max_x*max_x + max_y*max_y))
                if rand() < r:
                    moving[i] = 1.
                    break

        x[i] = x1
        y[i] = y1

        x_waypoint[i] = x2
        y_waypoint[i] = y2

    # steady-state positions
    # initially the node has traveled a proportion u2 of the path from (x1,y1) to (x2,y2)
    u2 = rand(*x.shape)
    x[:] = u2*x + (1 - u2)*x_waypoint
    y[:] = u2*y + (1 - u2)*y_waypoint

    # steady-state speed and pause time
    paused_bool = moving==0.
    paused_idx = np.where(paused_bool)[0]
    pause_time[paused_idx] = residual_time(pause_mean, pause_delta, paused_idx.shape)
    speed[paused_idx] = 0.0

    moving_bool = np.logical_not(paused_bool)
    moving_idx = np.where(moving_bool)[0]
    pause_time[moving_idx] = 0.0
    speed[moving_idx] = initial_speed(speed_mean,speed_delta, moving_idx.shape)

    return x,y,x_waypoint,y_waypoint,speed,pause_time

class RandomWaypoint(object):
    
    def __init__(self, nr_nodes, dimensions, velocity=(0.1, 1.), wt_max=None):
        '''
        Random Waypoint model.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
          
        keyword arguments:
        
          *velocity*:
            Tuple of Integers, the minimum and maximum values for node velocity.
          
          *wt_max*:
            Integer, the maximum wait time for node pauses.
            If wt_max is 0 or None, there is no pause time.
        '''
        
        self.nr_nodes = nr_nodes
        self.dimensions = dimensions
        self.velocity = velocity
        self.wt_max = wt_max
        self.init_stationary = True
    
    def __iter__(self):
        
        MAX_X,MAX_Y = self.dimensions
        MIN_V, MAX_V = self.velocity
        
        wt_min = 0.
        
        if self.init_stationary:

            x, y, x_waypoint, y_waypoint, velocity, wt = \
                init_random_waypoint(self.nr_nodes, MAX_X, MAX_Y, MIN_V, MAX_V, wt_min, 
                             (self.wt_max if self.wt_max is not None else 0.))

        else:

            NODES = np.arange(self.nr_nodes)
            print NODES
            x = U(0, MAX_X, NODES)
            y = U(0, MAX_Y, NODES)
            x_waypoint = U(0, MAX_X, NODES)
            y_waypoint = U(0, MAX_Y, NODES)
            wt = np.zeros(self.nr_nodes)
            velocity = U(MIN_V, MAX_V, NODES)

        theta = np.arctan2(y_waypoint - y, x_waypoint - x)
        costheta = np.cos(theta)
        sintheta = np.sin(theta)
        
        while True:
            # update node position
            x += velocity * costheta
            y += velocity * sintheta
            # calculate distance to waypoint
            d = np.sqrt(np.square(y_waypoint-y) + np.square(x_waypoint-x))
            # update info for arrived nodes
            arrived = np.where(np.logical_and(d<=velocity, wt<=0.))[0]
            
            # step back for nodes that surpassed waypoint
            x[arrived] = x_waypoint[arrived]
            y[arrived] = y_waypoint[arrived]
            
            if self.wt_max:
                velocity[arrived] = 0.
                wt[arrived] = U(0, self.wt_max, arrived)
                # update info for paused nodes
                wt[np.where(velocity==0.)[0]] -= 1.
                # update info for moving nodes
                arrived = np.where(np.logical_and(velocity==0., wt<0.))[0]
            
            if arrived.size > 0:
                x_waypoint[arrived] = U(0, MAX_X, arrived)
                y_waypoint[arrived] = U(0, MAX_Y, arrived)
                velocity[arrived] = U(MIN_V, MAX_V, arrived)
                theta[arrived] = np.arctan2(y_waypoint[arrived] - y[arrived], x_waypoint[arrived] - x[arrived])
                costheta[arrived] = np.cos(theta[arrived])
                sintheta[arrived] = np.sin(theta[arrived])
            
            self.velocity = velocity
            self.wt = wt
            yield np.dstack((x,y))[0]

class StochasticWalk(object):
    
    def __init__(self, nr_nodes, dimensions, FL_DISTR, VELOCITY_DISTR, WT_DISTR=None, border_policy='reflect'):
        '''
        Base implementation for models with direction uniformly chosen from [0,pi]:
        random_direction, random_walk, truncated_levy_walk
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
            
          *FL_DISTR*:
            A function that, given a set of samples, 
             returns another set with the same size of the input set.
            This function should implement the distribution of flight lengths
             to be used in the model.
             
          *VELOCITY_DISTR*:
            A function that, given a set of flight lengths, 
             returns another set with the same size of the input set.
            This function should implement the distribution of velocities
             to be used in the model, as random or as a function of the flight lengths.
          
        keyword arguments:
        
          *WT_DISTR*:
            A function that, given a set of samples, 
             returns another set with the same size of the input set.
            This function should implement the distribution of wait times
             to be used in the node pause.
            If WT_DISTR is 0 or None, there is no pause time.
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        self.collect_fl_stats = False
        self.collect_wt_stats = False
        self.border_policy = border_policy
        self.dimensions = dimensions
        self.nr_nodes = nr_nodes
        self.FL_DISTR = FL_DISTR
        self.VELOCITY_DISTR = VELOCITY_DISTR
        self.WT_DISTR = WT_DISTR
        
    def __iter__(self):
        def reflect(xy):
            # node bounces on the margins
            b = np.where(xy[:,0]<0)[0]
            if b.size > 0:
                xy[b,0] = - xy[b,0]
                cosintheta[b,0] = -cosintheta[b,0]
            b = np.where(xy[:,0]>MAX_X)[0]
            if b.size > 0:
                xy[b,0] = 2*MAX_X - xy[b,0]
                cosintheta[b,0] = -cosintheta[b,0]
            b = np.where(xy[:,1]<0)[0]
            if b.size > 0:
                xy[b,1] = - xy[b,1]
                cosintheta[b,1] = -cosintheta[b,1]
            b = np.where(xy[:,1]>MAX_Y)[0]
            if b.size > 0:
                xy[b,1] = 2*MAX_Y - xy[b,1]
                cosintheta[b,1] = -cosintheta[b,1]
                
        
        def wrap(xy):
            b = np.where(xy[:,0]<0)[0]
            if b.size > 0: xy[b,0] += MAX_X
            b = np.where(xy[:,0]>MAX_X)[0]
            if b.size > 0: xy[b,0] -= MAX_X
            b = np.where(xy[:,1]<0)[0]
            if b.size > 0: xy[b,1] += MAX_Y
            b = np.where(xy[:,1]>MAX_Y)[0]
            if b.size > 0: xy[b,1] -= MAX_Y
        
        if self.border_policy == 'reflect':
            borderp = reflect
        elif self.border_policy == 'wrap':
            borderp = wrap
        else:
            borderp = self.border_policy
        
        MAX_X, MAX_Y = self.dimensions
        NODES = np.arange(self.nr_nodes)
        xy = U(0, MAX_X, np.dstack((NODES,NODES))[0])
        fl = self.FL_DISTR(NODES)
        velocity = self.VELOCITY_DISTR(fl)
        theta = U(0, 1.8*np.pi, NODES)
        cosintheta = np.dstack((np.cos(theta), np.sin(theta)))[0] * np.dstack((velocity,velocity))[0]
        wt = np.zeros(self.nr_nodes)
        
        if self.collect_fl_stats: self.fl_stats = list(fl)
        if  self.collect_wt_stats: self.wt_stats = list(wt)
        while True:
    
            xy += cosintheta
            fl -= velocity
          
            # step back for nodes that surpassed fl
            arrived = np.where(np.logical_and(velocity>0., fl<=0.))[0]
            
            if arrived.size > 0:
                diff = fl.take(arrived) / velocity.take(arrived)
                xy[arrived] += np.dstack((diff,diff))[0] * cosintheta[arrived]
                
            # apply border policy
            borderp(xy)
            
            if self.WT_DISTR:
                velocity[arrived] = 0.
                wt[arrived] = self.WT_DISTR(arrived)
                if self.collect_wt_stats: self.wt_stats.extend(wt[arrived])
                # update info for paused nodes
                wt[np.where(velocity==0.)[0]] -= 1.
                arrived = np.where(np.logical_and(velocity==0., wt<0.))[0]
            
            # update info for moving nodes
            if arrived.size > 0:
                theta = U(0, 2*np.pi, arrived)
                fl[arrived] = self.FL_DISTR(arrived)
                if self.collect_fl_stats: self.fl_stats.extend(fl[arrived])
                velocity[arrived] = self.VELOCITY_DISTR(fl[arrived])
                v = velocity[arrived]
                cosintheta[arrived] = np.dstack((v * np.cos(theta), v * np.sin(theta)))[0]
                          
            yield xy

class RandomWalk(StochasticWalk):
    
    def __init__(self, nr_nodes, dimensions, velocity=1., distance=1., border_policy='reflect'):
        '''
        Random Walk mobility model.
        This model is based in the Stochastic Walk, but both the flight length and node velocity distributions are in fact constants,
        set to the *distance* and *velocity* parameters. The waiting time is set to None.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
          
        keyword arguments:
        
          *velocity*:
            Double, the value for the constant node velocity. Default is 1.0
          
          *distance*:
            Double, the value for the constant distance traveled in each step. Default is 1.0
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        if velocity>distance:
            # In this implementation, each step is 1 second,
            # it is not possible to have a velocity larger than the distance
            raise Exception('Velocity must be <= Distance')
        
        fl = np.zeros(nr_nodes)+distance
        vel = np.zeros(nr_nodes)+velocity
       
        FL_DISTR = lambda SAMPLES: np.array(fl[:len(SAMPLES)])
        VELOCITY_DISTR = lambda FD: np.array(vel[:len(FD)])
        
        StochasticWalk.__init__(self, nr_nodes, dimensions, FL_DISTR, VELOCITY_DISTR,border_policy=border_policy)

class RandomDirection(StochasticWalk):
    
    def __init__(self, nr_nodes, dimensions, wt_max=None, velocity=(0.1, 1.), border_policy='reflect'):
        '''
        Random Direction mobility model.
        This model is based in the Stochastic Walk. The flight length is chosen from a uniform distribution, 
        with minimum 0 and maximum set to the maximum dimension value.
        The velocity is also chosen from a uniform distribution, with boundaries set by the *velocity* parameter.
        If wt_max is set, the waiting time is chosen from a uniform distribution with values between 0 and wt_max.
        If wt_max is not set, waiting time is set to None.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
          
        keyword arguments:
        
          *wt_max*:
            Double, maximum value for the waiting time distribution.
            If wt_max is set, the waiting time is chosen from a uniform distribution with values between 0 and wt_max.
            If wt_max is not set, the waiting time is set to None.
            Default is None.
          
          *velocity*:
            Tuple of Doubles, the minimum and maximum values for node velocity.
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        
        MIN_V, MAX_V = velocity
        FL_MAX = max(dimensions)
        
        FL_DISTR = lambda SAMPLES: U(0, FL_MAX, SAMPLES)
        if wt_max:
            WT_DISTR = lambda SAMPLES: U(0, wt_max, SAMPLES)
        else:
            WT_DISTR = None
        VELOCITY_DISTR = lambda FD: U(MIN_V, MAX_V, FD)
        
        StochasticWalk.__init__(self, nr_nodes, dimensions, FL_DISTR, VELOCITY_DISTR, WT_DISTR=WT_DISTR, border_policy=border_policy)

class TruncatedLevyWalk(StochasticWalk):
    
    def __init__(self, nr_nodes, dimensions, FL_EXP=-2.6, FL_MAX=50., WT_EXP=-1.8, WT_MAX=100., border_policy='reflect'):
        '''
        Truncated Levy Walk mobility model, based on the following paper:
        Injong Rhee, Minsu Shin, Seongik Hong, Kyunghan Lee, and Song Chong. On the Levy-Walk Nature of Human Mobility. 
            In 2008 IEEE INFOCOM - Proceedings of the 27th Conference on Computer Communications, pages 924-932. April 2008.
        
        The implementation is a special case of the more generic Stochastic Walk, 
        in which both the flight length and waiting time distributions are truncated power laws,
        with exponents set to FL_EXP and WT_EXP and truncated at FL_MAX and WT_MAX.
        The node velocity is a function of the flight length.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
          
        keyword arguments:
        
          *FL_EXP*:
            Double, the exponent of the flight length distribution. Default is -2.6
            
          *FL_MAX*:
            Double, the maximum value of the flight length distribution. Default is 50
          
          *WT_EXP*:
            Double, the exponent of the waiting time distribution. Default is -1.8
            
          *WT_MAX*:
            Double, the maximum value of the waiting time distribution. Default is 100
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        
        FL_DISTR = lambda SAMPLES: P(FL_EXP, 1., FL_MAX, SAMPLES)
        if WT_EXP and WT_MAX:
            WT_DISTR = lambda SAMPLES: P(WT_EXP, 1., WT_MAX, SAMPLES)
        else:
            WT_DISTR = None
        VELOCITY_DISTR = lambda FD: np.sqrt(FD)/10.
        
        StochasticWalk.__init__(self, nr_nodes, dimensions, FL_DISTR, VELOCITY_DISTR, WT_DISTR=WT_DISTR, border_policy=border_policy)

class HeterogeneousTruncatedLevyWalk(StochasticWalk):

    def __init__(self, nr_nodes, dimensions, WT_EXP=-1.8, WT_MAX=100., FL_EXP=-2.6, FL_MAX=50., border_policy='reflect'):
        '''
        This is a variant of the Truncated Levy Walk mobility model.
        This model is based in the Stochastic Walk.
        The waiting time distribution is a truncated power law with exponent set to WT_EXP and truncated WT_MAX.
        The flight length is a uniform distribution, different for each node. These uniform distributions are 
        created by taking both min and max values from a power law with exponent set to FL_EXP and truncated FL_MAX.
        The node velocity is a function of the flight length.
        
        Required arguments:
        
          *nr_nodes*:
            Integer, the number of nodes.
          
          *dimensions*:
            Tuple of Integers, the x and y dimensions of the simulation area.
          
        keyword arguments:
        
          *WT_EXP*:
            Double, the exponent of the waiting time distribution. Default is -1.8
            
          *WT_MAX*:
            Double, the maximum value of the waiting time distribution. Default is 100
        
          *FL_EXP*:
            Double, the exponent of the flight length distribution. Default is -2.6
            
          *FL_MAX*:
            Double, the maximum value of the flight length distribution. Default is 50
            
          *border_policy*:
            String, either 'reflect' or 'wrap'. The policy that is used when the node arrives to the border.
            If 'reflect', the node reflects off the border.
            If 'wrap', the node reappears at the opposite edge (as in a torus-shaped area).
        '''
        
        NODES = np.arange(nr_nodes)
        FL_MAX = P(-1.8, 10., FL_MAX, NODES)
        FL_MIN = FL_MAX/10.
        
        FL_DISTR = lambda SAMPLES: rand(len(SAMPLES)) * (FL_MAX[SAMPLES] - FL_MIN[SAMPLES]) + FL_MIN[SAMPLES]
        WT_DISTR = lambda SAMPLES: P(WT_EXP, 1., WT_MAX, SAMPLES)
        VELOCITY_DISTR = lambda FD: np.sqrt(FD)/10.
        
        StochasticWalk.__init__(self, nr_nodes, dimensions, FL_DISTR, VELOCITY_DISTR, WT_DISTR=WT_DISTR, border_policy=border_policy)
        
def random_waypoint(*args, **kwargs):
    return iter(RandomWaypoint(*args, **kwargs))

def stochastic_walk(*args, **kwargs):
    return iter(StochasticWalk(*args, **kwargs))

def random_walk(*args, **kwargs):
    return iter(RandomWalk(*args, **kwargs))

def random_direction(*args, **kwargs):
    return iter(RandomDirection(*args, **kwargs))

def truncated_levy_walk(*args, **kwargs):
    return iter(TruncatedLevyWalk(*args, **kwargs))

def heterogeneous_truncated_levy_walk(*args, **kwargs):
    return iter(HeterogeneousTruncatedLevyWalk(*args, **kwargs))

def gauss_markov(nr_nodes, dimensions, velocity_mean=1., alpha=1., variance=1.):
    '''
    Gauss-Markov Mobility Model, as proposed in 
    Camp, T., Boleng, J. & Davies, V. A survey of mobility models for ad hoc network research. 
    Wireless Communications and Mobile Computing 2, 483-502 (2002).
    
    Required arguments:
    
      *nr_nodes*:
        Integer, the number of nodes.
      
      *dimensions*:
        Tuple of Integers, the x and y dimensions of the simulation area.
        
    keyword arguments:
    
      *velocity_mean*:
        The mean velocity
        
      *alpha*:
        The tuning parameter used to vary the randomness
        
      *variance*:
        The randomness variance
    '''
    
    MAX_X, MAX_Y = dimensions
    NODES = np.arange(nr_nodes)
    x = U(0, MAX_X, NODES)
    y = U(0, MAX_Y, NODES)
    velocity =  np.zeros(nr_nodes)+velocity_mean
    theta = U(0, 2*np.pi, NODES)
    angle_mean = theta
    
    alpha2 = 1.0 - alpha
    alpha3 = np.sqrt(1.0 - alpha * alpha) * variance
    
    while True:

        x = x + velocity * np.cos(theta)
        y = y + velocity * np.sin(theta)
        
        # node bounces on the margins
        b = np.where(x<0)[0]
        x[b] = - x[b]; theta[b] = np.pi-theta[b]; angle_mean[b] = np.pi-angle_mean[b]
        b = np.where(x>MAX_X)[0]
        x[b] = 2*MAX_X - x[b]; theta[b] = np.pi-theta[b]; angle_mean[b] = np.pi-angle_mean[b]
        b = np.where(y<0)[0]
        y[b] = - y[b]; theta[b] = -theta[b]; angle_mean[b] = -angle_mean[b]
        b = np.where(y>MAX_Y)[0]
        y[b] = 2*MAX_Y - y[b]; theta[b] = -theta[b]; angle_mean[b] = -angle_mean[b]
        
        # calculate new speed and direction based on the model
        velocity = (alpha * velocity +
                    alpha2 * velocity_mean +
                    alpha3 * np.random.normal(0.0, 1.0, nr_nodes))
    
        theta = (alpha * theta +
                    alpha2 * angle_mean +
                    alpha3 * np.random.normal(0.0, 1.0, nr_nodes))
        
        yield np.dstack((x,y))[0]
        
def reference_point_group(nr_nodes, dimensions, velocity=(0.1, 1.), aggregation=0.1):
    '''
    Reference Point Group Mobility model, discussed in the following paper:
    
        Xiaoyan Hong, Mario Gerla, Guangyu Pei, and Ching-Chuan Chiang. 1999. 
        A group mobility model for ad hoc wireless networks. In Proceedings of the 
        2nd ACM international workshop on Modeling, analysis and simulation of 
        wireless and mobile systems (MSWiM '99). ACM, New York, NY, USA, 53-60.
    
    In this implementation, group trajectories follow a random direction model,
    while nodes follow a random walk around the group center.
    The parameter 'aggregation' controls how close the nodes are to the group center.
    
    Required arguments:
    
      *nr_nodes*:
        list of integers, the number of nodes in each group.
      
      *dimensions*:
        Tuple of Integers, the x and y dimensions of the simulation area.
        
    keyword arguments:
    
      *velocity*:
        Tuple of Doubles, the minimum and maximum values for group velocity.
        
      *aggregation*:
        Double, parameter (between 0 and 1) used to aggregate the nodes in the group.
        Usually between 0 and 1, the more this value approximates to 1,
        the nodes will be more aggregated and closer to the group center.
        With a value of 0, the nodes are randomly distributed in the simulation area.
        With a value of 1, the nodes are close to the group center.
    '''
    
    try:
        iter(nr_nodes)
    except TypeError:
        nr_nodes = [nr_nodes]
    
    NODES = np.arange(sum(nr_nodes))
    
    groups = []
    prev = 0
    for (i,n) in enumerate(nr_nodes):
        groups.append(np.arange(prev,n+prev))
        prev += n
    
    g_ref = np.empty(sum(nr_nodes), dtype=np.int)
    for (i,g) in enumerate(groups):
        for n in g:
            g_ref[n] = i
    
    FL_MAX = max(dimensions)
    MIN_V,MAX_V = velocity
    FL_DISTR = lambda SAMPLES: U(0, FL_MAX, SAMPLES)
    VELOCITY_DISTR = lambda FD: U(MIN_V, MAX_V, FD)
    
    MAX_X, MAX_Y = dimensions
    x = U(0, MAX_X, NODES)
    y = U(0, MAX_Y, NODES)
    velocity = 1.
    theta = U(0, 2*np.pi, NODES)
    costheta = np.cos(theta)
    sintheta = np.sin(theta)
    
    GROUPS = np.arange(len(groups))
    g_x = U(0, MAX_X, GROUPS)
    g_y = U(0, MAX_X, GROUPS)
    g_fl = FL_DISTR(GROUPS)
    g_velocity = VELOCITY_DISTR(g_fl)
    g_theta = U(0, 2*np.pi, GROUPS)
    g_costheta = np.cos(g_theta)
    g_sintheta = np.sin(g_theta)
        
    while True:

        x = x + velocity * costheta
        y = y + velocity * sintheta
        
        g_x = g_x + g_velocity * g_costheta
        g_y = g_y + g_velocity * g_sintheta
        
        for (i,g) in enumerate(groups):
            
            # step to group direction + step to group center
            x_g = x[g]
            y_g = y[g]
            c_theta = np.arctan2(g_y[i] - y_g, g_x[i] - x_g)
            
            x[g] = x_g + g_velocity[i] * g_costheta[i] + aggregation*np.cos(c_theta)
            y[g] = y_g + g_velocity[i] * g_sintheta[i] + aggregation*np.sin(c_theta)
            
        # node and group bounces on the margins
        b = np.where(x<0)[0]
        if b.size > 0:
            x[b] = - x[b]; costheta[b] = -costheta[b]
            g_idx = np.unique(g_ref[b]); g_costheta[g_idx] = -g_costheta[g_idx]
        b = np.where(x>MAX_X)[0]
        if b.size > 0:
            x[b] = 2*MAX_X - x[b]; costheta[b] = -costheta[b]
            g_idx = np.unique(g_ref[b]); g_costheta[g_idx] = -g_costheta[g_idx]
        b = np.where(y<0)[0]
        if b.size > 0:
            y[b] = - y[b]; sintheta[b] = -sintheta[b]
            g_idx = np.unique(g_ref[b]); g_sintheta[g_idx] = -g_sintheta[g_idx]
        b = np.where(y>MAX_Y)[0]
        if b.size > 0:
            y[b] = 2*MAX_Y - y[b]; sintheta[b] = -sintheta[b]
            g_idx = np.unique(g_ref[b]); g_sintheta[g_idx] = -g_sintheta[g_idx]

        # update info for nodes
        theta = U(0, 2*np.pi, NODES)
        costheta = np.cos(theta)
        sintheta = np.sin(theta)
        
        # update info for arrived groups
        g_fl = g_fl - g_velocity
        g_arrived = np.where(np.logical_and(g_velocity>0., g_fl<=0.))[0]
        
        if g_arrived.size > 0:
            g_theta = U(0, 2*np.pi, g_arrived)
            g_costheta[g_arrived] = np.cos(g_theta)
            g_sintheta[g_arrived] = np.sin(g_theta)
            g_fl[g_arrived] = FL_DISTR(g_arrived)
            g_velocity[g_arrived] = VELOCITY_DISTR(g_fl[g_arrived])

        yield np.dstack((x,y))[0]
        
def tvc(nr_nodes, dimensions, velocity=(0.1, 1.), aggregation=[0.5,0.], epoch=[100,100]):
    '''
    Time-variant Community Mobility Model, discussed in the paper
    
        Wei-jen Hsu, Thrasyvoulos Spyropoulos, Konstantinos Psounis, and Ahmed Helmy, 
        "Modeling Time-variant User Mobility in Wireless Mobile Networks," INFOCOM 2007, May 2007.
    
    This is a variant of the original definition, in the following way:
    - Communities don't have a specific area, but a reference point where the 
       community members aggregate around.
    - The community reference points are not static, but follow a random direction model.
    - You can define a list of epoch stages, each value is the duration of the stage.
       For each stage a different aggregation value is used (from the aggregation parameter).
    - Aggregation values should be doubles between 0 and 1.
       For aggregation 0, there's no attraction point and the nodes move in a random walk model.
       For aggregation near 1, the nodes move closer to the community reference point.
       
    Required arguments:
    
      *nr_nodes*:
        list of integers, the number of nodes in each group.
      
      *dimensions*:
        Tuple of Integers, the x and y dimensions of the simulation area.
        
    keyword arguments:
    
      *velocity*:
        Tuple of Doubles, the minimum and maximum values for community velocities.
        
      *aggregation*:
        List of Doubles, parameters (between 0 and 1) used to aggregate the nodes around the community center.
        Usually between 0 and 1, the more this value approximates to 1,
        the nodes will be more aggregated and closer to the group center.
        With aggregation 0, the nodes are randomly distributed in the simulation area.
        With aggregation near 1, the nodes are closer to the group center.
        
      *epoch*:
        List of Integers, the number of steps each epoch stage lasts.
    '''
    if len(aggregation) != len(epoch):
        raise Exception("The parameters 'aggregation' and 'epoch' should be of same size")
    
    try:
        iter(nr_nodes)
    except TypeError:
        nr_nodes = [nr_nodes]
    
    NODES = np.arange(sum(nr_nodes))
    
    epoch_total = sum(epoch)
    
    def AGGREGATION(t):
        acc = 0
        for i in range(len(epoch)):
            acc+=epoch[i]
            if t%epoch_total <= acc: return aggregation[i]
        raise Exception("Something wrong here")
    
    groups = []
    prev = 0
    for (i,n) in enumerate(nr_nodes):
        groups.append(np.arange(prev,n+prev))
        prev += n
    
    g_ref = np.empty(sum(nr_nodes), dtype=np.int)
    for (i,g) in enumerate(groups):
        for n in g:
            g_ref[n] = i
    
    FL_MAX = max(dimensions)
    MIN_V,MAX_V = velocity
    FL_DISTR = lambda SAMPLES: U(0, FL_MAX, SAMPLES)
    VELOCITY_DISTR = lambda FD: U(MIN_V, MAX_V, FD)
    
    def wrap(x,y):
        b = np.where(x<0)[0]
        if b.size > 0:
            x[b] += MAX_X
        b = np.where(x>MAX_X)[0]
        if b.size > 0:
            x[b] -= MAX_X
        b = np.where(y<0)[0]
        if b.size > 0:
            y[b] += MAX_Y
        b = np.where(y>MAX_Y)[0]
        if b.size > 0:
            y[b] -= MAX_Y
    
    MAX_X, MAX_Y = dimensions
    x = U(0, MAX_X, NODES)
    y = U(0, MAX_Y, NODES)
    velocity = 1.
    theta = U(0, 2*np.pi, NODES)
    costheta = np.cos(theta)
    sintheta = np.sin(theta)
    
    GROUPS = np.arange(len(groups))
    g_x = U(0, MAX_X, GROUPS)
    g_y = U(0, MAX_X, GROUPS)
    g_fl = FL_DISTR(GROUPS)
    g_velocity = VELOCITY_DISTR(g_fl)
    g_theta = U(0, 2*np.pi, GROUPS)
    g_costheta = np.cos(g_theta)
    g_sintheta = np.sin(g_theta)
    
    t = 0
        
    while True:
        
        t += 1
        # get aggregation value for this step
        aggr = AGGREGATION(t)

        x = x + velocity * costheta
        y = y + velocity * sintheta
        
        # move reference point only if nodes have to go there
        if aggr > 0:
        
            g_x = g_x + g_velocity * g_costheta
            g_y = g_y + g_velocity * g_sintheta
            
            # group wrap around when outside the margins (torus shaped area)
            wrap(g_x, g_y)
            
            # update info for arrived groups
            g_arrived = np.where(np.logical_and(g_velocity>0., g_fl<=0.))[0]
            g_fl = g_fl - g_velocity
            
            if g_arrived.size > 0:
                g_theta = U(0, 2*np.pi, g_arrived)
                g_costheta[g_arrived] = np.cos(g_theta)
                g_sintheta[g_arrived] = np.sin(g_theta)
                g_fl[g_arrived] = FL_DISTR(g_arrived)
                g_velocity[g_arrived] = VELOCITY_DISTR(g_fl[g_arrived])
            
            # update node position according to group center
            for (i,g) in enumerate(groups):
                
                # step to group direction + step to reference point
                x_g = x[g]
                y_g = y[g]
                
                dy = g_y[i] - y_g
                dx = g_x[i] - x_g
                c_theta = np.arctan2(dy, dx)
                
                # invert angle if wrapping around
                invert = np.where((np.abs(dy)>MAX_Y/2)!=(np.abs(dx)>MAX_X/2))[0]
                c_theta[invert] = c_theta[invert] + np.pi
                
                x[g] = x_g + g_velocity[i] * g_costheta[i] + aggr*np.cos(c_theta)
                y[g] = y_g + g_velocity[i] * g_sintheta[i] + aggr*np.sin(c_theta)
            
        # node wrap around when outside the margins (torus shaped area)
        wrap(x,y)
        
        # update info for nodes
        theta = U(0, 2*np.pi, NODES)
        costheta = np.cos(theta)
        sintheta = np.sin(theta)
        
        yield np.dstack((x,y))[0]
