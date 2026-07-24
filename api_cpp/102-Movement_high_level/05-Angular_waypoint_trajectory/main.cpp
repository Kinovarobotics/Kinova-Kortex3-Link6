// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example executes an angular (joint-space) waypoint trajectory.
// Three waypoints are defined by joint angles (degrees), validated, and
// submitted for execution using optimal blending.

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <ProgramRunnerClientRpc.h>
#include <Base.pb.h>
#include <Common.pb.h>
#include <ProgramRunner.pb.h>
#include <Session.pb.h>

#include <iostream>
#include <array>
#include <vector>
#include <thread>
#include <chrono>
#include <future>

#include "../../utilities.h"

// Number of joints on the arm.
static const int kJointCount = 6;

// First-run safety: cap every joint's speed low so you can watch the motion,
// then raise this once the trajectory looks correct.
static const float kMaxJointVelocity = 15.0f;   // deg/s, per joint

// ---------------------------------------------------------------------------
// Plain data type for a single angular waypoint definition
// ---------------------------------------------------------------------------
struct AngularWaypointData
{
    std::array<float, 6> angles;  // Joint angles in degrees
    float blending;               // Blending radius in degrees
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
    auto notification = programRunner.OnNotificationExecutionEventTopic(
        [&done](Kinova::Api::ProgramRunner::ExecutionEventNotification notification)
        {
            if (notification.event() == Kinova::Api::ProgramRunner::EXECUTION_EVENT_STARTED)
                std::cout << "Program started\n";
            if (notification.event() == Kinova::Api::ProgramRunner::EXECUTION_EVENT_COMPLETED)
            {
                std::cout << "Program completed\n";
                done.set_value();
            }
        },
        Kinova::Api::Common::NotificationOptions{});

    Kinova::Api::ProgramRunner::ProgramStartConfiguration config;
    config.mutable_handle()->mutable_program_handle()->set_identifier(programId);
    programRunner.Start(config);
    future.wait();
    base.Unsubscribe(notification);
    return true;
}

// ---------------------------------------------------------------------------
// Build an AngularWaypoint protobuf message from an AngularWaypointData struct
// ---------------------------------------------------------------------------
Kinova::Api::Base::AngularWaypoint PopulateAngularWaypoint(const AngularWaypointData& wp)
{
    Kinova::Api::Base::AngularWaypoint waypoint;
    for (float angle : wp.angles)
        waypoint.add_angles(angle);
    // Per-joint speed cap (one entry per joint) for a slow, observable first run.
    for (int j = 0; j < kJointCount; ++j)
        waypoint.add_maximum_velocities(kMaxJointVelocity);
    waypoint.set_blending(wp.blending);
    return waypoint;
}

// ---------------------------------------------------------------------------
// Build and execute an angular waypoint trajectory
// ---------------------------------------------------------------------------
void ExampleAngularTrajectory(Kinova::Api::Base::BaseClient& base)
{
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_AUTO);

    // Joint angles in degrees per waypoint. The sequence starts and ends at a
    // known-reachable configuration and sweeps several joints through a wide
    // range in between so the resulting motion is clearly visible (the previous
    // set moved only a few degrees). ValidateWaypointList below rejects any
    // configuration that would exceed the joint limits.
    const std::vector<AngularWaypointData> waypointsDefinition =
    {
        { {  40.0f, -22.0f,  75.0f, 0.0f,  10.0f,  20.0f }, 1.0f },  // start config
        { {  90.0f, -45.0f,  95.0f, 0.0f,  35.0f,  60.0f }, 1.0f },  // swing base, open elbow
        { {   0.0f,  10.0f,  55.0f, 0.0f, -25.0f, -30.0f }, 1.0f },  // swing back and lift
        { { -50.0f, -35.0f, 100.0f, 0.0f,  45.0f, -50.0f }, 1.0f },  // opposite side
        { {  40.0f,  15.0f,  50.0f, 0.0f, -15.0f,  30.0f }, 1.0f },  // forward reach
        { {  40.0f, -22.0f,  75.0f, 0.0f,  10.0f,  20.0f }, 0.0f },  // return to start config
    };

    Kinova::Api::Base::WaypointList wptList;
    wptList.set_use_optimal_blending(true);

    for (std::size_t i = 0; i < waypointsDefinition.size(); ++i)
    {
        auto* waypoint = wptList.add_waypoints();
        waypoint->set_name("waypoint_" + std::to_string(i));
        *waypoint->mutable_angular_waypoint() =
            PopulateAngularWaypoint(waypointsDefinition[i]);
    }

    // Validate before execution
    auto result = base.ValidateWaypointList(wptList);
    if (result.trajectory_error_report().trajectory_error_elements_size() == 0)
    {
        std::cout << "Waypoint list validated. Executing angular trajectory...\n";
        base.ExecuteWaypointTrajectory(wptList);
        std::cout << "Angular trajectory execution started.\n";
    }
    else
    {
        std::cerr << "Trajectory validation failed:\n"
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

    ExampleAngularTrajectory(base);

    sessionClient->CloseSession();
    return 0;
}
