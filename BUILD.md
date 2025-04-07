# BUILD - Table of Contents

<!-- TOC -->

- [BUILD - Table of Contents](#build-table-of-contents)
- [Prerequisites](#prerequisites)
- [Windows Build](#windows-build)
- [Ubuntu 24.04 Noble Numbat Build](#ubuntu-2404-noble-numbat-build)
- [Linux Build](#linux-build)
- [OS X Build](#os-x-build)

<!-- /TOC -->

# Prerequisites

Building the gtest-runner from source requires having the following programs installed

* A working compiler
    * Windows:
        * _Recommended_: [Visual Studio 2022](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx).
    * Linux:
        * _Recommended_: gcc 13.3.0
* Qt
    * _Recommended_: [Qt 6.9](http://www.qt.io/download/)
    * You need to adapt the path of _QT_ROOT_DIR_ in _BuildEnv.bat_
* CMake
    * _Recommended_: [CMake 4.0.0](https://cmake.org/download/)
* _OPTIONAL_
    * git 2.49.0 or later
    * NSIS 3.10 or later if you want to build the .exe installer.

# Windows Build

These steps have been tested on Windows 11, but should be similar or the same for Windows 10.

_Make sure you've installed all the [prerequisites](#prerequisites) before proceeding!_

1. Clone this repo: `git clone https://github.com/OliverKrr/gtest-runner.git`
2. Start _BuildEnv.bat_
3. Ensure that the `QT_ROOT_DIR` environment variable is properly set by typing `echo %QT_ROOT_DIR%`. You should see something like

    > C:\Qt\6.9.0\msvc2022_64

    * If your `QT_ROOT_DIR` is not set, then set it using `set QT_ROOT_DIR=[path\to\Qt]`, where
  `[path\to\Qt] is replaced with your install path, example: `set QT_ROOT_DIR=C:\Qt\6.9.0\msvc2022_64`

4. Ensure the cmake path is visible to the command line by typing `cmake --version`. If you see

    > cmake version 4.0.0

    or similar, proceed to step 6. Otherwise, determine the path to you cmake install, and add it to the path:

    `set PATH="C:\Program Files\CMake\bin";%PATH%` (be sure to use the proper path for YOUR install)

5. Type the following commands into the prompt, and press enter after each one

   * `sh make.sh`
   * you can now run `gtest-runner.exe` from the `build\RelWithDebInfo` directory!

6. (optional) If you have `NSIS` installed, you can create the installer package and install gtest-runner into your
   `Program Files`.

   * `sh release.sh`
   * `cd Release`
   * Run the installer using `gtest-runner-v[VERSION]-[TARGET].exe`, where version is the gtest-version that you downloaded, and target is win32 or win64 depending on your platform. If you're not sure what to use, type the `dir` command to see which executable was generated.
       * example: `gtest-runner-v1.4.0-win64.exe`

The installer is used to create program shortcuts and links in your application menu. If you prefer not to use the installer (or can't use it), you can still run `gtest-runner.exe` directly from the Release directory.

# Ubuntu 24.04 Noble Numbat Build

1. Open a terminal window
2. Clone the repository and checkout the latest version of the code

   - `git clone https://github.com/OliverKrr/gtest-runner.git`
   - `cd ~/gtest-runner`
   - `git tag -l` to see the available versions
   - `git checkout [version]`, where [version] was the newest version tag, example: `git checkout v1.4.0`

3. Build and install the code

   - `cd ~/gtest-runner`
   - `mkdir build`
   - `cd build`
   - `cmake -DCMAKE_BUILD_TYPE=Release ..`
   - `make` (or use `make -j2` if you have a dual-core machine)
   - `sudo make install`

You can now run the gtest-runner by typing `gtest-runner` into your console.

# Linux Build

Follow the instructions for the [Ubuntu 24.04 Noble Numbat Build](#ubuntu-2404-noble-numbat-build), with the following  exceptions:

Steps 3-5:

- use your systems package manager to get the prerequisites.
- if they are not available, download and build them from source. Make sure to set your `QT_ROOT_DIR`. Make sure your `PATH` variable includes the cmake `bin` directory.

# OS X Build

No testing has been done with gtest-runner/OS X to date. If you're interested in using gtest-runner on a Mac, get in contact.
