"""

author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
        ramonfontes.com

"""

import scipy.spatial.distance as distance 
import numpy as np


class emulationEnvironment ( object ):
    """ Emulation Environment Configuration """
    propagationModel = ''
    ismobility = False
    rate = 0
    wifiRadios = 0
    virtualWlan = []
    physicalWlan = []
    wpa_supplicantIsRunning = False
    isWiFi = False
    isCode = False
    
    @classmethod 
    def getDistance(self, src, dst):
        """ Get the distance between two points """
        pos_src = src.position
        pos_dst = dst.position
        points = np.array([(pos_src[0], pos_src[1], pos_src[2]), (pos_dst[0], pos_dst[1], pos_dst[2])])
        dist = distance.pdist(points)
        return dist
    

