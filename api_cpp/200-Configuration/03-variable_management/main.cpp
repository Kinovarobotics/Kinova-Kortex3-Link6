// KINOVA (R) KORTEX (TM) 3
//
// Copyright (c) 2026 Kinova inc. All rights reserved.
//
// This software may be modified and distributed
// under the terms of the BSD 3-Clause license.
//
// Refer to the LICENSE file for details.

// This example demonstrates how to create, read, list, and delete variables
// in the robot's global namespace using the VariableManager API.
//
// Variable types:
//   VARIABLE_TYPE_JSON   - structured object value (most flexible)
//   VARIABLE_TYPE_STRING - plain text string
//   VARIABLE_TYPE_BOOL   - boolean flag
//   VARIABLE_TYPE_INT    - integer number
//   VARIABLE_TYPE_FLOAT  - floating-point number
//
// Common schema_key values (used with VARIABLE_TYPE_JSON):
//   "default_jointAngles" - joint angle set  { "angles": [...] }
//   "default_pose"        - Cartesian pose   { "x","y","z","theta_x",... }

#include <iostream>
#include <algorithm>

#include <RouterMQTT.h>
#include <SessionClientRpc.h>
#include <VariableManagerClientRpc.h>
#include <KDetailedException.h>
#include <VariableManager.pb.h>
#include <Session.pb.h>

#include "../../utilities.h"

namespace k_api = Kinova::Api;

// ----------------------------------------------------------------------------
// CreateVariable
//
// Creates (or updates) a JSON variable named "var_example" in the global
// namespace, then retrieves and prints its value.
// ----------------------------------------------------------------------------
void CreateVariable(k_api::VariableManager::VariableManagerClient& vm)
{
    // --- Build the namespace handle ---
    k_api::VariableManager::NamespaceHandle nsHandle;
    nsHandle.set_identifier("globals");

    // --- Build the variable handle ---
    k_api::VariableManager::VariableHandle varHandle;
    varHandle.mutable_namespace_handle()->set_identifier("globals");
    varHandle.set_identifier("var_example");

    // --- Build the variable ---
    k_api::VariableManager::Variable var;
    var.mutable_handle()->CopyFrom(varHandle);
    var.set_type(k_api::VariableManager::VARIABLE_TYPE_JSON);
    var.set_schema_key("default_jointAngles");
    var.set_json_value(R"({"angles":[1,0,-12,1,-2,1]})");

    vm.SetVariable(var);
    std::cout << "Variable created / updated.\n";

    // --- List all variables in the global namespace ---
    const auto allVars = vm.GetAllVariables(nsHandle);
    std::cout << "All variables in 'globals':\n";
    for (const auto& v : allVars.variables())
    {
        std::cout << "  "
                  << v.handle().namespace_handle().identifier()
                  << "."
                  << v.handle().identifier()
                  << "\n";
    }

    // --- Read back the variable we just created ---
    const auto retrieved = vm.GetVariable(varHandle);
    std::cout << "Retrieved var_example.json_value: "
              << retrieved.json_value() << "\n";
}

// ----------------------------------------------------------------------------
// DeleteVariable
//
// Deletes the variable with the given name from the global namespace if it
// exists.
// ----------------------------------------------------------------------------
void DeleteVariable(k_api::VariableManager::VariableManagerClient& vm,
                    const std::string& varName)
{
    k_api::VariableManager::NamespaceHandle nsHandle;
    nsHandle.set_identifier("globals");

    k_api::VariableManager::VariableHandle toDelete;
    toDelete.mutable_namespace_handle()->set_identifier("globals");
    toDelete.set_identifier(varName);

    // Only delete if the variable currently exists.
    const auto allVars = vm.GetAllVariables(nsHandle);
    const bool found = std::any_of(
        allVars.variables().begin(), allVars.variables().end(),
        [&varName](const k_api::VariableManager::Variable& v) {
            return v.handle().identifier() == varName;
        });

    if (found)
    {
        vm.DeleteVariable(toDelete);
        std::cout << "Variable '" << varName << "' deleted.\n";
    }
    else
    {
        std::cout << "Variable '" << varName << "' not found; nothing to delete.\n";
    }
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
    k_api::VariableManager::VariableManagerClient vm(router.get());

    CreateVariable(vm);
    DeleteVariable(vm, "var_example");

    // ── Cleanup ──────────────────────────────────────────────────────────────
    sessionClient->CloseSession();
    return 0;
}
