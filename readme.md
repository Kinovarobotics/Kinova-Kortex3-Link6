<!--
* KINOVA (R) KORTEX (TM) 3
*
* Copyright (c) 2018 Kinova inc. All rights reserved.
*
* This software may be modified and distributed
* under the terms of the BSD 3-Clause license.
*
* Refer to the LICENSE file for details.
*
-->

<h1>KINOVA<sup>®</sup> KORTEX™ 3 API Reference</h1>

<a id="markdown-description" name="description"></a>
# Description

The official repository contains documentation and examples explaining how to use the KINOVA<sup>®</sup> KORTEX™ 3 API client with Python and C++. The repository has been tested on Windows 10 and Ubuntu 20.04.

<h1>Table of Contents</h1>

<!-- TOC -->

- [Description](#description)
- [Licensing](#licensing)
- [Role of Google Protocol Buffer in KINOVA KORTEX™ 3 API](#role-of-google-protocol-buffer-in-kortex-api)
  - [Quick Start for Python users](#quick-start-for-python-users)
  - [Quick Start for C++ users](#quick-start-for-c-users)
  - [Quick Start for Modbus and Ethernet IP users](#quick-start-for-modbus-ethernet-ip-users)
- [Download links](#download-links)
- [Build and Run instructions](#build-and-run-instructions)
- [Reference](#reference)
      - [Useful Links](#useful-links)

<!-- /TOC -->

<a id="markdown-licensing" name="licensing"></a>
# Licensing 
This repository is licenced under the [BSD 3-Clause "Revised" License](./LICENSE) 

<a id="markdown-role-of-google-protobuf-in-kortex-api" name="role-of-google-protobuf-in-kortex-api"></a>
# Role of Google Protocol Buffer in KINOVA KORTEX™ 3 API 

The KINOVA KORTEX™ 3 API uses Google Protocol Buffer message objects<sup>**[1](#useful-links)**</sup> to exchange data between client and server.  

Google Protocol Buffer offers structured data objects with standard methods for each member field:  
+ structured, nested objects
+ basic types and collections
+ getter/setter methods on basic types
+ iterators, dimension and appending methods on collections
+ many helpers (e.g. serialize/deserialize, I/O functions)
  

When using the KINOVA KORTEX™ 3 API a developer will need to understand the Google Protocol Buffer feature set to maximize their efficiency.  


<a id="markdown-quick-start-howto-python" name="quick-start-howto-python"></a>
# Quick Start for Python users

  To run the Python examples you will need to install the Python interpreter and the pip installation module.

  Here is some general information about the Python interpreter and the pip module manager.  
  - [Python General Information](./linked_md/python_quick_start.md)
  - [API mechanism](./linked_md/python_api_mechanism.md)
  - [Transport / Router / Session / Notification](./linked_md/python_transport_router_session_notif.md)
  - [Device routing](./linked_md/python_device_routing.md)
  - [Error management](./linked_md/python_error_management.md)
  - [Examples](./api_python/README.md)

<a id="markdown-quick-start-howto-cpp" name="quick-start-howto-cpp"></a>
# Quick Start for C++ users

  To build the C++ examples you will need CMake, a C++17 compiler, and optionally the Conan package manager.

  - [API mechanism](./linked_md/cpp_api_mechanism.md)
  - [Transport / Router / Session / Notification](./linked_md/cpp_transport_router_session_notif.md)
  - [Device routing](./linked_md/cpp_device_routing.md)
  - [Error management](./linked_md/cpp_error_management.md)
  - [Examples](./api_cpp/README.md)

<a id="markdown-quick-start-howto-modbus" name="quick-start-howto-modbus"></a>
# Using Modbus and Ethernet IP Users
The robot handle the communication via  Modbus  and Ethernet/IP. This is done via the FieldBusAdapter plugin provided by Kinova.

[Plugin Download](https://artifactory.kinovaapps.com:443/artifactory/generic-public/kortex/plugins/fieldbus_adapter/1.1.0/fieldbus_adapter_1.1.0-r.5.kp)

[Documentation](https://artifactory.kinovaapps.com:443/artifactory/generic-documentation-public/Documentation/Link%206/Plugins/Fieldbus%20Adapter/EN-UG-021-FieldBusAdapter-Plugin-user-guide-r1.1.pdf)


<a id="markdown-api-download-links" name="api-download-links"></a>
# Download links
Before getting started, please review the release notes to learn about any limitations or known issues of the KINOVA KORTEX™ 3 API for Link 6. Ensure proper training from reviewing the user guide on how to safely use the product.

> **C++ API:** Each package is compiled against a specific protobuf version shown in the table below. With Conan (Option A), pass the matching `protobuf_version` option to `conan install` as described in the [C++ examples readme](./api_cpp/README.md#build--option-a-conan). For CMake-only builds (Option B) you must install the matching version.

| Firmware | Release notes | Python API | C++ API docs | Python API docs | C++ API (Jetson) | C++ API (Linux, Ubuntu 20) | C++ API (Linux, Ubuntu 22) | C++ API (Linux, Ubuntu 24) | C++ API (Linux, Default) | C++ API (Pi5) |
| :----------: | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: | :-----------: |
| **Required protobuf** | — | — | — | — | **3.20.0** | **3.6.1** | **3.12.4** | **3.21** | **3.20.0** | **3.20.0** |
| [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-local-public/kortex/link6/3.4.0/cobot-3.4.0-r.3.swu) | [release notes](https://artifactory.kinovaapps.com:443/artifactory/generic-documentation-public/Documentation/Link%206/Technical%20documentation/User%20Guide/EN-eRN-020-Link-6-release-notes.pdf) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/kortex_api-3.4.0.15-py3-none-any.whl) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/kortex_api_cpp_3.4.0-documentation.zip) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/kortex_api_python_3.4.0-documentation.zip) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/jetson.zip) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/ubuntu_20.zip) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/ubuntu_22.zip) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/ubuntu_24.zip) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/proto_3_20_0.zip) | [3.4.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.4.0/rpi5.zip) |
| [3.3.0](https://artifactory.kinovaapps.com/ui/native/generic-local-public/kortex/link6/3.3.0/link6-3.3.0-r.6.swu) | [release notes](https://artifactory.kinovaapps.com:443/artifactory/generic-documentation-public/Documentation/Link%206/Technical%20documentation/User%20Guide/EN-eRN-020-Link-6-release-notes.pdf) | [3.3.0](https://artifactory.kinovaapps.com/ui/native/generic-public/kortex/API/3.3.0/kortex_api-3.3.0.2-py3-none-any.whl) | - | - | - | - | - | - | - | - |
| [3.2.0](https://artifactory.kinovaapps.com:443/artifactory/generic-local-public/kortex/link6/3.2.0/link6-3.2.0-r.38.swu) | [release notes](https://artifactory.kinovaapps.com:443/artifactory/generic-documentation-public/Documentation/Link%206/Technical%20documentation/User%20Guide/EN-eRN-020-Link-6-release-notes.pdf) | [3.2.0](https://artifactory.kinovaapps.com/artifactory/generic-public/kortex/API/3.2.0/kortex_api-3.2.0.9-py3-none-any.whl) | - | - | - | - | - | - | - | - |

</details>
<a id="markdown-build-and-run-instructions" name="build-and-run-instructions"></a>

# Build and Run instructions

[Python API](./api_python/README.md)

[C++ API](./api_cpp/README.md) 

<a id="markdown-reference" name="reference"></a>
# Reference
<a id="markdown-useful-links" name="useful-links"></a>
#### Useful Links
|  |  |
| ---: | --- |
| Kinova home page: | [https://www.kinovarobotics.com](https://www.kinovarobotics.com)|
| Google Protocol Buffers home page: | [https://developers.google.com/protocol-buffers](https://developers.google.com/protocol-buffers) |
