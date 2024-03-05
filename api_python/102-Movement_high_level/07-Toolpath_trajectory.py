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

import sys, os, numpy as np, threading, time
from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient
from kortex_api.autogen.messages import Base_pb2, ProgramRunner_pb2, Common_pb2
from kortex_api.autogen.messages.Common_pb2 import ModeSelection,OperatingModeType, CartesianReferenceFrame
global base

# This function is part of a mechanism that waits for the previous program to finish before starting a new one
def wait_for_completed(e):
    
    def check(notif, e = e):
        global base
        #base.GetArmState()
        if notif.event == ProgramRunner_pb2.EXECUTION_EVENT_STARTED:
                print(f"Starting: {notif.handle.program_handle.identifier}")
        
        if notif.event == ProgramRunner_pb2.EXECUTION_EVENT_COMPLETED:
                print("Completed")
                e.set()
    return check


def change_operating_mode(base, operating_mode_type : str):

    # The possible operating mode types of the robots are:

    # OPERATING_MODE_UNSPECIFIED (0):       Unspecified operating mode
    # OPERATING_MODE_JOG_MANUAL (1):        Jog manual operating mode
    # OPERATING_MODE_HAND_GUIDING (2):      Hand guiding operating mode
    # OPERATING_MODE_HOLD_TO_RUN (3):       Hold to run operating mode
    # OPERATING_MODE_AUTO (4):              Automatic operating mode
    # OPERATING_MODE_MONITORED_STOP (5):    Monitored stop operating mode

    mode = ModeSelection(operating_mode = OperatingModeType.Value(operating_mode_type))
    base.SelectOperatingMode(mode)
    time.sleep(2)
    return


def example_move_to_home_position(base, program_runner):

    change_operating_mode(base, "OPERATING_MODE_AUTO")

    # Check available programs
    programs = program_runner.ReadAllPrograms()
    programs_map = {p.name: p.handle.identifier for p in programs.programs}

    """
    In order to move to the home position, you must go on the robot's Teach Pendant and create a program named "Newhome". 
    This program consist of a single waypoint tile, that has a single waypoint to a desired home position. 
    This position is not the default, factory setting, home position of the robot. 
    It is a position chosen by the user, that the robot can safely reach depending on the robot's surroundings.
    The program's name can be different than "Newhome", as long as that name is adjusted on line 68 of the code.
    """

    program_name = 'Newhome'

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
    notification =  program_runner.OnNotificationExecutionEventTopic(wait_for_completed(e), Common_pb2.NotificationOptions())
    program_runner.Start(config)
    e.wait()
    base.Unsubscribe(notification)

    return True

# This function populates a toolpath Arc point waypoint object with the data provided by an array of numbers
def populate_arc_coordinates(waypointInformation):

   
    # This function links the information in waypointDefinitions array to its corresponding data
    waypoint = Base_pb2.ArcPointToolpath()
    waypoint.pose.x = waypointInformation[0]                # in meters
    waypoint.pose.y = waypointInformation[1]                # in meters
    waypoint.pose.z = waypointInformation[2]                # in meters
    waypoint.blending_radius = waypointInformation[3]       # in meters
    waypoint.pose.theta_x = waypointInformation[4]          # in degrees
    waypoint.pose.theta_y = waypointInformation[5]          # in degrees
    waypoint.pose.theta_z = waypointInformation[6]          # in degrees
    waypoint.linear_speed = waypointInformation[8]          # in meters /secondes
    waypoint.linear_acceleration = waypointInformation[9]   # in meters /secondes
    waypoint.via_point_x = waypointInformation[10]          # in meters
    waypoint.via_point_y = waypointInformation[11]          # in meters
    waypoint.via_point_z = waypointInformation[12]          # in meters
    waypoint.angular_acceleration = waypointInformation[13] # in degree /secondes
    return waypoint

# This function populates a toolpath straight segment waypoint object with the data provided by an array of numbers
def populate_segment_coordinates(waypointInformation):        

    # This function links the information in waypointDefinitions array to its corresponding data
    waypoint = Base_pb2.StraightSegmentToolpath()
    waypoint.pose.x = waypointInformation[0]                # in meters
    waypoint.pose.y = waypointInformation[1]                # in meters
    waypoint.pose.z = waypointInformation[2]                # in meters
    waypoint.blending_radius = waypointInformation[3]       # in meters
    waypoint.pose.theta_x = waypointInformation[4]          # in degrees
    waypoint.pose.theta_y = waypointInformation[5]          # in degrees
    waypoint.pose.theta_z = waypointInformation[6]          # in degrees
    waypoint.linear_speed = waypointInformation[8]          # in meters /secondes
    waypoint.linear_acceleration = waypointInformation[9]   # in meters /secondes 
    return waypoint

# This function creates a trajectory using multiple Cartesian waypoints and runs it
def example_trajectory(base: BaseClient):
    
    change_operating_mode(base, "OPERATING_MODE_AUTO")
    
    # define the angular orientation poses of the waypoints
    kTheta_x = 0
    kTheta_y = 180
    kTheta_z = 90

    """
    Array of waypoints and their information as described in fonction populate_segment_coordinates or populate_arc_coordinates depending on waypoint type
    always need to start by a "segment" waypoint
    
    (pose_X,pose_Y,pose_Z,blending_radius,pose_theta_X,pose_theta_Y,pose_theta_Z,"segment",linear_speed,linear_acceleration)
    (pose_X,pose_Y,pose_Z,blending_radius,pose_theta_X,pose_theta_Y,pose_theta_Z,"arc",linear_speed,linear_acceleration,via_point_X,via_point_Y,via_point_Z,angular_acceleration)
    """
    waypointsDefinition = (
    (0.646, 0.158, 0.397, 0.0, kTheta_x, kTheta_y, kTheta_z,"segment",0.2,0.1), 
    (0.646, -0.039, 0.397, 0.0, kTheta_x, kTheta_y, kTheta_z,"segment",0.2,0.1),
    (0.646, -0.39, 0.397, 0.0, kTheta_x, kTheta_y, 70,"arc",0.1,2.5, 0.776,-0.131,0.397,25)
    )

    waypoint_count = len(waypointsDefinition)

    wptlist = Base_pb2.WaypointList()
    wptlist.use_optimal_blending = True
            
    #Create waypoints from the waypointsDefinition array according to its waypoint type
    index = 0
    for i in range(0,waypoint_count):
        waypoint = wptlist.waypoints.add()
        waypoint.name = "waypoint_" + str(index)
        if waypointsDefinition[i][7] == "arc":    
            waypoint.arc_point_toolpath.CopyFrom(populate_arc_coordinates(waypointsDefinition[i]))
        elif  waypointsDefinition[i][7] == "segment":   
            waypoint.straight_segment_toolpath.CopyFrom(populate_segment_coordinates(waypointsDefinition[i]))
        index = index + 1
    
    result = base.ValidateWaypointList(wptlist)

    # If the list is valid, execute the waypoint list. If not, print an error
    if len(result.trajectory_error_report.trajectory_error_elements) == 0:
        
        print("Processing toolpath trajectory...")
        base.ExecuteWaypointTrajectory(wptlist)

    else:
        print("Error found in trajectory")
        print(result.trajectory_error_report)
        return 



def main():
    global base
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
        example_trajectory(base)

        return 


if __name__ == "__main__":
    exit(main())
