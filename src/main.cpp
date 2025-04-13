//--------------------------------------------------------------------------------------------------
// 
/// @PROJECT	gtest-runner
/// @BRIEF		main file for gtest-runner
/// @DETAILS	
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
// Copyright (c) 2016 Nic Holthaus
// Copyright (c) 2024 Oliver Karrenbauer
// 
//--------------------------------------------------------------------------------------------------


//------------------------------
//	INCLUDE
//------------------------------

// Qt
#include <QApplication>
#include <QDebug>
#include <QtGlobal>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStyleFactory>

// gtest-runner
#include "appinfo.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(resources);

	const QApplication app(argc, argv);
	QApplication::setStyle(QStyleFactory::create("Fusion"));
	QApplication::setOrganizationName(APPINFO::organization);
	QApplication::setOrganizationDomain(APPINFO::oranizationDomain);
	QApplication::setApplicationName(APPINFO::name);
	QApplication::setApplicationVersion(APPINFO::version);

	QCommandLineParser parser;

	const QCommandLineOption addTestsOption(QStringList() << "a" << "add", "Add tests executables (comma separated)", "tests", "");
	const QCommandLineOption resetOption(QStringList() << "r" << "reset", "Reset gtest-runner to it's original factory settings. This removes all tests and test data.");

	parser.setApplicationDescription("An automated test runner and user interface for google test unit tests.");
	const auto helpOption = parser.addHelpOption();
	const auto versionOption = parser.addVersionOption();
	parser.addOption(addTestsOption);
	parser.addOption(resetOption);
	
	parser.process(app);

	if (parser.isSet(helpOption))
	{
		parser.showHelp();
		exit(0);
	}

	if (parser.isSet(versionOption))
	{
		parser.showVersion();
		exit(0);
	}

	const bool reset = parser.isSet(resetOption);
	const QStringList tests = parser.value(addTestsOption).split(',');

	MainWindow mainWin(tests, reset);
	mainWin.show();
	return QApplication::exec();
}