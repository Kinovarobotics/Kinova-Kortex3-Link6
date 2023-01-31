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

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.client_stubs.IndustrialIOClientRpc import IndustrialIOClient
from kortex_api.autogen.client_stubs.PluginManagerClientRpc import PluginManagerClient
from kortex_api.autogen.messages import (
    PluginManager_pb2,
    IndustrialIO_pb2,
    Common_pb2,
    Plugin_pb2,
)
from kortex_api.autogen.messages.Common_pb2 import ModeSelection, OperatingModeType

# This example illustrates how to use the IndustrialIOClient to change IO configurations, read inputs and write outputs without requiring the usual plugin action mechanisms
# Additional, unused in the example, convenient functions are provided as a reference.
# To run this example, make sure that the controller IO panel is powered.
# For the read/write part of this example to work as intended, a jumper wire should connect DO_4 to DI_0. Connecting other IOs will also work but will require adjusting the selected channels in the code.


e = threading.Event()
_DIGITAL_MIN = 0
_DIGITAL_MAX = 7
_ANALOG_MIN = 0
_ANALOG_MAX = 3


# This function in called asynchronously when a notification is received by the Industrial IO client. We use it to determine when digital inputs are changing
def inputChangeCallback(notif):

    print("State change detected on input DI_" + str(notif.channel.identifier - 1))

    if notif.info.state == IndustrialIO_pb2.DIGITAL_PIN_STATE_HIGH:
        print("Current state is now High")
    else:
        print("Current state is now Low")

    print("Change detected at timestamp: " + str(notif.generic_info.timestamp))

    return


def check_if_plugin_is_ready(plugin_manager: PluginManagerClient, plugin_name: str):
    plugin_list: PluginManager_pb2.PluginInfoList = plugin_manager.GetPluginsList()
    for plugin in plugin_list.plugin_info_list:
        if plugin_name == plugin.handle.identifier:
            # STATE_ACTIVE means the plugin is ready to run actions
            if plugin.plugin_state.state == Plugin_pb2.STATE_ACTIVE:
                return True
            else:
                return False

    return False


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


# This function converts a binary list to an integer
def binary_list_to_int(binary_list):

    int_value = 0
    for bool_value in map(int, reversed(binary_list)):
        int_value = (int_value << 1) | bool_value

    return int_value

# This function gets the state of a digital input given its channel_id
def get_digital_input(io_client, channel_id):
    identifier = IndustrialIO_pb2.DigitalChannelIdentifier()
    identifier.identifier = channel_id + 1

    di_info = io_client.GetDigitalInputInfo(identifier)

    return di_info.state == IndustrialIO_pb2.DigitalPinState.DIGITAL_PIN_STATE_HIGH

# This function returns the state of all digital inputs
def get_digital_inputs(io_client, start_id=_DIGITAL_MIN, end_id=_DIGITAL_MAX, to_int=False):

    if end_id <= start_id:
        raise ValueError("The end_id must be greater than the start_id")

    di_info = io_client.GetAllDigitalInputInfo().infos

    binary_list = [
        bool(
            di_info[channel_id].state
            == IndustrialIO_pb2.DigitalPinState.DIGITAL_PIN_STATE_HIGH
        )
        for channel_id in range(start_id, end_id + 1)
    ]

    if to_int:
        return binary_list_to_int(binary_list)
    else:
        return binary_list

# This function returns the state of a digital output given its channel id
def get_digital_output(io_client, channel_id):

    identifier = IndustrialIO_pb2.DigitalChannelIdentifier()
    identifier.identifier = channel_id + 1

    do_info = io_client.GetDigitalOutputInfo(identifier)

    return do_info.state == IndustrialIO_pb2.DigitalPinState.DIGITAL_PIN_STATE_HIGH

# This function returns the state of all digital outputs
def get_digital_outputs(io_client, start_id=_DIGITAL_MIN, end_id=_DIGITAL_MAX, to_int=False):

    if end_id <= start_id:
        raise ValueError("The end_id must be greater than the start_id")

    do_info = io_client.GetAllDigitalOutputInfo().infos

    binary_list = [
        bool(do_info[channel_id].state == IndustrialIO_pb2.DigitalPinState.DIGITAL_PIN_STATE_HIGH)
        for channel_id in range(start_id, end_id + 1)
    ]

    if to_int:
        return binary_list_to_int(binary_list)
    else:
        return binary_list

# This function sets the state of a chosen digital output to a given value
def set_digital_output(io_client, channel_id, value):

    identifier = IndustrialIO_pb2.DigitalChannelIdentifier()
    identifier.identifier = channel_id + 1

    if value:
        io_client.SetDigitalOutputHighState(identifier)
    else:
        io_client.SetDigitalOutputLowState(identifier)

# This function sets all digitial outputs to values provided as an array
def set_digital_outputs(io_client, digital_outputs, start_id=_DIGITAL_MIN, end_id=_DIGITAL_MAX):

    if end_id <= start_id:
        raise ValueError("The end_id must be greater than the start_id")

    for channel_id in range(start_id, end_id + 1):
        set_digital_output(io_client, channel_id, digital_outputs[channel_id - start_id])

# This function returns the value read on a given analog channel
def get_analog_input_value(io_client, channel_id):

    input_identifier = IndustrialIO_pb2.AnalogIOChannelIdentifier()
    input_identifier.identifier = channel_id + 1
    analog_info = io_client.GetAnalogIOInfo(input_identifier)

    return analog_info.adc_value

# This function returns the value output on a given analog channel
def get_analog_output_value(io_client, channel_id):

    input_identifier = IndustrialIO_pb2.AnalogIOChannelIdentifier()
    input_identifier.identifier = channel_id + 1
    analog_info = io_client.GetAnalogIOInfo(input_identifier)

    return analog_info.dac_value


# This function outputs a desired value on a given analog channel
def set_analog_output_value(io_client, channel_id, value):
# Dac value to output ([0.0; 11.0V] in voltage mode and [0.0; 25.0mA] in current mode)
    analog_output = IndustrialIO_pb2.AnalogOutput()
    analog_output.channel.identifier = channel_id + 1
    analog_output.dac_value = value
    io_client.SetAnalogValue(analog_output)

# This function configures the mode of operation of a given analog channel
def set_analog_mode(io_client, channel_id, analog_mode):
    input_config = IndustrialIO_pb2.AnalogIOConfiguration()
    input_config.channel.identifier = channel_id + 1
    input_config.mode = analog_mode

    io_client.SetAnalogIOConfiguration(input_config)

# This function configures the mode of operation of a given digital output channel
def set_digital_output_mode(io_client, channel_id, digital_mode):
    outpout_config = IndustrialIO_pb2.DigitalOutputConfiguration()
    outpout_config.channel.identifier = channel_id + 1
    outpout_config.mode = digital_mode

    io_client.SetDigitalOutputConfiguration(outpout_config)

# This method prints the full configuration of the all output channels
def get_output_config(io_client):
    for channel_id in range(_DIGITAL_MAX):
        identifier = IndustrialIO_pb2.DigitalChannelIdentifier()
        identifier.identifier = channel_id + 1
        print(io_client.GetDigitalOutputConfiguration(identifier))

    for channel_id in range(_ANALOG_MAX):
        identifier = IndustrialIO_pb2.AnalogIOChannelIdentifier()
        identifier.identifier = channel_id + 1
        print(io_client.GetAnalogIOConfiguration(identifier))

# This function modifies and reads the configuration of outputs, as defined in the plugin configuration
def example_edit_plugin_config(io_client):

    # Get the current config
    print("Initial output config")
    get_output_config(io_client)

    # Modify the config of an analog and a digital output
    # Contrary to when configuration is done through the web app, it is not necessary to turn off the plugin to edit its configuration
    set_analog_mode(
        io_client, 0, IndustrialIO_pb2.AnalogMode.Value("ANALOG_MODE_CURRENT_OUTPUT")
    )
    set_digital_output_mode(io_client, 5, IndustrialIO_pb2.DigitalOutputMode.Value("DIGITAL_OUTPUT_MODE_PUSH_PULL"))

    # Observe the modified configuration
    print("Configured output config")
    get_output_config(io_client)

# This function sends and receives signals using the controller digital IOs
def example_read_write(io_client):

    # Get the current state
    print("Initial state of the input")
    print(get_digital_input(io_client, 0))

    # Set an ouput that will also be read by the input
    set_digital_output(io_client, 4, True)
    print("Updated state of the input")
    print(get_digital_input(io_client, 0))

    # Release the output
    set_digital_output(io_client, 4, False)
    print("Final state of the input")
    print(get_digital_input(io_client, 0))


# In this example, we observe that we can receive notifications when the state of an input is changed.
def example_read_via_notification(io_client: IndustrialIOClient):
    # First we subscribe to notification
    notif_handle = io_client.OnNotificationDigitalInputChangeTopic(inputChangeCallback, Common_pb2.NotificationOptions())

    # We use an output to send a signal to an input
    set_digital_output(io_client, 4, True)
    time.sleep(5)

    # Revert to the initial state
    set_digital_output(io_client, 4, False)
    time.sleep(5)

    # Unsubscribe
    io_client.Unsubscribe(notif_handle)


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
        plugin_name = "industrial_io_plugin"

        if check_if_plugin_is_ready(plugin_manager, plugin_name):
            print(plugin_name + " is ready!")
            io_client = IndustrialIOClient(router)

            change_operating_mode(base, "OPERATING_MODE_AUTO")
            example_edit_plugin_config(io_client)
            example_read_write(io_client)
            example_read_via_notification(io_client)

        else:
            print(plugin_name + " is not ready. Make sure it is installed and running")

    return


if __name__ == "__main__":
    exit(main())
