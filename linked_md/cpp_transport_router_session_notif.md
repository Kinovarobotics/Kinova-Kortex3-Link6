<!--
* KINOVA (R) KORTEX (TM) 3
*
* Copyright (c) 2026 Kinova inc. All rights reserved.
*
* This software may be modified and distributed
* under the terms of the BSD 3-Clause license.
*
* Refer to the LICENSE file for details.
*
-->

<h1>Transport, RouterClient, SessionManager and Notifications</h1>

<h2>Table of Contents</h2>

<!-- TOC -->

- [Overview](#overview)
- [RouterMQTT — high-level control](#routermqtt)
- [RouterClient and TransportClientUdp — low-level cyclic](#routerclient-udp)
- [SessionManager](#sessionmanager)
- [Notifications](#notifications)

<!-- /TOC -->

<a id="markdown-overview" name="overview"></a>
## Overview

This document covers how to establish a connection to the robot and communicate with it in both directions.

The connection is handled by a **Router** class, which manages the underlying transport. Once connected, a **SessionManager** authenticates the client. Commands are then sent through service clients. Unsolicited updates from the robot are received via **notification subscriptions**.

<a id="markdown-routermqtt" name="routermqtt"></a>
## RouterMQTT — high-level control

`RouterMQTT` is the router for all high-level communication with the robot. It uses the MQTT protocol on port **1883**. All standard services (base control, configuration, programs, plugins, etc.) use this router.

An error callback must be provided to handle transport-level failures.

```cpp
#include <RouterMQTT.h>

namespace k_api = Kinova::Api;

const std::string robotIp  = "192.168.1.10";
const int         mqttPort = 1883;

auto router = std::make_shared<k_api::RouterMQTT>(
    robotIp, mqttPort, "",
    [](k_api::KError err)
    {
        std::cerr << "Router error: " << err.toString() << "\n";
    });

router->SpinProcess(); // start the internal processing thread
```

<a id="markdown-routerclient-udp" name="routerclient-udp"></a>
## RouterClient and TransportClientUdp — low-level cyclic

For low-level cyclic control at **1 kHz**, a separate `RouterClient` backed by a `TransportClientUdp` is required. This router is used exclusively with the cyclic service and runs alongside the MQTT router.

```cpp
#include <RouterClient.h>
#include <TransportClientUdp.h>

const int udpPort = 10001;

auto* udpTransport = new k_api::TransportClientUdp();
udpTransport->connect(robotIp, udpPort);

auto* udpRouter = new k_api::RouterClient(
    udpTransport,
    [](k_api::KError err)
    {
        std::cerr << "UDP router error: " << err.toString() << "\n";
    });
```

<a id="markdown-sessionmanager" name="sessionmanager"></a>
## SessionManager

A `SessionManager` must be created on an active router before any service calls can be made. It authenticates the client and keeps the session alive.

```cpp
#include <SessionManager.h>
#include <Session.pb.h>

// Create session on the MQTT router
auto sessionManager = std::make_shared<k_api::SessionManager>(router.get());

k_api::Session::CreateSessionInfo sessionInfo;
sessionInfo.set_username("admin");
sessionInfo.set_password("admin");
sessionInfo.set_session_inactivity_timeout(60000);   // ms
sessionInfo.set_connection_inactivity_timeout(2000); // ms

sessionManager->CreateSession(sessionInfo);

// Close session when done
sessionManager->CloseSession();
```

<a id="markdown-notifications" name="notifications"></a>
## Notifications

The robot uses a **Publish/Subscribe** pattern. A client subscribes to a topic and receives notifications whenever a relevant change occurs — regardless of which session triggered it.

Subscription methods follow the naming convention `OnNotification<Topic>Topic()` and return a `NotificationHandle` used to unsubscribe later.

```cpp
#include <BaseClientRpc.h>
#include <Common.pb.h>

k_api::Base::BaseClient base(router.get());

// Subscribe to configuration change notifications
auto notifHandle = base.OnNotificationConfigurationChangeTopic(
    [](k_api::Base::ConfigurationChangeNotification notification)
    {
        std::cout << "Configuration changed — event: "
                  << notification.generic_info().event() << "\n";
    },
    k_api::Common::NotificationOptions{});

// ... later, unsubscribe
base.Unsubscribe(notifHandle);
```

__________________________
## Back to root topic: **[readme.md](../readme.md)**
