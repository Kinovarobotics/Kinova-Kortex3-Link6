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

import os, sys, time, threading, random

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient
from kortex_api.autogen.messages import (
    Base_pb2,
    ProgramRunner_pb2,
    Common_pb2,
    ControlConfig_pb2,
    ProtectionZone_pb2,
)
from kortex_api.autogen.client_stubs.ProtectionZoneClientRpc import ProtectionZoneClient
from kortex_api.autogen.messages.Common_pb2 import ModeSelection, OperatingModeType


# Position of the protection zone (in meters)
PROTECTION_ZONE_POS = [0.4, 0.0, 0.4]

# Size of the protection zone (in meters)
PROTECTION_ZONE_DIMENSIONS = [0.05, 0.3, 0.4]

# This function is part of a mechanism that waits for the currently running program to finish before starting a new one
def wait_for_completed(e):
    def check(notif, e=e):
        if notif.event == ProgramRunner_pb2.EXECUTION_EVENT_STARTED:
            print(f"Starting: {notif.handle.program_handle.identifier}")

        if notif.event == ProgramRunner_pb2.EXECUTION_EVENT_COMPLETED:
            print("Completed")
            e.set()

    return check


# This function changes the operating mode of the robot to a desired mdoe.
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

    # Wait for the operating mode change to be completed before proceeding
    time.sleep(2)
    return


# This function creates a twist command to mobe the robot while in recovery state and then exits the recovery state.
def exit_protection_zone(base):

    base.GetArmState()
    # Set the operating mode to jog manual if you want to move the robot outside the protection zone
    change_operating_mode(base, "OPERATING_MODE_JOG_MANUAL")

    # Create twist command that will move the robot outside the protection zone
    command = Base_pb2.TwistCommand()
    command.reference_frame = Base_pb2.CARTESIAN_REFERENCE_FRAME_TOOL
    command.duration = 2
    command.twist.linear_x = 0.00
    command.twist.linear_y = 0.1
    command.twist.linear_z = 0.00
    command.twist.angular_x = 0
    command.twist.angular_y = 0
    command.twist.angular_z = 0

    # While in recovery mode, moving the robot always requires pressing an enabling device
    print("Moving outside protection zone: HOLD ENABLING DEVICE BUTTON")
    time.sleep(3)  # Take this time to press the enabling button

    base.SendTwistCommand(command)
    print(
        "Moved outside the zone successfully, YOU MAY RELEASE THE ENABLING DEVICE BUTTON"
    )
    time.sleep(3)

    # Once the robot is in a safe zone, we can tell it to exit its recovery state
    if base.GetArmState().active_state == Base_pb2.ARMSTATE_RECOVERY:

        base.ExitRecoveryState()
        print(base.GetArmState())

    return


# This function assumes a program called Newhome exists and runs it.
# The suggested Newhome program should bring the robot to a safe position of your choosing in your environment.
def move_to_home_position(base, program_runner):

    # In order to move to the home position, you must go on the robot's Teach Pendant and create a program named "Newhome".
    # This program consist of a single waypoint tile, that has a single waypoint to a desired home position.
    # This position is not the default, factory setting, home position of the robot.
    # It is a position chosen by the user, that the robot can safely reach depending on the robot's surroundings.

    # The program's name can be different than "Newhome", as long as that name is adjusted on line 113 of the code.
    # Finally, the program needs to be validated. This can be done by checking the Validate box, in the top right corner of the TCP's program interface

    change_operating_mode(base, "OPERATING_MODE_AUTO")

    # Going through all the programs available in the TCP
    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}

    # If the program containing the home position is named otherwise, the name must be changed between the brackets below
    program_handle = programs_map["Newhome"]

    # If the program has not been validated with the teach pendant, it is possible to validate it through the API
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
    print("Action completed")
    base.Unsubscribe(notification)

    return 1


# This function voluntarily moves the robot to collide with the previously created protection zone
def move_to_protectionzone(base, program_runner):

    change_operating_mode(base, "OPERATING_MODE_AUTO")

    # Going through all the programs available in the TCP
    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}

    # Choose the program called 'protection zone', which moves the robot inside the protection zone
    program_handle = programs_map["protection zone"]

    # Validate the program through the API if it's not done through the Teach Pendant
    validation = ProgramRunner_pb2.ProgramValidationConfiguration()
    validation.is_valid = True
    validation.program_handle.identifier = program_handle
    program_runner.ValidateProgram(validation)

    config = ProgramRunner_pb2.ProgramStartConfiguration()
    config.handle.program_handle.identifier = program_handle

    program_runner.Start(config)
    # The code below is part of a mechanism that waits for the previous program to finish before starting a new one
    e = threading.Event()
    notification = program_runner.OnNotificationExecutionEventTopic(wait_for_completed(e), Common_pb2.NotificationOptions())
    program_runner.Start(config)
    e.wait(3)
    print("Action completed")
    base.Unsubscribe(notification)

    return


# This function prints all the formation of the existing protection zones
def print_protection_zones(protect_zone_client: ProtectionZoneClient):

    all_protection_zones = protect_zone_client.ReadAllProtectionZones()

    print("PROTECTION ZONES")
    for protection_zone in all_protection_zones.protection_zones:
        message = (
            "Protection Zone : "
            + protection_zone.name
            + ", Handle: "
            + str(protection_zone.handle.identifier)
            + ", Origin : [ "
            + str(round(protection_zone.shape.origin.x, 3))
            + " "
            + str(round(protection_zone.shape.origin.y, 3))
            + " "
            + str(round(protection_zone.shape.origin.z, 3))
            + " ] Dimensions : [ "
        )
        for dim in protection_zone.shape.dimensions:
            message += str(round(dim, 3)) + " "
        message += "]"
        print(message)


# This function creates a new protection zone.
def create_protection_zone(base, protect_zone_client: ProtectionZoneClient):

    # The arm must be powered off to create new protection zones
    if base.GetArmState().active_state == Base_pb2.ARMSTATE_IDLE:
        print("Robot's already powered off.")

    else:
        print("Shutting down the robot to create protection zone.")
        base.DeactivateRobot()

    # Defining protection zone parameters: Defining SHAPE of protection zone
    zone_origin = ControlConfig_pb2.Position(
        x=PROTECTION_ZONE_POS[0], y=PROTECTION_ZONE_POS[1], z=PROTECTION_ZONE_POS[2]
    )
    row1 = Common_pb2.RotationMatrixRow(column1=1.0, column2=0.0, column3=0.0)
    row2 = Common_pb2.RotationMatrixRow(column1=0.0, column2=1.0, column3=0.0)
    row3 = Common_pb2.RotationMatrixRow(column1=0.0, column2=0.0, column3=1.0)
    zone_orientation = Common_pb2.RotationMatrix(row1=row1, row2=row2, row3=row3)

    zone_shape = ProtectionZone_pb2.ZoneShape()

    # Here are the possible shape types for a protection zone:
    # SHAPE_TYPE_UNSPECIFIED (0):   Unspecified shape type
    # SHAPE_TYPE_CYLINDER (1):   Cylinder shape type
    # SHAPE_TYPE_SPHERE (2):   Sphere shape type
    # SHAPE_TYPE_RECTANGULAR_PRISM (3):   Rectangular prism shape type

    zone_shape.shape_type = 3
    zone_shape.origin.CopyFrom(zone_origin)
    zone_shape.orientation.CopyFrom(zone_orientation)
    zone_shape.dimensions.MergeFrom(PROTECTION_ZONE_DIMENSIONS)

    # Defining all protection zone configurations and protection zone handle
    zone_config = ProtectionZone_pb2.ProtectionZoneConfig()

    zone_config.name = "Example Protection Zone"
    zone_config.is_enabled = True
    zone_config.shape.CopyFrom(zone_shape)

    handle = protect_zone_client.CreateProtectionZone(zone_config)
    print("Protection Zone set")
    print_protection_zones(protect_zone_client)

    # Before the robot can be activated again, a delay larger than 8 second needs to be set.
    # This gives enought time to the robot's capacitors to empty themselves before powering on again.
    time.sleep(9)
    base.ActivateRobot()

    # The code below stops the script from running until the robot is done initializing and powering on.
    while base.GetArmState().active_state == Base_pb2.ARMSTATE_INITIALIZATION:
        time.sleep(1)

    print("Robot is ready.")

    return handle


# This function deletes a protection zone when given its handle
def delete_protection_zone(base: BaseClient, protect_zone_client: ProtectionZoneClient, protection_zone_handle):

    # Deactivate robot to allow the deletion of a protection zone
    if base.GetArmState().active_state == Base_pb2.ARMSTATE_IDLE:
        print("Robot's already powered off.")
    else:
        print("Shutting down the robot to delete protection zone")
        base.DeactivateRobot()
        time.sleep(9)

    protect_zone_client.DeleteProtectionZone(protection_zone_handle)
    print("Protection Zone deleted.")

    # When the arm is powered off, its internal capacitors need around 8 seconds before the arm can be powered on again.
    time.sleep(9)
    base.ActivateRobot()

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
        protect_zone_client = ProtectionZoneClient(router)

        # Create protection zone
        protect_zone_handle = create_protection_zone(base, protect_zone_client)
        move_to_home_position(base, program_runner)
        move_to_protectionzone(base, program_runner)

        success = 0
        while success == 0:

            try:
                # Once it has exited its recovery state, we can resume running programs
                success = move_to_home_position(base, program_runner)

            except:

                if base.GetArmState().active_state == Base_pb2.ARMSTATE_IN_FAULT:

                    print(base.GetArmState())
                    base.ClearFaults()

                    # Wait for robot to clear its faults
                    time.sleep(1)

                    # Re-print the arm state to get feedback that the fault state has been cleared successfully
                    print(base.GetArmState())
                    exit_protection_zone(base)

        delete_protection_zone(base, protect_zone_client, protect_zone_handle)


if __name__ == "__main__":
    exit(main())
