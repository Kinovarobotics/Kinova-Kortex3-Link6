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

<h1>C++ API mechanism</h1>

<h2>Table of Contents</h2>

<!-- TOC -->

- [Overview](#overview)
- [Blocking method](#blocking-method)
- [Async method](#async-method)
- [Callback method](#callback-method)

<!-- /TOC -->

<a id="markdown-overview" name="overview"></a>
## Overview

The C++ KINOVA KORTEX™ 3 API offers three mechanisms to call a service method:

| Mechanism | Method suffix | Returns |
|-----------|--------------|---------|
| Blocking  | *(none)*     | Response message directly |
| Async     | `_async`     | `std::future<Response>` |
| Callback  | `_callback`  | `void` (result delivered to a callback) |

All three mechanisms are generated for every service method.

<a id="markdown-blocking-method" name="blocking-method"></a>
## Blocking method

The blocking call is the simplest mechanism. The function is called and the current thread waits until the robot responds or the request times out.

An optional `RouterClientSendOptions` can be passed to override the default timeout.

```cpp
#include <BaseClientRpc.h>
#include <RouterMQTT.h>
#include <SessionManager.h>

namespace k_api = Kinova::Api;

// Default call — uses router default timeout
auto programList = base.GetProgramList();
for (const auto& program : programList.programs())
{
    std::cout << "Program: " << program.name() << "\n";
}

// Call with custom timeout
k_api::RouterClientSendOptions options;
options.timeout_ms = 5000; // 5 seconds
auto programList2 = base.GetProgramList(0, options);
```

<a id="markdown-async-method" name="async-method"></a>
## Async method

The async method returns a `std::future<Response>`. The current thread is not blocked — it can continue doing work and retrieve the result later by calling `.get()` on the future.

```cpp
// Fire the request without blocking
std::future<k_api::ProgramConfig::ProgramList> futurePrograms =
    base.GetProgramList_async();

// ... do other work here ...

// Retrieve the result (blocks until available)
auto programList = futurePrograms.get();
for (const auto& program : programList.programs())
{
    std::cout << "Program: " << program.name() << "\n";
}
```

<a id="markdown-callback-method" name="callback-method"></a>
## Callback method

The callback method delivers the result asynchronously to a user-supplied function. The callback receives both an `Error` and the response message.

```cpp
base.GetProgramList_callback(
    [](const k_api::Error& err, const k_api::ProgramConfig::ProgramList& programList)
    {
        if (err.error_code() != k_api::ErrorCodes::ERROR_NONE)
        {
            std::cerr << "Error: " << err.error_string() << "\n";
            return;
        }
        for (const auto& program : programList.programs())
        {
            std::cout << "Program: " << program.name() << "\n";
        }
    });
```

__________________________
## Back to root topic: **[readme.md](../readme.md)**
