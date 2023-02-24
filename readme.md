<!--
* KINOVA (R) KORTEX 3(TM)
*
* Copyright (c) 2018 Kinova inc. All rights reserved.
*
* This software may be modified and distributed
* under the terms of the BSD 3-Clause license.
*
* Refer to the LICENSE file for details.
*
-->

<h1>KINOVA<sup>®</sup> KORTEX 3™ API Reference</h1>

<a id="markdown-description" name="description"></a>
# Description

The official repository contains documentation and examples explaining how to use the KINOVA<sup>®</sup> KORTEX 3™ API client with Python. The repository has been tested on Windows 10 and Ubuntu 20.04.

<h1>Table of Contents</h1>

<!-- TOC -->

- [Description](#description)
- [Licensing](#licensing)
- [Role of Google Protocol Buffer in Kortex API](#role-of-google-protocol-buffer-in-kortex-api)
  - [Quick Start for Python users](#quick-start-for-python-users)
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
# Role of Google Protocol Buffer in Kortex API 

The Kortex API uses Google Protocol Buffer message objects<sup>**[1](#useful-links)**</sup> to exchange data between client and server.  

Google Protocol Buffer offers structured data objects with standard methods for each member field:  
+ structured, nested objects
+ basic types and collections
+ getter/setter methods on basic types
+ iterators, dimension and appending methods on collections
+ many helpers (e.g. serialize/deserialize, I/O functions)
  

When using the Kortex API a developer will need to understand the Google Protocol Buffer feature set to maximize their efficiency.  


<a id="markdown-quick-start-howto-python" name="quick-start-howto-python"></a>
## Quick Start for Python users

  To run the Python examples you will need to install the Python interpreter and the pip installation module.

  Here is some general information about the Python interpreter and the pip module manager.  
  - [Python General Information](./linked_md/python_quick_start.md)
  - [API mechanism](./linked_md/python_api_mechanism.md)
  - [Transport / Router / Session / Notification](./linked_md/python_transport_router_session_notif.md)
  - [Device routing](./linked_md/python_device_routing.md)
  - [Error management](./linked_md/python_error_management.md)
  - [Examples](./api_python/readme.md)

<a id="markdown-quick-start-howto-modbus" name="quick-start-howto-modbus"></a>
## Quick Start for Modbus and Ethernet Ip Users
The robot handle the communication via a modbus interface and a ethernet ip interface. It is not done via the Kortex API.

To communicate with the arm you must use the FieldBusAdapter plugin that Kinova provide.

[Plugin Download](https://artifactory.kinovaapps.com:443/artifactory/generic-public/kortex/plugins/fieldbus_adapter/1.1.0/fieldbus_adapter_1.1.0-r.5.kp)

[Documentation](https://artifactory.kinovaapps.com:443/artifactory/generic-documentation-public/Documentation/Link%206/Plugins/Fieldbus%20Adapter/EN-UG-021-FieldBusAdapter-Plugin-user-guide-r1.1.pdf)




<a id="markdown-api-download-links" name="api-download-links"></a>
# Download links


| Firmware     | Release notes      | API |
| :----------: | :-----------: | :-----------:|
| [3.2.0](https://artifactory.kinovaapps.com:443/artifactory/generic-local-public/kortex/link6/3.2.0/link6-3.2.0-r.38.swu)   | [release notes](https://artifactory.kinovaapps.com:443/artifactory/generic-documentation-public/Documentation/Link%206/Technical%20documentation/User%20Guide/EN-eRN-020-Link-6-release-notes.pdf)    | [3.2.0](https://artifactory.kinovaapps.com/artifactory/generic-public/kortex/API/3.2.0/kortex_api-3.2.0.9-py3-none-any.whl)|

</details>
<a id="markdown-build-and-run-instructions" name="build-and-run-instructions"></a>

# Build and Run instructions

[Python API](./api_python/readme.md) 

<a id="markdown-reference" name="reference"></a>
# Reference
<a id="markdown-useful-links" name="useful-links"></a>
#### Useful Links
|  |  |
| ---: | --- |
| Kinova home page: | [https://www.kinovarobotics.com](https://www.kinovarobotics.com)|
| Google Protocol Buffers home page: | [https://developers.google.com/protocol-buffers](https://developers.google.com/protocol-buffers) |
