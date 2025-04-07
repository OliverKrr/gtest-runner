# gtest-runner

![license](https://img.shields.io/badge/license-MIT-orange.svg) ![copyright](https://img.shields.io/badge/%C2%A9-Nic_Holthaus-orange.svg) ![copyright](https://img.shields.io/badge/%C2%A9-Oliver_Karrenbauer-orange.svg) ![language](https://img.shields.io/badge/language-c++-blue.svg) ![c++](https://img.shields.io/badge/std-c++20-blue.svg) ![Qt](https://img.shields.io/badge/Qt-6-blue.svg)<br>![msvc2022](https://img.shields.io/badge/MSVC-2022-ff69b4.svg) ![gcc-13](https://img.shields.io/badge/GCC-13-ff69b4.svg)

A Qt6 based automated test-runner and Graphical User Interface for Google Test unit tests.
This project is a fork of [nholthaus/gtest-runner](https://github.com/nholthaus/gtest-runner) with further improvements and adaptions.

# Table of Contents

<!-- TOC -->

- [gtest-runner](#gtest-runner)
- [Table of Contents](#table-of-contents)
- [Light Theme](#light-theme)
- [Dark Theme](#dark-theme)
- [Features](#features)
- [Supported Platforms](#supported-platforms)
- [Installers](#installers)
    - [Windows](#windows)
    - [Ubuntu](#ubuntu)
    - [Other Linux Distributions](#other-linux-distributions)
- [Build Instructions](#build-instructions)

<!-- /TOC -->

# Light Theme

![Light Theme Screenshot](resources/screenshots/screen.png)

# Dark Theme

![Dark Theme Screenshot](resources/screenshots/screen2.png)

# Features

`gtest-runner` is an automated test runner that will ensure you are always looking at the latest test results, whenever you build a gtest executable. Check the [features guide](FEATURES.md) to see what else `gtest-runner` is capable of.

# Supported Platforms

To date, gtest-runner has been tested on:

- Windows 11/10
- Ubuntu 24.04/22.04

# Installers

## Windows

Visit the [Latest release page](https://github.com/OliverKrr/gtest-runner/releases) for binary installers. 

Since version 1.1.2, all Windows binaries are signed.

## Ubuntu

Binary packages are available through the [gtest-runner PPA](https://launchpad.net/~oliverkrr/+archive/ubuntu/gtest-runner) for many versions of Ubuntu. To install gtest-runner via the PPA, type the following commands:

```bash
sudo add-apt-repository ppa:oliverkrr/gtest-runner
sudo apt-get update
sudo apt-get install gtest-runner
```

Of course, you can also [build gtest-runner from source](BUILD.md).

## Other Linux Distributions

Please see the instructions on [how to build gtest-runner from source](BUILD.md).

# Build Instructions

See the [Build Instructions](BUILD.md) for information on how to build gtest-runner from source on your platform.
