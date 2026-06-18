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

<h1>Device routing</h1>

<h2>Table of Contents</h2>

<!-- TOC -->

- [Overview](#overview)
- [Device Manager service](#service-device-manager)
	- [Service description](#devMng-description)
	- [Good to know](#devMng-gtk)
	- [Example](#devMng-example)
- [Device Config service](#srv-devConfig)
	- [Service description](#devConfig-description)
	- [Example](#devConfig-example)
- [Other Services](#other)
	- [Example](#other-example)

<!-- /TOC -->

<a id="markdown-overview" name="overview"></a>
## Overview

Device routing is a mechanism that allows you to send commands to a specific device using the connection with the base. This is done by specifying the *device_identifier* when sending a command through a service. The service does not need to be implemented by the base.

In other words, a command can be sent to a sub-device using a service known only by that sub-device (e.g. `ActuatorConfigClient`) as long as the *device_identifier* is specified in the command parameters.

To obtain the *device_identifier* you need to use the **Device Manager** service.

<a id="markdown-srv-devMgn" name="service-device-manager"></a>
## Device Manager service

<a id="markdown-devMng-description" name="devMng-description"></a>
### Service description

The sole purpose of the **Device Manager** service is to return a device handle list containing handles for all devices present, using the method `ReadAllDevices()`.

<a id="markdown-devMng-gtk" name="devMng-gtk"></a>
### Good to know

- The device handle list returned by `ReadAllDevices()` is *not* in any specific device order
- Device handles also have the fields `device_type` and `order`

<a id="markdown-devMng-example" name="devMng-example"></a>
### Example

```cpp
#include <DeviceManagerClientRpc.h>

namespace k_api = Kinova::Api;

k_api::DeviceManager::DeviceManagerClient deviceManager(router.get());
auto subDevicesInfo = deviceManager.ReadAllDevices();
```

<a id="markdown-srv-devConfig" name="srv-devConfig"></a>
## Device Config service

<a id="markdown-devConfig-description" name="devConfig-description"></a>
### Service description

The `DeviceConfig` service provides information about the interrogated device:

- device type
- firmware and bootloader version
- model, part, and serial number
- MAC address
- hardware revision

<a id="markdown-devConfig-example" name="devConfig-example"></a>
### Example

```cpp
#include <DeviceManagerClientRpc.h>
#include <DeviceConfigClientRpc.h>
#include <RouterMQTT.h>

namespace k_api = Kinova::Api;

k_api::DeviceManager::DeviceManagerClient deviceManager(router.get());
k_api::DeviceConfig::DeviceConfigClient   deviceConfig(router.get());

// Get all device routing information
auto allDevicesInfo = deviceManager.ReadAllDevices();

k_api::RouterClientSendOptions options;
options.timeout_ms = 4000;

for (const auto& handle : allDevicesInfo.device_handle())
{
    uint32_t id = handle.device_identifier();

    auto deviceType      = deviceConfig.GetDeviceType(id, options);
    auto firmwareVersion = deviceConfig.GetFirmwareVersion(id, options);
    auto bootloaderVer   = deviceConfig.GetBootloaderVersion(id, options);
    auto modelNumber     = deviceConfig.GetModelNumber(id, options);
    auto partNumber      = deviceConfig.GetPartNumber(id, options);
    auto serialNumber    = deviceConfig.GetSerialNumber(id, options);

    // Build hexadecimal MAC address string
    auto macMsg = deviceConfig.GetMACAddress(id, options);
    std::string macHex;
    for (int b : macMsg.mac_address())
    {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X:", static_cast<unsigned char>(b));
        macHex += buf;
    }
    if (!macHex.empty()) macHex.pop_back(); // remove trailing ':'

    std::cout << "-----------------------------\n";
    std::cout << "Device id=" << id
              << "  type=" << deviceType.device_type() << "\n";
    std::cout << "Firmware : " << firmwareVersion.major() << "."
              << firmwareVersion.minor() << "."
              << firmwareVersion.patch()  << "\n";
    std::cout << "Serial   : " << serialNumber.serial_number() << "\n";
    std::cout << "MAC      : " << macHex << "\n";
}
```

<a id="markdown-other" name="other"></a>
## Other Services

The *device_identifier* can be passed to other services to directly interrogate a specific device on the bus.

<a id="markdown-example" name="other-example"></a>
### Example

```cpp
#include <DeviceManagerClientRpc.h>
#include <ActuatorConfigClientRpc.h>
#include <DeviceConfig.pb.h>

namespace k_api = Kinova::Api;

k_api::DeviceManager::DeviceManagerClient    deviceManager(router.get());
k_api::ActuatorConfig::ActuatorConfigClient  actuatorConfig(router.get());

auto allDevicesInfo = deviceManager.ReadAllDevices();

for (const auto& handle : allDevicesInfo.device_handle())
{
    // Filter for actuator devices only
    if (handle.device_type() != k_api::Common::BIG_ACTUATOR &&
        handle.device_type() != k_api::Common::SMALL_ACTUATOR)
        continue;

    uint32_t id = handle.device_identifier();

    // Query the actuator directly through the base connection
    auto controlMode = actuatorConfig.GetControlMode(id);
    std::cout << "Actuator " << id
              << " control mode: " << controlMode.control_mode() << "\n";
}
```

__________________________
## Back to root topic: **[readme.md](../readme.md)**
