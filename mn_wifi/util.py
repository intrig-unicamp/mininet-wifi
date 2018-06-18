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
