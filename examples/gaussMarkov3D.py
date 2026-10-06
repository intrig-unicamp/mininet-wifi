#!/usr/bin/env python

# Author: Bruno Fernandes <bruno.fernandes@tum.de>
# Last update: 04.10.2026

"""AdHoc + 3D Gauss-Markov mobility example for Mininet-WiFi.

Features:
- Single world volume of size WORLD_SIZE (X, Y, Z).
- Single cluster of size CLUSTER_SIZE placed at CLUSTER_OFFSET.
  This allows to develop more clusters inside the same world.
- Gauss-Markov 3D mobility with a threaded 3D animation (push hook),
  so the simulation and CLI keep running while the graph is rendered
  in the background.
- Live mode records a csv trace that can be replayed offline.

Mobility timing:
- Each mobility step advances `time_step` seconds of simulated time.
- The wall-clock pace is set by `timed_model_mob_tick` (seconds per step).

  sudo python3 gaussMarkov3D.py                  # Live
  sudo python3 gaussMarkov3D.py --mode replay    # Replay
"""

import argparse

from mininet.log import setLogLevel, info
from mn_wifi.cli import CLI
from mn_wifi.link import wmediumd, adhoc
from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference
from mn_wifi.animation3d import Animation3D

# ========== TUNEABLE PARAMETERS ===========
# World dimensions (X_max, Y_max, Z_max)
WORLD_SIZE = (100.0, 100.0, 100.0)
# Cluster volume size (width, depth, height)
CLUSTER_SIZE = (50.0, 50.0, 50.0)
# Cluster origin offset within world (X_offset, Y_offset, Z_offset)
CLUSTER_OFFSET = (25.0, 25.0, 25.0)

# Mobility model settings
MOBILITY_MODEL = 'GaussMarkov3D'
MOBILITY_PARAMS = {
    'velocity_mean': 2.0, 'alpha': 0.85, 'variance': 1.0,
    'time_step': 1.0,  # simulated seconds per mobility step
    'seed': 20,
    'ac_method': 'ssf',
}

# Propagation model: log-distance exponent
PROP_MODEL = dict(model='logDistance', exp=6)

# Ad-hoc link parameters
LINK_PARAMS = dict(
    ssid='adhocNet', proto=None, mode='g', channel=5,
    ht_cap='HT40+', bw=1, delay=5, loss=5
)

# Animation and trace settings
TRACE_CSV = 'positions.csv'
STEPS_PER_SECOND = 10  # wall-clock mobility steps per second


def topology(mode='live'):
    """
    Build the network, record or replay mobility.
    mode: 'live', 'replay'
    """
    if mode == 'replay':
        info('*** Replaying %s\n' % TRACE_CSV)
        anim = Animation3D.from_csv(
            TRACE_CSV,
            min_x=0, max_x=WORLD_SIZE[0],
            min_y=0, max_y=WORLD_SIZE[1],
            min_z=0, max_z=WORLD_SIZE[2],
            loop=True, speed=2.0)
        anim.start()
        try:
            anim.thread.join()
        except KeyboardInterrupt:
            pass
        anim.stop()
        return

    net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference,
                       noise_th=-91, fading_cof=3)

    info('*** Creating stations\n')
    names = list('abcdefghijkl')
    for idx, name in enumerate(names, start=1):
        net.addStation(name,
                       mac='00:00:00:00:00:%02d' % idx,
                       ip='10.0.0.%d/8' % idx,
                       min_x=CLUSTER_OFFSET[0],
                       max_x=CLUSTER_OFFSET[0] + CLUSTER_SIZE[0],
                       min_y=CLUSTER_OFFSET[1],
                       max_y=CLUSTER_OFFSET[1] + CLUSTER_SIZE[1],
                       min_z=CLUSTER_OFFSET[2],
                       max_z=CLUSTER_OFFSET[2] + CLUSTER_SIZE[2])

    info('*** Configuring propagation model\n')
    net.setPropagationModel(**PROP_MODEL)

    info('*** Configuring wifi nodes\n')
    net.configureWifiNodes()

    info('*** Creating adhoc links\n')
    for sta in net.stations:
        net.addLink(sta, cls=adhoc, intf='%s-wlan0' % sta.name,
                    **LINK_PARAMS)

    info('*** Configuring the 3D animation\n')
    net.plotGraph(min_x=0, max_x=WORLD_SIZE[0],
                  min_y=0, max_y=WORLD_SIZE[1],
                  min_z=0, max_z=WORLD_SIZE[2],
                  animation=True, record=TRACE_CSV,
                  trace=True, show_ranges=True)

    info('*** Starting mobility\n')
    net.setMobilityModel(time=0, model=MOBILITY_MODEL, **MOBILITY_PARAMS)
    net.startMobility(time=0, use_timed_model_mob=True,
                      timed_model_mob_tick=1.0 / STEPS_PER_SECOND)

    info('*** Starting network\n')
    net.build()

    info('*** Running CLI\n')
    try:
        CLI(net)
    finally:
        info('*** Stopping network\n')
        net.stop()


if __name__ == '__main__':
    setLogLevel('info')
    parser = argparse.ArgumentParser(
        description='Single World AdHoc Gauss Markov 3D Mobility')
    parser.add_argument('--mode', choices=['live', 'replay'], default='live')
    args = parser.parse_args()
    topology(args.mode)
