// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example shows how to command joint velocities directly using
// SendJointSpeedsCommand. The robot alternates between two speed patterns
// four times, then comes to a stop.

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
#include <future>

#include "../../utilities.h"

// Joint speed magnitude in degrees/second
static const float SPEED = 20.0f;

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
// Send alternating joint speed patterns four times, then stop
// ---------------------------------------------------------------------------
void ExampleSendJointSpeeds(Kinova::Api::Base::BaseClient& base)
{
    // Pass through MONITORED_STOP before entering JOG_MANUAL; switching directly
    // from AUTO is rejected with WRONG_MODE.
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_MONITORED_STOP);
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_JOG_MANUAL);

    // Two alternating speed patterns for 6 joints:
    //   Pattern A (even iterations): joints 0 and 3 move in opposite directions
    //   Pattern B (odd  iterations): joints 0 and 3 swap directions
    const float patternA[6] = {  SPEED, 0.0f, 0.0f, -SPEED, 0.0f, 0.0f };
    const float patternB[6] = { -SPEED, 0.0f, 0.0f,  SPEED, 0.0f, 0.0f };

    for (int iteration = 0; iteration < 4; ++iteration)
    {
        const float* speeds = (iteration % 2 == 0) ? patternA : patternB;

        Kinova::Api::Base::JointSpeeds jointSpeeds;
        for (int joint = 0; joint < 6; ++joint)
        {
            auto* js = jointSpeeds.add_joint_speeds();
            js->set_joint_identifier(static_cast<uint32_t>(joint));
            js->set_value(speeds[joint]);
        }

        std::cout << "Iteration " << iteration
                  << ": sending joint speeds (pattern "
                  << (iteration % 2 == 0 ? 'A' : 'B') << ")\n";

        base.SendJointSpeedsCommand(jointSpeeds);
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    }

    base.Stop();
    std::cout << "Joint speeds example complete.\n";
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

    ExampleSendJointSpeeds(base);

    sessionClient->CloseSession();
    return 0;
}
