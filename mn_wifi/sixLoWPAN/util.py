"Utility functions for Mininet-WiFi"

from mininet.util import retry

def moveIntfNoRetry(intf, dstNode, printError=False):
    """Move interface to node, without retrying.
       intf: string, interface
        dstNode: destination Node
        printError: if true, print error"""
    from mn_wifi.node import Station, Car, AP

    if (isinstance(dstNode, Station) or isinstance(dstNode, Car)
            or isinstance(dstNode, AP) and 'eth' not in str(intf)):
        if isinstance(dstNode, Station) or isinstance(dstNode, Car):
            return True
    else:
        return True

def moveIntf(intf, dstNode, printError=True,
             retries=3, delaySecs=0.001):
    """Move interface to node, retrying on failure.
       intf: string, interface
       dstNode: destination Node
       printError: if true, print error"""
    from mn_wifi.node import AP

    if not isinstance(dstNode, AP):
        retry(retries, delaySecs, moveIntfNoRetry, intf, dstNode,
              printError=printError)

def ipAdd6(i, prefixLen=64, ipBaseNum=0x0a000000):
    """Return IP address string from ints
       i: int to be added to ipbase
       prefixLen: optional IP prefix length
       ipBaseNum: option base IP address as int
       returns IP address as string"""
    #imax = 0xffffffff >> prefixLen
    #assert i <= imax, 'Not enough IP addresses in the subnet'
    #mask = 0xffffffff ^ imax
    mask = 64
    ipnum = (ipBaseNum & mask) + i
    return ipStr(ipnum)

def ipStr(ip):
    """Generate IP address string from an unsigned int.
       ip: unsigned int of form w << 24 | x << 16 | y << 8 | z
       returns: ip address string w.x.y.z"""
    x1 = (ip >> 112) & 0xffff
    x2 = (ip >> 96) & 0xffff
    x3 = (ip >> 80) & 0xffff
    x4 = (ip >> 64) & 0xffff
    x5 = (ip >> 48) & 0xffff
    x6 = (ip >> 32) & 0xffff
    x7 = (ip >> 16) & 0xffff
    x8 = ip & 0xffff
    x1 = 2001
    return "%s:%s:%s:%s:%s:%s:%s:%s" % (x1, x2, x3, x4, x5, x6, x7, x8)

def ipNum(x1, x2, x3, x4, x5, x6, x7, x8):
    "Generate unsigned int from components of IP address"
    return (x1 << 112) | (x2 << 96) | (x3 << 80) | (x4 << 64) | (x5 << 48) | (x6 << 32) | (x7 << 16) | x8

def ipParse(ip):
    "Parse an IP address and return an unsigned int."
    args = [ int(arg) for arg in ip.split(':') ]
    while len(args) < 8:
        args.append(0)
    return ipNum(*args)

def netParse(ipstr):
    """Parse an IP network specification, returning
       address and prefix len as unsigned ints"""
    prefixLen = 0
    if '/' in ipstr:
        ip, pf = ipstr.split('/')
        prefixLen = int(pf)
    # if no prefix is specified, set the prefix to 24
    else:
        ip = ipstr
        prefixLen = 24
    return ipParse(ip), prefixLen
