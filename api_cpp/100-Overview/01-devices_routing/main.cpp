// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example fetches device information from every device in the robot
// network by routing calls through the DeviceManager. For each device it
// retrieves the device type, firmware bundle versions, bootloader version,
// model number, part number, serial number, and MAC address.

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <DeviceManagerClientRpc.h>
#include <DeviceConfigClientRpc.h>
#include <BaseClientRpc.h>
#include <DeviceManager.pb.h>
#include <DeviceConfig.pb.h>
#include <Session.pb.h>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "../../utilities.h"

// ---------------------------------------------------------------------------
// Helper: format a raw MAC address byte string as "AA:BB:CC:DD:EE:FF"
// ---------------------------------------------------------------------------
static std::string FormatMacAddress(const std::string& raw)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < raw.size(); ++i)
    {
        if (i > 0)
            oss << ':';
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << (static_cast<unsigned int>(static_cast<unsigned char>(raw[i])));
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// Core example: iterate every device and print a per-device summary
// ---------------------------------------------------------------------------
void ExampleRoutedDeviceConfig(
    Kinova::Api::DeviceManager::DeviceManagerClient& deviceManager,
    Kinova::Api::DeviceConfig::DeviceConfigClient&   deviceConfig,
    Kinova::Api::Base::BaseClient&                   base)
{
    // Read all connected devices
    auto allDevices = deviceManager.ReadAllDevices();
    std::cout << "=== All devices ===\n"
              << allDevices.DebugString() << "\n";

    // Options used for every call that must be routed to a specific device
    Kinova::Api::RouterClientSendOptions options;
    options.timeout_ms = 4000;

    // Fetch and print firmware bundle versions once (applies to the whole arm)
    auto firmwareVersions = base.GetFirmwareBundleVersions();
    std::cout << "=== Firmware bundle versions ===\n"
              << firmwareVersions.DebugString() << "\n";

    for (const auto& handle : allDevices.device_handle())
    {
        const uint32_t devId = handle.device_identifier();

        std::cout << "----------------------------------------\n";
        std::cout << "Device ID : " << devId << "\n";

        // Device type
        auto deviceType = deviceConfig.GetDeviceType(devId, options);
        std::cout << "Device type      : " << deviceType.DebugString();

        // Bootloader version
        auto bootloader = deviceConfig.GetBootloaderVersion(devId, options);
        std::cout << "Bootloader       : " << bootloader.DebugString();

        // Model number
        auto modelNumber = deviceConfig.GetModelNumber(devId, options);
        std::cout << "Model number     : " << modelNumber.model_number() << "\n";

        // Part number
        auto partNumber = deviceConfig.GetPartNumber(devId, options);
        std::cout << "Part number      : " << partNumber.part_number() << "\n";

        // Serial number
        auto serial = deviceConfig.GetSerialNumber(devId, options);
        std::cout << "Serial number    : " << serial.serial_number() << "\n";

        // MAC address
        auto mac = deviceConfig.GetMACAddress(devId, options);
        std::cout << "MAC address      : " << FormatMacAddress(mac.mac_address()) << "\n";
    }
    std::cout << "----------------------------------------\n";
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

    Kinova::Api::DeviceManager::DeviceManagerClient deviceManager(router.get());
    Kinova::Api::DeviceConfig::DeviceConfigClient   deviceConfig(router.get());
    Kinova::Api::Base::BaseClient                   base(router.get());

    ExampleRoutedDeviceConfig(deviceManager, deviceConfig, base);

    sessionClient->CloseSession();
    return 0;
}
