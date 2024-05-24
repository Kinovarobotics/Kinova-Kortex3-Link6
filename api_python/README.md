<!--
* KINOVA (R) KORTEX 3 (TM)
*
* Copyright (c) 2018 Kinova inc. All rights reserved.
*
* This software may be modified and distributed
* under the terms of the BSD 3-Clause license.
*
* Refer to the LICENSE file for details.
*
-->

<h1>Python examples</h1>

<a id="markdown-description" name="description"></a>
# Description

This folder contains the Python API installation process, as well as examples going over the main functionalities of the API. 
The full documentation of the API is available at https://docs.kinovarobotics.com/index.html .
If you have any questions, reach out to us at support@kinova.ca.
<h2>Table of Contents</h2>

<!-- TOC -->

- [Description](#description)
- [Setup (Python environment)](#setup-python-environment)
  - [Required Python version and module](#required-python-version-and-module)
  - [Install Kortex Python API and required dependencies](#install-kortex-python-api-and-required-dependencies)
- [How to use the examples](#how-to-use-the-examples)
- [Reference](#reference)
  - [useful links](#useful-links)
- [Back to root topic: **readme.md**](#back-to-root-topic-readmemd)

<!-- /TOC -->

<a id="markdown-setup-example-python-environment" name="setup-example-python-environment"></a>
# Setup (Python environment)

<a id="markdown-requested-basic-python--python-modules" name="requested-basic-python--python-modules"></a>
## Required Python version and module

- Python >= 3.9
- pip

Refer to the python quick start for Python installation details: [python_quick_start](../linked_md/python_quick_start.md)

<a id="markdown-install-python-module-kortex-api--the-needed-dependencies" name="install-python-module-kortex-api--the-needed-dependencies"></a>
## Install Kortex Python API and required dependencies  

Install what is needed to run the examples via a downloaded whl file (Python wheel package).

The whl file can be downloaded via the Kinova Artifactory: [kortex_api](https://artifactory.kinovaapps.com/artifactory/generic-public/kortex/API/3.2.0/kortex_api-3.2.0.9-py3-none-any.whl)  

On Linux:

```sh
python3 -m pip install <whl relative fullpath name>.whl
```
**Note:** root privilege is usually required to install a new module under Linux.

On Windows:

```sh
python -m pip install <whl relative fullpath name>.whl
```

<a id="markdown-how-to-use-examples-with-your-robot" name="how-to-use-examples-with-your-robot"></a>
# How to use the examples

We assume the robot is using its default IP address: ``192.168.1.10``

If you don't use the default IP address, you can add the argument --ip with the right IP of the robot.
```sh
python3 --ip 192.168.2.22 <example-file>.py
```


**Before starting, ensure that you run the test in a safe area since some examples contain wide movements at full speed (default behaviour, but you can reduce speeds by modyfing the example code). Also, verify that your robot is correctly fixed to the working surface.**

Prerequisites:
+ The examples require a wired network connection to your computer
+ Configure a static IP address on your network interface (e.g. ``192.168.1.11/24``)

Some examples may require json programs to be uploaded on your controller to be able to run as is. You can either find these programs in 400-Json_programs, or modify the examples to use your own programs.

Now you're ready to run the examples. Each Python example has its own ``__main__`` instruction. You need only to ``cd`` into the example directory and invoke the Python file using the command:

- On Linux:
```sh
python3 <example-file>.py
```
- On Windows:
```sh
python <example-file>.py
```

<a id="markdown-reference" name="reference"></a>
# Reference
<a id="markdown-useful-links" name="useful-links"></a>
#### useful links
|  |  |  
| ---: | --- |  
| Google Proto Buffer generated Python: | [https://developers.google.com/protocol-buffers/docs/reference/python-generated](https://developers.google.com/protocol-buffers/docs/reference/python-generated) | 

__________________________
<a id="markdown-back-to-root-topic-readmemd" name="back-to-root-topic-readmemd"></a>
## Back to root topic: **[readme.md](../../readme.md)**  
