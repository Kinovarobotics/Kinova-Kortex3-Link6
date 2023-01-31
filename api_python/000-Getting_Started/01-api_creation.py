#! /usr/bin/env python3

###
# KINOVA (R) KORTEX (TM)
#
# Copyright (c) 2023 Kinova inc. All rights reserved.
#
# This software may be modified and distributed
# under the under the terms of the BSD 3-Clause license.
#
# Refer to the LICENSE file for details.
#
###

import os
import sys

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.DeviceConfigClientRpc import DeviceConfigClient


def main():
    # Import the utilities helper module
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:
        print("Session created")

        # Created Base service and device config client

        # This object handles most of the robot arm and motion logic
        base = BaseClient(router) 
        # This object can survey all the devices available on your robot and get their configuration information
        configclient = DeviceConfigClient(router)
    return


if __name__ == "__main__":
    main()
