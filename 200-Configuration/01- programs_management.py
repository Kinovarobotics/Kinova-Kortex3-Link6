#! /usr/bin/env python3

###
# KINOVA (R) KORTEX (TM)
#
# Copyright (c) 2023 Kinova inc. All rights reserved.
#
# This software may be modified and distributed
# under the terms of the BSD 3-Clause license.
#
# Refer to the LICENSE file for details.
#
###
import json
import os
import sys
from pathlib import Path

from kortex_api.autogen.client_stubs.ProgramRunnerClientRpc import ProgramRunnerClient
from kortex_api.autogen.messages.Common_pb2 import ProgramHandle, Permission
from kortex_api.autogen.messages.ProgramConfig_pb2 import ProgramJSON, ProgramList


# This function backs up all the programs by exporting  them as JSON in a folder called "backup"
def example_backup(runner_client):

    prog_list: ProgramList = runner_client.ReadAllPrograms()

    prog_dir = Path("backup")
    prog_dir.mkdir(exist_ok=True)

    for prog in prog_list.programs:

        prog.handle.permission = (
            Permission.READ_PERMISSION
            | Permission.UPDATE_PERMISSION
            | Permission.DELETE_PERMISSION
        )
        print(f"Loading: {prog.name}")
        handle: ProgramHandle = prog.handle
        prog_json: ProgramJSON = runner_client.ExportProgram(handle)
        filepath: Path = prog_dir / f"{prog.name}.json"

        with filepath.open("w", encoding="utf-8") as f:
            data = json.loads(prog_json.payload)
            f.write(json.dumps(data, indent=4))


# This program loads every program .json file in the folder given to Path()
def example_load(runner_client):

    prog_dir = Path("backup")

    prog_files = prog_dir.glob("*.json")
    for fprog in prog_files:

        with fprog.open("r", encoding="utf-8") as f:
            payload = f.read()
        program = ProgramJSON()

        program.payload = payload
        handle: ProgramHandle = runner_client.ImportProgram(program)


def main():

    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
    import utilities

    # Parse arguments
    args = utilities.parseConnectionArguments()

    # Create connection to the device and get the router
    with utilities.DeviceConnection.createMqttConnection(args) as router:

        runner_client = ProgramRunnerClient(router)
        example_backup(runner_client)
        example_load(runner_client)

    return


if __name__ == "__main__":
    exit(main())
