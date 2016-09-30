#!/usr/bin/python
#Script created by VND - Visual Network Description (SDN version) 
#You can start floodlight controller with command: java -jar target/floodlight.jar

import httplib
import json
class StaticFlowPusher(object):
    def __init__(self, server):
        self.server = server
    def get(self, data):
        ret = self.rest_call({}, 'GET')
        return json.loads(ret[2])
    def set(self, data):
        ret = self.rest_call(data, 'POST')
        return ret[0] == 200
    def remove(self, objtype, data):
        ret = self.rest_call(data, 'DELETE')
        return ret[0] == 200
    def rest_call(self, data, action):
        path = '/wm/staticflowentrypusher/json'
        headers = {
            'Content-type': 'application/json',
            'Accept': 'application/json',
            }
        body = json.dumps(data)
        conn = httplib.HTTPConnection(self.server, 8080)
        conn.request(action, path, body, headers)
        response = conn.getresponse()
        ret = (response.status, response.reason, response.read())
        print ret
        conn.close()
        return ret
pusher = StaticFlowPusher('127.0.0.1')

flow_0 = {
    'switch':"00:00:00:00:00:00:00:01",
    "name":"Flow_flow1",
    "cookie":"0",
    "dst-mac":"00:00:00:00:00:04",
    "ingress-port":"2",
    "active":"true",
    "actions":"output=1"
#    "actions":"set-dst-ip=192.168.0.1,output=1"
    }
pusher.set(flow_0)

flow_1 = {
    'switch':"00:00:00:00:00:00:00:01",
    "name":"Flow_flow2",
    "cookie":"0",
    "ingress-port":"1",
    "src-mac":"00:00:00:00:00:04",
    "active":"true",
    "actions":"set-dst-ip=192.168.0.100,output=2"
    }
pusher.set(flow_1)

flow_2 = {
    'switch':"00:00:00:00:00:00:00:05",
    "name":"Flow_flow3",
    "cookie":"0",
    "dst-mac":"02:00:00:00:02:00",
    "ingress-port":"1",
    "active":"true",
    "actions":"set-dst-ip=192.168.0.100,output=2"
    }
pusher.set(flow_2)

flow_3 = {
    'switch':"00:00:00:00:00:00:00:05",
    "name":"Flow_flow4",
    "cookie":"0",
    "src-mac":"02:00:00:00:02:00",
    "ingress-port":"2",
    "active":"true",
    "actions":"set-dst-ip=192.168.0.1,output=1"
    }
pusher.set(flow_3)


flow_4 = {
    'switch':"00:00:00:00:00:00:00:02",
    "name":"Flow_flow5",
    "cookie":"0",
    "dst-mac":"00:00:00:00:00:04",
    "ingress-port":"2",
    "active":"true",
    "actions":"output=1"
#    "actions":"set-dst-ip=192.168.1.1,output=1"
    }
pusher.set(flow_4)

flow_5 = {
    'switch':"00:00:00:00:00:00:00:02",
    "name":"Flow_flow6",
    "cookie":"0",
    "ingress-port":"1",
    "src-mac":"00:00:00:00:00:04",
    "active":"true",
    "actions":"set-dst-ip=192.168.1.100,output=2"
    }
pusher.set(flow_5)

flow_6 = {
    'switch':"00:00:00:00:00:00:00:05",
    "name":"Flow_flow7",
    "cookie":"0",
    "dst-mac":"02:00:00:00:03:00",
    "ingress-port":"1",
    "active":"true",
    "actions":"set-dst-ip=192.168.1.100,output=3",
    }
pusher.set(flow_6)

flow_7 = {
    'switch':"00:00:00:00:00:00:00:05",
    "name":"Flow_flow8",
    "cookie":"0",
    "src-mac":"02:00:00:00:03:00",
    "ingress-port":"3",
    "active":"true",
    "actions":"output=1"
#    "actions":"set-dst-ip=192.168.0.1,output=1"
    }
pusher.set(flow_7)

