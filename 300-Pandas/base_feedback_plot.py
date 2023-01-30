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


# Pandas is necessary for this example to work. You may install pandas using pip with the command 'pip install pandas'
import pandas as pd
from google.protobuf.json_format import MessageToDict
from kortex_api.autogen.client_stubs.BaseCyclicClientRpc import BaseCyclicClient
import time, sys, os

# This funcion returns real time information on the robot's tool pose and the tool external wrench torque, every 0.01 seconds
def yield_tool_feedback(base_cyclic_client, count):
    for i in range(count):
        feedback = base_cyclic_client.RefreshFeedback()
        feedback_dict = MessageToDict(feedback.base)

        tool_feedback = {k: v for k, v in feedback_dict.items() if k.startswith("tool")}
        tool_feedback["feedback_id"] = i
        yield tool_feedback
        time.sleep(0.001)


def main():

    # Import the utilities helper module
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
    
    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:

        base_cyclic_client = BaseCyclicClient(router)

        # pd.DataFrame() generates a two-dimensional, size-mutable, potentially heterogeneous tabular data.
        df = pd.DataFrame(yield_tool_feedback(base_cyclic_client, 1000))
        df = df.set_index(["feedback_id"])
        # you need to install matplotlib for this to work. If you are on linux, open the terminal and enter 'python3 -mpip install matplotlib'
        # The filter() function is used to subset rows or columns of dataframe according to labels in the specified index.
        df.filter(like="toolPose", axis=1).plot()
        print(df)

if __name__ == "__main__":
     exit(main())
