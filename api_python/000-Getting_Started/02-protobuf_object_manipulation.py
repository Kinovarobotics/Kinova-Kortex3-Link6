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

from kortex_api.autogen.client_stubs.BaseClientRpc import BaseClient
from kortex_api.autogen.messages import Base_pb2
from kortex_api.autogen.messages.ProtectionZone_pb2 import LimitationType

import json


def example_manipulation_protobuf_basic():

    # In Google Protocol Buffer, there are many scalar value types you can declare.
    # All have a corresponding type in Python.
    # Here's the list:

    # Proto type : Python type
    # double : float
    # float : float
    # int32 : int
    # int64 : int
    # uint32 : int/long
    # uint64 : int/long
    # sint32 : int
    # sint64 : int/long
    # fixed32 : int/long
    # fixed64 : int/long
    # sfixed32 : int
    # sfixed64 : int/long
    # bool : bool
    # string : str
    # bytes : str

    # These types can be used just like regular Python variables.
    # For more information about scalar value types visits:
    # https://developers.google.com/protocol-buffers/docs/proto3#scalar

    # You can regroup many of these scalar value in a message. The message is a structure used in Google Protocol Buffer
    # to ensure all object information is in scope. Scalar values don't exist on their own
    # if they are not contained in a message.

    # Here's a quick example using the Kinova API UserProfile message:
    # message UserProfile {
    #     Kinova.Api.Common.UserProfileHandle handle = 1; // User handle (no need to set it with CreateUserProfile()
    #     string username = 2;                            // username, which is used to connect to robot (or via Web App login)
    #     string firstname = 3;                           // user first name
    #     string lastname = 4;                            // user last name
    #     string application_data = 5;                    // other application data (used by Web App)
    # }

    user_profile = Base_pb2.UserProfile()
    # Each scalar value in a message has a set_<field> function to set the value
    # and a getter which is simply the variable name
    user_profile.username = (
        "jcash"  # We can directly affect data to a variable attribute
    )
    user_profile.firstname = "Johnny"
    user_profile.lastname = "Cash"
    # Handle and application_data are ignored on purpose


def example_manipulation_protobuf_object():

    # Messages are the main element in Google Protocol Buffer in the same way classes are to Python. You need a message
    # to make a workable object. A message can contain many kind of elements. We have already
    # covered the scalar value and in this section we are going to cover the message.
    #

    # A message can make a reference to another message to make more comprehensive element.

    # For this example we'll use the FullUserProfile and UserProfile messages.
    # message FullUserProfile {
    #     UserProfile user_profile = 1; //User Profile, which includes username.
    #     string password = 2; //User's password
    # }
    # message UserProfile {
    #     Kinova.Api.Common.UserProfileHandle handle = 1; // User handle (no need to be set)
    #     string username = 2;                            // username, which is used to connect to robot (or via Web App login)
    #     string firstname = 3;                           // user first name
    #     string lastname = 4;                            // user last name
    #     string application_data = 5;                    // other application data (used by Web App)
    # }

    # https://developers.google.com/protocol-buffers/docs/proto3#simple

    full_user_profile = Base_pb2.FullUserProfile()
    # Now we'll add data to the scalar
    full_user_profile.password = "MyPassword"

    # Now I want to work with the user profile attribute which is a message itself.
    # Since user profile is a message you can use the '.' to access
    # these attributes.
    full_user_profile.user_profile.username = "jcash"
    full_user_profile.user_profile.firstname = "Johnny"
    full_user_profile.user_profile.lastname = "Cash"

    # Another basic element is the enum. Enum is directly available from
    # the message - no need to use the enum 'message'.
    # Here's an example:

    # enum CartesianlimitEventType {
    # UNSPECIFIED_CARTESIAN_LIMIT_EVENT (0):   Unspecified cartesian limit event
    # TOOL_LINEAR_VELOCITY_LIMIT (1):   Tool linear velocity limit reached
    # TOOL_ANGULAR_VELOCITY_LIMIT (2):   Tool angular velocity limit reached
    # ELBOW_LINEAR_VELOCITY_LIMIT (3):   Elbow linear velocity limit reached
    # TOOL_LINEAR_VELOCITY_SATURATED (4):   Tool linear velocity saturated
    # TOOL_ANGULAR_VELOCITY_SATURATED (5):   Tool angular velocity saturated
    # ELBOW_LINEAR_VELOCITY_SATURATED (6):   Elbow linear velocity saturated
    # }

    # message CartesianLimitation {
    #     LimitationType type = 1;     // limitation type
    # }

    # https://developers.google.com/protocol-buffers/docs/proto3#enum

    # Here is how to create protobuff objects and use the enum to set the objects' parameters.
    # This method of object creation and parameter setting is fundamental when using Link 6's API.
    # It will be repeated in the examples to follow

    # First, we create an object of type CartesianLimitEvent
    CartesianLimit = Base_pb2.CartesianLimitEvent()
    # Then, we create the enum CartesianLimitEventType and give it a value
    limit_type = Base_pb2.CartesianLimitEventType.Value("UNSPECIFIED_CARTESIAN_LIMIT_EVENT")
    # Finally, we set the "event" parameter of our CartesianLimitEvent Object to the enum created below
    CartesianLimit.event = limit_type
    # We also have to set the "limit" parameter of our CartesianLimitEvent object
    CartesianLimit.cartesian_limit = 1.0


def example_manipulation_protobuf_list():

    # In Google Protocol Buffer, 'repeated' is used to designate a list of indeterminate length. Once affected to a Python
    # variable they can be used in the same way as a standard list.
    # Note that a repeated message field doesn't have an append() function, it has an add() function which creates the new message object.

    # First, we create an object of type JointAngles
    joint_angles_message = Base_pb2.JointAngles()

    # To access the array of joint angles, we call "joint_angles_message.joint_angles". The line below allows us to create a shortcut "joint_angles_array"
    # instead of having to call "joint_angles_message.joint_angles" every time we want to modify said array
    joint_angles_array = joint_angles_message.joint_angles

    from google.protobuf import (json_format)  #  import the module that allows us to use json objects
    from google.protobuf.json_format import ParseDict

    # Since Link 6 has 6 joints, we create a for loop that increments 6 times. In other words, from i = 0 to i = 5.
    for i in range(6):

        # For each increment, we create a reference "joint_angle_message" using joint_angles_array.add() that allows us to store the joint angle's ID and value
        joint_angle_message = joint_angles_array.add()
        joint_angle_message.joint_identifier = i
        joint_angle_message.value = 45.0

        print(
            "\nJoint ID and Angle : "
            + str(joint_angle_message.joint_identifier + 1)
            + ", "
            + str(joint_angle_message.value)
            + " deg"
        )

        # From the Google Protocol Buffer library you can use the json_format module. One useful function is the MessageToJson.
        # This function serializes the Google Protocol Buffer message to a JSON object. It is helpful when you want to print a large object into a human-readable format.
        json_object = json_format.MessageToJson(joint_angle_message)
        print("\nJson object: ")
        print(json_object)

        # We can also convert a JSON representation back to its "message" representation using Parse
        pyDict_obj = json.loads(json_object)
        parse_object = json_format.ParseDict(pyDict_obj, joint_angle_message, ignore_unknown_fields=False)
        print("\nMessage format:")
        print(parse_object)

        print("*********************************")


def example_manipulation_protobuf_helpers():

    # All the module google.protobuf documentation is available here:
    # https://developers.google.com/protocol-buffers/docs/reference/python/

    # First, the function include with message instance. We will cover the next function
    #     Clear
    #     MergeFrom
    #     CopyFrom

    # The Clear function is straightforward - it clears all the message attributes.

    # MergeFrom and CopyFrom have the same purpose: to duplicate data into another object.
    # The difference between them is that CopyFrom will do a Clear before a MergeFrom.

    # For its part MergeFrom will merge data if the new field is not empty.
    # In the case of repeated, the content received in a parameter will be appended.

    # For this example, we'll used the SSID message
    # message Ssid {
    #     string identifier = 1;
    # }

    # First we'll create four of them, each with a unique string
    ssid_1 = Base_pb2.Ssid()
    ssid_1.identifier = ""

    ssid_2 = Base_pb2.Ssid()
    ssid_2.identifier = "123"

    ssid_3 = Base_pb2.Ssid()
    ssid_3.identifier = "@#$"

    # Now merge ssid_1 into ssid_2 and print the identifier of ssid_2
    ssid_2.MergeFrom(ssid_1)
    print("Content ssid_2: {0}".format(ssid_2.identifier))
    # output : Content ssid_2: 123

    # Now copy ssid_1 into ssid_3 and print the identifier of ssid_3
    ssid_3.CopyFrom(ssid_1)
    print("Content ssid_3: {0}".format(ssid_3.identifier))
    # output : Content ssid_3:

    # For more functions consult the Class Message documentation
    # https://developers.google.com/protocol-buffers/docs/reference/python/google.protobuf.message.Message-class


if __name__ == "__main__":
    # Example core
    # Basic manipulation
    example_manipulation_protobuf_basic()

    # Manipulating messages with nested messages
    example_manipulation_protobuf_object()

    # Manipulation messages containing lists
    example_manipulation_protobuf_list()

    # Using the google::protobuf helper functions
    example_manipulation_protobuf_helpers()
