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
from kortex_api.autogen.client_stubs.ToolManagerClientRpc import ToolManagerClient
from kortex_api.autogen.messages import ToolPlugin_pb2
from numpy import append
import pandas as pd
import sys, os

# This function organizes the available tools information in a data frame, and prints it in the terminal
def tool_managing(tool_manager):
    
    tools = tool_manager.GetAllToolsInformation()

    ti = ToolPlugin_pb2.ToolInformation()
    columns = []
    columns.append("handle")
    columns.append("active_index")
    columns.append("name")
    columns.append("mass")
    columns.extend([f"tcp_{s}" for s in ti.transform.DESCRIPTOR.fields_by_name.keys()])
    columns.extend(
        [f"com_{s}" for s in ti.center_of_mass.DESCRIPTOR.fields_by_name.keys()]
    )
    columns.extend(ti.inertia.DESCRIPTOR.fields_by_name.keys())

    df = pd.DataFrame(columns=columns)
    temp = []

    for tool in tools.tools_information:

        new_tool = dict.fromkeys(columns)
        new_tool["handle"] = tool.handle.identifier
        new_tool["active_index"] = tool.active_index
        new_tool["name"] = tool.friendly_name

        for field in tool.transform.DESCRIPTOR.fields_by_name.keys():
            new_tool[f"tcp_{field}"] = getattr(tool.transform, field)
        for field in tool.center_of_mass.DESCRIPTOR.fields_by_name.keys():
            new_tool[f"com_{field}"] = getattr(tool.center_of_mass, field)
        for field in tool.inertia.DESCRIPTOR.fields_by_name.keys():
            new_tool[field] = getattr(tool.inertia, field)
        temp.append(new_tool)

    df = pd.DataFrame(temp)
    print(df)


def main():

    # Import the utilities helper module
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:

        tool_manager = ToolManagerClient(router)
        tool_managing(tool_manager)

    return


if __name__ == "__main__":
    exit(main())
