// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

//
// 04-error_management
//
// Demonstrates how to catch and inspect API errors using KDetailedException.
//
// When a Kortex API call fails (e.g. the robot rejects the request due to
// invalid parameters, permission issues, or an internal error), the C++ API
// throws a Kinova::Api::KDetailedException.  This exception carries:
//   - error_code      : the top-level error category
//   - error_sub_code  : a more specific sub-category
//   - a human-readable description via what()
//
// This example intentionally causes an API error by calling CreateUserProfile
// with an empty FullUserProfile message, then shows how to catch and inspect
// the resulting exception.
//

#include <iostream>
#include <memory>
#include <string>

#include <KDetailedException.h>
#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <Base.pb.h>
#include <Session.pb.h>

#include "../../utilities.h"

// ---------------------------------------------------------------------------
// ExampleErrorManagement
//
// Intentionally passes an empty FullUserProfile to CreateUserProfile so that
// the robot rejects it and the API throws KDetailedException.
// ---------------------------------------------------------------------------
void ExampleErrorManagement(Kinova::Api::Base::BaseClient& base)
{
    try
    {
        // An empty FullUserProfile is invalid (no username / password), so
        // the robot will refuse it and the API will throw.
        base.CreateUserProfile(Kinova::Api::Base::FullUserProfile{});
    }
    catch (const Kinova::Api::KDetailedException& ex)
    {
        // getErrorInfo() returns a KDetailedError that wraps the protobuf
        // Error message received from the robot.
        const auto& error = ex.getErrorInfo().getError();

        std::cout << "Caught expected KDetailedException:\n"
                  << "  error_code     : " << error.error_code()     << "\n"
                  << "  error_sub_code : " << error.error_sub_code() << "\n"
                  << "  description    : " << ex.what()              << "\n";
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

    Kinova::Api::Base::BaseClient base(router.get());

    ExampleErrorManagement(base);

    sessionClient->CloseSession();
    return 0;
}
