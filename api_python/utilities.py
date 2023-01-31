#! /usr/bin/env python3

###
# KINOVA (R) KORTEX (TM)
#
# Copyright (c) 2022 Kinova inc. All rights reserved.
#
# This software may be modified and distributed
# under the terms of the BSD 3-Clause license.
#
# Refer to the LICENSE file for details.
#
###
import argparse

from kortex_api.autogen.client_stubs.SessionClientRpc import SessionClient
from kortex_api.autogen.messages import Session_pb2
from kortex_api.MqttTransport import MqttTransport
from kortex_api.RouterClient import RouterClient
from kortex_api.UDPTransport import UDPTransport

# To connect, you must import the utilities.py file in your executable files and change the DEFAULT_IP variable on line 27 to your robots IP address.
# This IP adress can be found by logging in on the robot's controller tablet,
# clicking on the MENU icon in the top left corner, then on SYSTEMS, then NETWORKS, and you'll find your IPv4 ADDRESS (robot's IP)

# Link6 physical robot IP ADDRESS
DEFAULT_IP = "192.168.1.10"


def parseConnectionArguments(parser=argparse.ArgumentParser()):
    parser.add_argument(
        "--ip", type=str, help="IP address of destination", default=DEFAULT_IP
    )
    parser.add_argument(
        "-u", "--username", type=str, help="username to login", default="admin"
    )
    parser.add_argument(
        "-p", "--password", type=str, help="password to login", default="admin"
    )
    return parser.parse_args()


class DeviceConnection:

    UDP_PORT = 10001
    MQTT_PORT = 1883

    @staticmethod
    def createUdpConnection(args):
        """        
        returns RouterClient that allows to create services and send requests to a device or its sub-devices @ 1khz.
        """

        return DeviceConnection(
            args.ip,
            port=DeviceConnection.UDP_PORT,
            credentials=(args.username, args.password),
        )

    @staticmethod
    def createMqttConnection(args):
        """        
        returns RouterClient that allows to create services and send requests to a device or its sub-devices @ 1khz.
        """

        return DeviceConnection(
            args.ip,
            port=DeviceConnection.MQTT_PORT,
            credentials=(args.username, args.password),
        )

    def __init__(self, ipAddress: str = DEFAULT_IP, port: int = MQTT_PORT, credentials=("", "")):

        self.ipAddress = ipAddress
        self.port = port
        self.credentials = credentials

        self.sessionManager = None

        # Setup API
        port_mapping = {
            
            self.UDP_PORT: UDPTransport,
            self.MQTT_PORT: MqttTransport,
        }

        self.transport = port_mapping[port]()

        self.router = RouterClient(self.transport, errorCallback = None)

    # Called when entering 'with' statement
    def __enter__(self):
        
        self.transport.connect(self.ipAddress, self.port)

        if self.credentials[0] != "":
            
            session_info = Session_pb2.CreateSessionInfo()
            session_info.username = self.credentials[0]
            session_info.password = self.credentials[1]
            session_info.session_inactivity_timeout = 10000  # (milliseconds)
            session_info.connection_inactivity_timeout = 2000  # (milliseconds)

            self.sessionManager = SessionClient(self.router)
            print("Logging as", self.credentials[0], "on device", self.ipAddress)
            self.sessionManager.CreateSession(session_info)

        return self.router

    # Called when exiting 'with' statement
    def __exit__(self, exc_type, exc_value, traceback):

        if self.sessionManager != None:

            self.sessionManager.CloseSession()

        self.transport.disconnect()
