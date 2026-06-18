// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

//
// 01-api_creation
//
// Demonstrates how to establish an MQTT session with the robot and instantiate
// the two most commonly used API clients:
//   - BaseClient       : high-level robot control and configuration
//   - DeviceConfigClient : per-device configuration (firmware versions, etc.)
//

#include <iostream>
#include <memory>
#include <string>

#include <KDetailedException.h>
#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <DeviceConfigClientRpc.h>
#include <Session.pb.h>

#include "../../utilities.h"

int main(int argc, char* argv[])
{
    // Use the IP address supplied on the command line, or fall back to the default.
    const kortex::ConnectionOptions opts = kortex::ParseArgs(argc, argv);
    const std::string& robotIp = opts.ip;

    // Create the MQTT router.  The error callback fires on transport-level errors.
    auto router = std::make_shared<Kinova::Api::RouterMQTT>(
        robotIp, kortex::MQTT_PORT, "",
        [](Kinova::Api::KError err) {
            std::cerr << "Router error: " << err.toString() << "\n";
        });

    // Start the router's internal processing thread.
    router->SpinProcess();

    // Build session credentials.
    auto sessionInfo = kortex::BuildSessionInfo(opts);

    // Open the session on the robot.
    auto sessionClient = std::make_unique<Kinova::Api::Session::SessionClient>(router.get());
    sessionClient->CreateSession(sessionInfo);

    std::cout << "Session created\n";

    // BaseClient provides access to high-level robot operations: motion, programs,
    // user management, safety limits, etc.
    Kinova::Api::Base::BaseClient base(router.get());

    // DeviceConfigClient gives access to per-device configuration such as firmware
    // version queries, device identification, and network settings.
    Kinova::Api::DeviceConfig::DeviceConfigClient configClient(router.get());

    // --- example logic would go here ---

    // Gracefully close the session before exiting.
    sessionClient->CloseSession();
    return 0;
}
