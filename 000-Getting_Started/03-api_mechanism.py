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

# The following example shows how to read the available programs on your Link 6 robot.

import os
import sys

from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient

# This function uses the program runner client to get all the programs currently saved on the controller, stores them in a map to print their information
def read_available_programs(program_runner):

    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}
    print(programs_map)


def main():
    # Import the utilities helper module
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:

        # Create required services
        program_runner = ProgramRunnerClient(router)

        # Example core
        read_available_programs(program_runner)


if __name__ == "__main__":
    main()
