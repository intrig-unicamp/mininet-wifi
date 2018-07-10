import sys

from mininet.cli import CLI
from mininet.log import output, error

class CLI_wifi(CLI):
    "Simple command-line interface to talk to nodes."
    CLI.prompt = 'mininet-wifi> '

    def __init__(self, mn_wifi, stdin=sys.stdin, script=None):
        CLI.__init__(self, mn_wifi, stdin=sys.stdin, script=None)

    def do_stop(self, line):
        "stop mobility for a while"
        self.mn.stop_simulation()

    def do_start(self, line):
        "pause mobility for a while"
        self.mn.start_simulation()

    def do_distance(self, line):
        "Distance between two nodes."
        args = line.split()
        if len(args) != 2:
            error('invalid number of args: distance [sta or ap] [sta or ap]\n')
        elif len(args) == 2 and args[ 0 ] == args[ 1 ]:
            error('invalid. Source and Destination are equals\n')
        else:
            self.mn.get_distance(*args)

    def do_link(self, line):
        from mn_wifi.plot import plot2d
        """Bring link(s) between two nodes up or down.
           Usage: link node1 node2 [up/down]"""
        args = line.split()
        if len(args) != 3:
            error('invalid number of args: link end1 end2 [up down]\n')
        elif args[2] not in ['up', 'down']:
            error('invalid type: link end1 end2 [up down]\n')
        else:
            self.mn.configLinkStatus(*args)
        nodes = []
        for node in args:
            if node != 'down' and node != 'up':
                nodes.append(self.mn.getNodeByName(node))
        if 'position' in nodes[0].params and 'position' in nodes[1].params:
            if len(args) == 3 and args[2] == 'down':
                plot2d.hideLine(nodes[0], nodes[1])
            elif len(args) == 3 and args[2] == 'up':
                plot2d.showLine(nodes[0], nodes[1])

    def do_dpctl(self, line):
        """Run dpctl (or ovs-ofctl) command on all switches.
           Usage: dpctl command [arg1] [arg2] ..."""
        args = line.split()
        if len(args) < 1:
            error('usage: dpctl command [arg1] [arg2] ...\n')
            return
        nodesL2 = self.mn.switches + self.mn.aps
        for sw in nodesL2:
            output('*** ' + sw.name + ' ' + ('-' * 72) + '\n')
            output(sw.dpctl(*args))
