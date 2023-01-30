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

import os
import sys
import time
import threading

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.messages import Base_pb2, ProgramRunner_pb2, Common_pb2
from kortex_api.autogen.messages.Common_pb2 import (
    ModeSelection,
    NotificationOptions,
    OperatingModeType,
)
from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient

# The following code is used to execute Joint Speed Commands

# Actuator speed (deg/s)
SPEED = 20.0

# This function is part of a mechanism that waits for the previous program to finish before starting a new one
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


def example_move_to_home_position(base, program_runner):

    # Set robot mode to AUTO in order to run a program
    change_operating_mode(base, "OPERATING_MODE_AUTO")

    # Check available programs
    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}
    print(programs_map)

    # For this code to work, you need create a program using Link 6's teach pendant or Web App.
    # We have a program called 'Newhome' that consists of a home position waypoint that we run.
    # However you can select your created program by entering its name in the brackets, instead of 'Newhome'.

    if "Newhome" in programs_map:
        print("\nSelected program exists")
        program_handle = programs_map["Newhome"]

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

    # The code below is part of a mechanism that waits for the previous program to finish before starting a new one
    e = threading.Event()
    notification = program_runner.OnNotificationExecutionEventTopic(wait_for_completed(e), Common_pb2.NotificationOptions())
    program_runner.Start(config)
    e.wait()
    base.Unsubscribe(notification)

    return True


# This function sends a command containing velocities to the joints
def example_send_joint_speeds(base):

    # Set robot mode to JOG_MANUAL in order to execute a Joint Speed Command
    change_operating_mode(base, "OPERATING_MODE_JOG_MANUAL")

    # Create joint speeds
    joint_speeds = Base_pb2.JointSpeeds()

    # The robot will alternate between 4 spins, each for 2.5 seconds
    for times in range(4):

        del joint_speeds.joint_speeds[:]
        # The code below allows us to send a joint speed movement followed by the opposite of said movement
        if times % 2:
            speeds = [-SPEED, 0.0, 0.0, SPEED, 0.0, 0.0]
        else:
            speeds = [SPEED, 0.0, 0.0, -SPEED, 0.0, 0.0]

        i = 0
        # Initializing joint speed parameters
        for speed in speeds:

            joint_speed = joint_speeds.joint_speeds.add()
            joint_speed.joint_identifier = i
            joint_speed.value = speed
            i = i + 1

        base.SendJointSpeedsCommand(joint_speeds)
        time.sleep(2.5)

    print("Stopping the robot")
    base.Stop()

    return True


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

        # Example core
        success = True
        success = example_move_to_home_position(base, program_runner)
        success = example_send_joint_speeds(base)

        return 0 if success else 1


if __name__ == "__main__":
    exit(main())
