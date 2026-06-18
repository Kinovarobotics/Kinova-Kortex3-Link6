// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

//
// 03-api_mechanism
//
// Demonstrates the basic API call mechanism: synchronous (blocking) RPCs.
//
// All API calls in the Kortex C++ API are synchronous by default.  When you
// call a client method (e.g. ReadAllPrograms()), the calling thread blocks
// until the robot has processed the request and returned a response, or until
// a timeout occurs.  This makes the control flow straightforward — no
// callbacks or futures are needed for ordinary request/response operations.
//
// This example reads the list of programs available on the robot to
// illustrate a typical blocking RPC call.
//

#include <iostream>
#include <memory>
#include <string>

#include <KDetailedException.h>
#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <ProgramRunnerClientRpc.h>
#include <ProgramConfig.pb.h>
#include <Session.pb.h>

#include "../../utilities.h"

// ---------------------------------------------------------------------------
// ReadAvailablePrograms
//
// Calls the synchronous ReadAllPrograms RPC and prints the name and unique
// identifier of every program stored on the controller.
// ---------------------------------------------------------------------------
void ReadAvailablePrograms(Kinova::Api::ProgramRunner::ProgramRunnerClient& programRunner)
{
    // ReadAllPrograms() blocks until the robot replies with a ProgramList.
    Kinova::Api::ProgramConfig::ProgramList programs = programRunner.ReadAllPrograms();

    std::cout << "Available programs (" << programs.programs_size() << "):\n";
    for (const auto& program : programs.programs())
    {
        std::cout << "  [" << program.handle().identifier() << "] "
                  << program.name() << "\n";
    }
}

int main(int argc, char* argv[])
{
    const kortex::ConnectionOptions opts = kortex::ParseArgs(argc, argv);
    const std::string& robotIp = opts.ip;

    auto router = std::make_shared<Kinova::Api::RouterMQTT>(
        robotIp, kortex::MQTT_PORT, "",
        [](Kinova::Api::KError err) {
            std::cerr << "Router error: " << err.toString() << "\n";
        });
    router->SpinProcess();

    auto sessionInfo = kortex::BuildSessionInfo(opts);

    auto sessionClient = std::make_unique<Kinova::Api::Session::SessionClient>(router.get());
    sessionClient->CreateSession(sessionInfo);

    std::cout << "Session created\n";

    // ProgramRunnerClient provides access to program management and execution.
    Kinova::Api::ProgramRunner::ProgramRunnerClient programRunner(router.get());

    ReadAvailablePrograms(programRunner);

    sessionClient->CloseSession();
    return 0;
}
