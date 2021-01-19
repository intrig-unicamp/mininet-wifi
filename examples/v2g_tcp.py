import socket

class TCPClient:
    '''
    TCP Client which can send a messsage and get the response if available.
    '''
    def __init__(self, addrinfo, client_name = "client", server_name = 'server'):
        # Connect to the server
        self.client_name = client_name
        self.server_name = server_name
        (family, socktype, proto, canonname, sockaddr) = addrinfo[0]
        self.sockaddr = sockaddr
        self.s = socket.socket(family, socktype, proto)
        # self.s.bind(('', src_port))
        
    def connect(self):
        self.s.connect(self.sockaddr)
        print('Connected to', self.sockaddr)

    def send(self, message=b'Hello, world'):
        # Send the data and wait for response
        # print('%s -> %s : %s' % (self.client_name, self.server_name, hexlify(message)))
        len_sent = self.s.send(message)

    def get_response(self):
        # Receive a response
        response = self.s.recv(1024)
        # print('%s <- %s : %s' % (self.client_name, self.server_name, hexlify(response)))
        return response

    def close(self):
        # Clean up
        self.s.close()

    def __exit__(self):
        self.close()

class TCPServer:
    '''
    TCP Server which can send a messsage and get the response if available.
    '''
    def __init__(self, port):
        # Connect to the server
        self.s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
        self.s.bind(('', port))
        self.s.listen(1)
        print('Server: Listening TCP messages on port %d' % (port))
        
    def accept(self):
        return self.s.accept()

    def close(self):
        # Clean up
        self.s.close()

    def __exit__(self):
        self.close()
