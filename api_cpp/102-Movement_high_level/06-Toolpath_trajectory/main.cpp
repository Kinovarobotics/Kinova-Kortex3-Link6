// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example executes a toolpath trajectory composed of straight-segment
// and arc-point waypoints. The WaypointList oneof field is used to select
// the appropriate waypoint type for each entry before the list is validated
// and executed.

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <ProgramRunnerClientRpc.h>
#include <Base.pb.h>
#include <Common.pb.h>
#include <ProgramRunner.pb.h>
#include <Session.pb.h>

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <future>

#include "../../utilities.h"

// ---------------------------------------------------------------------------
// Waypoint type selector
// ---------------------------------------------------------------------------
enum class WaypointType { SEGMENT, ARC };

// ---------------------------------------------------------------------------
// Unified data struct for both toolpath waypoint types.
// Fields via_x/via_y/via_z and angular_acceleration are only used for ARC.
// ---------------------------------------------------------------------------
struct ToolpathWaypointData
{
    float x, y, z;
    float blending_radius;
    float theta_x, theta_y, theta_z;
    WaypointType type;
    float linear_speed;
    float linear_acceleration;
    // ARC-only fields
    float via_x = 0.0f;
    float via_y = 0.0f;
    float via_z = 0.0f;
    float angular_acceleration = 0.0f;
};

// ---------------------------------------------------------------------------
// Helper: switch the robot operating mode and wait for it to settle
// ---------------------------------------------------------------------------
void ChangeOperatingMode(Kinova::Api::Base::BaseClient&       base,
                         Kinova::Api::Common::OperatingModeType mode)
{
    Kinova::Api::Common::ModeSelection modeSelection;
    modeSelection.set_operating_mode(mode);
    base.SelectOperatingMode(modeSelection);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// ---------------------------------------------------------------------------
// Helper: run a stored program by name and wait for completion
// ---------------------------------------------------------------------------
bool RunProgram(Kinova::Api::Base::BaseClient&                   base,
                Kinova::Api::ProgramRunner::ProgramRunnerClient& programRunner,
                const std::string&                               programName)
{
    auto programs = programRunner.ReadAllPrograms();
    uint32_t programId = 0;
    bool found = false;
    for (const auto& prog : programs.programs())
    {
        if (prog.name() == programName)
        {
            programId = prog.handle().identifier();
            found = true;
            break;
        }
    }
    if (!found)
    {
        std::cerr << "Program '" << programName << "' not found\n";
        return false;
    }

    Kinova::Api::ProgramRunner::ProgramValidationConfiguration validation;
    validation.set_is_valid(true);
    validation.mutable_program_handle()->set_identifier(programId);
    programRunner.ValidateProgram(validation);

    std::promise<void> done;
    auto future = done.get_future();
    bool signaled = false;  // callbacks are delivered on a single router thread
    auto notification = programRunner.OnNotificationExecutionEventTopic(
        [&done, &signaled](Kinova::Api::ProgramRunner::ExecutionEventNotification notification)
        {
            if (notification.event() == Kinova::Api::ProgramRunner::EXECUTION_EVENT_STARTED)
                std::cout << "Program started\n";
            if (notification.event() == Kinova::Api::ProgramRunner::EXECUTION_EVENT_COMPLETED)
            {
                std::cout << "Program completed\n";
                if (!signaled) { signaled = true; done.set_value(); }
            }
        },
        Kinova::Api::Common::NotificationOptions{});

    Kinova::Api::ProgramRunner::ProgramStartConfiguration config;
    config.mutable_handle()->mutable_program_handle()->set_identifier(programId);
    programRunner.Start(config);
    future.wait();
    // Unsubscribe on the SAME client that created the subscription. Using a
    // different client (e.g. base) silently leaves the subscription active, so a
    // duplicate event would later fire this callback against a destroyed promise.
    programRunner.Unsubscribe(notification);
    return true;
}

// ---------------------------------------------------------------------------
// Populate a StraightSegmentToolpath from a ToolpathWaypointData
// ---------------------------------------------------------------------------
Kinova::Api::Base::StraightSegmentToolpath PopulateSegmentToolpath(
    const ToolpathWaypointData& wp)
{
    Kinova::Api::Base::StraightSegmentToolpath segment;
    segment.mutable_pose()->set_x(wp.x);
    segment.mutable_pose()->set_y(wp.y);
    segment.mutable_pose()->set_z(wp.z);
    segment.mutable_pose()->set_theta_x(wp.theta_x);
    segment.mutable_pose()->set_theta_y(wp.theta_y);
    segment.mutable_pose()->set_theta_z(wp.theta_z);
    segment.set_blending_radius(wp.blending_radius);
    segment.set_linear_speed(wp.linear_speed);
    segment.set_linear_acceleration(wp.linear_acceleration);
    return segment;
}

// ---------------------------------------------------------------------------
// Populate an ArcPointToolpath from a ToolpathWaypointData
// ---------------------------------------------------------------------------
Kinova::Api::Base::ArcPointToolpath PopulateArcToolpath(const ToolpathWaypointData& wp)
{
    Kinova::Api::Base::ArcPointToolpath arc;
    arc.mutable_pose()->set_x(wp.x);
    arc.mutable_pose()->set_y(wp.y);
    arc.mutable_pose()->set_z(wp.z);
    arc.mutable_pose()->set_theta_x(wp.theta_x);
    arc.mutable_pose()->set_theta_y(wp.theta_y);
    arc.mutable_pose()->set_theta_z(wp.theta_z);
    arc.set_blending_radius(wp.blending_radius);
    arc.set_linear_speed(wp.linear_speed);
    arc.set_linear_acceleration(wp.linear_acceleration);
    arc.set_via_point_x(wp.via_x);
    arc.set_via_point_y(wp.via_y);
    arc.set_via_point_z(wp.via_z);
    arc.set_angular_acceleration(wp.angular_acceleration);
    return arc;
}

// ---------------------------------------------------------------------------
// Build and execute a toolpath trajectory (segments and arcs)
// ---------------------------------------------------------------------------
void ExampleToolpathTrajectory(Kinova::Api::Base::BaseClient& base)
{
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_AUTO);

    // Waypoint definitions:
    //   fields: x, y, z, blending, theta_x, theta_y, theta_z, type,
    //           linear_speed, linear_acceleration[, via_x, via_y, via_z, angular_acc]
    const std::vector<ToolpathWaypointData> waypointsDefinition =
    {
        // Straight segment 1
        {
            0.646f,  0.158f, 0.397f, 0.0f,
            0.0f, 180.0f, 90.0f,
            WaypointType::SEGMENT,
            0.2f, 0.1f
        },
        // Straight segment 2
        {
            0.646f, -0.039f, 0.397f, 0.0f,
            0.0f, 180.0f, 90.0f,
            WaypointType::SEGMENT,
            0.2f, 0.1f
        },
        // Arc point: endpoint=(0.646, -0.39, 0.397), via=(0.776, -0.131, 0.397)
        {
            0.646f, -0.390f, 0.397f, 0.0f,
            0.0f, 180.0f, 70.0f,
            WaypointType::ARC,
            0.1f, 2.5f,
            0.776f, -0.131f, 0.397f, 25.0f
        },
    };

    Kinova::Api::Base::WaypointList wptList;
    wptList.set_use_optimal_blending(false);  // Toolpath uses explicit speeds

    for (std::size_t i = 0; i < waypointsDefinition.size(); ++i)
    {
        const auto& wpData = waypointsDefinition[i];
        auto* waypoint = wptList.add_waypoints();
        waypoint->set_name("waypoint_" + std::to_string(i));

        if (wpData.type == WaypointType::SEGMENT)
            *waypoint->mutable_straight_segment_toolpath() = PopulateSegmentToolpath(wpData);
        else
            *waypoint->mutable_arc_point_toolpath() = PopulateArcToolpath(wpData);
    }

    // Validate before execution
    auto result = base.ValidateWaypointList(wptList);
    if (result.trajectory_error_report().trajectory_error_elements_size() == 0)
    {
        std::cout << "Toolpath validated. Executing trajectory...\n";
        base.ExecuteWaypointTrajectory(wptList);
        std::cout << "Toolpath trajectory execution started.\n";
    }
    else
    {
        std::cerr << "Toolpath validation failed:\n"
                  << result.trajectory_error_report().DebugString() << "\n";
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    const kortex::ConnectionOptions opts = kortex::ParseArgs(argc, argv);
    const std::string& robotIp = opts.ip;

    auto router = std::make_shared<Kinova::Api::RouterMQTT>(
        robotIp, kortex::MQTT_PORT, "",
        [](Kinova::Api::KError err)
        {
            std::cerr << "Router error: " << err.toString() << "\n";
        });
    router->SpinProcess();

    auto sessionInfo = kortex::BuildSessionInfo(opts);

    auto sessionClient = std::make_unique<Kinova::Api::Session::SessionClient>(router.get());
    sessionClient->CreateSession(sessionInfo);

    Kinova::Api::Base::BaseClient                   base(router.get());
    Kinova::Api::ProgramRunner::ProgramRunnerClient programRunner(router.get());

    // Move to a known starting position
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_MONITORED_STOP);
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_AUTO);
    RunProgram(base, programRunner, "Home Position");

    ExampleToolpathTrajectory(base);

    sessionClient->CloseSession();
    return 0;
}
