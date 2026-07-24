// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example demonstrates three ways to send Cartesian velocity (twist)
// commands to the robot:
//   1. ExecuteAction  - wraps a TwistCommand in an Action (HOLD_TO_RUN mode)
//   2. SendTwistCommand - direct one-shot command (JOG_MANUAL mode)
//   3. Streaming     - 40 Hz sinusoidal Z velocity loop (JOG_MANUAL mode)

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <ProgramRunnerClientRpc.h>
#include <Base.pb.h>
#include <Common.pb.h>
#include <ProgramRunner.pb.h>
#include <Session.pb.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <future>

#include "../../utilities.h"

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
    // later run would fire this stale callback against a destroyed promise.
    programRunner.Unsubscribe(notification);
    return true;
}

// ---------------------------------------------------------------------------
// Wrap a TwistCommand inside an Action and execute it (HOLD_TO_RUN mode)
// ---------------------------------------------------------------------------
void ExampleTwistAction(Kinova::Api::Base::BaseClient& base)
{
    Kinova::Api::Base::TwistCommand command;
    command.set_reference_frame(Kinova::Api::Common::CARTESIAN_REFERENCE_FRAME_TOOL);
    command.set_duration(5);
    command.mutable_twist()->set_linear_x(0.01f);
    command.mutable_twist()->set_linear_y(0.01f);
    command.mutable_twist()->set_linear_z(0.01f);
    command.mutable_twist()->set_angular_x(0.0f);
    command.mutable_twist()->set_angular_y(3.0f);
    command.mutable_twist()->set_angular_z(0.0f);

    Kinova::Api::Base::Action action;
    action.set_name("Twist_Action");
    action.mutable_handle()->set_identifier(100);
    *action.mutable_send_twist_command() = command;

    // Transitions into an active control mode must pass through MONITORED_STOP;
    // switching directly from AUTO to HOLD_TO_RUN is rejected with WRONG_MODE.
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_MONITORED_STOP);
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_HOLD_TO_RUN);
    base.ExecuteAction(action);
    std::cout << "Twist action sent.\n";
    // Allow the action to run for its declared duration
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((command.duration() + 1.0f) * 1000)));
}

// ---------------------------------------------------------------------------
// Send a single TwistCommand directly (JOG_MANUAL mode)
// ---------------------------------------------------------------------------
void ExampleTwistCommand(Kinova::Api::Base::BaseClient& base)
{
    Kinova::Api::Base::TwistCommand command;
    command.set_reference_frame(Kinova::Api::Common::CARTESIAN_REFERENCE_FRAME_TOOL);
    command.set_duration(2);
    command.mutable_twist()->set_linear_x(0.05f);
    command.mutable_twist()->set_linear_y(0.0f);
    command.mutable_twist()->set_linear_z(0.0f);
    command.mutable_twist()->set_angular_x(0.0f);
    command.mutable_twist()->set_angular_y(0.0f);
    command.mutable_twist()->set_angular_z(5.0f);

    // Pass through MONITORED_STOP before entering JOG_MANUAL to avoid an
    // invalid direct transition (WRONG_MODE) from the previous mode.
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_MONITORED_STOP);
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_JOG_MANUAL);
    base.SendTwistCommand(command);
    std::cout << "Twist command sent.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((command.duration() + 1.0f) * 1000)));
}

// ---------------------------------------------------------------------------
// Stream twist commands at 40 Hz with a sinusoidal Z velocity
// ---------------------------------------------------------------------------
void ExampleTwistStream(Kinova::Api::Base::BaseClient& base, int durationSeconds)
{
    // Pass through MONITORED_STOP before entering JOG_MANUAL to avoid an
    // invalid direct transition (WRONG_MODE) from the previous mode.
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_MONITORED_STOP);
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_JOG_MANUAL);

    const int totalSteps = durationSeconds * 40;  // 40 Hz
    const float amplitude = 0.05f;                 // m/s peak
    const float omega     = 2.0f * static_cast<float>(M_PI) * 0.5f; // 0.5 Hz sine wave

    std::cout << "Streaming sinusoidal Z velocity for " << durationSeconds << " s...\n";

    for (int step = 0; step < totalSteps; ++step)
    {
        const float t = static_cast<float>(step) / 40.0f;

        Kinova::Api::Base::TwistCommand command;
        command.set_reference_frame(Kinova::Api::Common::CARTESIAN_REFERENCE_FRAME_TOOL);
        command.set_duration(0);  // Streaming: no fixed duration
        command.mutable_twist()->set_linear_x(0.0f);
        command.mutable_twist()->set_linear_y(0.0f);
        command.mutable_twist()->set_linear_z(amplitude * std::sin(omega * t));
        command.mutable_twist()->set_angular_x(0.0f);
        command.mutable_twist()->set_angular_y(0.0f);
        command.mutable_twist()->set_angular_z(0.0f);

        base.SendTwistCommand(command);
        std::this_thread::sleep_for(std::chrono::milliseconds(25)); // 40 Hz
    }

    // Send a zero-velocity command to stop the robot
    Kinova::Api::Base::TwistCommand stop;
    stop.set_reference_frame(Kinova::Api::Common::CARTESIAN_REFERENCE_FRAME_TOOL);
    stop.set_duration(0);
    base.SendTwistCommand(stop);
    std::cout << "Twist stream complete.\n";
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

    // 1. Direct twist command
    ExampleTwistCommand(base);

    // 2. Return home before the next demo
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_MONITORED_STOP);
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_AUTO);
    RunProgram(base, programRunner, "Home Position");

    // 3. Twist action via ExecuteAction
    ExampleTwistAction(base);

    // 4. Sinusoidal streaming demo (5 seconds)
    ExampleTwistStream(base, 5);

    sessionClient->CloseSession();
    return 0;
}
