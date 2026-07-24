// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

//
// utilities.h
//
// Connection parameters shared by every C++ example in this repository.
//
// Edit the values below once to point all examples at your robot and to change
// the credentials/timeouts used for the control session. Each example includes
// this header (via "../../utilities.h") instead of defining its own copy.
//

#pragma once

#include <string>

#include <Session.pb.h>

namespace kortex
{
// ── Shared connection parameters ─────────────────────────────────────────────
// Change these once here and every example picks up the new values.
constexpr char DEFAULT_IP[] = "192.168.1.10";
constexpr int  MQTT_PORT    = 1883;   // high-level RPCs
constexpr int  UDP_PORT     = 10001;  // real-time cyclic control (low level)

// ── Shared control-session parameters ────────────────────────────────────────
constexpr char USERNAME[] = "admin";
constexpr char PASSWORD[] = "admin";
// session_inactivity_timeout    : how long (ms) the robot waits before
//                                 automatically closing an idle session.
// connection_inactivity_timeout : how long (ms) the API waits for a response
//                                 before considering the connection lost.
constexpr int  SESSION_INACTIVITY_TIMEOUT_MS    = 60000;
constexpr int  CONNECTION_INACTIVITY_TIMEOUT_MS = 2000;

// Connection settings resolved for a run: the shared defaults above, overridden
// by any command-line arguments (see ParseArgs).
struct ConnectionOptions
{
    std::string ip       = DEFAULT_IP;
    std::string username = USERNAME;
    std::string password = PASSWORD;
};

// Resolve connection options from the command line, falling back to the shared
// defaults:
//   argv[1] -> robot IP   (optional)
//   argv[2] -> username   (optional)
//   argv[3] -> password   (optional)
inline ConnectionOptions ParseArgs(int argc, char* argv[])
{
    ConnectionOptions opts;
    if (argc > 1) opts.ip       = argv[1];
    if (argc > 2) opts.username = argv[2];
    if (argc > 3) opts.password = argv[3];
    return opts;
}

// Build a CreateSessionInfo from the shared session parameters and the resolved
// credentials. This is the session configuration common to every example.
inline Kinova::Api::Session::CreateSessionInfo BuildSessionInfo(const ConnectionOptions& opts = {})
{
    auto info = Kinova::Api::Session::CreateSessionInfo();
    info.set_username(opts.username);
    info.set_password(opts.password);
    info.set_session_inactivity_timeout(SESSION_INACTIVITY_TIMEOUT_MS);
    info.set_connection_inactivity_timeout(CONNECTION_INACTIVITY_TIMEOUT_MS);
    return info;
}

} // namespace kortex
