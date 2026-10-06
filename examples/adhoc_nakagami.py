#!/usr/bin/env python

# Author: Bruno Fernandes <bruno.fernandes@tum.de>
# Last update: 27.09.2026

"""Adhoc (IBSS) network with 2 nodes showing the Nakagami-m model.

n1 is static; n2 moves away from it (x 10..90 m, y -30..30 m, 2-5 m/s), so
the RSSI falls with the distance while the Nakagami fading adds the noise.
The RSSI of the n1 -> n2 link and the iperf throughput are recorded.

  sudo python3 adhoc_nakagami.py         # Python model only (no wmediumd)
  sudo python3 adhoc_nakagami.py -w      # the model is also used by wmediumd
  sudo python3 adhoc_nakagami.py -p      # do not plot node positions
"""

import atexit
import os
import shutil
import subprocess
import sys
from statistics import mean, pstdev
from time import sleep, time

from matplotlib._pylab_helpers import Gcf
from matplotlib.backends.backend_agg import FigureCanvasAgg
from matplotlib.figure import Figure

from mininet.log import setLogLevel, info
from mn_wifi.link import wmediumd, adhoc
from mn_wifi.net import Mininet_wifi
from mn_wifi.propagationModels import PropagationModel as ppm
from mn_wifi.wmediumdConnector import interference

MODEL, M, EXP, XG = 'nakagami', 1.5, 4, 0.0
SSID, MODE, CHANNEL = 'adhocNet', 'n2', 6
BSSID, TXPOWER = '02:CA:FF:EE:BA:01', 20
DURATION, INTERVAL, DISTANCE = 20, 0.5, 56.6
SRC, DST = 'n1', 'n2'
IPERF = '/tmp/nakagami_iperf_client.log'
PING = '/tmp/nakagami_ping.log'
PLOT = '/tmp/nakagami_rssi.png'


def read(path):
    try:
        with open(path) as f:
            return f.read()
    except OSError:
        return ''


def model_check(src, dst):
    "Print the RSSI samples and their standard deviation at a fixed distance"
    rssis = [src.wintfs[0].get_rssi(dst.wintfs[0], DISTANCE)
             for _ in range(12)]
    info('*** %s check at %.1f m (%s -> %s):\n'
         % (ppm.model, DISTANCE, src.name, dst.name))
    info('    ' + '  '.join('%.1f' % v for v in rssis) + ' dBm\n')
    info('*** RSSI mean=%.2f dBm std=%.2f dB '
         '(logDistance would give std=0.00 dB)\n'
         % (mean(rssis), pstdev(rssis)))


def run_traffic(src, dst):
    "Run iperf and ping from src to dst"
    dst.cmd('iperf -s > /tmp/nakagami_iperf_server.log 2>&1 &')
    sleep(2)
    t0 = time()
    while time() - t0 < 20:
        if '1 received' in src.cmd('ping -c 1 -W 1 %s' % dst.IP()):
            break
        sleep(1)
    src.cmd('iperf -c %s -t %d -i 1 > %s 2>&1 &'
            % (dst.IP(), DURATION, IPERF))
    src.cmd('ping -i 0.5 -c %d %s > %s 2>&1 &'
            % (DURATION * 2, dst.IP(), PING))


def record_rssi(src, dst):
    "Record the RSSI of the src -> dst link while the traffic runs"
    times, rssis = [], []
    t0 = time()
    while time() - t0 < DURATION:
        dist = src.get_distance_to(dst)
        rssis.append(src.wintfs[0].get_rssi(dst.wintfs[0], dist))
        times.append(time() - t0)
        sleep(INTERVAL)
    return times, rssis


def throughput():
    "Per-second throughput (Mbits/sec) from the iperf client log"
    scale = {'bits/sec': 1e-6, 'Kbits/sec': 1e-3, 'Mbits/sec': 1,
             'Gbits/sec': 1e3}
    series = []
    for line in read(IPERF).splitlines():
        parts = line.split()
        if len(parts) == 8 and parts[7] in scale:
            series.append(float(parts[6]) * scale[parts[7]])
    return series


def stop(net):
    "Stop the network, the mobility thread and the plotting cleanly"
    net.stop()
    net.mob_object.pause_simulation = True
    sleep(0.3)
    atexit.unregister(Gcf.destroy_all)


def plot_series(times, rssis, src, dst):
    "Plot the RSSI time series and the per-second iperf throughput"
    series = throughput()[:-1]
    fig = Figure(figsize=(10, 6))
    FigureCanvasAgg(fig)

    ax = fig.add_subplot(2, 1, 1)
    ax.plot(times, rssis, color='b', lw=1, marker='.', ms=5)
    ax.set_ylabel('RSSI [dBm]')
    ax.set_title('Mininet-WiFi Graph - %s RSSI (%s -> %s)'
                 % (ppm.model, src.name, dst.name), fontsize=10)
    ax.tick_params(labelsize=8)
    ax.grid(True)

    ax2 = fig.add_subplot(2, 1, 2)
    ax2.plot(range(1, len(series) + 1), series, color='g', lw=1,
             marker='.', ms=5)
    ax2.set_xlabel('time [s]')
    ax2.set_ylabel('throughput [Mbits/sec]')
    ax2.tick_params(labelsize=8)
    ax2.grid(True)

    fig.tight_layout()
    fig.savefig(PLOT, dpi=120)
    info('*** Plot saved to %s\n' % PLOT)
    if os.environ.get('DISPLAY') and shutil.which('xdg-open'):
        subprocess.Popen(['xdg-open', PLOT], stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
        info('*** Opening %s with xdg-open\n' % PLOT)


def topology(args):
    "Create a network."
    if '-w' in args:
        net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)
        info("*** Using wmediumd (frame-level emulation)\n")
    else:
        net = Mininet_wifi()
        info("*** Using the Python model only (no wmediumd)\n")

    info("*** Creating nodes\n")
    n1 = net.addStation('n1', position='0,0,0')
    n2 = net.addStation('n2', min_x=10, max_x=90, min_y=-30, max_y=30,
                        min_v=2, max_v=5)

    info("*** Configuring propagation model: %s (m=%s, exp=%s, xg=%s)\n"
         % (MODEL, M, EXP, XG))
    net.setPropagationModel(model=MODEL, m=M, exp=EXP, xg=XG)

    info("*** Configuring nodes\n")
    net.configureNodes()

    info("*** Creating IBSS (adhoc) links: ssid=%s, mode=%s, channel=%s, "
         "bssid=%s, txpower=%s\n" % (SSID, MODE, CHANNEL, BSSID, TXPOWER))
    for node in (n1, n2):
        net.addLink(node, cls=adhoc, intf='%s-wlan0' % node.name, ssid=SSID,
                    mode=MODE, channel=CHANNEL, ibss=BSSID, txpower=TXPOWER)

    if '-p' not in args:
        net.plotGraph(min_x=-110, min_y=-110, max_x=200, max_y=110)

    info("*** Starting mobility\n")
    net.setMobilityModel(time=0, model='RandomDirection', max_x=200, max_y=110,
                         seed=20, use_timed_model_mob=True,
                         timed_model_mob_tick=0.1)

    info("*** Starting network\n")
    net.build()

    src, dst = net.get(SRC), net.get(DST)
    model_check(src, dst)

    run_traffic(src, dst)
    info("*** Recording RSSI for %d s\n" % DURATION)
    times, rssis = record_rssi(src, dst)
    sleep(2)

    series = throughput()[:-1]
    if series:
        info('*** Throughput per second: mean=%.2f min=%.2f max=%.2f std=%.2f '
             'Mbits/sec\n' % (mean(series), min(series), max(series),
             pstdev(series)))
    ping_lines = [line for line in read(PING).splitlines() if line][-2:]
    info('*** Ping (%s -> %s):\n%s\n'
         % (src.name, dst.name, '\n'.join(ping_lines)))

    plot_series(times, rssis, src, dst)

    src.cmd('pkill iperf; pkill ping')
    dst.cmd('pkill iperf')

    info("*** Stopping network\n")
    stop(net)


if __name__ == '__main__':
    setLogLevel('info')
    topology(sys.argv)
