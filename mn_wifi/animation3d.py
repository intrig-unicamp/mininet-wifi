"""
    Mininet-WiFi: A simple networking testbed for Wireless OpenFlow/SDWN!
    author: Ramon Fontes (ramonrf@dca.fee.unicamp.br)
"""

import csv
import warnings

import numpy as np

from threading import Thread as thread, Lock, Event
from time import sleep, time

from mininet.log import info

warnings.filterwarnings(
    'ignore',
    message='Starting a Matplotlib GUI outside of the main thread'
)


class _ReplayNode(object):
    "Minimal node used when replaying a csv trace"

    def __init__(self, name):
        self.name = name
        self.position = (0, 0, 0)
        self.wintfs = {}


class Animation3D(object):
    """
    Threaded 3D animation for Mininet-WiFi mobility 3D models.

    The animation runs in its own thread and is fed by the 3D mobility
    model (push hook), so the simulation and the CLI keep running in
    the main thread while the graph is rendered in the background. One
    pushed frame corresponds to one mobility step, which makes the
    animation deterministic and step-controllable.
    """

    ax = None
    _instance = None

    def __init__(self, nodes, min_x=0, max_x=100, min_y=0, max_y=100,
                 min_z=0, max_z=0, interval=0.05, trace=True,
                 show_ranges=True, in_range=True, record=None,
                 render_every=1, colors=None, speed=1.0, loop=False,
                 title='Mininet-WiFi 3D Graph', **kwargs):
        self.nodes = nodes
        self.names = [node.name for node in nodes]
        self.min_x, self.max_x = min_x, max_x
        self.min_y, self.max_y = min_y, max_y
        self.min_z, self.max_z = min_z, max_z
        self.interval = interval
        self.trace = trace
        self.show_ranges = show_ranges
        self.in_range = in_range
        self.record = record
        self.render_every = render_every
        self.colors = colors
        self.speed = speed
        self.loop = loop
        self.title = title

        self.step = 0
        self.rendered_step = 0
        self.trajectories = {name: [] for name in self.names}
        self.lock = Lock()
        self.stop_event = Event()
        self.frame = None
        self.thread = None
        self.fig = None
        self.record_file = None
        self.writer = None
        self.recording = True
        self.replay_steps = None
        Animation3D._instance = self

    # ------------------------------------------------------------------
    # Push hook: called from the mobility thread, one call per step
    # ------------------------------------------------------------------
    def update(self, positions):
        "Publishes the positions of one mobility step"
        self.step += 1
        positions = [tuple(pos) for pos in positions]
        if self.record:
            self.record_positions(self.step, positions)
        with self.lock:
            self.frame = (self.step, positions)

    def record_positions(self, step, positions):
        "Records the pushed positions into a csv trace"
        if not self.recording:
            return
        if self.writer is None:
            self.record_file = open(self.record, 'w', newline='')
            self.writer = csv.writer(self.record_file)
            self.writer.writerow(['step', 'timestamp', 'node', 'x', 'y', 'z'])
        timestamp = time()
        for name, pos in zip(self.names, positions):
            self.writer.writerow([step, timestamp, name, pos[0], pos[1], pos[2]])
        self.record_file.flush()

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------
    def start(self):
        "Starts the rendering thread"
        if self.thread and self.thread.is_alive():
            return
        self.stop_event.clear()
        self.thread = thread(name='animation3d', target=self._run)
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        "Stops the rendering thread and closes the trace file"
        self.recording = False
        self.stop_event.set()
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=10)
        if self.record_file and not self.record_file.closed:
            self.record_file.close()
            self.record_file = None
            self.writer = None

    @classmethod
    def close_plot(cls):
        "Stops the current animation (used by the cleanup routines)"
        if cls._instance:
            cls._instance.stop()

    @classmethod
    def from_csv(cls, trace, **kwargs):
        "Creates an animation that replays a previously recorded csv trace"
        data = {}
        names = []
        with open(trace, 'r') as record_file:
            for row in csv.DictReader(record_file):
                step = int(row['step'])
                if row['node'] not in names:
                    names.append(row['node'])
                data.setdefault(step, {})[row['node']] = (
                    float(row['x']), float(row['y']), float(row['z']))
        nodes = [_ReplayNode(name) for name in names]
        animation = cls(nodes=nodes, **kwargs)
        animation.replay_steps = [
            (step, [data[step][name] for name in names])
            for step in sorted(data)
        ]
        return animation

    # ------------------------------------------------------------------
    # Rendering
    # ------------------------------------------------------------------
    def _get_range(self, node):
        wintfs = getattr(node, 'wintfs', None)
        if not wintfs:
            return 0
        if hasattr(wintfs, 'values'):
            wintfs = wintfs.values()
        ranges = [getattr(intf, 'range', 0) for intf in wintfs]
        return max(ranges) if ranges else 0

    def _run(self):
        import matplotlib.pyplot as plt
        from mpl_toolkits.mplot3d import Axes3D  # noqa: F401

        if self.max_z == 0:
            self.max_z = max((node.position[2] for node in self.nodes
                              if hasattr(node, 'position')), default=0) + 1

        plt.ion()
        self.fig = plt.figure(figsize=(12, 9))
        ax = self.fig.add_subplot(111, projection='3d')
        Animation3D.ax = ax
        ax.set_xlabel('meters (x)')
        ax.set_ylabel('meters (y)')
        ax.set_zlabel('meters (z)')
        ax.set_xlim([self.min_x, self.max_x])
        ax.set_ylim([self.min_y, self.max_y])
        ax.set_zlim([self.min_z, self.max_z])
        ax.grid(True)
        ax.set_title(self.title)

        self.theta = np.linspace(0, 2 * np.pi, 100)
        self.sphere_u, self.sphere_v = np.mgrid[0:2 * np.pi:24j, 0:np.pi:16j]

        if self.colors is None:
            palette = plt.cm.tab10(np.linspace(0, 1, len(self.names)))
            self.colors = {name: palette[i]
                           for i, name in enumerate(self.names)}

        self.lines = {}
        self.points = {}
        self.labels = {}
        self.rings = {}
        self.spheres = {}
        for name in self.names:
            color = self.colors[name]
            line, = ax.plot([], [], [], color=color, alpha=0.5, linewidth=1.0)
            point = ax.scatter([], [], [], color=color, s=60, depthshade=False)
            label = ax.text(0, 0, 0, name, zdir=None)
            ring, = ax.plot([], [], [], '--', color='red', alpha=0.7,
                            linewidth=0.8)
            self.lines[name] = line
            self.points[name] = point
            self.labels[name] = label
            self.rings[name] = ring
            self.spheres[name] = None

        info('*** Starting 3D animation\n')
        self.fig.canvas.draw_idle()
        self.fig.canvas.flush_events()

        if self.replay_steps is not None:
            self._play()
        else:
            self._follow()
        self._close()

    def _follow(self):
        "Renders the frames pushed by the mobility model"
        while not self.stop_event.is_set():
            if not self._window_open():
                break
            with self.lock:
                frame = self.frame
            if frame is not None and frame[0] != self.rendered_step:
                self.rendered_step = frame[0]
                if self.rendered_step % self.render_every == 0:
                    self._draw(frame[0], frame[1])
            sleep(self.interval)

    def _play(self):
        "Renders a csv trace loaded with from_csv()"
        pause = self.interval / self.speed if self.speed else self.interval
        while not self.stop_event.is_set():
            for step, positions in self.replay_steps:
                if self.stop_event.is_set() or not self._window_open():
                    return
                self._draw(step, positions)
                sleep(pause)
            if not self.loop:
                break

    def _window_open(self):
        import matplotlib.pyplot as plt
        return self.fig is not None and plt.fignum_exists(self.fig.number)

    def _draw(self, step, positions):
        "Updates the graph with the positions of one step"
        for i, name in enumerate(self.names):
            pos = positions[i]
            trajectory = self.trajectories[name]
            trajectory.append(pos)
            if self.trace and len(trajectory) > 1:
                trace = np.array(trajectory)
                self.lines[name].set_data(trace[:, 0], trace[:, 1])
                self.lines[name].set_3d_properties(trace[:, 2])
            self.points[name]._offsets3d = ([pos[0]], [pos[1]], [pos[2]])
            self.labels[name].set_position((pos[0], pos[1]))
            self.labels[name].set_3d_properties(pos[2], zdir=None)

            node_range = self._get_range(self.nodes[i])
            if node_range > 0:
                if self.in_range:
                    in_range = any(
                        (other[0] - pos[0]) ** 2 +
                        (other[1] - pos[1]) ** 2 +
                        (other[2] - pos[2]) ** 2 <= node_range ** 2
                        for j, other in enumerate(positions) if j != i)
                    color = 'green' if in_range else 'red'
                    self.rings[name].set_color(color)
                    self.rings[name].set_data(
                        pos[0] + node_range * np.cos(self.theta),
                        pos[1] + node_range * np.sin(self.theta))
                    self.rings[name].set_3d_properties(
                        [pos[2]] * len(self.theta))
                if self.show_ranges:
                    if self.spheres[name] is not None:
                        self.spheres[name].remove()
                    x = pos[0] + node_range * np.cos(self.sphere_u) * np.sin(self.sphere_v)
                    y = pos[1] + node_range * np.sin(self.sphere_u) * np.sin(self.sphere_v)
                    z = pos[2] + node_range * np.cos(self.sphere_v)
                    self.spheres[name] = Animation3D.ax.plot_surface(
                        x, y, z, alpha=0.08, color=self.colors[name],
                        linewidth=0, edgecolor='none')

        try:
            Animation3D.ax.set_title('%s - Step %d' % (self.title, step))
            self.fig.canvas.draw_idle()
            self.fig.canvas.flush_events()
        except Exception:
            self.stop_event.set()

    def _close(self):
        import matplotlib.pyplot as plt
        try:
            plt.close(self.fig)
        except Exception:
            pass
        info('*** Stopping 3D animation\n')
