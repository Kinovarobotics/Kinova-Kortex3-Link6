// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

#include <iostream>
#include <iomanip>

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <KDetailedException.h>
#include <Base.pb.h>
#include <Session.pb.h>

#include "../../utilities.h"

namespace k_api = Kinova::Api;

// ----------------------------------------------------------------------------
// ExampleForwardKinematics
//
// Reads the current joint angles from the robot and computes the corresponding
// Cartesian pose using forward kinematics.
// ----------------------------------------------------------------------------
bool ExampleForwardKinematics(k_api::Base::BaseClient& base)
{
    k_api::Base::JointAngles inputJointAngles;
    try
    {
        inputJointAngles = base.GetMeasuredJointAngles();
    }
    catch (k_api::KDetailedException& ex)
    {
        std::cerr << "Error getting joint angles.\n"
                  << "Error code: " << ex.getErrorInfo().getError().error_code() << "\n"
                  << "Error sub-code: " << ex.getErrorInfo().getError().error_sub_code() << "\n";
        return false;
    }

    std::cout << "Current joint angles:\n";
    for (const auto& joint : inputJointAngles.joint_angles())
    {
        std::cout << "  Joint " << joint.joint_identifier()
                  << " : " << std::fixed << std::setprecision(4) << joint.value() << " deg\n";
    }

    k_api::Base::Pose pose;
    try
    {
        pose = base.ComputeForwardKinematics(inputJointAngles);
    }
    catch (k_api::KDetailedException& ex)
    {
        std::cerr << "Error computing forward kinematics.\n"
                  << "Error code: " << ex.getErrorInfo().getError().error_code() << "\n"
                  << "Error sub-code: " << ex.getErrorInfo().getError().error_sub_code() << "\n";
        return false;
    }

    std::cout << "\nForward kinematics result:\n"
              << "  Position : x=" << pose.x() << "  y=" << pose.y() << "  z=" << pose.z() << "\n"
              << "  Theta    : theta_x=" << pose.theta_x()
              << "  theta_y=" << pose.theta_y()
              << "  theta_z=" << pose.theta_z() << "\n";

    return true;
}

// ----------------------------------------------------------------------------
// ExampleInverseKinematics
//
// Reads the current pose and joint angles, then computes the inverse kinematics
// using the current joint angles (offset by -1 deg) as the initial guess.
// ----------------------------------------------------------------------------
bool ExampleInverseKinematics(k_api::Base::BaseClient& base)
{
    k_api::Base::JointAngles inputJointAngles;
    k_api::Base::Pose        currentPose;

    try
    {
        inputJointAngles = base.GetMeasuredJointAngles();
        currentPose      = base.GetMeasuredCartesianPose();
    }
    catch (k_api::KDetailedException& ex)
    {
        std::cerr << "Error reading robot state.\n"
                  << "Error code: " << ex.getErrorInfo().getError().error_code() << "\n"
                  << "Error sub-code: " << ex.getErrorInfo().getError().error_sub_code() << "\n";
        return false;
    }

    // Build IKData from the current Cartesian pose and use the measured joint
    // angles (shifted slightly) as the solver's initial guess.
    k_api::Base::IKData ikData;

    ikData.mutable_cartesian_pose()->set_x(currentPose.x());
    ikData.mutable_cartesian_pose()->set_y(currentPose.y());
    ikData.mutable_cartesian_pose()->set_z(currentPose.z());
    ikData.mutable_cartesian_pose()->set_theta_x(currentPose.theta_x());
    ikData.mutable_cartesian_pose()->set_theta_y(currentPose.theta_y());
    ikData.mutable_cartesian_pose()->set_theta_z(currentPose.theta_z());

    for (const auto& joint : inputJointAngles.joint_angles())
    {
        auto* guess = ikData.mutable_guess()->add_joint_angles();
        guess->set_value(joint.value() - 1.0f);  // slight offset as initial guess
    }

    k_api::Base::JointAngles computed;
    try
    {
        computed = base.ComputeInverseKinematics(ikData);
    }
    catch (k_api::KDetailedException& ex)
    {
        std::cerr << "Error computing inverse kinematics.\n"
                  << "Error code: " << ex.getErrorInfo().getError().error_code() << "\n"
                  << "Error sub-code: " << ex.getErrorInfo().getError().error_sub_code() << "\n";
        return false;
    }

    std::cout << "\nInverse kinematics result:\n";
    int idx = 0;
    for (const auto& ja : computed.joint_angles())
    {
        std::cout << "  Joint " << idx++ << " : "
                  << std::fixed << std::setprecision(4) << ja.value() << " deg\n";
    }

    return true;
}

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    const kortex::ConnectionOptions opts = kortex::ParseArgs(argc, argv);
    const std::string& robotIp = opts.ip;

    // ── MQTT connection ──────────────────────────────────────────────────────
    auto router = std::make_shared<k_api::RouterMQTT>(
        robotIp, kortex::MQTT_PORT, "",
        [](k_api::KError err) { std::cerr << "Router error: " << err.toString() << "\n"; });
    router->SpinProcess();

    // ── Session ──────────────────────────────────────────────────────────────
    auto sessionInfo = kortex::BuildSessionInfo(opts);

    auto sessionClient = std::make_unique<k_api::Session::SessionClient>(router.get());
    sessionClient->CreateSession(sessionInfo);

    // ── Example logic ────────────────────────────────────────────────────────
    k_api::Base::BaseClient base(router.get());

    std::cout << "=== Forward Kinematics ===\n";
    ExampleForwardKinematics(base);

    std::cout << "\n=== Inverse Kinematics ===\n";
    ExampleInverseKinematics(base);

    // ── Cleanup ──────────────────────────────────────────────────────────────
    sessionClient->CloseSession();
    return 0;
}
