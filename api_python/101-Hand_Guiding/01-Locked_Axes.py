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

import numpy as np
import math

import sys
import os
import time
import threading

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.ControlConfigClientRpc import ControlConfigClient
from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient
from kortex_api.autogen.messages import (
    ProgramRunner_pb2,
    Common_pb2,
    ControlConfig_pb2,
)
from kortex_api.autogen.messages.Common_pb2 import ModeSelection, OperatingModeType


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

    # Set robot mode to AUTO in order to run this program
    change_operating_mode(base, "OPERATING_MODE_AUTO")

    # Check available programs
    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}
    print(programs_map)

    # For this code to work, you need create a program using Link 6's teach pendant or Web App.
    # We have a program called 'Newhome' that consists of a home position waypoint that we run.
    # However you can select your created program by entering its name next to program_name, instead of 'Newhome'.
    program_name = "Home Position"

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


# This function illustrates how to use the API to prevent motion in given directions during hand-guiding
# Contrary to what is possible via the Web interface, using the API allows locked axes to be in arbitrary directions
def example_locked_axes(base, controlConfig: ControlConfigClient):
    # Get Current axis locked config
    controlModeInfo = ControlConfig_pb2.ControlModeInformation()
    controlModeInfo.control_mode = 6  # ControlConfig_pb2.ControlMode.Value('CARTESIAN_HAND_GUIDING')
    lockedAxes: ControlConfig_pb2.AxisLockConfig = controlConfig.GetLockedCartesianAxes(controlModeInfo)
    print(lockedAxes)
    print("\n")
    # Update locked Axes
    lockedAxes.translation.axes.clear()
    lockedAxes.translation.reference_frame = Common_pb2.CARTESIAN_REFERENCE_FRAME_BASE

    # Locks the motion on a diagonal line
    vector = Common_pb2.CartesianVector()
    vector.x = 1
    vector.y = 1
    vector.z = 0
    lockedAxes.translation.axes.append(vector)
    vector.x = 0
    vector.y = 0
    vector.z = 1
    lockedAxes.translation.axes.append(vector)
    vector.x = 1
    vector.y = 0
    vector.z = 0
    lockedAxes.orientation.axes.append(vector)
    vector.x = 0
    vector.y = 1
    vector.z = 0
    lockedAxes.orientation.axes.append(vector)
    vector.x = 0
    vector.y = 0
    vector.z = 1
    lockedAxes.orientation.axes.append(vector)

    print(lockedAxes)
    print("\n")
    controlConfig.SetLockedCartesianAxes(lockedAxes)

    newLockedAxes = controlConfig.GetLockedCartesianAxes(controlModeInfo)
    print(newLockedAxes)


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
        controlConfig = ControlConfigClient(router)
        program_runner = ProgramRunnerClient(router)

        # example_move_to_home_position(base, program_runner)
        example_locked_axes(base, controlConfig)

        return


if __name__ == "__main__":
    exit(main())
