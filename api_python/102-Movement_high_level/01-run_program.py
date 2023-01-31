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

import threading, sys, os, time

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient
from kortex_api.autogen.messages import ProgramRunner_pb2, Common_pb2
from kortex_api.autogen.messages.Common_pb2 import ModeSelection, OperatingModeType

# The following code shows how to run an available program by using the API.
# This method will be used in every example that runs the program moving the robot to its home position.

# This function is part of a mechanism that waits for the previous program to finish before starting a new one
e = threading.Event()


def wait_for_completed(e):
    def check(notif, e=e):
        if notif.event == ProgramRunner_pb2.EXECUTION_EVENT_STARTED:
            print(f"Starting: {notif.handle.program_handle.identifier}")

        if notif.event == ProgramRunner_pb2.EXECUTION_EVENT_COMPLETED:
            print("Completed")
            e.set()

    return check


def change_operating_mode(base, operating_mode_type: str):

    # The possible operating mode types of the robots are:

    # OPERATING_MODE_UNSPECIFIED (0):       Unspecified operating mode
    # OPERATING_MODE_JOG_MANUAL (1):        Jog manual operating mode
    # OPERATING_MODE_HAND_GUIDING (2):      Hand guiding operating mode
    # OPERATING_MODE_HOLD_TO_RUN (3):       Hold to run operating mode
    # OPERATING_MODE_AUTO (4):              Automatic operating mode
    # OPERATING_MODE_MONITORED_STOP (5):    Monitored stop operating mode

    mode = ModeSelection(operating_mode=OperatingModeType.Value(operating_mode_type))
    base.SelectOperatingMode(mode)
    time.sleep(2)

    return


# This function launches a program given a string containing its name
def run_program(program_name: str, base_client, program_runner):

    # Check available programs
    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}
    print(programs_map)

    if program_name in programs_map:
        print("\nSelected program exists")
        program_handle = programs_map[program_name]

    else:
        print("\nSelected program doesn't exist")
        return

    # Validates this program through the API, since you can't run a program through the API if it's not validated.
    validation = ProgramRunner_pb2.ProgramValidationConfiguration()
    validation.is_valid = True
    validation.program_handle.identifier = program_handle
    program_runner.ValidateProgram(validation)

    config = ProgramRunner_pb2.ProgramStartConfiguration()
    config.handle.program_handle.identifier = program_handle

    # Set robot in operating mode auto
    change_operating_mode(base_client, "OPERATING_MODE_MONITORED_STOP")
    change_operating_mode(base_client, "OPERATING_MODE_AUTO")

    # The code below is part of a mechanism that waits for the previous program to finish before starting a new one
    e = threading.Event()
    notification = program_runner.OnNotificationExecutionEventTopic(wait_for_completed(e), Common_pb2.NotificationOptions())
    program_runner.Start(config)
    e.wait()
    base_client.Unsubscribe(notification)


def main():

    # Import the utilities helper module
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:

        # Create required services
        base = BaseClient(router)
        program_runner = ProgramRunnerClient(router)

        # For this code to work, you need create a program using Link 6's teach pendant or Web App.
        # We have a program called 'Newhome' that consists of a home position waypoint that we run.
        # However you can select your created program by entering its name next as a parameter for run_program, instead of 'Newhome'.
        # Same should be done with 'highpos'
        run_program("Newhome", base, program_runner)

        # We run a program after the other to show that the waiting mechanism works correctly
        run_program("highpos", base, program_runner)

    return


if __name__ == "__main__":
    exit(main())
