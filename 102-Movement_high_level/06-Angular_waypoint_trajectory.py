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

import sys, os, threading, time, numpy as np
from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient
from kortex_api.autogen.messages import Base_pb2, ProgramRunner_pb2
from kortex_api.autogen.messages.Common_pb2 import ModeSelection, OperatingModeType

# This function is part of a mechanism that waits for the previous program to finish before starting a new one
def wait_for_completed_program(e):
    def check(notif, e=e):
        if notif.event == ProgramRunner_pb2.EXECUTION_EVENT_STARTED:
            print(f"Starting: {notif.handle.program_handle.identifier}")

        if notif.event == ProgramRunner_pb2.EXECUTION_EVENT_COMPLETED:
            print("Completed Program")
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

    change_operating_mode(base, "OPERATING_MODE_AUTO")

    # Check available programs
    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}

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
    notification = program_runner.OnNotificationExecutionEventTopic(wait_for_completed_program(e), Base_pb2.NotificationOptions())
    program_runner.Start(config)
    e.wait()
    base.Unsubscribe(notification)

    return True

# This function populates an Angular waypoint object with the data provided by an array
def populate_angular_coordinates(waypointInformation):


    waypoint = Base_pb2.AngularWaypoint()
    waypoint.angles.MergeFrom(waypointInformation[0])  # array of 6 angles, in degrees
    waypoint.duration = waypointInformation[1]  # in seconds
    waypoint.blending = waypointInformation[2]  # float from 0 to 1, 1 being the optimal blending

    return waypoint


def example_angular_trajectory(base):

    change_operating_mode(base, "OPERATING_MODE_AUTO")

    waypointsDefinition = (
        ([-15.0, 50.6, 84.8, 0.8, -23.8, -13.9], 5, 0),
        ([0.6, 40.1, 60.1, 5.3, 0.0, -20.1], 5, 0),
        ([30.5, 76.5, 90.5, 0.4, -20.1, -15.6], 5, 0),
        ([-15.6, 30.8, 85.1, 10.9, -10.7, -20.3], 5, 0),
        ([-20.4, 30.5, 80.5, 0.4, -20.1, -10.6], 5, 0),
        ([20.8, 30.7, 70.2, 0.8, -30.5, -15.8], 5, 0),
        ([50.0, 45.6, 84.9, 0.2, -20.3, -5.0], 5, 0),
    )

    waypoint_count = len(waypointsDefinition)

    wptlist = Base_pb2.WaypointList()
    wptlist.use_optimal_blending = True

    # Create waypoints from the waypointsDefinition array
    array_wpts = np.array([])
    index = 0
    for i in range(0, waypoint_count):

        np.append(array_wpts, waypointsDefinition[i])
        waypoint = wptlist.waypoints.add()
        waypoint.name = "waypoint_" + str(index)
        waypoint.angular_waypoint.CopyFrom(populate_angular_coordinates(waypointsDefinition[i]))
        index = index + 1

    # Add waypoints to waypoint list
    wptlist.waypoints.MergeFrom(array_wpts)
    wptlist.duration = 30  # in seconds
    result = base.ValidateWaypointList(wptlist)

    # If list is valid, execute the waypoint list. If not, print an error
    if len(result.trajectory_error_report.trajectory_error_elements) == 0:

        print("Reaching cartesian pose trajectory...")
        base.ExecuteWaypointTrajectory(wptlist)

    else:
        print("Error found in trajectory")
        print(result.trajectory_error_report)
        return


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
        example_move_to_home_position(base, program_runner)
        example_angular_trajectory(base)

        return


if __name__ == "__main__":
    exit(main())
