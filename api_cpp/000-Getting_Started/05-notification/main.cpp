// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

//
// 05-notification
//
// Demonstrates the pub/sub notification pattern provided by the Kortex API.
//
// In addition to synchronous (blocking) RPCs, the API supports asynchronous
// event notifications.  You subscribe to a topic by passing a callback
// lambda (or function object) to an OnNotification<Topic>() method.  The
// robot pushes a message to your callback whenever the corresponding event
// occurs.  You hold a handle that lets you unsubscribe when you no longer
// need the notifications.
//
// This example:
//   1. Subscribes to ConfigurationChange notifications.
//   2. Creates a user profile to trigger one such notification.
//   3. Unsubscribes and cleans up.
//

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include <KDetailedException.h>
#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <BaseClientRpc.h>
#include <Base.pb.h>
#include <Common.pb.h>
#include <Session.pb.h>

#include "../../utilities.h"

// ---------------------------------------------------------------------------
// ExampleNotification
//
// Subscribes to configuration-change events, triggers one by creating a user,
// then unsubscribes and deletes the user.
// ---------------------------------------------------------------------------
void ExampleNotification(Kinova::Api::Base::BaseClient& base)
{
    // Define the notification callback.  The robot will call this lambda on
    // a router thread whenever a configuration-change event occurs.
    auto notificationCallback = [](Kinova::Api::Base::ConfigurationChangeNotification notif)
    {
        std::cout << "Callback called\n";
        std::cout << notif.DebugString() << "\n";
    };

    // Subscribe to the ConfigurationChange topic.
    // OnNotificationConfigurationChangeTopic returns a handle used to
    // unsubscribe later.
    auto notifHandle = base.OnNotificationConfigurationChangeTopic(
        notificationCallback,
        Kinova::Api::Common::NotificationOptions{});

    std::cout << "Subscribed to ConfigurationChange notifications\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Create a user profile.  This change triggers a ConfigurationChange
    // notification, which will invoke the callback above.
    Kinova::Api::Base::FullUserProfile fullUserProfile;
    fullUserProfile.set_password("pwd");

    Kinova::Api::Base::UserProfile* up = fullUserProfile.mutable_user_profile();
    up->set_username("jcash");
    up->set_firstname("Johnny");
    up->set_lastname("Cash");
    up->set_application_data("Custom Application Stuff");

    Kinova::Api::Common::UserProfileHandle userProfileHandle =
        base.CreateUserProfile(fullUserProfile);

    std::cout << "User created (handle: " << userProfileHandle.identifier() << ")\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Unsubscribe from the notification topic.
    base.Unsubscribe(notifHandle);
    std::cout << "Unsubscribed from notifications\n";

    // Clean up: delete the user that was created.
    try
    {
        base.DeleteUserProfile(userProfileHandle);
        std::cout << "User deleted\n";
    }
    catch (const Kinova::Api::KDetailedException& ex)
    {
        std::cerr << "Failed to delete user: " << ex.what() << "\n";
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
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

    ExampleNotification(base);

    sessionClient->CloseSession();
    return 0;
}
