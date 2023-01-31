#! /usr/bin/env python3

###
# KINOVA (R) KORTEX (TM)
#
# Copyright (c) 2023 Kinova inc. All rights reserved.
#
# This software may be modified and distributed
# under the terms of the BSD 3-Clause license.
#
# Refer to the LICENSE file for details.
#
###

# This example shows how to create a session and connect to the robot through the API, it will be used in all examples.
# To connect, you must import the utilities.py file, and change the DEFAULT_IP variable on line 27 to your robot's IP address.
# This IP adress can be found by logging in on the teach pendant,
# clicking on the MENU icon in the top left corner, then on SYSTEMS, then NETWORKS, and you'll find your IPv4 ADDRESS (robot's IP)

import os, sys
from kortex_api.autogen.client_stubs.SessionClientRpc import SessionClient


def main():

    # Import the utilities helper module
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:

        # This is how to sign out of your session
        session_manager = SessionClient(router)
        session_manager.CloseSession()

    return


if __name__ == "__main__":
    exit(main())
