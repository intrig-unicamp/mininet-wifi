#!/usr/bin/env python
"""Print some basic information about wireless interfaces on the host.
This is a pure-Python port of the C program available here:
    github.com/Robpol86/libnl/blob/7186e04/example_c/show_wifi_interface.c
Derived from a Python test of libnl available here:
    https://github.com/Robpol86/libnl/blob/7186e04/tests/test_nl80211.py#L29
This script is a showcase for the Linux Netlink libnl Python library, ported
from the C library with the same name. Using libnl, Python scripts/applications
can communicate with the Linux kernel and device drivers directly just like
C/C++ programs do, without having to call a binary on the system.
More information about the Python libnl is available here:
    https://github.com/Robpol86/libnl
Debug messages are available with the -v option, and even more debug messages
are available by setting the NLCB environment variable to either 'verbose' or
'debug' like so:
    NLCB=verbose example_show_wifi_interface.py print -v
    NLCB=debug example_show_wifi_interface.py print -v wlan0
Usage:
    example_show_wifi_interface.py print [options] [<interface>]
    example_show_wifi_interface.py -h | --help
Options:
    -v --verbose    Print debug messages to stderr.
"""

from __future__ import print_function

import fcntl
import logging
import signal
import socket
import struct
import sys

from docopt import docopt
from terminaltables import AsciiTable

from libnl.attr import nla_data, nla_get_string, nla_get_u32, nla_parse, nla_put_u32, NLA_U8, nla_policy, nla_parse_nested
from libnl.error import errmsg
from libnl.genl.ctrl import genl_ctrl_resolve
from libnl.genl.genl import genl_connect, genlmsg_attrdata, genlmsg_attrlen, genlmsg_put
from libnl.handlers import NL_CB_CUSTOM, NL_CB_VALID, NL_SKIP
from libnl.linux_private.genetlink import genlmsghdr
from libnl.linux_private.netlink import NLM_F_DUMP, nlattr
from libnl.msg import nlmsg_alloc, nlmsg_data, nlmsg_hdr
from libnl.nl import nl_recvmsgs_default, nl_send_auto
import nl80211
from libnl.socket_ import nl_socket_alloc, nl_socket_modify_cb

OPTIONS = docopt(__doc__) if __name__ == '__main__' else dict()


def error(message, code=1):
    """Print an error message to stderr and exits with a status of 1 by default."""
    if message:
        print('ERROR: {0}'.format(message), file=sys.stderr)
    else:
        print(file=sys.stderr)
    sys.exit(code)


def callback(msg, has_printed):
    """Callback function called by libnl upon receiving messages from the kernel.
    Positional arguments:
    msg -- nl_msg class instance containing the data sent by the kernel.
    has_printed -- simple pseudo boolean (if list is empty) to keep track of when to print emtpy lines.
    Returns:
    An integer, value of NL_SKIP. It tells libnl to stop calling other callbacks for this message and proceed with
    processing the next kernel message.
    """
    table = AsciiTable([['Data Type', 'Data Value']])
    # First convert `msg` into something more manageable.
    gnlh = genlmsghdr(nlmsg_data(nlmsg_hdr(msg)))
     
    tb = dict((i, None) for i in range(nl80211.NL80211_ATTR_MAX + 1)) 
    info = dict((i, None) for i in range(nl80211.NL80211_SURVEY_INFO_MAX + 1))
    stainfo = dict((i, None) for i in range(nl80211.NL80211_STA_INFO_MAX + 1))
      
    # Need to populate dict with all possible keys.
    nla_parse(tb, nl80211.NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), None)    
    nla_parse(info, nl80211.NL80211_SURVEY_INFO_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), None)
    nla_parse(stainfo, nl80211.NL80211_SURVEY_INFO_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), None)
    
    #info.update({
    #             nl80211.NL80211_SURVEY_INFO_NOISE: nla_policy(type_=NLA_U8),
    #                                                           })
    info[nl80211.NL80211_SURVEY_INFO_NOISE] = 10
   
    #print (nla_get_u32(stainfo[nl80211.NL80211_STA_INFO_TX_BYTES]))
    #print(stainfo)
    print(info)
    #print(tb)
    #print (info[nl80211.NL80211_SURVEY_INFO_NOISE])
    
    #table.title = nla_get_string(info[nl80211.NL80211_SURVEY_INFO_NOISE]).decode('ascii')
    if info[nl80211.NL80211_SURVEY_INFO_NOISE]:
        print("cccc")
        print( "%s" % (info[nl80211.NL80211_SURVEY_INFO_NOISE]))
    
    # Now it's time to grab the juicy data!
    if tb[nl80211.NL80211_ATTR_IFNAME]:
        table.title = nla_get_string(tb[nl80211.NL80211_ATTR_IFNAME]).decode('ascii')
    else:
        table.title = 'Unnamed Interface'

    if tb[nl80211.NL80211_ATTR_WIPHY]:
        wiphy_num = nla_get_u32(tb[nl80211.NL80211_ATTR_WIPHY])
        wiphy = ('wiphy {0}' if OPTIONS['<interface>'] else 'phy#{0}').format(wiphy_num)
        table.table_data.append(['NL80211_ATTR_WIPHY', wiphy])

    if tb[nl80211.NL80211_ATTR_MAC]:
        mac_address = ':'.join(format(x, '02x') for x in nla_data(tb[nl80211.NL80211_ATTR_MAC])[:6])
        table.table_data.append(['NL80211_ATTR_MAC', mac_address])

    if tb[nl80211.NL80211_ATTR_IFINDEX]:
        table.table_data.append(['NL80211_ATTR_IFINDEX', str(nla_get_u32(tb[nl80211.NL80211_ATTR_IFINDEX]))])

    # Print all data.
    if has_printed:
        print()
    else:
        has_printed.append(True)
    print(table.table)
    return NL_SKIP


def main():
    """Main function called upon script execution."""
    # First get the wireless interface index.
    if OPTIONS['<interface>']:
        pack = struct.pack('16sI', OPTIONS['<interface>'].encode('ascii'), 0)
        sk = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            info = struct.unpack('16sI', fcntl.ioctl(sk.fileno(), 0x8933, pack))
        except OSError:
            return error('Wireless interface {0} does not exist.'.format(OPTIONS['<interface>']))
        finally:
            sk.close()
        if_index = int(info[1])
    else:
        if_index = -1

    # Then open a socket to the kernel. Same one used for sending and receiving.
    sk = nl_socket_alloc()  # Creates an `nl_sock` instance.
    ret = genl_connect(sk)  # Create file descriptor and bind socket.
    if ret < 0:
        reason = errmsg[abs(ret)]
        return error('genl_connect() returned {0} ({1})'.format(ret, reason))

    # Now get the nl80211 driver ID. Handle errors here.
    driver_id = genl_ctrl_resolve(sk, b'nl80211')  # Find the nl80211 driver ID.
    if driver_id < 0:
        reason = errmsg[abs(driver_id)]
        return error('genl_ctrl_resolve() returned {0} ({1})'.format(driver_id, reason))

    # Setup the Generic Netlink message.
    msg = nlmsg_alloc()  # Allocate a message.
    if OPTIONS['<interface>']:
        genlmsg_put(msg, 0, 0, driver_id, 0, 0, nl80211.NL80211_CMD_GET_INTERFACE, 0)  # Tell kernel: send iface info.
        nla_put_u32(msg, nl80211.NL80211_ATTR_IFINDEX, if_index)  # This is the interface we care about.
    else:
        # Ask kernel to send info for all wireless interfaces.
        genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, nl80211.NL80211_CMD_GET_INTERFACE, 0)

    # Add the callback function to the nl_sock.
    has_printed = list()
    nl_socket_modify_cb(sk, NL_CB_VALID, NL_CB_CUSTOM, callback, has_printed)

    # Now send the message to the kernel, and get its response, automatically calling the callback.
    ret = nl_send_auto(sk, msg)
    if ret < 0:
        reason = errmsg[abs(ret)]
        return error('nl_send_auto() returned {0} ({1})'.format(ret, reason))
    print('Sent {0} bytes to the kernel.'.format(ret))
    ret = nl_recvmsgs_default(sk)  # Blocks until the kernel replies. Usually it's instant.
    if ret < 0:
        reason = errmsg[abs(ret)]
        return error('nl_recvmsgs_default() returned {0} ({1})'.format(ret, reason))


def setup_logging():
    """Called when __name__ == '__main__' below. Sets up logging library.
    All logging messages go to stderr, from DEBUG to CRITICAL. This script uses print() for regular messages.
    """
    fmt = 'DBG<0>%(pathname)s:%(lineno)d  %(funcName)s: %(message)s'

    handler_stderr = logging.StreamHandler(sys.stderr)
    handler_stderr.setFormatter(logging.Formatter(fmt))

    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG)
    root_logger.addHandler(handler_stderr)


if __name__ == '__main__':
    signal.signal(signal.SIGINT, lambda *_: sys.exit(0))  # Properly handle Control+C
    if OPTIONS.get('--verbose'):
        setup_logging()
    main()
