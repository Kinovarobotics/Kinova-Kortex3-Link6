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
    """
    Populate AngularWaypoint message with provided angular coordinates and blending.

    Args:
        waypointInformation (tuple): Tuple containing angular coordinates (list), and blending (float).

    Returns:
        Base_pb2.AngularWaypoint: AngularWaypoint message populated with the provided information.
    """
    waypoint = Base_pb2.AngularWaypoint()

    # Extract values from the tuple
    angles_list, blending = waypointInformation

    # Populate AngularWaypoint fields
    waypoint.angles.extend(map(float, angles_list))  # Array of 6 angles, in degrees
    waypoint.blending = blending  # Blending factor, float from 0 to 1 (1 being optimal blending)

    return waypoint
def example_angular_trajectory(base):
    # Set the operating mode to AUTO
    change_operating_mode(base, "OPERATING_MODE_AUTO")

    # Define waypoints with angles and blending values
    waypointsDefinition = [
        ([40, -22, 75, 0, 10, 20], 1),
        ([40, -20, 70, 0, 11, 21], 1),
        ([40, -22, 75, 0, 10, 20], 0),
    ]

    # Create a WaypointList to hold the waypoints
    wptlist = Base_pb2.WaypointList()
    wptlist.use_optimal_blending = True

    # Iterate over waypoints and add them to the WaypointList
    for i, (angles, blending) in enumerate(waypointsDefinition):
        waypoint = wptlist.waypoints.add()
        waypoint.name = f"waypoint_{i}"
        # Populate Angular waypoint coordinates using the provided function
        waypoint.angular_waypoint.CopyFrom(populate_angular_coordinates((angles, blending)))

    # Validate the waypoint list
    result = base.ValidateWaypointList(wptlist)

    # If the list is valid, execute the waypoint list. If not, print an error
    if len(result.trajectory_error_report.trajectory_error_elements) == 0:
        print("Executing angular waypoint trajectory...")
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
