#include "GTestFailureModel.h"
#include "QStdOutSyntaxHighlighter.h"
#include "mainwindow_p.h"
#include "executableModelDelegate.h"

#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QTimer>
#include <QScrollBar>
#include <QStack>
#include <QStyle>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


namespace
{
static const QString GTEST_RESULT_NAME = "gtest-runner_result.xml";
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: MainWindowPrivate
//--------------------------------------------------------------------------------------------------
MainWindowPrivate::MainWindowPrivate(QStringList tests, bool reset, MainWindow* q) :
	q_ptr(q),
        toolBar_(new QToolBar(q)),
        runEnvComboBox_(new QComboBox(toolBar_)),
        runEnvModel_(new QStringListModel(toolBar_)),
	executableDock(new QDockWidget(q)),
	executableDockFrame(new QFrame(q)),
	executableTreeView(new QExecutableTreeView(q)),
	executableModel(new QExecutableModel(q)),
	testCaseProxyModel(new QBottomUpSortFilterProxy(q)),
        updateTestsButton(new QPushButton(q)),
	fileWatcher(new QFileSystemWatcher(q)),
	centralFrame(new QFrame(q)),
	testCaseFilterEdit(new QLineEdit(q)),
	testCaseTreeView(new QTreeView(q)),
	statusBar(new QStatusBar(q)),
	failureDock(new QDockWidget(q)),
	failureTreeView(new QTreeView(q)),
	failureProxyModel(new QBottomUpSortFilterProxy(q)),
	consoleDock(new QDockWidget(q)),
	consoleTextEdit(new QTextEdit(q)),
	consoleFrame(new QFrame(q)),
	consoleButtonLayout(new QVBoxLayout()),
	consoleLayout(new QHBoxLayout()),
	consolePrevFailureButton(new QPushButton(q)),
	consoleNextFailureButton(new QPushButton(q)),
	consoleHighlighter(new QStdOutSyntaxHighlighter(consoleTextEdit)),
	consoleFindDialog(new FindDialog(consoleTextEdit)),
	systemTrayIcon(new QSystemTrayIcon(QIcon(":/images/logo"), q)),
	mostRecentFailurePath("")
{
	qRegisterMetaType<QVector<int>>("QVector<int>");

	if (reset)
	{
		clearData();
		clearSettings();
	}

	QFontDatabase fontDB;
	fontDB.addApplicationFont(":/fonts/consolas");
	QFont consolas("consolas", 10);

	centralFrame->setLayout(new QVBoxLayout);
	centralFrame->layout()->addWidget(testCaseFilterEdit);
	centralFrame->layout()->addWidget(testCaseTreeView);
	centralFrame->layout()->setContentsMargins(0, 5, 0, 0);

	executableDock->setObjectName("executableDock");
	executableDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	executableDock->setWindowTitle("Test Executables");
	executableDock->setWidget(executableDockFrame);

	executableTreeView->setModel(executableModel);
	executableTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
	executableTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
 	executableTreeView->setDragDropMode(QTreeView::InternalMove);
	executableTreeView->setHeaderHidden(true);
	executableTreeView->setIndentation(0);
	executableTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	executableTreeView->setItemDelegateForColumn(QExecutableModel::ProgressColumn, new QProgressBarDelegate(executableTreeView));

	executableDockFrame->setLayout(new QVBoxLayout);
	executableDockFrame->layout()->addWidget(executableTreeView);
        executableDockFrame->layout()->addWidget(updateTestsButton);

        updateTestsButton->setText("Update tests (search RunEnv)");

	testCaseFilterEdit->setPlaceholderText("Filter Test Output...");
	testCaseFilterEdit->setClearButtonEnabled(true);

	testCaseTreeView->setSortingEnabled(true);
	testCaseTreeView->sortByColumn(GTestModel::TestNumber, Qt::AscendingOrder);
	testCaseTreeView->setModel(testCaseProxyModel);

	testCaseProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

	failureDock->setObjectName("failureDock");
	failureDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
	failureDock->setWindowTitle("Failures");
	failureDock->setWidget(failureTreeView);

	failureTreeView->setModel(failureProxyModel);
	failureTreeView->setAlternatingRowColors(true);

	consoleDock->setObjectName("consoleDock");
	consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
	consoleDock->setWindowTitle("Console Output");
	consoleDock->setWidget(consoleFrame);
	
	consoleFrame->setLayout(consoleLayout);
	
	consoleLayout->addLayout(consoleButtonLayout);
	consoleLayout->addWidget(consoleTextEdit);
	consoleTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	consoleButtonLayout->addWidget(consolePrevFailureButton);
	consoleButtonLayout->addWidget(consoleNextFailureButton);

	consolePrevFailureButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	consolePrevFailureButton->setMaximumWidth(20);
	consolePrevFailureButton->setIcon(q->style()->standardIcon(QStyle::SP_ArrowUp));
	consolePrevFailureButton->setToolTip("Show Previous Test-case Failure");
	
	consoleNextFailureButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	consoleNextFailureButton->setMaximumWidth(20);
	consoleNextFailureButton->setIcon(q->style()->standardIcon(QStyle::SP_ArrowDown));
	consoleNextFailureButton->setToolTip("Show Next Test-case Failure");
	
	consoleTextEdit->setFont(consolas);
	QPalette p = consoleTextEdit->palette();
	p.setColor(QPalette::Base, Qt::black);
	p.setColor(QPalette::Text, Qt::white);
	consoleTextEdit->setPalette(p);
	consoleTextEdit->setReadOnly(true);

	consoleFindDialog->setTextEdit(consoleTextEdit);
	
	systemTrayIcon->show();

	createTestMenu();
	createOptionsMenu();
	createWindowMenu();
	createThemeMenu();
	createHelpMenu();

	createExecutableContextMenu();
	createConsoleContextMenu();
	createTestCaseViewContextMenu();

        createToolBar();

	connect(this, &MainWindowPrivate::setStatus, statusBar, &QStatusBar::setStatusTip, Qt::QueuedConnection);
	connect(this, &MainWindowPrivate::testResultsReady, this, &MainWindowPrivate::loadTestResults, Qt::QueuedConnection);
	connect(this, &MainWindowPrivate::testResultsReady, statusBar, &QStatusBar::clearMessage, Qt::QueuedConnection);
	connect(this, &MainWindowPrivate::showMessage, statusBar, &QStatusBar::showMessage, Qt::QueuedConnection);

        // Update Tests
        connect(updateTestsButton, &QPushButton::clicked, [this]()
        {
            if (currentRunEnvPath_.isEmpty())
            {
                addRunEnvAction->trigger();
            }
            else
            {
                updateTestExecutables();
            }
        });

	// switch testCase models when new tests are clicked
	connect(executableTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& selected, const QItemSelection& deselected)
	{
		if(!selected.isEmpty())
		{
			auto index = selected.indexes().first();
			selectTest(index.data(QExecutableModel::PathRole).toString());
		}
	});

	// run the test whenever the executable changes
	connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path)
	{	
		QModelIndex m = executableModel->index(path);
		if (m.isValid())
		{
			// only auto-run if the test is checked
			if (m.data(QExecutableModel::AutorunRole).toBool())
			{
				executableModel->setData(m, QDateTime::currentDateTime(), QExecutableModel::LastModifiedRole);

				emit showMessage("Change detected: " + path + "...");
				// add a little delay to avoid running multiple instances of the same test build,
				// and to avoid running the file before visual studio is done writting it.
				QTimer::singleShot(500, [this, path] {emit runTestInThread(path, true); });
				
				// the directories tend to change A LOT for a single build, so let the watcher
				// cool off a bit. Anyone who is actually building there code multiple times
				// within 500 msecs on purpose is an asshole, and we won't support them.
				fileWatcher->blockSignals(true);
				QTimer::singleShot(500, [this, path] {emit fileWatcher->blockSignals(false); });
			}
			else
			{
				executableModel->setData(m, ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);
			}
		}
		
	});

	// run test when signaled to. Queued connection so that multiple quick invocations will
	// be collapsed together.
	connect(this, &MainWindowPrivate::runTest, this, &MainWindowPrivate::runTestInThread, Qt::QueuedConnection);

	// update filewatcher when directory changes
	connect(fileWatcher, &QFileSystemWatcher::directoryChanged, [this](const QString& path)
	{
		// This could be caused by the re-build of a watched test (which cause additionally cause the
		// watcher to stop watching it), so just in case add all the test paths back.
		this->fileWatcher->addPaths(executablePaths.filter(path));
	});

	// re-rerun tests when auto-testing is re-enabled
	connect(executableModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles)
	{
		QString path = topLeft.data(QExecutableModel::PathRole).toString();
		bool prevState = executableCheckedStateHash[path];

		// Only re-run IFF the check box state goes from unchecked to checked AND
		// the data has gotten out of date since the checkbox was off.
		if (topLeft.data(QExecutableModel::AutorunRole).toBool() && !prevState)
		{
			QFileInfo gtestResult(latestGtestResultPath(path));
			QFileInfo exe(path);

                        if (gtestResult.lastModified() < exe.lastModified())
			{
				// out of date! re-run.
				emit showMessage("Automatic testing enabled for: " + topLeft.data(Qt::DisplayRole).toString() + ". Re-running tests...");
				runTestInThread(topLeft.data(QExecutableModel::PathRole).toString(), true);
			}			
		}

		// update previous state
		executableCheckedStateHash[path] = topLeft.data(QExecutableModel::AutorunRole).toBool();
	}, Qt::QueuedConnection);

	// filter test results when the filter is changed
	connect(testCaseFilterEdit, &QLineEdit::textChanged, this, [this](const QString& text)
	{
		if (QRegExp(text).isValid())
		{
			testCaseProxyModel->setFilterRegExp(text);
			if(testCaseProxyModel->rowCount())
			{
				testCaseTreeView->expandAll();
				for (int i = 0; i < testCaseProxyModel->columnCount(); ++i)
				{
					testCaseTreeView->resizeColumnToContents(i);
				}
			}
		}
	});

	// create a failure model when a test is clicked
	connect(testCaseTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& selected, const QItemSelection& deselected)
	{
		if (selected.indexes().size() == 0)
			return;

		auto index = testCaseProxyModel->mapToSource(selected.indexes().first());
		DomItem* item = static_cast<DomItem*>(index.internalPointer());

		if (index.isValid())
		{
			if (index.data(GTestModel::FailureRole).toInt() > 0)
				failureTreeView->header()->show();
			else
				failureTreeView->header()->hide();
		}

		failureTreeView->setSortingEnabled(false);
		delete failureProxyModel->sourceModel();
		failureProxyModel->setSourceModel(new GTestFailureModel(item));
		failureTreeView->setSortingEnabled(true);
		for (int i = 0; i < failureProxyModel->columnCount(); ++i)
		{
			failureTreeView->resizeColumnToContents(i);
		}
	});

	// open failure dock on test double-click
	connect(testCaseTreeView, &QTreeView::doubleClicked, [this](const QModelIndex& index)
	{
		if (index.isValid())
			failureDock->show();
	});

	// copy failure line to clipboard (to support IDE Ctrl-G + Ctrl-V)
	// also, highlight it in the console model
	connect(failureTreeView, &QTreeView::clicked, [this](const QModelIndex& index)
	{
		if (index.isValid())
		{
			QApplication::clipboard()->setText(index.data(GTestFailureModel::LineRole).toString());
			// yay, the path strings are TOTALLY different between the different OS's
#ifdef _MSC_VER
			QString findString = QDir::toNativeSeparators(index.data(GTestFailureModel::PathRole).toString()) + "(" + index.data(GTestFailureModel::LineRole).toString() + ")";
#else
			QString findString = index.data(GTestFailureModel::PathRole).toString() + ":" + index.data(GTestFailureModel::LineRole).toString();
#endif
			consoleTextEdit->find(findString, QTextDocument::FindBackward);
			consoleTextEdit->find(findString);
			scrollToConsoleCursor();
		}
	});

	// open file on double-click
	connect(failureTreeView, &QTreeView::doubleClicked, [this](const QModelIndex& index)
	{
		if (index.isValid())
			QDesktopServices::openUrl(QUrl::fromLocalFile(index.data(GTestFailureModel::PathRole).toString()));
	});

	// display test output in the console window
	connect(this, &MainWindowPrivate::testOutputReady, this, [this](QString text)
	{
		// add the new test output
		consoleTextEdit->moveCursor(QTextCursor::End);
		consoleTextEdit->insertPlainText(text);
		consoleTextEdit->moveCursor(QTextCursor::End);
		consoleTextEdit->ensureCursorVisible();
	}, Qt::QueuedConnection);

	// update test progress
	connect(this, &MainWindowPrivate::testProgress, this, [this](QString test, int complete, int total)
	{
		QModelIndex index = executableModel->index(test);
		executableModel->setData(index, (double)complete / total, QExecutableModel::ProgressRole);
	});

	// open the GUI when a tray message is clicked
	connect(systemTrayIcon, &QSystemTrayIcon::messageClicked, [this]
	{		
		q_ptr->setWindowState(Qt::WindowActive);
		q_ptr->raise();
		if (!mostRecentFailurePath.isEmpty())
			selectTest(mostRecentFailurePath);
	});

	// find the previous failure when the button is pressed
	connect(consolePrevFailureButton, &QPushButton::pressed, [this, q]
	{
		QRegularExpression regex("\\[\\s+RUN\\s+\\].*?[\n](.*?): ((?!OK).)*?\\[\\s+FAILED\\s+\\]", QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);
		auto matches = regex.globalMatch(consoleTextEdit->toPlainText());

		QRegularExpressionMatch match;
		QTextCursor c = consoleTextEdit->textCursor();
		
		while(matches.hasNext())
		{		
			auto nextMatch = matches.peekNext();
			if(nextMatch.capturedEnd() >= c.position())
			{
				break;
			}
			match = matches.next();
		}
		
		if(match.capturedStart() > 0)
		{
			c.setPosition(match.capturedStart(1));
			consoleTextEdit->setTextCursor(c);
			scrollToConsoleCursor();
			c.setPosition(match.capturedEnd(1), QTextCursor::KeepAnchor);
			consoleTextEdit->setTextCursor(c);
		}
	});
	
	// find the next failure when the button is pressed
	connect(consoleNextFailureButton, &QPushButton::pressed, [this, q]
	{
		QRegularExpression regex("\\[\\s+RUN\\s+\\].*?[\n](.*?): ((?!OK).)*?\\[\\s+FAILED\\s+\\]", QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);
		auto matches = regex.globalMatch(consoleTextEdit->toPlainText());
		
		QRegularExpressionMatch match;
		QTextCursor c = consoleTextEdit->textCursor();
		
		while(matches.hasNext())
		{
			match = matches.next();
			if(match.capturedEnd() >= c.position())
			{
				if(matches.hasNext())
					match = matches.next();
				break;
			}
		}
		
		if(match.capturedStart() > 0)
		{
			c.setPosition(match.capturedStart(1));
			consoleTextEdit->setTextCursor(c);
			scrollToConsoleCursor();
			c.setPosition(match.capturedEnd(1), QTextCursor::KeepAnchor);
			consoleTextEdit->setTextCursor(c);
		}
	});
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: xmlPath
//--------------------------------------------------------------------------------------------------
QString MainWindowPrivate::xmlPath(const QString& testPath) const
{
    QString name = executableModel->index(testPath).data(QExecutableModel::NameRole).toString();
    QString hash = QCryptographicHash::hash(testPath.toLatin1(), QCryptographicHash::Md5).toHex();
    return dataPath() + "/" + name + "_" + hash;
}

QString MainWindowPrivate::latestGtestResultPath(const QString& testPath)
{
    const QString basicPath = xmlPath(testPath);
    if (testLatestTestRun_.find(testPath) == testLatestTestRun_.end())
    {
        const QStringList results = QDir(basicPath).entryList(QDir::Dirs, QDir::Name | QDir::Reversed);
        if (results.isEmpty())
        {
            return QString();
        }
        testLatestTestRun_[testPath] = results[0];
    }
    return basicPath + "/" + testLatestTestRun_[testPath] + "/" + GTEST_RESULT_NAME;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: addTestExecutable
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::addTestExecutable(const QString& path, const QString& testDriver, bool autorun, QDateTime lastModified,
	QString filter /*= ""*/, int repeat /*= 0*/, Qt::CheckState runDisabled /*= Qt::Unchecked*/, 
	Qt::CheckState shuffle /*= Qt::Unchecked*/, int randomSeed /*= 0*/, QString otherArgs /*= ""*/)
{
	QFileInfo fileinfo(path);

	if (!fileinfo.exists())
		return;

        if (!fileinfo.isExecutable() || !fileinfo.isFile())
		return;

	if (executableModel->index(path).isValid())
		return;

	if (lastModified == QDateTime())
		lastModified = fileinfo.lastModified();

	executableCheckedStateHash[path] = autorun;
	
	QModelIndex newRow = executableModel->insertRow(QModelIndex(), path);

	executableModel->setData(newRow, 0, QExecutableModel::ProgressRole);
	executableModel->setData(newRow, path, QExecutableModel::PathRole);
        executableModel->setData(newRow, testDriver, QExecutableModel::TestDriverRole);
	executableModel->setData(newRow, autorun, QExecutableModel::AutorunRole);
	executableModel->setData(newRow, lastModified, QExecutableModel::LastModifiedRole);
	executableModel->setData(newRow, ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);
	executableModel->setData(newRow, filter, QExecutableModel::FilterRole);
	executableModel->setData(newRow, repeat, QExecutableModel::RepeatTestsRole);
	executableModel->setData(newRow, runDisabled, QExecutableModel::RunDisabledTestsRole);
	executableModel->setData(newRow, shuffle, QExecutableModel::ShuffleRole);
	executableModel->setData(newRow, randomSeed, QExecutableModel::RandomSeedRole);
	executableModel->setData(newRow, otherArgs, QExecutableModel::ArgsRole);

	fileWatcher->addPath(fileinfo.dir().canonicalPath());
	fileWatcher->addPath(path);
	executablePaths << path;

	bool previousResults = loadTestResults(path, false);
	bool outOfDate = lastModified < fileinfo.lastModified();

	executableTreeView->setCurrentIndex(newRow);
	for (int i = 0; i < executableModel->columnCount(); i++)
	{
		executableTreeView->resizeColumnToContents(i);
	}

	testRunningHash[path] = false;

	// if there are no previous results but the test is being watched, run the test
	if ((!previousResults || outOfDate) && autorun)
	{
		this->runTestInThread(path, false);
		QFileInfo newInfo(path);
		executableModel->setData(newRow, newInfo.lastModified(), QExecutableModel::LastModifiedRole);
	}
	else if (outOfDate && !autorun)
	{
		executableModel->setData(newRow, ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);
	}
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: runTestInThread
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::runTestInThread(const QString& pathToTest, bool notify)
{
	std::thread t([this, pathToTest, notify]
	{
		QEventLoop loop;

		// kill the running test instance first if there is one
		if (testRunningHash[pathToTest])
		{
			emit killTest(pathToTest);

			std::unique_lock<std::mutex> lock(threadKillMutex);
			threadKillCv.wait(lock, [&, pathToTest] {return !testRunningHash[pathToTest]; });
		}

		testRunningHash[pathToTest] = true;

		executableModel->setData(executableModel->index(pathToTest), ExecutableData::RUNNING, QExecutableModel::StateRole);
		
		QFileInfo info(pathToTest);
		QProcess testProcess;

		bool first = true;
		int tests = 0;
		int progress = 0;

		// when the process finished, read any remaining output then quit the loop
                connect(&testProcess, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), &loop, [&, pathToTest]
                        (int exitCode, QProcess::ExitStatus exitStatus)
		{
			QString output = testProcess.readAllStandardOutput();
                        // 0 for success and 1 if test have failed -> in both cases a result xml was generated
                        if (exitStatus == QProcess::NormalExit && (exitCode == 0 || exitCode == 1))
			{
				output.append("\nTEST RUN COMPLETED: " + QDateTime::currentDateTime().toString("yyyy-MMM-dd hh:mm:ss.zzz") + "\n\n");
				emit testResultsReady(pathToTest, notify);
			}
			else
			{
				output.append("\nTEST RUN EXITED WITH ERRORS: " + QDateTime::currentDateTime().toString("yyyy-MMM-dd hh:mm:ss.zzz") + "\n\n");
				executableModel->setData(executableModel->index(pathToTest), ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);
			}
			
			emit testOutputReady(output);
			emit testProgress(pathToTest, 0, 0);

			testRunningHash[pathToTest] = false;
			threadKillCv.notify_all();

			loop.exit();
		}, Qt::QueuedConnection);

		// get killed if asked to do so
		connect(this, &MainWindowPrivate::killTest, &loop, [&, pathToTest]
                        (const QString& pathToTestToKill)
		{
                        if (pathToTest != pathToTestToKill)
                        {
                            return;
                        }
                        // terminate over std::in, as terminate() sends WM_CLOSE under windows that is complex to catch
                        testProcess.write("terminate");
                        testProcess.closeWriteChannel();
                        // Give 0.5s to finish
                        if (!testProcess.waitForFinished(500))
                        {
                            testProcess.kill();
                        }
			QString output = testProcess.readAllStandardOutput();
			output.append("\nTEST RUN KILLED: " + QDateTime::currentDateTime().toString("yyyy-MMM-dd hh:mm:ss.zzz") + "\n\n");

			executableModel->setData(executableModel->index(pathToTest), ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);

			emit testOutputReady(output);
			emit testProgress(pathToTest, 0, 0);

			testRunningHash[pathToTest] = false;
			threadKillCv.notify_all();

			loop.exit();
		}, Qt::QueuedConnection);


                // don't lock by default -> only if runTestsSynchronousAction_ is checked
                std::unique_lock<std::mutex> runTestThreadSynchronousLock(runTestThreadSynchronous_, std::defer_lock);
                if (runTestsSynchronousAction_->isChecked())
                {
                    runTestThreadSynchronousLock.lock();
                }


                QString currentDate = QDateTime::currentDateTime().toString("yyyy.MM.dd_hh.mm.ss.zzz");
                QString copyResultDir = xmlPath(pathToTest) + "/" + currentDate;
                QDir(copyResultDir).mkpath(".");
                testLatestTestRun_[pathToTest] = currentDate;


		// SET GTEST ARGS
		QModelIndex index = executableModel->index(pathToTest);
                QString testDriver = executableModel->data(index, QExecutableModel::TestDriverRole).toString();
                QString testDriverDir = QFileInfo(testDriver).dir().path();

                QStringList arguments(testDriver);

                arguments << "-C";
                arguments << info.dir().dirName();
                arguments << "--output-dir";
                arguments << copyResultDir;

                arguments << "--gtest_output=xml:\"" + testDriverDir + "/" + GTEST_RESULT_NAME + "\"";

		QString filter = executableModel->data(index, QExecutableModel::FilterRole).toString();
		if (!filter.isEmpty()) arguments << "--gtest_filter=" + filter;

		QString repeat = executableModel->data(index, QExecutableModel::RepeatTestsRole).toString();
		if (repeat != "0" && repeat != "1") arguments << "--gtest_repeat=" + repeat;
		double repeatCount = 1;
		if (repeat.toInt() > 1) repeatCount = repeat.toInt();

		int runDisabled = executableModel->data(index, QExecutableModel::RunDisabledTestsRole).toInt();
		if (runDisabled) arguments << "--gtest_also_run_disabled_tests";

		int shuffle = executableModel->data(index, QExecutableModel::ShuffleRole).toInt();
		if (shuffle) arguments << "--gtest_shuffle";

		int seed = executableModel->data(index, QExecutableModel::RandomSeedRole).toInt();
		if (shuffle) arguments << "--gtest_random_seed=" + QString::number(seed);

		QString otherArgs = executableModel->data(index, QExecutableModel::ArgsRole).toString();
		if(!otherArgs.isEmpty()) arguments << otherArgs;

		// Set working directory to be the same as the executable
		// (common standard for tests to find test-data files)
                testProcess.setWorkingDirectory(testDriverDir);

                QString cmd = "\"" + currentRunEnvPath_ + "\" && py";

		// Start the test
                testProcess.start(cmd, arguments);

		// get the first line of output. If we don't get it in a timely manner, the test is
		// probably bugged out so kill it.
		if (!testProcess.waitForReadyRead(500))
		{
			testProcess.kill();
			testRunningHash[pathToTest] = false;
				
			emit testProgress(pathToTest, 0, 0);
			emit testOutputReady("");
				
			return;
		}

		// print test output as it becomes available
		connect(&testProcess, &QProcess::readyReadStandardOutput, &loop, [&, pathToTest]
		{
			QString output = testProcess.readAllStandardOutput();

			// parse the first output line for the number of tests so we can
			// keep track of progress
			if (first)
			{
				// get the number of tests
				static QRegExp rx("([0-9]+) tests");
				rx.indexIn(output);
				tests = rx.cap(1).toInt();
                                if (tests)
                                {
                                    first = false;
                                    tests *= repeatCount;
                                }
                                else
                                {
                                    tests = 1;
                                }
			}
			else
			{
				QRegExp rx("(\\[.*OK.*\\]|\\[.*FAILED.*\\])");
				if (rx.indexIn(output) != -1)
					progress++;
			}

			emit testProgress(pathToTest, progress, tests);
			emit testOutputReady(output);
		}, Qt::QueuedConnection);

		loop.exec();

	});
        t.detach();
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: loadTestResults
//--------------------------------------------------------------------------------------------------
bool MainWindowPrivate::loadTestResults(const QString& testPath, bool notify)
{
	QFileInfo xmlInfo(latestGtestResultPath(testPath));

	if (!xmlInfo.exists())
	{
		return false;
	}

	QDomDocument doc(testPath);
	QFile file(xmlInfo.absoluteFilePath());
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(q_ptr, "Error", "Could not open file located at: " + xmlPath(testPath));
		return false;
	}
	if (!doc.setContent(&file))
	{
		file.close();
		return false;
	}
	file.close();

	testResultsHash[testPath] = doc;

	// if the test that just ran is selected, update the view
	QModelIndex index = executableTreeView->selectionModel()->currentIndex();

	if (index.data(QExecutableModel::PathRole).toString() == testPath)
	{
		selectTest(testPath);
	}

	// set executable icon
	int numErrors = doc.elementsByTagName("testsuites").item(0).attributes().namedItem("failures").nodeValue().toInt();
	if (numErrors)
	{
		executableModel->setData(executableModel->index(testPath), ExecutableData::FAILED, QExecutableModel::StateRole);
		mostRecentFailurePath = testPath;
		QString name = executableModel->index(testPath).data(QExecutableModel::NameRole).toString();
		// only show notifications AFTER the initial startup, otherwise the user
		// could get a ton of messages every time they open the program. The messages
		if (notify && notifyOnFailureAction->isChecked())
		{
			systemTrayIcon->showMessage("Test Failure", name + " failed with " + QString::number(numErrors) + " errors.");
		}
	}
	else
	{
		executableModel->setData(executableModel->index(testPath), ExecutableData::PASSED, QExecutableModel::StateRole);
		QString name = executableModel->index(testPath).data(QExecutableModel::NameRole).toString();
		if(notify && notifyOnSuccessAction->isChecked())
		{
			systemTrayIcon->showMessage("Test Successful", name + " ran with no errors.");
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: selectTest
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::selectTest(const QString& testPath)
{
	QStack<QString> selectionStack;

	// Store the path the current selection on a stack
	QModelIndex index = testCaseTreeView->selectionModel()->currentIndex();
	while (index != QModelIndex())
	{
		selectionStack.push(index.data(GTestModel::Name).toString());
		index = index.parent();
	}

	// Delete the old test case and failure models and make new ones
	delete testCaseProxyModel->sourceModel();
	delete failureProxyModel->sourceModel();
	testCaseTreeView->setSortingEnabled(false);
	testCaseProxyModel->setSourceModel(new GTestModel(testResultsHash[testPath]));
	failureProxyModel->clear();
	testCaseTreeView->setSortingEnabled(true);
	testCaseTreeView->expandAll();
	
	// make sure the right entry is selected
	executableTreeView->setCurrentIndex(executableModel->index(testPath));
	
	// resize the columns
	for (int i = 0; i < testCaseTreeView->model()->columnCount(); i++)
	{
		testCaseTreeView->resizeColumnToContents(i);
	}

	// reset the test case selection
	auto originalStackSize = selectionStack.size();
	index = testCaseTreeView->model()->index(0, 0);
	for (int i = 0; i < originalStackSize; ++i)	// don't use a while-loop in case the test changed and what we are searching for doesn't exist
	{
		QModelIndexList matches = testCaseTreeView->model()->match(index, GTestModel::Name, selectionStack.pop(), 1, Qt::MatchRecursive);
		if (matches.size() > 0)
		{
			index = matches.first();
		}
		else
		{
			index = QModelIndex();
		}
	}

	if (index.isValid())
	{
		testCaseTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
	}
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: saveSettings
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::saveSettings() const
{
    QDir settingsDir(settingsPath());
    if (!settingsDir.exists())
    {
        settingsDir.mkpath(".");
    }

    saveCommonSettings(settingsPath() + "/" + "settings.json");
    // Only save current -> multiple instances of gtest-runner don't overwrite each other
    saveTestSettingsForCurrentRunEnv();
}

void MainWindowPrivate::saveCommonSettings(const QString& path) const
{
    Q_Q(const MainWindow);

    QJsonObject root;
    auto data = q->saveGeometry();
    root.insert("geometry", QLatin1String(data.constData(), data.size()));
    data = q->saveState();
    root.insert("windowState", QLatin1String(data.constData(), data.size()));
    consoleFindDialog->writeSettings(root);

    root.insert("runEnvs", QJsonArray::fromStringList(runEnvModel_->stringList()));

    QJsonObject options;
    options.insert("runTestsSynchronous", runTestsSynchronousAction_->isChecked());
    options.insert("notifyOnFailure", notifyOnFailureAction->isChecked());
    options.insert("notifyOnSuccess", notifyOnSuccessAction->isChecked());
    options.insert("theme", themeActionGroup->checkedAction()->objectName());
    options.insert("currentRunEnvPath", currentRunEnvPath_);
    root.insert("options", options);

    QJsonDocument document(root);
    QFile settingsFile(path);
    if (!settingsFile.open(QIODevice::WriteOnly))
    {
        return;
    }
    settingsFile.write(document.toJson());
    settingsFile.close();
}

void MainWindowPrivate::saveTestSettingsForCurrentRunEnv() const
{
    if (currentRunEnvPath_.isEmpty())
    {
        return;
    }
    QString hash = QCryptographicHash::hash(currentRunEnvPath_.toLatin1(), QCryptographicHash::Md5).toHex();
    saveTestSettings(settingsPath() + "/" + hash + ".json");
}

void MainWindowPrivate::saveTestSettings(const QString& path) const
{
    Q_Q(const MainWindow);

    QJsonObject root;
    QJsonArray tests;
    for (auto itr = executableModel->begin(); itr != executableModel->end(); ++itr)
    {
        QModelIndex index = executableModel->iteratorToIndex(itr);
        index = index.sibling(index.row(), QExecutableModel::NameColumn);

        QJsonObject test;
        test.insert("path", QJsonValue::fromVariant(index.data(QExecutableModel::PathRole)));
        test.insert("testDriver", QJsonValue::fromVariant(index.data(QExecutableModel::TestDriverRole)));
        test.insert("autorun", QJsonValue::fromVariant(index.data(QExecutableModel::AutorunRole)));
        test.insert("lastModified", index.data(QExecutableModel::LastModifiedRole).toDateTime().toString(Qt::DateFormat::ISODate));
        test.insert("filter", QJsonValue::fromVariant(index.data(QExecutableModel::FilterRole)));
        test.insert("repeat", QJsonValue::fromVariant(index.data(QExecutableModel::RepeatTestsRole)));
        test.insert("runDisabled", QJsonValue::fromVariant(index.data(QExecutableModel::RunDisabledTestsRole)));
        test.insert("shuffle", QJsonValue::fromVariant(index.data(QExecutableModel::ShuffleRole)));
        test.insert("seed", QJsonValue::fromVariant(index.data(QExecutableModel::RandomSeedRole)));
        test.insert("args", QJsonValue::fromVariant(index.data(QExecutableModel::ArgsRole)));
        tests.append(test);
    }
    root.insert("tests", tests);

    QJsonDocument document(root);
    QFile settingsFile(path);
    if (!settingsFile.open(QIODevice::WriteOnly))
    {
        return;
    }
    settingsFile.write(document.toJson());
    settingsFile.close();
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: loadSettings
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::loadSettings()
{
    loadCommonSettings(settingsPath() + "/" + "settings.json");
    // Only keep current in memory
    loadTestSettingsForCurrentRunEnv();

    auto alreadyIndex = runEnvComboBox_->findText(currentRunEnvPath_);
    if (alreadyIndex != -1)
    {
        runEnvComboBox_->setCurrentIndex(alreadyIndex);
        return;
    }
}

void MainWindowPrivate::loadCommonSettings(const QString& path)
{
    Q_Q(MainWindow);
    QFile settingsFile(path);
    if (!settingsFile.open(QIODevice::ReadOnly))
    {
        defaultThemeAction->setChecked(true);
        return;
    }
    QJsonDocument document = QJsonDocument::fromJson(settingsFile.readAll());
    settingsFile.close();

    QJsonObject root = document.object();

    q->restoreGeometry(root["geometry"].toString().toLatin1());
    q->restoreState(root["windowState"].toString().toLatin1());
    consoleFindDialog->readSettings(root);

    for (const auto& runEnv : root["runEnvs"].toArray())
    {
        if (runEnvModel_->insertRow(runEnvModel_->rowCount()))
        {
            auto index = runEnvModel_->index(runEnvModel_->rowCount() - 1, 0);
            runEnvModel_->setData(index, runEnv.toString());
        }
    }

    QJsonObject options = root["options"].toObject();
    runTestsSynchronousAction_->setChecked(options["runTestsSynchronous"].toBool());
    notifyOnFailureAction->setChecked(options["notifyOnFailure"].toBool());
    notifyOnSuccessAction->setChecked(options["notifyOnSuccess"].toBool());
    themeMenu->findChild<QAction*>(options["theme"].toString())->trigger();
    currentRunEnvPath_ = options["currentRunEnvPath"].toString();
}

void MainWindowPrivate::loadTestSettingsForCurrentRunEnv()
{
    if (currentRunEnvPath_.isEmpty())
    {
        return;
    }
    QString hash = QCryptographicHash::hash(currentRunEnvPath_.toLatin1(), QCryptographicHash::Md5).toHex();
    loadTestSettings(settingsPath() + "/" + hash + ".json");
}

void MainWindowPrivate::loadTestSettings(const QString& path)
{
    Q_Q(const MainWindow);
    QFile settingsFile(path);
    if (!settingsFile.open(QIODevice::ReadOnly))
    {
        return;
    }
    QJsonDocument document = QJsonDocument::fromJson(settingsFile.readAll());
    settingsFile.close();

    QJsonObject root = document.object();
    QJsonArray tests = root["tests"].toArray();

    for (const auto& testRef : tests)
    {
        QJsonObject test = testRef.toObject();

        QString path = test["path"].toString();
        QString testDriver = test["testDriver"].toString();
        bool autorun = test["autorun"].toBool();
        QDateTime lastModified = QDateTime::fromString(test["lastModified"].toString(), Qt::DateFormat::ISODate);
        QString filter = test["filter"].toString();
        int repeat = test["repeat"].toInt();
        Qt::CheckState runDisabled = static_cast<Qt::CheckState>(test["runDisabled"].toInt());
        Qt::CheckState shuffle = static_cast<Qt::CheckState>(test["shuffle"].toInt());
        int seed = test["seed"].toInt();
        QString args = test["args"].toString();

        addTestExecutable(path, testDriver, autorun, lastModified, filter, repeat, runDisabled, shuffle, seed, args);
    }
}

void MainWindowPrivate::removeAllTest(const bool confirm)
{
    if (executableModel->rowCount() == 0)
    {
        // nothing to remove
        return;
    }

    if (confirm || QMessageBox::question(this->q_ptr, "Remove All Test?", "Do you want to remove all tests?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        for (size_t i = 0; i < executableModel->rowCount(); ++i)
        {
            auto index = executableModel->index(i, 0);
            QString path = index.data(QExecutableModel::PathRole).toString();

            // remove all data related to this test
            executablePaths.removeAll(path);
            testResultsHash.remove(path);
            fileWatcher->removePath(path);
        }

        QAbstractItemModel* oldFailureModel = failureProxyModel->sourceModel();
        QAbstractItemModel* oldtestCaseModel = testCaseProxyModel->sourceModel();
        failureProxyModel->setSourceModel(new GTestFailureModel(nullptr));
        testCaseProxyModel->setSourceModel(new GTestModel(QDomDocument()));
        delete oldFailureModel;
        delete oldtestCaseModel;
        executableModel->removeRows(0, executableModel->rowCount());
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: removeTest
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::removeTest(const QModelIndex &index, const bool confirm)
{
	if (!index.isValid())
		return;

	QString path = index.data(QExecutableModel::PathRole).toString();

        if (confirm || QMessageBox::question(this->q_ptr, QString("Remove Test?"), "Do you want to remove test " + index.data(QExecutableModel::NameRole).toString() + "?",
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		executableTreeView->setCurrentIndex(index);

		// remove all data related to this test
		executablePaths.removeAll(path);
		testResultsHash.remove(path);
		fileWatcher->removePath(path);

		QAbstractItemModel* oldFailureModel = failureProxyModel->sourceModel();
		QAbstractItemModel* oldtestCaseModel = testCaseProxyModel->sourceModel();
		failureProxyModel->setSourceModel(new GTestFailureModel(nullptr));
		testCaseProxyModel->setSourceModel(new GTestModel(QDomDocument()));
		delete oldFailureModel;
		delete oldtestCaseModel;

		executableModel->removeRow(index.row(), index.parent());
	}
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: clearData
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::clearData()
{
	QDir dataDir(dataPath());
	if (dataDir.exists())
	{
		dataDir.removeRecursively();
		for (int i = 0; i < executableModel->rowCount(); ++i)
		{
			QModelIndex index = executableModel->index(i, 0);
			executableModel->setData(index, ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);
		}
	}
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: clearSettings
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::clearSettings()
{
    QDir settingsDir(settingsPath());
    if (settingsDir.exists())
    {
        settingsDir.removeRecursively();
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: getTestDialog
//--------------------------------------------------------------------------------------------------
QModelIndex MainWindowPrivate::getTestIndexDialog(const QString& label, bool running /*= false*/)
{
	bool ok;
	QHash<QString, QString> tests;

	for (auto itr = executableModel->begin(); itr != executableModel->end(); ++itr)
	{
		QString path = itr->path;
		if(!path.isEmpty() && (!running || testRunningHash[path]))
			tests[executableModel->iteratorToIndex(itr).data(QExecutableModel::NameRole).toString()] = path;
	}

	if (tests.isEmpty())
		return QModelIndex();

	QString selected = QInputDialog::getItem(this->q_ptr, "Select Test", label, tests.keys(), 0, false, &ok);
	QModelIndex match = executableModel->index(tests[selected]);
	if (ok)
		return match;
	else
		return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createExecutableContextMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createExecutableContextMenu()
{
	Q_Q(MainWindow);

	executableContextMenu = new QMenu(executableTreeView);

	runTestAction = new QAction(q->style()->standardIcon(QStyle::SP_BrowserReload), "Run Test...", executableContextMenu);
	killTestAction = new QAction(q->style()->standardIcon(QStyle::SP_DialogCloseButton), "Kill Test...", executableContextMenu);
        revealExplorerTestAction_ = new QAction(q->style()->standardIcon(QStyle::SP_DirOpenIcon), "Reveal Test Results...", executableContextMenu);
	removeTestAction = new QAction(q->style()->standardIcon(QStyle::SP_TrashIcon), "Remove Test", executableContextMenu);

	executableContextMenu->addAction(runTestAction);
	executableContextMenu->addAction(killTestAction);
        executableContextMenu->addAction(revealExplorerTestAction_);
	executableContextMenu->addSeparator();	
	executableContextMenu->addAction(removeTestAction);
	executableContextMenu->addAction(selectAndRemoveTestAction);

	executableTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	
	connect(executableTreeView, &QListView::customContextMenuRequested, [this, q](const QPoint& pos)
	{
		QModelIndex index = executableTreeView->indexAt(pos);
		if (index.isValid())
		{
			runTestAction->setEnabled(true);
			if (testRunningHash[index.data(QExecutableModel::PathRole).toString()])
				killTestAction->setEnabled(true);
			else
				killTestAction->setEnabled(false);
			removeTestAction->setVisible(true);
                        revealExplorerTestAction_->setEnabled(QDir(xmlPath(index.data(QExecutableModel::PathRole).toString())).exists());
			selectAndRemoveTestAction->setVisible(false);
		}
		else
		{
			runTestAction->setEnabled(false);
			killTestAction->setEnabled(false);
                        revealExplorerTestAction_->setEnabled(false);
			removeTestAction->setVisible(false);
			selectAndRemoveTestAction->setVisible(true);
		}
		executableContextMenu->exec(executableTreeView->mapToGlobal(pos));
		selectAndRemoveTestAction->setVisible(true);	// important b/c this is a shared action
	});

	connect(runTestAction, &QAction::triggered, [this]
	{
		QModelIndex index = executableTreeView->currentIndex();
		QString path = index.data(QExecutableModel::PathRole).toString();
		runTestInThread(path, false);
	});

	connect(killTestAction, &QAction::triggered, [this]
	{
		Q_Q(MainWindow);
		QString path = executableTreeView->currentIndex().data(QExecutableModel::PathRole).toString();
		QString name = executableTreeView->currentIndex().data(QExecutableModel::NameRole).toString();
		if (QMessageBox::question(q, "Kill Test?", "Are you sure you want to kill test: " + name + "?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
			emit killTest(path);
	});

        connect(revealExplorerTestAction_, &QAction::triggered, [this]
        {
            QString path = executableTreeView->currentIndex().data(QExecutableModel::PathRole).toString();
            QDesktopServices::openUrl(QUrl::fromLocalFile(xmlPath(path)));
        });

	connect(removeTestAction, &QAction::triggered, [this]
	{
		removeTest(executableTreeView->currentIndex());
	});
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createTestCaseViewContextMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createTestCaseViewContextMenu()
{
	testCaseTreeView->setContextMenuPolicy(Qt::ActionsContextMenu);

	testCaseViewContextMenu = new QMenu(testCaseTreeView);

	testCaseViewExpandAllAction = new QAction("Expand All", testCaseViewContextMenu);
	testCaseViewCollapseAllAction = new QAction("Collapse All", testCaseViewContextMenu);

	testCaseTreeView->addAction(testCaseViewExpandAllAction);
	testCaseTreeView->addAction(testCaseViewCollapseAllAction);

	connect(testCaseViewExpandAllAction, &QAction::triggered, testCaseTreeView, &QTreeView::expandAll);
	connect(testCaseViewCollapseAllAction, &QAction::triggered, testCaseTreeView, &QTreeView::collapseAll);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createConsoleContextMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createConsoleContextMenu()
{
	Q_Q(MainWindow);

	consoleTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);

	clearConsoleAction = new QAction("Clear", this);
	consoleFindShortcut = new QShortcut(QKeySequence("Ctrl+F"), q);
	consoleFindAction = new QAction("Find...", this);
	consoleFindAction->setShortcut(consoleFindShortcut->key());

	connect(consoleTextEdit, &QTextEdit::customContextMenuRequested, [this, q](const QPoint& pos)
	{
		QScopedPointer<QMenu> consoleContextMenu(consoleTextEdit->createStandardContextMenu(consoleTextEdit->mapToGlobal(pos)));
		consoleContextMenu->addSeparator();
		consoleContextMenu->addAction(consoleFindAction);
		consoleContextMenu->addSeparator();
		consoleContextMenu->addAction(clearConsoleAction);
		consoleContextMenu->exec(consoleTextEdit->mapToGlobal(pos));
	});

	connect(clearConsoleAction, &QAction::triggered, consoleTextEdit, &QTextEdit::clear);
	connect(consoleFindShortcut, &QShortcut::activated, consoleFindAction, &QAction::trigger);
	connect(consoleFindAction, &QAction::triggered, [this, q]
		{
			consoleDock->setVisible(true);
			consoleDock->raise();
			consoleFindDialog->show();
		}
	);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createTestMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createTestMenu()
{
	Q_Q(MainWindow);

	testMenu = new QMenu("Test", q);

        addRunEnvAction = new QAction(QIcon(":/images/green"), "Add RunEnv...", q);
	selectAndRemoveTestAction = new QAction(q->style()->standardIcon(QStyle::SP_TrashIcon), "Remove Test...", testMenu);
        removeAllTestAction_ = new QAction(q->style()->standardIcon(QStyle::SP_TrashIcon), "Remove All Test...", testMenu);
	selectAndRunTest = new QAction(q->style()->standardIcon(QStyle::SP_BrowserReload), "Run Test...", testMenu);
	selectAndRunTest->setShortcut(QKeySequence(Qt::Key_F5));
	selectAndRunAllTest = new QAction(q->style()->standardIcon(QStyle::SP_BrowserReload), "Run All Test...", testMenu);
	selectAndRunAllTest->setShortcut(QKeySequence(Qt::Key_F6));
	selectAndKillTest = new QAction(q->style()->standardIcon(QStyle::SP_DialogCloseButton), "Kill Test...", testMenu);
	selectAndKillTest->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F5));
        selectAndKillAllTest_ = new QAction(q->style()->standardIcon(QStyle::SP_DialogCloseButton), "Kill All Test...", testMenu);
        selectAndKillAllTest_->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F6));

        testMenu->addAction(addRunEnvAction);
	testMenu->addAction(selectAndRemoveTestAction);
        testMenu->addAction(removeAllTestAction_);
	testMenu->addSeparator();
	testMenu->addAction(selectAndRunTest);
	testMenu->addAction(selectAndRunAllTest);
	testMenu->addAction(selectAndKillTest);
        testMenu->addAction(selectAndKillAllTest_);

	q->menuBar()->addMenu(testMenu);

        connect(addRunEnvAction, &QAction::triggered, [this]()
	{
		QString filter;
#ifdef Q_OS_WIN32
		filter = "RunEnv (*.bat)";
#else
                // TODO: extend filter?
		filter = "RunEnv (*.sh)";
#endif
                QString filename = QFileDialog::getOpenFileName(q_ptr, "Select RunEnv.bat/sh", currentRunEnvPath_, filter);

                if (filename.isEmpty())
                {
                    return;
                }
		else
		{
                    QFileInfo info(filename);
                    QString runEnvPath = info.absoluteFilePath();
                    auto alreadyIndex = runEnvComboBox_->findText(runEnvPath);
                    if (alreadyIndex != -1)
                    {
                        runEnvComboBox_->setCurrentIndex(alreadyIndex);
                        return;
                    }

                    saveTestSettingsForCurrentRunEnv();
                    removeAllTest(true);
                    currentRunEnvPath_ = runEnvPath;
                    updateTestExecutables();

                    if (runEnvModel_->insertRow(runEnvModel_->rowCount()))
                    {
                        auto index = runEnvModel_->index(runEnvModel_->rowCount() - 1, 0);
                        runEnvModel_->setData(index, runEnvPath);
                    }
                    runEnvComboBox_->setCurrentIndex(runEnvComboBox_->count() - 1);
		}
	});

	connect(selectAndRemoveTestAction, &QAction::triggered, [this]
	{
		removeTest(getTestIndexDialog("Select test to remove:"));
	});

        connect(removeAllTestAction_, &QAction::triggered, [this]
        {
            removeAllTest();
        });

	connect(selectAndRunTest, &QAction::triggered, [this]
	{
		QModelIndex index = getTestIndexDialog("Select Test to run:");
		if(index.isValid())
			runTestInThread(index.data(QExecutableModel::PathRole).toString(), false);
	});

	connect(selectAndRunAllTest, &QAction::triggered, [this]
	{
		for (size_t i = 0; i < executableTreeView->model()->rowCount(); ++i)
		{
			QModelIndex index = executableTreeView->model()->index(i, 0);
			runTestInThread(index.data(QExecutableModel::PathRole).toString(), false);
		}
	});

	connect(selectAndKillTest, &QAction::triggered, [this, q]
	{
		QModelIndex index = getTestIndexDialog("Select Test to kill:", true);
		QString name = index.data(QExecutableModel::NameRole).toString();
		if (index.isValid())
			if (QMessageBox::question(q, "Kill Test?", "Are you sure you want to kill test: " + name + "?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
				emit killTest(index.data(QExecutableModel::PathRole).toString());
	});

        connect(selectAndKillAllTest_, &QAction::triggered, [this, q]
        {
            if (QMessageBox::question(q, "Kill All Test?", "Are you sure you want to kill all test?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
            {
                for (size_t i = 0; i < executableTreeView->model()->rowCount(); ++i)
                {
                    QModelIndex index = executableTreeView->model()->index(i, 0);
                    emit killTest(index.data(QExecutableModel::PathRole).toString());
                }
            }
        });
}

void MainWindowPrivate::createToolBar()
{
    toolBar_->setObjectName("toolbar");
    toolBar_->addWidget(runEnvComboBox_);

    runEnvComboBox_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    runEnvComboBox_->setModel(runEnvModel_);
    connect(runEnvComboBox_, &QComboBox::currentTextChanged, [this]
            (const QString& selectedRunEnv)
    {
        // only reload if changed and had one selected before
        if (currentRunEnvPath_ != selectedRunEnv && !currentRunEnvPath_.isEmpty() && !selectedRunEnv.isEmpty())
        {
            saveTestSettingsForCurrentRunEnv();
            removeAllTest(true);
            currentRunEnvPath_ = selectedRunEnv;
            loadTestSettingsForCurrentRunEnv();
        }
    });
}

void MainWindowPrivate::updateTestExecutables()
{
    static QStringList testDriverFilter = { "TestDriver.py" };

    if (currentRunEnvPath_.isEmpty())
    {
        return;
    }

    QDir homeBase;
#ifdef Q_OS_WIN32
    homeBase = QFileInfo(currentRunEnvPath_).dir();
#else
    // TODO: make for env.sh
    return;
#endif

    // Temporally disable until test executables are added
    runEnvComboBox_->setEnabled(false);

    homeBase.setNameFilters(testDriverFilter);
    QDirIterator it(homeBase, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFileInfo testDriverFileInfo(it.next());
        
        // Get test executables with list-test-exess option
        QProcess testProcess;
        QStringList arguments(testDriverFileInfo.absoluteFilePath());
        arguments << "--list-test-exes";
        testProcess.start("py", arguments);

        if (testProcess.waitForFinished(500))
        {
            QString output = testProcess.readAllStandardOutput();
            for (const auto& testExe : output.split(QRegExp("[\r\n]"), QString::SkipEmptyParts))
            {
                addTestExecutable(testExe.trimmed(), testDriverFileInfo.absoluteFilePath(), false, QDateTime());
            }
        }
        else
        {
            testProcess.kill();
        }
    }

    runEnvComboBox_->setEnabled(true);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createOptionsMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createOptionsMenu()
{
	Q_Q(MainWindow);

	optionsMenu = new QMenu("Options", q);

        runTestsSynchronousAction_ = new QAction("Run tests synchronous and not parallel", optionsMenu);
	notifyOnFailureAction = new QAction("Notify on auto-run Failure", optionsMenu);
	notifyOnSuccessAction = new QAction("Notify on auto-run Success", optionsMenu);
        runTestsSynchronousAction_->setCheckable(true);
        runTestsSynchronousAction_->setChecked(false);
	notifyOnFailureAction->setCheckable(true);
	notifyOnFailureAction->setChecked(true);
	notifyOnSuccessAction->setCheckable(true);
	notifyOnSuccessAction->setChecked(false);

        optionsMenu->addAction(runTestsSynchronousAction_);
	optionsMenu->addAction(notifyOnFailureAction);
	optionsMenu->addAction(notifyOnSuccessAction);

	q->menuBar()->addMenu(optionsMenu);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createViewMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createWindowMenu()
{
	Q_Q(MainWindow);

	windowMenu = new QMenu("Window", q);
	windowMenu->addAction(executableDock->toggleViewAction());
	windowMenu->addAction(failureDock->toggleViewAction());
	windowMenu->addAction(consoleDock->toggleViewAction());
	
	q->menuBar()->addMenu(windowMenu);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createThemeMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createThemeMenu()
{
	Q_Q(MainWindow);

	themeMenu = new QMenu("Theme");

	defaultThemeAction = new QAction("Default Theme", themeMenu);
	defaultThemeAction->setObjectName("defaultThemeAction");
	defaultThemeAction->setCheckable(true);
	connect(defaultThemeAction, &QAction::triggered, [&]()
	{
		qApp->setStyleSheet("");
	});

	darkThemeAction = new QAction("Dark Theme", themeMenu);
	darkThemeAction->setObjectName("darkThemeAction");
	darkThemeAction->setCheckable(true);
	connect(darkThemeAction, &QAction::triggered, [&]()
	{
		QFile f(":styles/qdarkstyle");
		if (!f.exists())
		{
			printf("Unable to set stylesheet, file not found\n");
		}
		else
		{
			f.open(QFile::ReadOnly | QFile::Text);
			QTextStream ts(&f);
			qApp->setStyleSheet(ts.readAll());
		}
	});

	themeMenu->addAction(defaultThemeAction);
	themeMenu->addAction(darkThemeAction);

	themeActionGroup = new QActionGroup(themeMenu);
	themeActionGroup->addAction(defaultThemeAction);
	themeActionGroup->addAction(darkThemeAction);

	q->menuBar()->addMenu(themeMenu);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createHelpMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createHelpMenu()
{
	Q_Q(MainWindow);

	helpMenu = new QMenu("Help");

	aboutAction = new QAction("About...", helpMenu);

	helpMenu->addAction(aboutAction);

	connect(aboutAction, &QAction::triggered, [q]
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle("About");
		msgBox.setIconPixmap(QPixmap(":images/logo").scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		msgBox.setTextFormat(Qt::RichText);   //this is what makes the links clickable
		msgBox.setText("Application: " + APPINFO::name + "<br>version: " + APPINFO::version +
			"<br>Developer: Oliver Karrenbauer" + "<br>Organization: " + APPINFO::organization + "<br>Website: <a href='" + APPINFO::oranizationDomain + "'>" + APPINFO::oranizationDomain + "</a>" + "<br><br>" + 
			"The MIT License (MIT)<br><br>\
			\
			Copyright(c) 2016 Nic Holthaus<br><br>\
			\
			Permission is hereby granted, free of charge, to any person obtaining a copy	\
			of this software and associated documentation files(the 'Software'), to deal	\
			in the Software without restriction, including without limitation the rights	\
			to use, copy, modify, merge, publish, distribute, sublicense, and / or sell		\
			copies of the Software, and to permit persons to whom the Software is			\
			furnished to do so, subject to the following conditions :	<br><br>			\
																							\
			The above copyright notice and this permission notice shall be included in all	\
			copies or substantial portions of the Software.<br><br>							\
																							\
			THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR		\
			IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,		\
			FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE		\
			AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER			\
			LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,	\
			OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE	\
			SOFTWARE."
			);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
	});

	q->menuBar()->addMenu(helpMenu);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: scrollToConsoleCursor
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::scrollToConsoleCursor()
{
	int cursorY = consoleTextEdit->cursorRect().top();
    QScrollBar *vbar = consoleTextEdit->verticalScrollBar();
    vbar->setValue(vbar->value() + cursorY - 0);
}


QString MainWindowPrivate::settingsPath() const
{
    return QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).first() + "/" + "settings";
}

QString MainWindowPrivate::dataPath() const
{
    return QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).first() + "/" + "data";
}