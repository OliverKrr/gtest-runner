//--------------------------------------------------------------------------------------------------
//
///	@PROJECT	project
/// @BRIEF		brief
///	@DETAILS	details
//
//--------------------------------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//--------------------------------------------------------------------------------------------------
//
// ATTRIBUTION:
// Parts of this work have been adapted from:
//
//--------------------------------------------------------------------------------------------------
//
// Copyright (c) 2021 Oliver Karrenbauer
//
//--------------------------------------------------------------------------------------------------

#pragma once

#include <QThreadPool>


const QString TEST_DRIVER_NAME = "TestDriver.py";
const QString GTEST_RESULT_NAME = "gtest-runner_result.xml";
const QString LATEST_RESULT_DIR_NAME = "latest";
const QString DATE_FORMAT = "yyyy.MM.dd_hh.mm.ss.zzz";
const int MAX_PARALLEL_TEST_EXEC = QThreadPool::globalInstance()->maxThreadCount();
constexpr std::size_t MAX_HISTORY_TEST_RESULTS = 10;


QString settingsPath();

QString dataPath();

QString xmlPath(const QString& testPath, bool create = false);
