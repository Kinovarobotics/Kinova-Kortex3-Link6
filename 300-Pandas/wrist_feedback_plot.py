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
import time, os ,sys
import pandas as pd
from google.protobuf.json_format import MessageToDict
from kortex_api.autogen.client_stubs.BaseCyclicClientRpc import BaseCyclicClient

# This function yields the feedback obtained using the cyclic client
def yield_feedback(base_cyclic_client, count):
    for i in range(count):
        feedback = base_cyclic_client.RefreshFeedback()
        feedback_dict = MessageToDict(feedback.wrist.c61)
        feedback_dict["feedback_id"] = i

        yield feedback_dict
        time.sleep(0.001)


# This function returns real-time information about each wrist's ACTUATOR. 
def get_wrist_data(base_cyclic_client):
    
    # This information is usually viewed by signing in on the robot's teach pendant controller,
    # click on the menu icon, in the top left corner,
    # then clicking on Diagnostics, then Monitoring, and finally choosing the Detailed tab

    df = pd.DataFrame(yield_feedback(base_cyclic_client, 1000))
    df = df.set_index(["feedback_id"])

    df.filter(like="imu", axis=1).plot()
    df.filter(like="force", axis=1).plot()
    print(df)


def main():

    # Import the utilities helper module
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
    
    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:

        base_cyclic_client = BaseCyclicClient(router)
        get_wrist_data(base_cyclic_client)

    return


if __name__ == "__main__":
     exit(main())
