// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example shows how to establish a session with the robot via the MQTT
// connection and how to explicitly close it.  Session management is handled
// through the SessionClient and is a prerequisite for all other API calls.

#include <iostream>

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <Session.pb.h>

#include "../../utilities.h"

namespace k_api = Kinova::Api;

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    const kortex::ConnectionOptions opts = kortex::ParseArgs(argc, argv);
    const std::string& robotIp = opts.ip;

    // ── MQTT transport ───────────────────────────────────────────────────────
    // RouterMQTT manages the underlying MQTT connection.  SpinProcess() starts
    // the background thread that processes incoming messages.
    auto router = std::make_shared<k_api::RouterMQTT>(
        robotIp, kortex::MQTT_PORT, "",
        [](k_api::KError err) { std::cerr << "Router error: " << err.toString() << "\n"; });
    router->SpinProcess();

    // ── Session parameters ───────────────────────────────────────────────────
    // session_inactivity_timeout  : how long (ms) the robot waits before
    //                               automatically closing an idle session.
    // connection_inactivity_timeout : how long (ms) the API waits for a
    //                                 response before considering the
    //                                 connection lost.
    auto sessionInfo = kortex::BuildSessionInfo(opts);

    // ── Open session ─────────────────────────────────────────────────────────
    auto sessionClient = std::make_unique<k_api::Session::SessionClient>(router.get());
    sessionClient->CreateSession(sessionInfo);
    std::cout << "Session established.\n";

    // ── (place additional API calls here) ────────────────────────────────────

    // ── Close session ─────────────────────────────────────────────────────────
    // Always close the session explicitly so the robot can free its resources
    // immediately rather than waiting for the inactivity timeout.
    std::cout << "Closing session...\n";
    sessionClient->CloseSession();
    std::cout << "Session closed.\n";

    return 0;
}
