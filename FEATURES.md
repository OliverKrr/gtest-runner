# FEATURES - Table of Contents

<!-- TOC -->

- [FEATURES - Table of Contents](#features-table-of-contents)
- [Gtest-runner](#gtest-runner)
	- [Automatic test running](#automatic-test-running)
	- [Simple traffic-light output](#simple-traffic-light-output)
	- [Full console output](#full-console-output)
	- [Fully customizable layout](#fully-customizable-layout)
	- [Light and dark themes](#light-and-dark-themes)
	- [System notifications](#system-notifications)
	- [Gtest support](#gtest-support)
- [Test Executable dock](#test-executable-dock)
	- [Drag-and-drop](#drag-and-drop)
	- [Easily add and remove tests](#easily-add-and-remove-tests)
	- [Easily update tests](#Easily-update-tests)
	- [Re-run test](#re-run-test)
	- [Re-run specific Test Suite or Case](#re-run-specific-test-suite-or-case)
	- [Kill test](#kill-test)
	- [gtest command line options](#gtest-command-line-options)
	- [Enable/disable autorun](#enabledisable-autorun)
	- [Test progress](#test-progress)
	- [Test path tooltips](#test-path-tooltips)
- [Test case window](#test-case-window)
	- [Detailed failure information](#detailed-failure-information)
	- [Sort failures](#sort-failures)
	- [Filter failures](#filter-failures)
- [Failure window](#failure-window)
	- [Jump to console output](#jump-to-console-output)
	- [Open in IDE](#open-in-ide)
	- [Full GTest error message](#full-gtest-error-message)
- [Console Window](#console-window)
	- [Search](#search)
	- [Next/previous failure](#nextprevious-failure)
	- [Clear output from previous runs](#clear-output-from-previous-runs)
	- [Cout](#cout)

<!-- /TOC -->

# Gtest-runner

![](resources/screenshots/screen.png)

A Qt6 based automated test-runner and Graphical User Interface for our Google Test unit tests.

## Automatic test list update

![Automatic test list update](resources/screenshots/feature1.png)

- Enable `Options -> Auto-Update the Test List after Build` to automatically update the available test list
- Gtest-runner uses file system watchers do detect every time your test has changed (e.g. when you build it), and automatically update the available test list. This feature even works if you rebuild your test when gtest-runner is closed.
- Use `Update All Tests List` and `Update Test List` to manually update the available test list

## Automatic test running

![Automatic test running](resources/screenshots/feature0.png)

- Enable check-box beside test executable to execute tests automatically after each build
- Gtest-runner uses file system watchers do detect every time your test has changed (e.g. when you build it), and automatically re-runs the test and updates all the test case windows. This feature even works if you rebuild your test when gtest-runner is closed.
- You can decide in `Options -> Run tests synchronous and not parallel` to run tests sequential or in parallel (per number of core). Running tests in parallel can slow down your system significantly.
- _Note:_ for expensive or lengthy tests, automatic test running can be disabled by unchecking the box next to the test name.
- _Note 2:_ use *Toggle auto-run* for fast switch between auto-run state for all tests.

## Simple traffic-light output

![Traffic light](resources/screenshots/trafficlight.png)

- <img src="resources/images/green.png" height=20 align="center"/> - A green light means that the test was run and all test cases passed with no errors.
- <img src="resources/images/yellow.png" height=20 align="center"/> - A yellow light means that the test has changed on disc and needs to be re-run. If your test executable is corrupted, deadlocks, or segfaults, you may see a test in a "hung" yellow state.
- <img src="resources/images/red.png" height=20 align="center"/> - A red light means that the test executable failed at least one test case. 
- <img src="resources/images/gray.png" height=20 align="center"/> - A gray light indicates that the test is disabled, _or_ if autorun is disabled it can also indicate that test output is out-of-date.

## Full console output

![console](resources/screenshots/console.png)

- All gtest console output is retained (including syntax highlighting) and piped to the built-in console dock, so you get all the benefits of a GUI without losing any of your debugging output. Plus, `gtest-runner` adds additional capabilities like a [search dialog](#search-failures) and [forward and backward failure navigation buttons](#nextprevious-failure).
- _Note:_ you can enable all test output (std::cout, std::cerr) in `Options -> Pipe all test output to Console Output`. Some tests have a lot of console output.

## Fully customizable layout

![tab](resources/screenshots/tab.png)

- Gtest-runner uses detachable docks for the test executable, failure, and console docks, so you can tab, rearrange, detach, or hide them to the best suite your screen layout.

## Light and dark themes

![light theme](resources/screenshots/theme.png) ![dark theme](resources/screenshots/theme2.png)

- Easily switch between your system's default color theme, light or a dark theme.

## System notifications

![System Notification](resources/screenshots/systemNotification.png)

- When a test is autorun in the background, gtest-runner will generate a system notification letting you know if any tests failed. Notifications can also (optionally) be configured for tests which pass during autorun.
- _Note:_ notifications aren't generated for tests which are run manually (since you are presumably already looking at the output).

## Gtest support

- Gtest-runner supports both `gtest-1.7.0` and `gtest-1.8.0` style output.

# Test Executable dock

![Test Executable Dock](resources/screenshots/testExecutableDock.png)

## Drag-and-drop

- To add a test executable to watch, simply drag-and-drop the test executable file anywhere on the `gtest-runner` GUI.

## Easily add and remove tests

![Add/remove](resources/screenshots/addRemove.png)

- In addition to drag-and-drop, it's easy to add or remove a test using the right-click context menu in the `Test Executable` window.

## Easily update tests

![Test Executable Dock](resources/screenshots/addEnv.png)

- Add all unit tests by selecting your `RunEnv.bat/sh`
- If `RunEnv.bat/sh` is not required use a dummy script
- Click _Update Tests_ searching for `TestDriver.py` to update the executable list (e.g. new test targets added/build/removed)
- You can start multiple instances of gtest-runner for each RunEnv.bat/sh dir

## Re-run test

![Re-run test](resources/screenshots/run.png)

- You can manually re-run a test at any time by right-clicking on it and selecting `Run Test` or with `F5`
- You can run all tests with corresponding button in toolbar or with `F6` (toggle `Run tests synchronous and not parallel` option to disable parallel test execution)
- Use `Run Failed Tests` (`F3`) and `Run All Failed Tests` (`F4`) to re-run only the failed tests
	- This will create a temporary `gtest-filter` to run only the failed tests
    - The temporary filter is logged to the console

## Re-run specific Test Suite or Case

![Re-run Test Case](resources/screenshots/run2.png)
![Re-run Test Suite](resources/screenshots/run3.png)

- Runs the test suite or case with a temporary `gtest-filter`. So that only a sub-set of tests are run
- The temporary filter is logged to the console

## Kill test

![Kill test](resources/screenshots/kill.png)

- Did you just actually click run on your 4-hour CPU intensive test? No problem! Cancel a test at any time by right-clicking on it, and selecting `Kill Test` or `Shift + F5`.
- _Hint:_ if the `Kill Test` option is grayed out, that means your test isn't running (even if it has a yellow indicator).
- Use corresponding button in toolbar or `Shift + F6` to kill all running tests

## gtest command line options

![gtest command line](resources/screenshots/commandLine.png)

- Clicking on the hamburger menu to the left of the test executable name will pop up the `gtest command line` dialogue, allowing access to all the options you'd have if you ran your test directly from a terminal.

## Enable/disable autorun

![disable autorun](resources/screenshots/disableAutorun.png)

- Compiling frequently, or have a resource-intensive test that you don't want running in the background? Whatever the reason, it's simple to disable autorun by un-checking the autorun box next to the test executable name.

## Test progress

![Progress](resources/screenshots/progress.png)

- Each running test displays a progress bar indicator. Measured progress is based on the number of tests completed out of the overall number of tests, not time-to-go.

## Test path tooltips

![Path](resources/screenshots/path.png)

- Mousing over any of the test executables will show the full path to the test as a tooltip. This can be useful for disambiguating multiple tests with the same name. On Windows, `Debug` and `RelWithDebInfo` builds are differentiated automatically.

# Test case window

![Test Case Window](resources/screenshots/testCase.png)

## Detailed failure information

![Detailed failure information](resources/screenshots/detailedFailureInfo.png)

- Clicking on any failed test case will display detailed information about each `EXPECT/ASSERT` failure in the `Failures` window.
- _Note:_ No detailed information is available for test cases which pass or are disabled.

## Sort failures

![Sort](resources/screenshots/sort.png)

- The test case list can be sorted by any column. The default sort is the to show the tests in order. To change sort, just click on any of the column headers. Click the header again the alternate between ascending/descending order.
- _Note:_ The current sort column is indicated by the arrow of above the column title (in this example, `Failures`).

## Filter failures

- Use `Show Not Executed`, `Show Passed` and `Show Ignored` to quickly toggle between the different test types

## Filter tests

![filter](resources/screenshots/filter.png)

- Filter the test cases displayed by typing into the filter text box. Only matching results will be shown.
- _Note:_ The filter text edit accepts regex filters as well!

# Failure window

![failure window](resources/screenshots/failureWindow.png)

## Jump to console output

![Jump to console output](resources/screenshots/jumpToConsole.png)

- Single clicking any failure in the failure window will automatically jump to the corresponding output in the gtest console.

## Open in IDE

- Double-clicking any failure will automatically open the file in your IDE (or whichever program is associated with the file extension). It will also copy the line number of the failure to the clipboard. In almost every editor, you can then quickly jump to the failure location with the shortcut `Ctrl-G, Ctrl-V, ENTER`.

## Full GTest error message

![Full error](resources/screenshots/fullError.png)

- Hovering the mouse over any failure will show a tool-tip with the original, un-parsed gtest error message.

# Console Window

![Console](resources/screenshots/console.png)

## Search

![Find](resources/screenshots/find.png)

- Open a `Find` window with the keyboard shortcut `Ctrl-F`, or by right-clicking in the console window and selecting it from the menu. You can use the `Find` dialog to search the console output for specific failures or `std::cout` messages.

## Next/previous failure

![Next/previous failure](resources/screenshots/nextprev.png)

- Use the buttons on the left side of the console window to quickly scroll to the next or previous failure.

## Clear output from previous runs

![Clear](resources/screenshots/clear.png)

- Is your console output getting cluttered with too many runs? Easily clear it by selecting the clear option from the right-click context menu.

## Cout

![Cout](resources/screenshots/cout.png)

- Enable the `Pipe all test output to Console Output` option to pipe all output (std::cout/cerr) from the test to the console output.
