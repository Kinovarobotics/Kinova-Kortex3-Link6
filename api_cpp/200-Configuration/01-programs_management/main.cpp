// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <stdexcept>

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <ProgramRunnerClientRpc.h>
#include <KDetailedException.h>
#include <ProgramConfig.pb.h>
#include <Common.pb.h>
#include <Session.pb.h>

#include "../../utilities.h"

namespace k_api = Kinova::Api;
namespace fs    = std::filesystem;

// ----------------------------------------------------------------------------
// ExampleBackup
//
// Reads all programs from the robot and writes each one to a JSON file in the
// local "backup/" directory.  Existing files are overwritten.
// ----------------------------------------------------------------------------
void ExampleBackup(k_api::ProgramRunner::ProgramRunnerClient& runner)
{
    const auto progList = runner.ReadAllPrograms();

    const fs::path backupDir("backup");
    fs::create_directories(backupDir);

    std::cout << "Backing up " << progList.programs_size() << " program(s)...\n";

    for (auto prog : progList.programs())
    {
        // Grant full permissions on the handle before exporting.
        prog.mutable_handle()->set_permission(
            static_cast<uint32_t>(k_api::Common::Permission::READ_PERMISSION) |
            static_cast<uint32_t>(k_api::Common::Permission::UPDATE_PERMISSION) |
            static_cast<uint32_t>(k_api::Common::Permission::DELETE_PERMISSION));

        k_api::Common::ProgramHandle handle;
        handle.CopyFrom(prog.handle());

        const auto progJson = runner.ExportProgram(handle);

        const fs::path outPath = backupDir / (prog.name() + ".json");
        std::ofstream outFile(outPath);
        if (!outFile)
            throw std::runtime_error("Could not open file for writing: " + outPath.string());

        outFile << progJson.payload();
        std::cout << "  Saved: " << outPath << "\n";
    }

    std::cout << "Backup complete.\n";
}

// ----------------------------------------------------------------------------
// ExampleLoad
//
// Imports every *.json file found in the "backup/" directory back into the
// robot as a program.
// ----------------------------------------------------------------------------
void ExampleLoad(k_api::ProgramRunner::ProgramRunnerClient& runner)
{
    const fs::path backupDir("backup");

    if (!fs::exists(backupDir))
    {
        std::cerr << "Backup directory does not exist. Run the backup example first.\n";
        return;
    }

    std::cout << "Loading programs from backup...\n";

    for (const auto& entry : fs::directory_iterator(backupDir))
    {
        if (entry.path().extension() != ".json")
            continue;

        std::ifstream inFile(entry.path());
        if (!inFile)
            throw std::runtime_error("Could not open file: " + entry.path().string());

        const std::string payload((std::istreambuf_iterator<char>(inFile)),
                                   std::istreambuf_iterator<char>());

        k_api::ProgramConfig::ProgramJSON program;
        program.set_payload(payload);

        runner.ImportProgram(program);
        std::cout << "  Imported: " << entry.path().filename() << "\n";
    }

    std::cout << "Load complete.\n";
}

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    const kortex::ConnectionOptions opts = kortex::ParseArgs(argc, argv);
    const std::string& robotIp = opts.ip;

    // ── MQTT connection ──────────────────────────────────────────────────────
    auto router = std::make_shared<k_api::RouterMQTT>(
        robotIp, kortex::MQTT_PORT, "",
        [](k_api::KError err) { std::cerr << "Router error: " << err.toString() << "\n"; });
    router->SpinProcess();

    // ── Session ──────────────────────────────────────────────────────────────
    auto sessionInfo = kortex::BuildSessionInfo(opts);

    auto sessionClient = std::make_unique<k_api::Session::SessionClient>(router.get());
    sessionClient->CreateSession(sessionInfo);

    // ── Example logic ────────────────────────────────────────────────────────
    k_api::ProgramRunner::ProgramRunnerClient runner(router.get());

    ExampleBackup(runner);
    ExampleLoad(runner);

    // ── Cleanup ──────────────────────────────────────────────────────────────
    sessionClient->CloseSession();
    return 0;
}
