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

<h1>Error management</h1>

<h2>Table of Contents</h2>

<!-- TOC -->

- [Overview](#overview)
	- [Example](#example)
- [Special case](#special-case)
	- [Router error callback](#router-callback)

<!-- /TOC -->

<a id="markdown-overview" name="overview"></a>
## Overview

The C++ KINOVA KORTEX™ 3 API uses exceptions to report errors. Wrap service calls in a **try/catch** block and react to the exception type.

Two exception classes are provided:

| Exception | When thrown |
|-----------|-------------|
| `KDetailedException` | Error reported by the robot (server-side) |
| `KBasicException`    | Error detected on the client side (transport, timeout, etc.) |

Both derive from `std::exception`, so a single `catch (std::exception&)` can serve as a fallback.

<a id="markdown-exception-example" name="example"></a>
### Example

```cpp
#include <KDetailedException.h>
#include <BaseClientRpc.h>

namespace k_api = Kinova::Api;

k_api::Base::BaseClient base(router.get());

try
{
    k_api::Base::FullUserProfile profile;
    base.CreateUserProfile(profile);
}
catch (k_api::KDetailedException& ex)
{
    // Server-side error: contains error_code and sub_error_code
    auto& err = ex.getErrorInfo().getError();
    std::cerr << "KDetailedException — code: " << err.error_code()
              << "  sub-code: "                << err.error_sub_code()
              << "  msg: "                     << ex.what() << "\n";
}
catch (k_api::KBasicException& ex)
{
    // Client-side error (timeout, transport failure, etc.)
    std::cerr << "KBasicException: " << ex.what() << "\n";
}
catch (std::exception& ex)
{
    std::cerr << "Unexpected error: " << ex.what() << "\n";
}
```

A `KDetailedException` is thrown when the robot reports an error. It carries error code and sub-error code information describing the failure. A `KBasicException` is thrown when the error occurs on the client side (e.g. timeout or transport failure).

<a id="markdown-special-cases" name="special-case"></a>
## Special case

This section describes a case that does not follow the standard error management rules above.

<a id="markdown-special-router-client" name="router-callback"></a>
### Router error callback

When a `RouterMQTT` or `RouterClient` object is instantiated, an error callback must be provided. This callback is invoked if an unrecoverable transport-level exception occurs outside of a normal service call (e.g. unexpected disconnection).

```cpp
#include <RouterMQTT.h>

namespace k_api = Kinova::Api;

auto router = std::make_shared<k_api::RouterMQTT>(
    robotIp, mqttPort, "",
    [](k_api::KError err)
    {
        std::cerr << "Router error: " << err.toString() << "\n";
    });
```

__________________________
## Back to root topic: **[readme.md](../readme.md)**
