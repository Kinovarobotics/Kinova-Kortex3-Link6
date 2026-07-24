// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example executes a Cartesian waypoint trajectory. Seven waypoints are
// defined in the base reference frame and submitted as a WaypointList with
// optimal blending enabled. The list is validated before execution.

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

// First-run safety: cap the Cartesian speed low so you can watch the path, then
// raise these once the trajectory looks correct (arm max is ~0.5 m/s).
static const float kMaxLinearVelocity  = 0.05f;   // m/s
static const float kMaxAngularVelocity = 15.0f;   // deg/s

// ---------------------------------------------------------------------------
// A single waypoint expressed as a translation offset (in meters) from the
// starting pose. Orientation is inherited from the starting pose, so the whole
// trajectory stays reachable from wherever the robot homed.
// ---------------------------------------------------------------------------
struct WaypointOffset
{
    float dx, dy, dz;
    float blending;
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
// Build a CartesianWaypoint protobuf message by applying a translation offset
// to the starting pose. The starting orientation is carried over unchanged.
// ---------------------------------------------------------------------------
Kinova::Api::Base::CartesianWaypoint PopulateCartesianWaypoint(
    const Kinova::Api::Base::Pose& start, const WaypointOffset& off)
{
    Kinova::Api::Base::CartesianWaypoint waypoint;
    waypoint.mutable_pose()->set_x(start.x() + off.dx);
    waypoint.mutable_pose()->set_y(start.y() + off.dy);
    waypoint.mutable_pose()->set_z(start.z() + off.dz);
    waypoint.mutable_pose()->set_theta_x(start.theta_x());
    waypoint.mutable_pose()->set_theta_y(start.theta_y());
    waypoint.mutable_pose()->set_theta_z(start.theta_z());
    waypoint.set_blending_radius(off.blending);
    waypoint.set_maximum_linear_velocity(kMaxLinearVelocity);
    waypoint.set_maximum_angular_velocity(kMaxAngularVelocity);
    waypoint.set_reference_frame(Kinova::Api::Common::CARTESIAN_REFERENCE_FRAME_BASE);
    return waypoint;
}

// ---------------------------------------------------------------------------
// Build and execute a Cartesian waypoint trajectory
// ---------------------------------------------------------------------------
void ExampleCartesianTrajectory(Kinova::Api::Base::BaseClient& base)
{
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_AUTO);

    // Anchor the trajectory to the pose the robot is currently at (the
    // "Home Position" it was just moved to). Expressing each waypoint as a small
    // translation offset from this measured pose -- instead of hard-coding
    // absolute base-frame coordinates -- keeps the whole trajectory reachable
    // regardless of how the home position is configured.
    const Kinova::Api::Base::Pose start = base.GetMeasuredCartesianPose();
    std::cout << "Home pose (base frame): x=" << start.x()
              << " y=" << start.y() << " z=" << start.z() << " m\n";

    // Translation offsets (meters) relative to the home pose. Kept small so the
    // motion stays comfortably inside the workspace; ValidateWaypointList below
    // will still reject anything unreachable.
    const std::vector<WaypointOffset> waypointsDefinition =
    {
        {  0.10f,  0.00f,  0.00f, 0.0f },
        {  0.10f,  0.10f,  0.10f, 0.0f },
        {  0.10f, -0.10f,  0.10f, 0.0f },
        {  0.15f,  0.00f, -0.05f, 0.0f },
        {  0.05f,  0.10f,  0.05f, 0.0f },
        {  0.05f, -0.10f,  0.05f, 0.0f },
        {  0.00f,  0.00f,  0.00f, 0.0f },  // return to the home pose
    };

    Kinova::Api::Base::WaypointList wptList;
    wptList.set_use_optimal_blending(true);

    for (std::size_t i = 0; i < waypointsDefinition.size(); ++i)
    {
        auto* waypoint = wptList.add_waypoints();
        waypoint->set_name("waypoint_" + std::to_string(i));
        *waypoint->mutable_cartesian_waypoint() =
            PopulateCartesianWaypoint(start, waypointsDefinition[i]);
    }

    // Validate before execution
    auto result = base.ValidateWaypointList(wptList);
    if (result.trajectory_error_report().trajectory_error_elements_size() == 0)
    {
        std::cout << "Waypoint list validated. Executing trajectory...\n";
        base.ExecuteWaypointTrajectory(wptList);
        std::cout << "Cartesian trajectory execution started.\n";
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

    ExampleCartesianTrajectory(base);

    sessionClient->CloseSession();
    return 0;
}
