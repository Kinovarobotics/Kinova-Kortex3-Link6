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

import threading, sys, os, time
from jsonschema import validate
import json

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.PluginManagerClientRpc import PluginManagerClient
from kortex_api.autogen.client_stubs.PluginClientRpc import PluginClient
from kortex_api.autogen.messages import PluginManager_pb2, Plugin_pb2, Common_pb2
from kortex_api.autogen.messages.Common_pb2 import ModeSelection, OperatingModeType

# This example illustrates how to use the Plugin Manager to determine whether or not a plugin is ready to use and obtain the list of actions it provides
# Then, the example uses a PluginClient to obtain the json input schema of a plugin action, necessary to launch said action from the API.
# To run this example, make sure the Link Toolkit plugin is installed and running on your controller.

e = threading.Event()


def callback(notif):
    # UNSPECIFIED_ACTION_EVENT (0): Unspecified action event
    # ACTION_START (1): Action execution started
    # ACTION_END (2): Action execution end
    # ACTION_ABORT (3): Action execution aborted
    # ACTION_CANCEL (4): Action execution cancelled by a user
    # ACTION_PAUSE (5): Action execution paused
    # ACTION_RESUME (6): Action execution resumed
    # ACTION_FEEDBACK (7): Action provides new feedback
    if notif.action_event == Plugin_pb2.ACTION_START:
        print("Starting")

    if notif.action_event == Plugin_pb2.ACTION_END:
        print("Completed with output: ")
        print(notif.application_data)
        e.set()

    return


def change_operating_mode(base, operating_mode_type: str):

    # The possible operating mode types of the robots are:

    # OPERATING_MODE_UNSPECIFIED (0):       Unspecified operating mode
    # OPERATING_MODE_JOG_MANUAL (1):        Jog manual operating mode
    # OPERATING_MODE_HAND_GUIDING (2):      Hand guiding operating mode
    # OPERATING_MODE_HOLD_TO_RUN (3):       Hold to run operating mode
    # OPERATING_MODE_AUTO (4):              Automatic operating mode
    # OPERATING_MODE_MONITORED_STOP (5):    Monitored stop operating mode

    mode = ModeSelection()
    mode.operating_mode = OperatingModeType.Value(operating_mode_type)
    base.SelectOperatingMode(mode)
    time.sleep(2)

    return

# This function prints all actions from a given plugin
def print_all_plugin_actions(plugin):

    print("Available actions in " + plugin.handle.identifier + ":")
    actions: Plugin_pb2.ActionDescription = plugin.actions
    for action in actions:
        print(action.handle)

    print("\n")

# This function checks if a plugin is ready to run actions
def check_if_plugin_is_ready(plugin_manager: PluginManagerClient, plugin_name: str):
    plugin_list: PluginManager_pb2.PluginInfoList = plugin_manager.GetPluginsList()
    for plugin in plugin_list.plugin_info_list:
        if plugin_name == plugin.handle.identifier:
            print_all_plugin_actions(plugin)
            # STATE_ACTIVE means the plugin is ready to run actions
            if plugin.plugin_state.state == Plugin_pb2.STATE_ACTIVE:
                return True
            else:
                return False

    return False

# This function shows how to call a function made available from a plugin
def use_plugin_action(plugin: PluginClient, action_name: str):
    action_list = plugin.GetActionTypes()

    for available_action in action_list.actions:
        if available_action.friendly_name == action_name:
            action = Plugin_pb2.Action()
            action.serialization_type = Plugin_pb2.DataType.CONFIGURATION_TYPE_JSON
            action.handle.CopyFrom(available_action.handle)
            seek_input = fill_seek_input(15, 0.01, 0.2) # Replace this line with the appropriate input form filler if your program is using a plugin action other than Seek

            validate(seek_input, json.loads(available_action.input_schema))

            action.input = json.dumps(seek_input)
            print("Action found!")

            plugin.StartAction(action)

            e.wait()

            return

    print(action_name + " is not an available action for the selected plugin")

    return

# This function fills the expected json input form of a Seek action
def fill_seek_input(force, speed=0.05, displacement=0.1, frame="Tool", axis="Z+", customFrame=[0, 0, 0]):

    # By exporting a program, we can examine the configuration that should be submitted in json form, which is easily converted to a Python dictionairy

    seek_action = {
        "force_to_detect": force,
        "arm_movement": {
            "speed": speed,
            "max_displacement": displacement,
            "direction": {
                "reference_frame": frame,
                "frame_direction": axis,
                "custom_frame": {
                    "thetaX": customFrame[0],
                    "thetaY": customFrame[1],
                    "thetaZ": customFrame[2],
                },
            },
        },
    }

    return seek_action


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
        plugin_manager = PluginManagerClient(router)
        plugin_name = "link_toolkit"

        if check_if_plugin_is_ready(plugin_manager, plugin_name):
            print(plugin_name + " is ready!")
            link_toolkit_plugin = PluginClient(router, plugin_name)

            # Subscribe to the plugin notification to follow the progress of actions
            link_toolkit_plugin.OnNotificationActionTopic(
                callback, Common_pb2.NotificationOptions()
            )

            change_operating_mode(base, "OPERATING_MODE_AUTO")

            use_plugin_action(link_toolkit_plugin, "Seek")

        else:
            print(plugin_name + " is not ready. Make sure it is installed and running")

    return


if __name__ == "__main__":
    main()
