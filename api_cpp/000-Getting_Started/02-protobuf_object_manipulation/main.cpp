// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

//
// 02-protobuf_object_manipulation
//
// Demonstrates how to create and manipulate Protocol Buffer (protobuf) messages
// used by the Kortex API.  This example does NOT connect to a robot — all
// operations are purely in-process protobuf manipulation.
//
// Topics covered:
//   1. Basic scalar field manipulation
//   2. Nested message construction
//   3. Enum field usage
//   4. Repeated (list) fields
//   5. CopyFrom / MergeFrom semantics
//

#include <iostream>
#include <string>

#include <Base.pb.h>

// ---------------------------------------------------------------------------
// 1. Basic scalar manipulation
//
// Protobuf scalar fields are set with generated set_<field>() methods and
// read with plain <field>() accessors.  ShortDebugString() serialises the
// message to a human-readable one-liner (useful for logging / debugging).
// ---------------------------------------------------------------------------
void ExampleBasicScalarManipulation()
{
    std::cout << "\n--- Basic scalar manipulation ---\n";

    Kinova::Api::Base::UserProfile userProfile;
    userProfile.set_username("jcash");
    userProfile.set_firstname("Johnny");
    userProfile.set_lastname("Cash");

    std::cout << "UserProfile: " << userProfile.ShortDebugString() << "\n";
}

// ---------------------------------------------------------------------------
// 2. Nested message construction
//
// When a message contains another message as a field, you access the nested
// message through its mutable_<field>() accessor, which returns a pointer to
// the embedded sub-message that you can then populate normally.
// ---------------------------------------------------------------------------
void ExampleNestedMessage()
{
    std::cout << "\n--- Nested message ---\n";

    Kinova::Api::Base::FullUserProfile fullUserProfile;
    fullUserProfile.set_password("MyPassword");

    // mutable_user_profile() returns a pointer to the embedded UserProfile.
    Kinova::Api::Base::UserProfile* up = fullUserProfile.mutable_user_profile();
    up->set_username("jcash");
    up->set_firstname("Johnny");
    up->set_lastname("Cash");

    std::cout << "FullUserProfile: " << fullUserProfile.ShortDebugString() << "\n";
}

// ---------------------------------------------------------------------------
// 3. Enum field usage
//
// Protobuf enums are plain C++ enums.  Assign them directly to the field
// using set_<field>().  The generated DebugString() shows the symbolic name.
// ---------------------------------------------------------------------------
void ExampleEnumUsage()
{
    std::cout << "\n--- Enum usage ---\n";

    Kinova::Api::Base::CartesianLimitEvent cartesianLimit;

    // Assign the enum value directly — no string look-up needed in C++.
    cartesianLimit.set_event(
        Kinova::Api::Base::UNSPECIFIED_CARTESIAN_LIMIT_EVENT);
    cartesianLimit.set_cartesian_limit(1.0f);

    std::cout << "CartesianLimitEvent: " << cartesianLimit.ShortDebugString() << "\n";
}

// ---------------------------------------------------------------------------
// 4. Repeated (list) fields
//
// Repeated fields are managed through add_<field>() (which appends a new
// element and returns a pointer to it) and <field>_size() / <field>(i)
// accessors.  SerializeToString() produces the compact binary wire format.
// ---------------------------------------------------------------------------
void ExampleRepeatedField()
{
    std::cout << "\n--- Repeated field ---\n";

    Kinova::Api::Base::JointAngles jointAngles;

    for (int i = 0; i < 6; ++i)
    {
        // add_joint_angles() appends a new JointAngle entry in-place.
        Kinova::Api::Base::JointAngle* ja = jointAngles.add_joint_angles();
        ja->set_joint_identifier(i);
        ja->set_value(45.0f);

        std::cout << "Joint " << (ja->joint_identifier() + 1)
                  << " -> " << ja->value() << " deg\n";

        // Binary serialisation: compact wire format suitable for storage or
        // transmission.  DebugString() gives the human-readable alternative.
        std::string serialised;
        ja->SerializeToString(&serialised);
        std::cout << "  serialised size: " << serialised.size() << " bytes\n";
    }
}

// ---------------------------------------------------------------------------
// 5. CopyFrom / MergeFrom semantics
//
// CopyFrom replaces ALL fields in the destination with those from the source,
// including clearing fields that are absent in the source.
//
// MergeFrom only overwrites fields that are explicitly set (non-default) in
// the source; fields absent in the source are left untouched in the destination.
// ---------------------------------------------------------------------------
void ExampleCopyMerge()
{
    std::cout << "\n--- CopyFrom / MergeFrom ---\n";

    Kinova::Api::Base::Ssid ssid1;
    ssid1.set_identifier("");   // explicitly set to empty string

    Kinova::Api::Base::Ssid ssid2;
    ssid2.set_identifier("123");

    Kinova::Api::Base::Ssid ssid3;
    ssid3.set_identifier("@#$");

    // MergeFrom: ssid1 has identifier "" which is the default value for a
    // string field, so it is considered "not set" and ssid2 keeps "123".
    ssid2.MergeFrom(ssid1);
    std::cout << "After MergeFrom (ssid2 <- ssid1): "
              << ssid2.ShortDebugString() << "\n";  // still "123"

    // CopyFrom: unconditionally replaces ssid3 with the full content of ssid1.
    ssid3.CopyFrom(ssid1);
    std::cout << "After CopyFrom  (ssid3 <- ssid1): "
              << ssid3.ShortDebugString() << "\n";  // now ""
}

// ---------------------------------------------------------------------------
// Entry point — run all four demonstrations in sequence.
// ---------------------------------------------------------------------------
int main(int /*argc*/, char* /*argv*/[])
{
    ExampleBasicScalarManipulation();
    ExampleNestedMessage();
    ExampleEnumUsage();
    ExampleRepeatedField();
    ExampleCopyMerge();

    std::cout << "\nAll protobuf manipulation examples completed.\n";
    return 0;
}
