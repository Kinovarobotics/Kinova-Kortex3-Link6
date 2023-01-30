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
##
from kortex_api.autogen.client_stubs.VariableManagerClientRpc import VariableManagerClient
from kortex_api.autogen.messages import VariableManager_pb2
import os, sys, json


global_vars = {}

# This function allows you to create and edit variables through the API
def create_variable(variable_manager):

    # Choose namespace
    global_ns = VariableManager_pb2.NamespaceHandle()
    global_ns.identifier = "globals"

    # Create a new variable and configure it
    var1 = VariableManager_pb2.Variable()

    # Variable type JSON is equivalent to type Object when creating variables on Link 6's controller teach pendant
    var1.type = VariableManager_pb2.VARIABLE_TYPE_JSON

    var1.schema_key = "default_jointAngles"
    var1.json_value = '{"angles":[1,0,-12,1,-2,1]}'

    # The schema_key and associated json_value can also be:

    #   default_jointAngles,                    json_value = "{\"angles\":[0,0,0,0,0,0]}"
    #   default_pose,                           json_value = "{\"x\":0,\"y\":0,\"z\":0,\"thetaX\":0,\"thetaY\":0,\"thetaZ\":0}"

    #   arm_plugin_matrix_output,               json_value = "{\"poses\":[{\"x\":0,\"y\":0,\"z\":0,\"thetaX\":0,\"thetaY\":0,\"thetaZ\":0},
    #                                                         {\"x\":0.001,\"y\":0.001,\"z\":0.001,\"thetaX\":1,\"thetaY\":1,\"thetaZ\":1},
    #                                                         {\"x\":0.002,\"y\":0.002,\"z\":0.002,\"thetaX\":2,\"thetaY\":2,\"thetaZ\":2}]}"
    #                                                          add as many waypoints to the matrix waypoint list as you wish....

    #   arm_plugin_waypoints_output,            json_value = "{\"x\":0,\"y\":0,\"z\":0,\"thetaX\":0,\"thetaY\":0,\"thetaZ\":0,\"angles\":[0,0,0,0,0,0]}"
    #   industrial_io_plugin_read_input_output  json_value = "{\"result\":0}" or "{\"result\":\"string"}"

    var1handle = VariableManager_pb2.VariableHandle()
    var1handle.namespace_handle.identifier = "globals"
    # This will be the variable's name, you can't have spaces in the name
    var1handle.identifier = "var_example"
    var1.handle.MergeFrom(var1handle)

    # Call SetVariable to create your variable
    variable_manager.SetVariable(var1)

    vars = variable_manager.GetAllVariables(global_ns)

    for var in vars.variables:
        qualpath = f"{var.handle.namespace_handle.identifier}.{var.handle.identifier}"

        if var.type == VariableManager_pb2.VARIABLE_TYPE_JSON:
            payload = json.loads(var.json_value)
        global_vars[qualpath] = payload

    # Check all existing variable in the global namespace (global_ns)
    vars = variable_manager.GetAllVariables(global_ns)
    print(vars)

    # GetVariable(variable_handle) returns the information of a certain variable chosen by setting its handle as parameter,
    # while GetAllVariables(namespace_handle) returns all the variables available in a certain namespace.

    # This is how to return the joint angles of the variable "angles"
    myvar = variable_manager.GetVariable(var1handle)
    joint_angles = json.loads(myvar.json_value)["angles"]
    print(joint_angles)


# This function deletes an existing variable with the name var_name
def delete_var(var_name: str, variable_manager):

    # The namespace in which the variable is located is set ("globals")
    global_ns = VariableManager_pb2.NamespaceHandle()
    global_ns.identifier = "globals"

    to_delete_handle = VariableManager_pb2.VariableHandle()
    to_delete_handle.namespace_handle.identifier = "globals"
    to_delete_handle.identifier = var_name

    vars = variable_manager.GetAllVariables(global_ns)
    var_map = {v.handle.identifier for v in vars.variables}

    if to_delete_handle.identifier in var_map:
        print(
            "\nVariable "
            + str(to_delete_handle.identifier)
            + " exists, it will be deleted\n"
        )
        variable_manager.DeleteVariable(to_delete_handle)
    else:
        print(
            "\nVariable "
            + str(to_delete_handle.identifier)
            + " doesn't exist, can't be deleted\n"
        )


def main():

    # Import the utilities helper module
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:

        variable_manager = VariableManagerClient(router)
        create_variable(variable_manager)
        delete_var("var_example", variable_manager)

    return


if __name__ == "__main__":
    exit(main())
