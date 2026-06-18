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

<h1>C++ examples</h1>

<a id="markdown-description" name="description"></a>
# Description

This folder contains the C++ API installation process, as well as examples covering the main functionalities of the API.
The full documentation of the API is available at https://docs.kinovarobotics.com/index.html .
If you have any questions, reach out to us at support@kinova.ca.

<h2>Table of Contents</h2>

<!-- TOC -->

- [Description](#description)
- [Setup (C++ build environment)](#setup-c-build-environment)
  - [Requirements](#requirements)
  - [Option A — Conan (recommended)](#option-a--conan-recommended)
  - [Option B — CMake only (no Conan)](#option-b--cmake-only-no-conan)
- [How to use the examples](#how-to-use-the-examples)
  - [Build — all examples at once](#build--all-examples-at-once)
  - [Build — Option A (Conan)](#build--option-a-conan)
  - [Build — Option B (CMake only)](#build--option-b-cmake-only)
  - [Using the API in your own project (Conan)](#using-the-api-in-your-own-project-conan)
- [Examples](#examples)
- [Reference](#reference)
      - [useful links](#useful-links)
  - [Back to root topic: **readme.md**](#back-to-root-topic-readmemd)

<!-- /TOC -->

<a id="markdown-setup-c-build-environment" name="setup-c-build-environment"></a>
# Setup (C++ build environment)

<a id="markdown-requirements" name="requirements"></a>
## Requirements

- CMake >= 3.12
- C++17-compatible compiler (GCC >= 7, Clang >= 5, MSVC >= 2017)
- **Google Protocol Buffers** — version must match the downloaded package: `3.6.1` (Ubuntu 20), `3.12.4` (Ubuntu 22), `3.21` (Ubuntu 24), or `3.20.0` (Default) for Linux packages; `3.20.0` for Jetson and Pi5. With Option A (Conan) the matching version is selected through `conan install` arguments — see [Build — Option A (Conan)](#build--option-a-conan).
- One of the following:
  - **Conan** package manager (recommended), or
  - The **kortex_api_cpp** package extracted manually (CMake-only mode)

<a id="markdown-install-kinova-kortex-3-c-api" name="install-kinova-kortex-3-c-api"></a>
## Option A — Conan (recommended)

Install [Conan](https://conan.io/downloads) if not already present, then add the Kinova remote:

```sh
conan remote add kinova https://artifactory.kinovaapps.com/artifactory/api/conan/conan-public
```

On Ubuntu 20, no further setup is needed: the CMakeLists.txt of each example will run `conan install` automatically on the first `cmake` invocation. On Ubuntu 22 and Ubuntu 24, one extra step is required at build time — see [Build — Option A (Conan)](#build--option-a-conan).

## Option B — CMake only (no Conan)

Download the `kortex_api_cpp` package archive from the [Download links](../readme.md#download-links) section of the root readme and extract it to a directory of your choice (referred to as `<KORTEX_API_DIR>` below). Ensure the protobuf version installed on your system matches the variant you downloaded (e.g. `linux_proto_3_20_0.zip` requires protobuf 3.20.0).

<a id="markdown-how-to-use-examples-with-your-robot" name="how-to-use-examples-with-your-robot"></a>
# How to use the examples

We assume the robot is using its default IP address: ``192.168.1.10``

The robot IP and the parameters common to every control session (credentials and
session/connection timeouts) are defined once in [`utilities.h`](utilities.h), which
all examples share. Edit that file to change the defaults for every example at once.

You can also override the connection settings per run from the command line, without
recompiling:

```sh
./<example-executable> [robot-ip] [username] [password]
# e.g. use a non-default IP:
./<example-executable> 192.168.2.22
```

**Before starting, ensure that you run the examples in a safe area since some examples contain wide movements at full speed (default behaviour, but you can reduce speeds by modifying the example code). Also, verify that your robot is correctly fixed to the working surface.**

Prerequisites:
+ The examples require a wired network connection to your computer
+ Configure a static IP address on your network interface (e.g. ``192.168.1.11/24``)
+ Enable the Kortex API on the robot: in the web app, go to **Systems > Remote Access** and toggle **Kortex API** on. Without this, the API port is blocked and connection attempts fail with an `MQTT error [-1]: TCP connect timeout`. This switch is off by default and after a factory reset, so re-enable it whenever you reset the robot.
+ For examples that switch the robot into **AUTO** operating mode and run a stored program (e.g. [01-run_program](102-Movement_high_level/01-run_program/main.cpp) and the waypoint-trajectory examples), turn **off** the **Acknowledge Automatic Mode** switch in the web app under **Robot > Controller**. When this switch is on, entering automatic mode requires a manual acknowledgment from the web app, so the API never receives the execution-event notification and the example appears to stall (blocked on the completion wait).

Some examples may require JSON programs to be uploaded on your controller to be able to run as is. You can either find these programs in 400-Json_programs, or modify the examples to use your own programs.

Each example is a standalone executable built from its own `CMakeLists.txt`. You can either build a single example on its own (following the sections below) or build **all examples at once** from the top-level `CMakeLists.txt` in this folder.

## Build — all examples at once

The `CMakeLists.txt` at the root of `api_cpp/` builds every example in one configure/build. It supports the same two modes as the individual examples, and places all resulting executables together in `build/bin/`.

```sh
cd api_cpp
mkdir build && cd build
conan install .. <extra arguments from the table below>   # Ubuntu 22 / 24 only
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

For a CMake-only (no Conan) build, pass `-DKORTEX_API_DIR=<path/to/extracted/kortex_api_cpp>` to the `cmake ..` step instead of running `conan install` (see [Build — Option B](#build--option-b-cmake-only)).

All executables are written to `build/bin/`, one per example, named after the example (e.g. `build/bin/api_creation`).

## Build — Option A (Conan)

Prebuilt binaries are published for the following configurations. On Ubuntu 22 and Ubuntu 24, `conan install` must be run manually in the build directory with the extra arguments below **before** the first `cmake` invocation:

| Platform | Compiler | Extra `conan install` arguments |
| --- | --- | --- |
| Ubuntu 20 | gcc 9 | *(none — the manual step can be skipped entirely)* |
| Ubuntu 22 | gcc 11 | `-o kortex_api_cpp:protobuf_version=3.12.4` |
| Ubuntu 24 | gcc 13 | `-o kortex_api_cpp:protobuf_version=3.21.12 -s compiler.version=12` |

```sh
cd <example-directory>
mkdir build && cd build
conan install .. <extra arguments from the table>   # Ubuntu 22 / 24 only
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

On Ubuntu 20, `conan install` runs automatically during `cmake ..` on the first build.

Notes:
- The Ubuntu 24 package is built with gcc 12. Passing `-s compiler.version=12` only selects that prebuilt binary; your code is still compiled with your system compiler (gcc 13), which is ABI-compatible with the gcc 12 library.
- For a Debug build, also pass `-s build_type=Debug` to `conan install` and use `-DCMAKE_BUILD_TYPE=Debug`.

## Build — Option B (CMake only)

```sh
cd <example-directory>
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DKORTEX_API_DIR=<path/to/extracted/kortex_api_cpp>
cmake --build .
```

The compiled executable will be placed in the `build/bin/` directory.

## Using the API in your own project (Conan)

Add a `conanfile.txt` next to your `CMakeLists.txt`:

```ini
[requires]
kortex_api_cpp/3.4.0-r.15-public@link6/stable

[generators]
cmake
```

In your `CMakeLists.txt`, consume the generated build info:

```cmake
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
target_link_libraries(<your-target> CONAN_PKG::kortex_api_cpp Threads::Threads)
```

Then run `conan install` in your build directory — with the extra arguments from the table in [Build — Option A (Conan)](#build--option-a-conan) if you are on Ubuntu 22 or 24 — followed by `cmake` as usual.

<a id="markdown-examples-list" name="examples-list"></a>
# Examples

| Example | Description |
| --- | --- |
| **000-Getting_Started** | |
| [01-api_creation](000-Getting_Started/01-api_creation/main.cpp) | Establish an MQTT session and instantiate the two most commonly used clients (`BaseClient`, `DeviceConfigClient`) |
| [02-protobuf_object_manipulation](000-Getting_Started/02-protobuf_object_manipulation/main.cpp) | Build and inspect protobuf message objects without connecting to a robot |
| [03-api_mechanism](000-Getting_Started/03-api_mechanism/main.cpp) | Blocking RPC call pattern — read the program list from the robot |
| [04-error_management](000-Getting_Started/04-error_management/main.cpp) | Catch and inspect `KDetailedException` errors from the API |
| [05-notification](000-Getting_Started/05-notification/main.cpp) | Subscribe to configuration-change notifications using the pub/sub pattern |
| **100-Overview** | |
| [01-devices_routing](100-Overview/01-devices_routing/main.cpp) | Query device info (firmware, serial number, MAC) from every device on the bus via `DeviceManagerClient` |
| [02-protection_zones_configuration](100-Overview/02-protection_zones_configuration/main.cpp) | Create and delete a rectangular-prism protection zone, then verify it stops the robot |
| **102-Movement_high_level** | |
| [01-run_program](102-Movement_high_level/01-run_program/main.cpp) | Run a stored program by name and wait for completion via an execution-event notification |
| [02-twist_command](102-Movement_high_level/02-twist_command/main.cpp) | Send Cartesian velocity (twist) commands: wrapped action, direct one-shot, and 40 Hz streaming |
| [03-send_joint_speeds](102-Movement_high_level/03-send_joint_speeds/main.cpp) | Command joint velocities directly with `SendJointSpeedsCommand` |
| [04-Cartesian_waypoint_trajectory](102-Movement_high_level/04-Cartesian_waypoint_trajectory/main.cpp) | Execute a validated Cartesian waypoint trajectory with blending |
| [05-Angular_waypoint_trajectory](102-Movement_high_level/05-Angular_waypoint_trajectory/main.cpp) | Execute a validated joint-space waypoint trajectory with blending |
| [06-Toolpath_trajectory](102-Movement_high_level/06-Toolpath_trajectory/main.cpp) | Execute a toolpath built from straight-segment and arc-point waypoints |
| **111-Kinematics** | |
| [01-compute_kinematics](111-Kinematics/01-compute_kinematics/main.cpp) | Read current joint angles and compute the Cartesian pose via forward kinematics |
| **200-Configuration** | |
| [01-programs_management](200-Configuration/01-programs_management/main.cpp) | List, upload, and delete programs on the controller |
| [02-session_management](200-Configuration/02-session_management/main.cpp) | Establish and explicitly close a session |
| [03-variable_management](200-Configuration/03-variable_management/main.cpp) | Create, read, list, and delete global variables using `VariableManager` |
| **300-Low_Level** | |
| [01-low_level_cyclic_control](300-Low_Level/01-low_level_cyclic_control/main.cpp) | Hold joint positions at 1 kHz using `BaseCyclicClient` over UDP alongside an MQTT connection for mode changes |

<a id="markdown-reference" name="reference"></a>
# Reference

<a id="markdown-useful-links" name="useful-links"></a>
#### useful links

|  |  |
| ---: | --- |
| KINOVA KORTEX 3 documentation: | [https://docs.kinovarobotics.com/index.html](https://docs.kinovarobotics.com/index.html) |
| Google Protocol Buffers C++ API: | [https://protobuf.dev/reference/cpp/api-docs/](https://protobuf.dev/reference/cpp/api-docs/) |

__________________________
<a id="markdown-back-to-root-topic-readmemd" name="back-to-root-topic-readmemd"></a>
## Back to root topic: **[readme.md](../readme.md)**
