// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// Low-Level Cyclic Control Example
//
// This example demonstrates real-time position hold at 1 kHz using the
// BaseCyclic interface.  It uses two separate connections to the robot:
//
//   1. MQTT (port 1883) - used for high-level RPCs such as switching the
//      servoing mode.
//   2. UDP  (port 10001) - used for the real-time Refresh() loop.
//
// Execution flow:
//   1. Connect via MQTT and open a session.
//   2. Connect via UDP and open a second session.
//   3. Switch to LOW_LEVEL_SERVOING mode via MQTT.
//   4. Snapshot the current joint positions via UDP feedback.
//   5. Hold those positions for NUM_CYCLES cycles at 1 kHz.
//   6. Switch back to SINGLE_LEVEL_SERVOING via MQTT.
//   7. Clean up both connections.

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include <RouterMQTT.h>
#include <TransportClientUdp.h>
#include <RouterClient.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <BaseCyclicClientRpc.h>
#include <KDetailedException.h>
#include <Base.pb.h>
#include <BaseCyclic.pb.h>
#include <Session.pb.h>

#include "../../utilities.h"

namespace k_api = Kinova::Api;
using namespace std::chrono;
using namespace std::chrono_literals;

// ── Cyclic loop parameters ────────────────────────────────────────────────────
static constexpr int CYCLE_US   = 1000;   // 1 ms period → 1 kHz
static constexpr int NUM_CYCLES = 1000;   // run for 1 second

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    const kortex::ConnectionOptions opts = kortex::ParseArgs(argc, argv);
    const std::string& robotIp = opts.ip;

    // ── 1. MQTT connection (mode changes) ────────────────────────────────────
    auto mqttRouter = std::make_shared<k_api::RouterMQTT>(
        robotIp, kortex::MQTT_PORT, "",
        [](k_api::KError err) { std::cerr << "MQTT router error: " << err.toString() << "\n"; });
    mqttRouter->SpinProcess();

    k_api::Session::SessionClient mqttSession(mqttRouter.get());
    mqttSession.CreateSession(kortex::BuildSessionInfo(opts));
    std::cout << "MQTT session established.\n";

    k_api::Base::BaseClient baseClient(mqttRouter.get());

    // ── 2. UDP connection (real-time cyclic loop) ─────────────────────────────
    // The UDP transport is allocated on the heap so we can control its
    // lifetime explicitly and clean it up after the cyclic loop.
    auto* udpTransport = new k_api::TransportClientUdp();
    udpTransport->connect(robotIp, kortex::UDP_PORT);

    auto* udpRouter = new k_api::RouterClient(
        udpTransport,
        [](k_api::KError err) { std::cerr << "UDP router error: " << err.toString() << "\n"; });

    auto* udpSessionClient = new k_api::Session::SessionClient(udpRouter);
    udpSessionClient->CreateSession(kortex::BuildSessionInfo(opts));
    std::cout << "UDP session established.\n";

    k_api::BaseCyclic::BaseCyclicClient baseCyclic(udpRouter);

    // ── 3. Switch to LOW_LEVEL_SERVOING ───────────────────────────────────────
    // Set HOLD_TO_RUN first as an intermediate operating mode (required on some
    // firmware versions before entering LOW_LEVEL_SERVOING).
    {
        k_api::Common::ModeSelection modeSelection;
        modeSelection.set_operating_mode(k_api::Common::OPERATING_MODE_HOLD_TO_RUN);
        baseClient.SelectOperatingMode(modeSelection);
        std::this_thread::sleep_for(500ms);

        k_api::Base::ServoingModeInformation servoingMode;
        servoingMode.set_servoing_mode(k_api::Base::LOW_LEVEL_SERVOING);
        baseClient.SetServoingMode(servoingMode);
        std::cout << "Switched to LOW_LEVEL_SERVOING.\n";
    }

    // ── 4. Snapshot initial joint positions ────────────────────────────────────
    auto feedback   = baseCyclic.RefreshFeedback();
    const int numJoints = feedback.actuators_size();

    std::vector<float> targetPos(numJoints);
    for (int i = 0; i < numJoints; ++i)
        targetPos[i] = feedback.actuators(i).position();

    std::cout << "Holding " << numJoints << " joints at current positions for "
              << NUM_CYCLES << " cycles...\n";

    // ── 5. Real-time cyclic loop at 1 kHz ──────────────────────────────────────
    for (int cycle = 0; cycle < NUM_CYCLES; ++cycle)
    {
        const auto cycleStart = high_resolution_clock::now();

        // Build the command: send the same target position every cycle.
        k_api::BaseCyclic::Command command;
        command.set_frame_id(cycle);

        for (int i = 0; i < numJoints; ++i)
        {
            auto* act = command.add_actuators();
            act->set_flags(0);
            act->set_position(targetPos[i]);
        }

        // Send command and receive feedback in one round-trip.
        try
        {
            feedback = baseCyclic.Refresh(command);
        }
        catch (k_api::KDetailedException& ex)
        {
            std::cerr << "Refresh error at cycle " << cycle << ": "
                      << ex.what() << "\n";
            break;
        }
        catch (std::exception& ex)
        {
            std::cerr << "Unexpected error at cycle " << cycle << ": "
                      << ex.what() << "\n";
            break;
        }

        // Pace the loop to maintain the desired cycle time.
        const auto elapsed = duration_cast<microseconds>(
            high_resolution_clock::now() - cycleStart).count();
        if (elapsed < CYCLE_US)
            std::this_thread::sleep_for(microseconds(CYCLE_US - elapsed));
    }

    std::cout << "Cyclic loop finished.\n";

    // ── 6. Return to SINGLE_LEVEL_SERVOING ────────────────────────────────────
    // Brief delay to let the last Refresh complete before mode change.
    std::this_thread::sleep_for(300ms);
    {
        k_api::Base::ServoingModeInformation servoingMode;
        servoingMode.set_servoing_mode(k_api::Base::SINGLE_LEVEL_SERVOING);
        baseClient.SetServoingMode(servoingMode);
        std::cout << "Returned to SINGLE_LEVEL_SERVOING.\n";
    }
    // Allow the mode change to propagate before closing sessions.
    std::this_thread::sleep_for(2000ms);

    // ── 7. Cleanup ────────────────────────────────────────────────────────────
    // UDP session and transport must be released in order.
    udpSessionClient->CloseSession();
    udpTransport->disconnect();
    delete udpSessionClient;
    delete udpRouter;
    delete udpTransport;
    std::cout << "UDP session closed.\n";

    // MQTT session.
    mqttSession.CloseSession();
    std::cout << "MQTT session closed.\n";

    return 0;
}
