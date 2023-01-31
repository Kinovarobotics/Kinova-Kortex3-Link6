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

from itertools import zip_longest
import numpy as np

import sys
import os
import time
import threading

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient
from kortex_api.autogen.messages import Base_pb2, ProgramRunner_pb2, Common_pb2
from kortex_api.autogen.messages.Common_pb2 import ModeSelection, OperatingModeType

# The following code is used to execute Actions and TwistCommands. It also shows how to send a stream of twist commands

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


# This function sends a twist command via an action
def example_twist_action(base):

    action_name = "Twist_Action"

    command = Base_pb2.TwistCommand()
    command.reference_frame = Base_pb2.CARTESIAN_REFERENCE_FRAME_TOOL
    command.duration = 5  # seconds
    command.twist.linear_x = 0.01  # m/s
    command.twist.linear_y = 0.01  # m/s
    command.twist.linear_z = 0.01  # m/s
    command.twist.angular_x = 0  # deg/s
    command.twist.angular_y = 3  # deg/s
    command.twist.angular_z = 0  # deg/s

    action = Base_pb2.Action()
    action.send_twist_command.MergeFrom(command)
    action.name = action_name
    action.handle.identifier = 100
    action.application_data = ""

    # Actions only execute in the operating mode HOLD_TO_RUN
    change_operating_mode(base, "OPERATING_MODE_HOLD_TO_RUN")

    base.ExecuteAction(action)

    return True


# This function sends twist commands without the Action mechanism
def example_twist_command(base):

    command = Base_pb2.TwistCommand()
    command.reference_frame = Base_pb2.CARTESIAN_REFERENCE_FRAME_TOOL
    command.duration = 2  # seconds
    command.twist.linear_x = 0.05  # m/s
    command.twist.linear_y = 0.05  # m/s
    command.twist.linear_z = 0.05  # m/s
    command.twist.angular_x = 0  # deg/s
    command.twist.angular_y = 1  # deg/s
    command.twist.angular_z = 0  # deg/s

    # Twist commands only work in JOG_MANUAL mode
    change_operating_mode(base, "OPERATING_MODE_JOG_MANUAL")
    base.SendTwistCommand(command)

    # Waits for the command to be done
    time.sleep(command.duration)
    print("Twist Command Done")

    return True


# This function configures a twist command
def fill_twist_command(twist):

    twistcmd = Base_pb2.TwistCommand()
    descriptor = twistcmd.twist.DESCRIPTOR.fields_by_name.keys()

    twistcmd.reference_frame = (Base_pb2.CartesianReferenceFrame.CARTESIAN_REFERENCE_FRAME_BASE)
    twistcmd.duration = 2  # seconds

    for field, cmd in zip_longest(descriptor, twist):
        if cmd is not None:
            setattr(twistcmd.twist, field, float(cmd))
        else:
            setattr(twistcmd.twist, field, 0.0)

    return twistcmd


# This function generates multiple twist commands by using the fill_twist_command function to configure them
def yield_commands(duration):

    nframe = duration * 40
    t = np.linspace(0.0, 4 * 2 * np.pi, nframe)
    z = np.sin(t) * 0.2
    for zt in z:
        yield fill_twist_command([0.0, 0.0, zt])


# This function runs a trajectory defined by cartesian velocities by the yield_commands function
def example_twist_stream(duration, base):

    # Twist commands only work in JOG_MANUAL mode
    change_operating_mode(base, "OPERATING_MODE_JOG_MANUAL")

    FREQ = 1 / 40
    for twist_cmd in yield_commands(duration):

        base.SendTwistCommand(twist_cmd)
        time.sleep(FREQ)

    return True


def example_move_to_home_position(base, program_runner):

    # Set robot mode to AUTO in order to run this program
    change_operating_mode(base, "OPERATING_MODE_AUTO")

    # Check available programs
    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}
    print(programs_map)

    # For this code to work, you need create a program using Link 6's teach pendant or Web App.
    # We have a program called 'Newhome' that consists of a home position waypoint that we run.
    # However you can select your created program by entering its name next to program_name, instead of 'Newhome'.
    program_name = "Newhome"

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

    # The code below is part of a mechanism that waits for the previous program to finish before starting a new one
    e = threading.Event()
    notification = program_runner.OnNotificationExecutionEventTopic(wait_for_completed(e), Common_pb2.NotificationOptions())
    program_runner.Start(config)
    e.wait()
    base.Unsubscribe(notification)

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

        example_move_to_home_position(base, program_runner)
        example_twist_command(base)
        example_move_to_home_position(base, program_runner)
        example_twist_action(base)

        print("Running twist command stream: HOLD ENABLING DEVICE ON")
        time.sleep(2)
        example_twist_stream(4, base)

        return


if __name__ == "__main__":
    exit(main())
