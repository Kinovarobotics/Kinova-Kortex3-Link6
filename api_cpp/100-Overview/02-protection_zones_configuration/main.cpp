// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example demonstrates how to create and delete a rectangular-prism
// protection zone via the API, run a program to move into the zone so that
// the robot stops, then clean up the zone and retry the movement.

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <ProgramRunnerClientRpc.h>
#include <ProtectionZoneClientRpc.h>
#include <Base.pb.h>
#include <ProtectionZone.pb.h>
#include <ControlConfig.pb.h>
#include <Common.pb.h>
#include <ProgramRunner.pb.h>
#include <Session.pb.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <future>

#include "../../utilities.h"

// Protection zone geometry
static const float PROTECTION_ZONE_POS[3]        = { 0.4f, 0.0f, 0.4f };
static const float PROTECTION_ZONE_DIMENSIONS[3]  = { 0.05f, 0.3f, 0.4f };

// ---------------------------------------------------------------------------
// Helper: switch the robot operating mode and wait for it to settle
// ---------------------------------------------------------------------------
void ChangeOperatingMode(Kinova::Api::Base::BaseClient&    base,
                         Kinova::Api::Common::OperatingModeType mode)
{
    Kinova::Api::Common::ModeSelection modeSelection;
    modeSelection.set_operating_mode(mode);
    base.SelectOperatingMode(modeSelection);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// ---------------------------------------------------------------------------
// Helper: look up a program by name, validate it, run it and wait for
// completion using a std::promise/future pair.
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

    // Validate program
    Kinova::Api::ProgramRunner::ProgramValidationConfiguration validation;
    validation.set_is_valid(true);
    validation.mutable_program_handle()->set_identifier(programId);
    programRunner.ValidateProgram(validation);

    // Subscribe to execution events
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

    // Start the program
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
// Print all currently configured protection zones
// ---------------------------------------------------------------------------
void PrintProtectionZones(Kinova::Api::ProtectionZone::ProtectionZoneClient& protectZoneClient)
{
    auto allZones = protectZoneClient.ReadAllProtectionZones();
    std::cout << "=== Protection zones ===\n";
    for (const auto& zone : allZones.protection_zones())
    {
        std::cout << "  Name       : " << zone.name() << "\n"
                  << "  ID         : " << zone.handle().identifier() << "\n"
                  << "  Enabled    : " << (zone.is_enabled() ? "yes" : "no") << "\n"
                  << "  Origin (x,y,z): ("
                  << zone.shape().origin().x() << ", "
                  << zone.shape().origin().y() << ", "
                  << zone.shape().origin().z() << ")\n\n";
    }
}

// ---------------------------------------------------------------------------
// Create a rectangular-prism protection zone and return its handle
// ---------------------------------------------------------------------------
Kinova::Api::ProtectionZone::ProtectionZoneHandle CreateProtectionZone(
    Kinova::Api::Base::BaseClient&                   base,
    Kinova::Api::ProtectionZone::ProtectionZoneClient& protectZoneClient)
{
    // Deactivate the robot if it is not already idle
    if (base.GetArmState().active_state() != Kinova::Api::Common::ARMSTATE_IDLE)
    {
        std::cout << "Deactivating robot to configure protection zone...\n";
        base.DeactivateRobot();
        std::this_thread::sleep_for(std::chrono::seconds(9));
    }
    else
    {
        std::cout << "Robot is already in idle state.\n";
    }

    // Build zone origin
    Kinova::Api::ControlConfig::Position zoneOrigin;
    zoneOrigin.set_x(PROTECTION_ZONE_POS[0]);
    zoneOrigin.set_y(PROTECTION_ZONE_POS[1]);
    zoneOrigin.set_z(PROTECTION_ZONE_POS[2]);

    // Identity rotation matrix
    Kinova::Api::Common::RotationMatrixRow row1, row2, row3;
    row1.set_column1(1.0f); row1.set_column2(0.0f); row1.set_column3(0.0f);
    row2.set_column1(0.0f); row2.set_column2(1.0f); row2.set_column3(0.0f);
    row3.set_column1(0.0f); row3.set_column2(0.0f); row3.set_column3(1.0f);

    Kinova::Api::Common::RotationMatrix zoneOrientation;
    *zoneOrientation.mutable_row1() = row1;
    *zoneOrientation.mutable_row2() = row2;
    *zoneOrientation.mutable_row3() = row3;

    // Build zone shape (SHAPE_TYPE_RECTANGULAR_PRISM = 3)
    Kinova::Api::ProtectionZone::ZoneShape zoneShape;
    zoneShape.set_shape_type(
        static_cast<Kinova::Api::ProtectionZone::ShapeType>(3));
    *zoneShape.mutable_origin()      = zoneOrigin;
    *zoneShape.mutable_orientation() = zoneOrientation;
    zoneShape.add_dimensions(PROTECTION_ZONE_DIMENSIONS[0]);
    zoneShape.add_dimensions(PROTECTION_ZONE_DIMENSIONS[1]);
    zoneShape.add_dimensions(PROTECTION_ZONE_DIMENSIONS[2]);

    // Build zone config
    Kinova::Api::ProtectionZone::ProtectionZoneConfig zoneConfig;
    zoneConfig.set_name("Example Protection Zone");
    zoneConfig.set_is_enabled(true);
    *zoneConfig.mutable_shape() = zoneShape;

    auto handle = protectZoneClient.CreateProtectionZone(zoneConfig);
    std::cout << "Protection zone created (ID=" << handle.identifier() << ").\n";

    PrintProtectionZones(protectZoneClient);

    // Wait for zone to take effect, then reactivate
    std::this_thread::sleep_for(std::chrono::seconds(9));
    base.ActivateRobot();

    // Wait until arm finishes initialization
    while (base.GetArmState().active_state() == Kinova::Api::Common::ARMSTATE_INITIALIZATION)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Robot reactivated after protection zone creation.\n";
    return handle;
}

// ---------------------------------------------------------------------------
// Delete an existing protection zone identified by its handle
// ---------------------------------------------------------------------------
void DeleteProtectionZone(
    Kinova::Api::Base::BaseClient&                     base,
    Kinova::Api::ProtectionZone::ProtectionZoneClient& protectZoneClient,
    Kinova::Api::ProtectionZone::ProtectionZoneHandle& handle)
{
    if (base.GetArmState().active_state() != Kinova::Api::Common::ARMSTATE_IDLE)
    {
        std::cout << "Deactivating robot to remove protection zone...\n";
        base.DeactivateRobot();
        std::this_thread::sleep_for(std::chrono::seconds(9));
    }

    protectZoneClient.DeleteProtectionZone(handle);
    std::cout << "Protection zone deleted.\n";

    std::this_thread::sleep_for(std::chrono::seconds(9));
    base.ActivateRobot();

    while (base.GetArmState().active_state() == Kinova::Api::Common::ARMSTATE_INITIALIZATION)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Robot reactivated after protection zone removal.\n";
}

// ---------------------------------------------------------------------------
// Move the robot to the home position using a stored program
// ---------------------------------------------------------------------------
void MoveToHomePosition(Kinova::Api::Base::BaseClient&                   base,
                        Kinova::Api::ProgramRunner::ProgramRunnerClient& programRunner)
{
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_MONITORED_STOP);
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_AUTO);
    RunProgram(base, programRunner, "Home Position");
}

// ---------------------------------------------------------------------------
// Move the robot out of the protection zone using a manual twist command
// ---------------------------------------------------------------------------
void ExitProtectionZone(Kinova::Api::Base::BaseClient& base)
{
    // Pass through MONITORED_STOP before entering JOG_MANUAL. A direct switch
    // from AUTO -- especially after the robot was halted by the protection
    // zone -- is rejected as an invalid operating-mode transition (WRONG_MODE).
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_MONITORED_STOP);
    ChangeOperatingMode(base, Kinova::Api::Common::OPERATING_MODE_JOG_MANUAL);

    Kinova::Api::Base::TwistCommand command;
    command.set_reference_frame(Kinova::Api::Common::CARTESIAN_REFERENCE_FRAME_TOOL);
    command.set_duration(3);
    command.mutable_twist()->set_linear_x(0.0f);
    command.mutable_twist()->set_linear_y(0.0f);
    command.mutable_twist()->set_linear_z(-0.05f);  // Move down to exit zone
    command.mutable_twist()->set_angular_x(0.0f);
    command.mutable_twist()->set_angular_y(0.0f);
    command.mutable_twist()->set_angular_z(0.0f);

    base.SendTwistCommand(command);
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((command.duration() + 1.0f) * 1000)));
    std::cout << "Exited protection zone.\n";
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

    Kinova::Api::Base::BaseClient                       base(router.get());
    Kinova::Api::ProgramRunner::ProgramRunnerClient     programRunner(router.get());
    Kinova::Api::ProtectionZone::ProtectionZoneClient   protectZoneClient(router.get());

    // 1. Create a protection zone
    auto zoneHandle = CreateProtectionZone(base, protectZoneClient);

    // 2. Move the robot toward the protection zone (will be blocked)
    std::cout << "Attempting to move into protection zone...\n";
    MoveToHomePosition(base, programRunner);

    // 3. If robot stopped inside zone, nudge it out
    ExitProtectionZone(base);

    // 4. Delete the protection zone and retry the move
    DeleteProtectionZone(base, protectZoneClient, zoneHandle);
    std::cout << "Retrying home movement without protection zone...\n";
    MoveToHomePosition(base, programRunner);

    sessionClient->CloseSession();
    return 0;
}
