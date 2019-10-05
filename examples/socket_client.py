import socket

def client():
    host = socket.gethostname()  # get local machine name
    port = 12345  # Make sure it's within the > 1024 $$ <65535 range
    s = socket.socket()
    s.connect((host, port))

    message = raw_input('-> ')
    while message != 'q':
        s.send(str(message).encode('utf-8'))
        data = s.recv(1024).decode('utf-8')
        print('Received from server: ' + data)
        message = raw_input('-> ')
    s.close()

if __name__ == '__main__':
    client()
