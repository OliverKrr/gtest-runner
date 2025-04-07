#include "GTestFailureModel.h"
#include "QStdOutSyntaxHighlighter.h"
#include "mainwindow_p.h"

#include "executableModelDelegate.h"
#include "testsController.h"
#include "utilities.h"
#include "gtestModel.h"
#include "GTestModelSortFilterProxy.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QScrollBar>
#include <QStyle>
#include <QLabel>
#include <QProgressDialog>
#include <QDockWidget>
#include <QPushButton>
#include <QStatusBar>
#include <QMessageBox>
#include <QListWidget>
#include <QFileDialog>
#include <QActionGroup>
#include <QCryptographicHash>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QDirIterator>
#include <thread>

#ifndef Q_OS_WIN32
#define PYTHON "python3"
#else
#define PYTHON "py"
#endif

/* \brief Simple exception-safe semaphore locker
*/
class SemaphoreLocker final
{
public:
    explicit SemaphoreLocker(QSemaphore& semaphore, const int n = 1)
        : semaphore_(semaphore), n_(n)
    {
        semaphore_.acquire(n_);
    }

    ~SemaphoreLocker()
    {
        semaphore_.release(n_);
    }

private:
    QSemaphore& semaphore_;
    int n_;
};


//--------------------------------------------------------------------------------------------------
//	FUNCTION: MainWindowPrivate
//--------------------------------------------------------------------------------------------------
MainWindowPrivate::MainWindowPrivate(const QStringList&, const bool reset, MainWindow* q) : q_ptr(q),
    toolBar_(new QToolBar(q)),
    runEnvComboBox_(new QComboBox(toolBar_)),
    runEnvModel_(new QStringListModel(toolBar_)),
    executableDock(new QDockWidget(q)),
    executableDockFrame(new QFrame(q)),
    executableTreeView(new QExecutableTreeView(q)),
    executableModel(new QExecutableModel(q)),
    updateTestsButton(new QPushButton(q)),
    toggleAutoRun_(new QPushButton(q)),
    fileWatcher(new QFileSystemWatcher(q)),
    centralFrame(new QFrame(q)),
    testCaseFilterEdit(new QLineEdit(q)),
    testCaseFilterNotExecuted(new QCheckBox(q)),
    testCaseFilterPassed(new QCheckBox(q)),
    testCaseFilterIgnored(new QCheckBox(q)),
    testCaseTableView(new QTableView(q)),
    testCaseProxyModel(new GTestModelSortFilterProxy(q)),
    failureDock(new QDockWidget(q)),
    failureTreeView(new QTreeView(q)),
    failureProxyModel(new QFilterEmptyColumnProxy(q)),
    statusBar(new QStatusBar(q)),
    consoleDock(new QDockWidget(q)),
    consoleFrame(new QFrame(q)),
    consoleButtonLayout(new QVBoxLayout()),
    consoleLayout(new QHBoxLayout()),
    consolePrevFailureButton(new QPushButton(q)),
    consoleNextFailureButton(new QPushButton(q)),
    consoleTextEdit(new QTextEdit(q)),
    consoleHighlighter(new QStdOutSyntaxHighlighter(consoleTextEdit)),
    consoleFindDialog(new FindDialog(consoleTextEdit)),
    systemTrayIcon(new QSystemTrayIcon(QIcon(":/images/logo"), q)),
    testsController_(new TestsController(q)),
    mostRecentFailurePath(""),
    runTestParallelSemaphore_(MAX_PARALLEL_TEST_EXEC)
{
    qRegisterMetaType<QVector<int>>("QVector<int>");

    if (reset)
    {
        clearData();
        clearSettings();
    }

    QFontDatabase::addApplicationFont(":/fonts/consolas");
    const QFont consolas("consolas", 10);

    centralFrame->setLayout(new QVBoxLayout);
    auto* filterWidget = new QWidget(centralFrame);
    filterWidget->setLayout(new QHBoxLayout);
    filterWidget->layout()->addWidget(testCaseFilterEdit);
    filterWidget->layout()->addWidget(testCaseFilterNotExecuted);
    filterWidget->layout()->addWidget(testCaseFilterPassed);
    filterWidget->layout()->addWidget(testCaseFilterIgnored);
    centralFrame->layout()->addWidget(filterWidget);
    centralFrame->layout()->addWidget(testCaseTableView);
    centralFrame->layout()->setContentsMargins(0, 5, 0, 0);

    executableDock->setObjectName("executableDock");
    executableDock->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    executableDock->setWindowTitle("Test Executables");
    executableDock->setWidget(executableDockFrame);

    executableTreeView->setModel(executableModel);
    executableTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    executableTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    executableTreeView->setDragDropMode(QTreeView::InternalMove);
    executableTreeView->setHeaderHidden(true);
    executableTreeView->setIndentation(0);
    executableTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    executableTreeView->setItemDelegateForColumn(QExecutableModel::ProgressColumn,
                                                 new QProgressBarDelegate(executableTreeView));

    executableDockFrame->setLayout(new QVBoxLayout);
    executableDockFrame->layout()->addWidget(executableTreeView);
    executableDockFrame->layout()->addWidget(toggleAutoRun_);
    executableDockFrame->layout()->addWidget(updateTestsButton);

    toggleAutoRun_->setText("Toggle auto-run");
    toggleAutoRun_->setToolTip(
        "When auto-run is enabled, the test executables are being watched and automatically executed when being re-build.");

    updateTestsButton->setText("Update tests");
    updateTestsButton->setToolTip("Search current RunEnv.bat/sh dir for TestDriver.py and related test executables");

    testCaseFilterEdit->setPlaceholderText("Filter Test Output...");
    testCaseFilterEdit->setClearButtonEnabled(true);

    testCaseFilterNotExecuted->setChecked(true);
    testCaseFilterNotExecuted->setText("Show Not Executed");
    testCaseFilterPassed->setChecked(true);
    testCaseFilterPassed->setText("Show Passed");
    testCaseFilterIgnored->setChecked(true);
    testCaseFilterIgnored->setText("Show Ignored");

    testCaseTableView->setSortingEnabled(false);
    testCaseTableView->sortByColumn(GTestModel::TestNumber, Qt::AscendingOrder);
    testCaseTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    testCaseTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    testCaseTableView->setWordWrap(false);
    testCaseTableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    testCaseTableView->setTextElideMode(Qt::ElideMiddle);
    testCaseTableView->setModel(testCaseProxyModel);

    testCaseProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    testCaseProxyModel->setFilterKeyColumn(-1);
    testCaseProxyModel->setShowNotExecuted(testCaseFilterNotExecuted->isChecked());
    testCaseProxyModel->setShowPassed(testCaseFilterPassed->isChecked());
    testCaseProxyModel->setShowIgnored(testCaseFilterIgnored->isChecked());

    failureDock->setObjectName("failureDock");
    failureDock->setAllowedAreas(
        Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    failureDock->setWindowTitle("Failures");
    failureDock->setWidget(failureTreeView);

    failureTreeView->setModel(failureProxyModel);
    failureTreeView->setAlternatingRowColors(true);

    consoleDock->setObjectName("consoleDock");
    consoleDock->setAllowedAreas(
        Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
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

    toolBar_->setObjectName("toolbar");
    toolBar_->setWindowTitle("Toolbar");
    createToolBar();

    createTestMenu();
    createOptionsMenu();
    createWindowMenu();
    createThemeMenu();
    createHelpMenu();

    createExecutableContextMenu();
    createConsoleContextMenu();
    createTestCaseViewContextMenu();

    numberOfRunningTests_ = 0;
    updateButtonsForRunningTests();

    connect(this, &MainWindowPrivate::setStatus, statusBar, &QStatusBar::setStatusTip, Qt::QueuedConnection);
    connect(this, &MainWindowPrivate::testResultsReady, this, &MainWindowPrivate::loadTestResults,
            Qt::QueuedConnection);
    connect(this, &MainWindowPrivate::testResultsReady, statusBar, &QStatusBar::clearMessage, Qt::QueuedConnection);
    connect(this, &MainWindowPrivate::showMessage, statusBar, &QStatusBar::showMessage, Qt::QueuedConnection);

    connect(toggleAutoRun_, &QPushButton::clicked, [this]()
    {
        for (int i = 0; i < executableModel->rowCount(); ++i)
        {
            auto index = executableModel->index(i, 0);
            QString path = index.data(QExecutableModel::PathRole).toString();
            const bool prevState = testsController_->autoRun(path);
            executableModel->setData(index, !prevState, QExecutableModel::AutorunRole);
        }
    });

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
    connect(executableTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            [this](const QItemSelection& selected, const QItemSelection& deselected)
            {
                if (!selected.isEmpty())
                {
                    const auto index = selected.indexes().first();
                    selectTest(index.data(QExecutableModel::PathRole).toString());
                }
            });

    // run the test whenever the executable changes
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path)
    {
        const QModelIndex m = executableModel->index(path);
        if (m.isValid())
        {
            emit showMessage("Change detected: " + path + "...");

            auto currentTime = QDateTime::currentDateTime();
            latestBuildChangeTime_[path] = currentTime;

            RunMode runMode = NoTests;
            if (autoUpdateTestListAction_->isChecked())
            {
                runMode = ListTests;
            }
            // only auto-run if the test is checked
            if (m.data(QExecutableModel::AutorunRole).toBool())
            {
                runMode = runMode == ListTests ? ListAndRunTests : RunTests;
            }
            else
            {
                executableModel->setData(m, ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);
            }

            if (runMode != NoTests)
            {
                // add a little delay to avoid running multiple instances of the same test build,
                // and to avoid running the file before visual studio is done writing it.
                QTimer::singleShot(2000, [this, path, currentTime, runMode]
                {
                    // Only run after latest update after timeout and not by previous triggers
                    if (currentTime == latestBuildChangeTime_[path])
                    {
                        emit runTestInThread(path, {}, false, runMode);
                    }
                });
            }
        }
    });

    // update filewatcher when directory changes
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, [this](const QString& path)
    {
        // This could be caused by the re-build of a watched test (which cause additionally cause the
        // watcher to stop watching it), so just in case add all the test paths back.
        this->fileWatcher->addPaths(executablePaths.filter(path));
    });

    // re-rerun tests when auto-testing is re-enabled
    connect(executableModel, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
            {
                const QString path = topLeft.data(QExecutableModel::PathRole).toString();
                const bool prevState = testsController_->autoRun(path);

                // Only re-run IFF the checkbox state goes from unchecked to checked AND
                // the data has gotten out of date since the checkbox was off.
                if (topLeft.data(QExecutableModel::AutorunRole).toBool() && !prevState)
                {
                    const QFileInfo gtestResult(testsController_->latestTestResultFile(path));
                    const QFileInfo exe(path);

                    if (gtestResult.lastModified() < exe.lastModified())
                    {
                        // out of date! re-run.
                        emit showMessage(
                            "Automatic testing enabled for: " + topLeft.data(Qt::DisplayRole).toString() +
                            ". Re-running tests...");
                        runTestInThread(topLeft.data(QExecutableModel::PathRole).toString(), {}, true, RunTests);
                    }
                }

                // update previous state
                testsController_->setAutoRun(path, topLeft.data(QExecutableModel::AutorunRole).toBool());
            }, Qt::QueuedConnection);

    // filter test results when the filter is changed
    connect(testCaseFilterEdit, &QLineEdit::textChanged, this, [this](const QString& text)
    {
        if (QRegularExpression(text).isValid())
        {
            testCaseProxyModel->setFilterRegularExpression(text);
            if (testCaseProxyModel->rowCount())
            {
                for (int i = 0; i < testCaseProxyModel->columnCount(); ++i)
                {
                    testCaseTableView->resizeColumnToContents(i);
                }
            }
        }
    });

    connect(testCaseFilterNotExecuted, &QCheckBox::toggled, this, [this](const bool checked)
    {
        testCaseProxyModel->setShowNotExecuted(checked);
        if (testCaseProxyModel->rowCount())
        {
            for (int i = 0; i < testCaseProxyModel->columnCount(); ++i)
            {
                testCaseTableView->resizeColumnToContents(i);
            }
        }
    });

    connect(testCaseFilterPassed, &QCheckBox::toggled, this, [this](const bool checked)
    {
        testCaseProxyModel->setShowPassed(checked);
        if (testCaseProxyModel->rowCount())
        {
            for (int i = 0; i < testCaseProxyModel->columnCount(); ++i)
            {
                testCaseTableView->resizeColumnToContents(i);
            }
        }
    });


    connect(testCaseFilterIgnored, &QCheckBox::toggled, this, [this](const bool checked)
    {
        testCaseProxyModel->setShowIgnored(checked);
        if (testCaseProxyModel->rowCount())
        {
            for (int i = 0; i < testCaseProxyModel->columnCount(); ++i)
            {
                testCaseTableView->resizeColumnToContents(i);
            }
        }
    });

    // create a failure model when a test is clicked
    connect(testCaseTableView->selectionModel(), &QItemSelectionModel::currentChanged,
            [this](const QModelIndex& current, const QModelIndex&)
            {
                if (!current.isValid())
                    return;

                auto index = testCaseProxyModel->mapToSource(current);
                if (index.column() < GTestModel::ResultAndTime)
                {
                    // Auto-Select the first failure model
                    index = testCaseProxyModel->sourceModel()->index(index.row(), GTestModel::ResultAndTime,
                                                                     index.parent());
                }
                const FlatDomeItemPtr item = static_cast<GTestModel *>(testCaseProxyModel->sourceModel())->
                        itemForIndex(index);

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
    connect(testCaseTableView, &QTableView::doubleClicked, [this](const QModelIndex& index)
    {
        if (index.isValid())
            failureDock->show();
    });

    // copy failure line to clipboard (to support IDE Ctrl-G + Ctrl-V)
    // also, highlight it in the console model
    connect(failureTreeView, &QTableView::clicked, [this](const QModelIndex& index)
    {
        if (index.isValid())
        {
            QApplication::clipboard()->setText(index.data(GTestFailureModel::LineRole).toString());
            // yay, the path strings are TOTALLY different between the different OS's
#ifdef _MSC_VER
            QString findString = QDir::toNativeSeparators(index.data(GTestFailureModel::PathRole).toString()) + "(" +
                                 index.data(GTestFailureModel::LineRole).toString() + ")";
#else
			QString findString = index.data(GTestFailureModel::PathRole).toString() + ":" + index.data(GTestFailureModel::LineRole).toString();
#endif
            consoleTextEdit->find(findString, QTextDocument::FindBackward);
            consoleTextEdit->find(findString);
            scrollToConsoleCursor();
        }
    });

    // open file on double-click
    connect(failureTreeView, &QTableView::doubleClicked, [this](const QModelIndex& index)
    {
        if (index.isValid())
            QDesktopServices::openUrl(QUrl::fromLocalFile(index.data(GTestFailureModel::PathRole).toString()));
    });

    // display test output in the console window
    connect(this, &MainWindowPrivate::testOutputReady, this, [this](const QString& text)
    {
        // add the new test output
        if (!text.isEmpty())
        {
            consoleTextEdit->moveCursor(QTextCursor::End);
            consoleTextEdit->insertPlainText(text);
            consoleTextEdit->moveCursor(QTextCursor::End);
            consoleTextEdit->ensureCursorVisible();
        }
    }, Qt::QueuedConnection);

    // update test progress
    connect(this, &MainWindowPrivate::testProgress, this,
            [this](const QString& test, const int complete, const int total)
            {
                const QModelIndex index = executableModel->index(test);
                executableModel->setData(index, static_cast<double>(complete) / total, QExecutableModel::ProgressRole);
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
    connect(consolePrevFailureButton, &QPushButton::pressed, [this]
    {
        static const QRegularExpression regex("\\[\\s+RUN\\s+\\].*?[\n](.*?): ((?!OK).)*?\\[\\s+FAILED\\s+\\]",
                                              QRegularExpression::MultilineOption |
                                              QRegularExpression::DotMatchesEverythingOption);
        auto matches = regex.globalMatch(consoleTextEdit->toPlainText());

        QRegularExpressionMatch match;
        QTextCursor c = consoleTextEdit->textCursor();

        while (matches.hasNext())
        {
            auto nextMatch = matches.peekNext();
            if (nextMatch.capturedEnd() >= c.position())
            {
                break;
            }
            match = matches.next();
        }

        if (match.capturedStart() > 0)
        {
            c.setPosition(match.capturedStart(1));
            consoleTextEdit->setTextCursor(c);
            scrollToConsoleCursor();
            c.setPosition(match.capturedEnd(1), QTextCursor::KeepAnchor);
            consoleTextEdit->setTextCursor(c);
        }
    });

    // find the next failure when the button is pressed
    connect(consoleNextFailureButton, &QPushButton::pressed, [this]
    {
        static const QRegularExpression regex("\\[\\s+RUN\\s+\\].*?[\n](.*?): ((?!OK).)*?\\[\\s+FAILED\\s+\\]",
                                              QRegularExpression::MultilineOption |
                                              QRegularExpression::DotMatchesEverythingOption);
        auto matches = regex.globalMatch(consoleTextEdit->toPlainText());

        QRegularExpressionMatch match;
        QTextCursor c = consoleTextEdit->textCursor();

        while (matches.hasNext())
        {
            match = matches.next();
            if (match.capturedEnd() >= c.position())
            {
                if (matches.hasNext())
                    match = matches.next();
                break;
            }
        }

        if (match.capturedStart() > 0)
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
//	FUNCTION: addTestExecutable
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::addTestExecutable(const QString& path, const QString& name, const QString& testDriver,
                                          const bool autorun,
                                          QDateTime lastModified, const QString& filter /*= ""*/,
                                          const int repeat /*= 0*/,
                                          const Qt::CheckState runDisabled /*= Qt::Unchecked*/,
                                          const Qt::CheckState breakOnFailure /*= Qt::Unchecked*/,
                                          const Qt::CheckState failFast /*= Qt::Unchecked*/,
                                          const Qt::CheckState shuffle /*= Qt::Unchecked*/,
                                          const int randomSeed /*= 0*/, const QString& otherArgs /*= ""*/)
{
    const QFileInfo fileinfo(path);

    if (!fileinfo.exists())
        return;

    if (!fileinfo.isExecutable() || !fileinfo.isFile())
        return;

    if (executableModel->index(path).isValid())
        return;

    if (lastModified == QDateTime())
        lastModified = fileinfo.lastModified();

    testsController_->addTest(path);
    testsController_->setAutoRun(path, autorun);

    const QModelIndex newRow = executableModel->insertRow(QModelIndex(), path);

    executableModel->setData(newRow, 0, QExecutableModel::ProgressRole);
    executableModel->setData(newRow, path, QExecutableModel::PathRole);
    executableModel->setData(newRow, name, QExecutableModel::NameRole);
    executableModel->setData(newRow, testDriver, QExecutableModel::TestDriverRole);
    executableModel->setData(newRow, autorun, QExecutableModel::AutorunRole);
    executableModel->setData(newRow, lastModified, QExecutableModel::LastModifiedRole);
    executableModel->setData(newRow, ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);
    executableModel->setData(newRow, filter, QExecutableModel::FilterRole);
    executableModel->setData(newRow, repeat, QExecutableModel::RepeatTestsRole);
    executableModel->setData(newRow, runDisabled, QExecutableModel::RunDisabledTestsRole);
    executableModel->setData(newRow, breakOnFailure, QExecutableModel::BreakOnFailureRole);
    executableModel->setData(newRow, failFast, QExecutableModel::FailFastRole);
    executableModel->setData(newRow, shuffle, QExecutableModel::ShuffleRole);
    executableModel->setData(newRow, randomSeed, QExecutableModel::RandomSeedRole);
    executableModel->setData(newRow, otherArgs, QExecutableModel::ArgsRole);

    fileWatcher->addPath(fileinfo.dir().canonicalPath());
    fileWatcher->addPath(path);
    executablePaths << path;

    const bool previousResults = loadTestResults(path, false);
    const bool outOfDate = lastModified < fileinfo.lastModified();

    executableTreeView->setCurrentIndex(newRow);
    for (int i = 0; i < executableModel->columnCount(); i++)
    {
        executableTreeView->resizeColumnToContents(i);
    }

    testKillHandler_[path] = nullptr;
    testRunningHash[path] = false;
    latestBuildChangeTime_[path] = lastModified;

    RunMode runMode = NoTests;
    if (!testsController_->hasOverview(path) &&
        autoUpdateTestListAction_->isChecked())
    {
        runMode = ListTests;
    }

    // only run if test has run before and is out of date now
    if (outOfDate)
    {
        if (autoUpdateTestListAction_->isChecked())
        {
            runMode = ListTests;
        }
        // only auto-run if the test is checked
        if (previousResults && autorun)
        {
            runMode = runMode == ListTests ? ListAndRunTests : RunTests;
        }
        else
        {
            executableModel->setData(newRow, ExecutableData::NOT_RUNNING, QExecutableModel::StateRole);
        }
    }
    if (runMode != NoTests)
    {
        this->runTestInThread(path, {}, false, runMode);
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: runTestInThread
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::runTestInThread(const QString& pathToTest, const QString& tempTestFilter, const bool notify,
                                        const RunMode runMode)
{
    std::thread t([this, pathToTest, tempTestFilter, notify, runMode]
    {
        QEventLoop loop;

        // kill the running test instance first if there is one
        if (testRunningHash[pathToTest])
        {
            emitKillTest(pathToTest);

            std::unique_lock<std::mutex> lock(threadKillMutex);
            threadKillCv.wait(lock, [&, pathToTest]
            {
                return !testRunningHash[pathToTest];
            });
        }

        testRunningHash[pathToTest] = true;
        numberOfRunningTests_ += 1;
        updateButtonsForRunningTests();

        executableModel->setData(executableModel->index(pathToTest), ExecutableData::RUNNING,
                                 QExecutableModel::StateRole);

        const QFileInfo info(pathToTest);
        executableModel->setData(executableModel->index(pathToTest), info.lastModified(),
                                 QExecutableModel::LastModifiedRole);
        const QString testName = executableModel->index(pathToTest).data(QExecutableModel::NameRole).toString();
        QProcess testProcess;

        bool first = true;
        int tests = 0;
        int progress = 0;


        auto tearDown = [&, pathToTest](const QString& output)
        {
            emit testOutputReady(output);
            emit testProgress(pathToTest, 0, 0);

            testKillHandler_[pathToTest] = nullptr;
            testRunningHash[pathToTest] = false;
            numberOfRunningTests_ -= 1;
            updateButtonsForRunningTests();
            threadKillCv.notify_all();

            loop.exit();

            // Now run the test
            if (runMode == ListAndRunTests)
            {
                emit runTestInThread(pathToTest, tempTestFilter, notify, RunTests);
            }
        };

        // when the process finished, read any remaining output then quit the loop
        connect(&testProcess, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), &loop,
                [&, pathToTest]
        (const int exitCode, const QProcess::ExitStatus exitStatus)
                {
                    executableModel->setData(executableModel->index(pathToTest), ExecutableData::NOT_RUNNING,
                                             QExecutableModel::StateRole);

                    QString output;
                    if (runMode == RunTests)
                    {
                        output = testProcess.readAllStandardOutput();
                    }

                    // 0 for success and 1 if test have failed -> in both cases a result xml was generated
                    if (exitStatus == QProcess::NormalExit && (exitCode == 0 || exitCode == 1))
                    {
                        if (runMode == RunTests)
                        {
                            output.append(
                                "\nTEST RUN COMPLETED: " + QDateTime::currentDateTime().toString(
                                    "yyyy-MMM-dd hh:mm:ss.zzz")
                                + " " + testName + "\n\n");
                        }
                        emit testResultsReady(pathToTest, notify);
                    }
                    else
                    {
                        if (runMode == RunTests)
                        {
                            output.append(
                                "\nTEST RUN EXITED WITH ERRORS: " + QDateTime::currentDateTime().toString(
                                    "yyyy-MMM-dd hh:mm:ss.zzz") + " " + testName + "\n\n");
                        }
                        else
                        {
                            output.append(
                                "\nLIST TEST EXITED WITH ERRORS: " + QDateTime::currentDateTime().toString(
                                    "yyyy-MMM-dd hh:mm:ss.zzz") + " " + testName + "\n\n");
                        }
                    }

                    tearDown(output);
                }, Qt::QueuedConnection);

        testKillHandler_[pathToTest] = new KillTestWrapper(&testProcess);

        // get killed if asked to do so
        connect(testKillHandler_[pathToTest].load(), &KillTestWrapper::killTest, &loop, [&, pathToTest]
        {
            // Try at least a few times to terminate gracefully
            // If User uses "Kill All Tests" the TestDriver can be in a state,
            // where he is not ready to receive the "terminate" command, yet.
            bool terminated = false;
            for (int i = 0; i < 10; ++i)
            {
                // terminate over std::in, as testProcess.terminate() sends WM_CLOSE under windows that is complex to catch
                testProcess.write("terminate\n");
                testProcess.waitForBytesWritten();
                // Give 0.5s to finish
                if (testProcess.waitForFinished(500))
                {
                    terminated = true;
                    break;
                }
            }
            if (!terminated)
            {
                testProcess.kill();
                testProcess.waitForFinished();
            }
            QString output = testProcess.readAllStandardOutput();
            output.append(
                "\nTEST RUN KILLED: " + QDateTime::currentDateTime().toString("yyyy-MMM-dd hh:mm:ss.zzz") + " " +
                testName + "\n\n");

            executableModel->setData(executableModel->index(pathToTest), ExecutableData::NOT_RUNNING,
                                     QExecutableModel::StateRole);
            tearDown(output);
        }, Qt::QueuedConnection);


        // Register killTest before lock -> possible to Kill All Tests
        const int numberLock = runTestsSynchronousAction_->isChecked() ? MAX_PARALLEL_TEST_EXEC : 1;
        const SemaphoreLocker runTestThreadSynchronousLock(runTestParallelSemaphore_, numberLock);
        // Check if asked to be killed
        if (!testKillHandler_[pathToTest].load() || testKillHandler_[pathToTest].load()->isKillRequested())
        {
            executableModel->setData(executableModel->index(pathToTest), ExecutableData::NOT_RUNNING,
                                     QExecutableModel::StateRole);
            tearDown("");
            return;
        }

        const QString outputDir = xmlPath(pathToTest, true);

        // SET GTEST ARGS
        const QModelIndex index = executableModel->index(pathToTest);
        const QString testDriver = executableModel->data(index, QExecutableModel::TestDriverRole).toString();

        QStringList arguments;

        arguments << testDriver;
        arguments << "-C";
        arguments << info.dir().dirName();

        if (pipeAllTestOutput_->isChecked())
        {
            arguments << "--pipe-log";
        }

        QString gtestFileName;
        if (runMode != RunTests)
        {
            gtestFileName = outputDir + "/" + GTEST_LIST_NAME;
        }
        else
        {
            const auto currentTime = QDateTime::currentDateTime().toString(DATE_FORMAT);
            gtestFileName = outputDir + "/" + currentTime + "_" + GTEST_RESULT_NAME;
        }
        arguments << "--gtest_output=xml:\"" + gtestFileName + "\"";

        int repeatCount = 1;
        if (runMode != RunTests)
        {
            arguments << "--gtest_list_tests";
        }
        else
        {
            const QString latestCopyDir = outputDir + "/" + LATEST_RESULT_DIR_NAME;
            QDir(latestCopyDir).removeRecursively();
            (void) QDir(latestCopyDir).mkpath(".");

            arguments << "--output-dir";
            arguments << latestCopyDir;

            // Take temp test filter if given. Otherwise, take configured
            if (!tempTestFilter.isEmpty())
            {
                arguments << "--gtest_filter=" + tempTestFilter;
                emit testOutputReady(
                    "Run Test '" + pathToTest + "' with temporary filter:\n" + tempTestFilter + "\n\n");
            }
            else
            {
                const QString filter = executableModel->data(index, QExecutableModel::FilterRole).toString();
                if (!filter.isEmpty())
                {
                    arguments << "--gtest_filter=" + filter;
                    emit testOutputReady("Run Test '" + pathToTest + "' with fixed filter:\n" + filter + "\n\n");
                }
            }

            const QString repeat = executableModel->data(index, QExecutableModel::RepeatTestsRole).toString();
            if (repeat != "0" && repeat != "1") arguments << "--gtest_repeat=" + repeat;

            if (repeat.toInt() > 1) repeatCount = repeat.toInt();

            const int runDisabled = executableModel->data(index, QExecutableModel::RunDisabledTestsRole).toInt();
            if (runDisabled) arguments << "--gtest_also_run_disabled_tests";

            const int breakOnFailure = executableModel->data(index, QExecutableModel::BreakOnFailureRole).toInt();
            if (breakOnFailure) arguments << "--gtest_break_on_failure";

            const int failFast = executableModel->data(index, QExecutableModel::FailFastRole).toInt();
            if (failFast) arguments << "--gtest_fail_fast";

            const int shuffle = executableModel->data(index, QExecutableModel::ShuffleRole).toInt();
            if (shuffle) arguments << "--gtest_shuffle";

            const int seed = executableModel->data(index, QExecutableModel::RandomSeedRole).toInt();
            if (shuffle) arguments << "--gtest_random_seed=" + QString::number(seed);

            const QString otherArgs = executableModel->data(index, QExecutableModel::ArgsRole).toString();
            if (!otherArgs.isEmpty()) arguments << otherArgs;
        }

#ifdef Q_OS_WIN32
        QString cmd = "\"" + currentRunEnvPath_ + "\" && " PYTHON;
        // Start the test
        testProcess.start(cmd, arguments);
#else
                QString cmd = "bash -c \"source " + currentRunEnvPath_ + "; " PYTHON " " + arguments.join(" ") +  "\"";
                // Start the test
                testProcess.start(cmd);
#endif

        // get the first line of output. If we don't get it in a timely manner, the test is
        // probably bugged out so kill it.
        if (!testProcess.waitForReadyRead(10000))
        {
            testProcess.kill();

            executableModel->setData(executableModel->index(pathToTest), ExecutableData::NOT_RUNNING,
                                     QExecutableModel::StateRole);
            tearDown("");
            return;
        }

        // print test output as it becomes available
        if (runMode == RunTests)
        {
            connect(&testProcess, &QProcess::readyReadStandardOutput, &loop, [&, pathToTest]
            {
                const QString output = testProcess.readAllStandardOutput();

                // parse the first output line for the number of tests so we can
                // keep track of progress
                if (first)
                {
                    // get the number of tests
                    static const QRegularExpression rx("([0-9]+) tests");
                    tests = rx.match(output).captured(1).toInt();
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
                    static const QRegularExpression rx(R"((\[.*OK.*\]|\[.*FAILED.*\]))");
                    if (rx.globalMatch(output).hasNext())
                        progress++;
                }

                emit testProgress(pathToTest, progress, tests);
                emit testOutputReady(output);
            }, Qt::QueuedConnection);
        }

        loop.exec();
    });
    t.detach();
}

void MainWindowPrivate::updateButtonsForRunningTests() const
{
    const int value = numberOfRunningTests_;
    // Disable/Enable various UI buttons that are not supported if any test is running
    if (value == 1 || value == 0)
    {
        const bool areAnyRunning = value != 0;

        // invoke -> we could be running in another thread
        QMetaObject::invokeMethod(addRunEnvAction, "setEnabled", Q_ARG(bool, !areAnyRunning));
        QMetaObject::invokeMethod(removeRunEnvAction_, "setEnabled", Q_ARG(bool, !areAnyRunning));
        QMetaObject::invokeMethod(runEnvComboBox_, "setEnabled", Q_ARG(bool, !areAnyRunning));
        QMetaObject::invokeMethod(updateTestsButton, "setEnabled", Q_ARG(bool, !areAnyRunning));

        QMetaObject::invokeMethod(killAllTestsAction_, "setEnabled", Q_ARG(bool, areAnyRunning));
        QMetaObject::invokeMethod(removeAllTestsAction_, "setEnabled", Q_ARG(bool, !areAnyRunning));

        // If we want to update actions from ContextMenu -> we have to also check valid index
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: loadTestResults
//--------------------------------------------------------------------------------------------------
bool MainWindowPrivate::loadTestResults(const QString& testPath, const bool notify)
{
    int numberErrors = 0;
    bool newTestResult = false;
    if (!testsController_->loadLatestTestResult(testPath, numberErrors, newTestResult))
    {
        return false;
    }

    // if the test that just ran is selected, update the view
    const QModelIndex index = executableTreeView->selectionModel()->currentIndex();

    if (index.data(QExecutableModel::PathRole).toString() == testPath)
    {
        selectTest(testPath);
    }

    // set executable icon
    if (numberErrors)
    {
        executableModel->setData(executableModel->index(testPath), ExecutableData::FAILED, QExecutableModel::StateRole);
        mostRecentFailurePath = testPath;
        const QString name = executableModel->index(testPath).data(QExecutableModel::NameRole).toString();
        // only show notifications AFTER the initial startup, otherwise the user
        // could get a ton of messages every time they open the program. The messages
        if (notify && newTestResult && notifyOnFailureAction->isChecked())
        {
            systemTrayIcon->showMessage("Test Failure",
                                        name + " failed with " + QString::number(numberErrors) + " errors.");
        }
    }
    else
    {
        executableModel->setData(executableModel->index(testPath), ExecutableData::PASSED, QExecutableModel::StateRole);
        const QString name = executableModel->index(testPath).data(QExecutableModel::NameRole).toString();
        if (notify && newTestResult && notifyOnSuccessAction->isChecked())
        {
            systemTrayIcon->showMessage("Test Successful", name + " ran with no errors.");
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: selectTest
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::selectTest(const QString& testPath) const
{
    const auto indices = testCaseTableView->selectionModel()->selectedRows(GTestModel::Sections::Name);
    QStringList currentSelectedTestNames;
    currentSelectedTestNames.reserve(indices.size());
    for (const auto& index : indices)
    {
        currentSelectedTestNames.append(index.data(Qt::DisplayRole).toString());
    }

    // Delete the old failure models and make new ones
    delete failureProxyModel->sourceModel();
    testCaseTableView->setSortingEnabled(false);

    testCaseProxyModel->setSourceModel(testsController_->gTestModel(testPath).get());
    failureProxyModel->invalidate();
    testCaseTableView->setSortingEnabled(true);

    // make sure the right entry is selected
    executableTreeView->setCurrentIndex(executableModel->index(testPath));

    // resize the columns
    for (int i = 0; i < testCaseTableView->model()->columnCount(); i++)
    {
        testCaseTableView->resizeColumnToContents(i);
    }

    QItemSelection selection;
    // reset the test case selection
    if (!currentSelectedTestNames.isEmpty())
    {
        for (const auto& currentSelectedTestName : currentSelectedTestNames)
        {
            auto index = testCaseTableView->model()->index(0, GTestModel::Name);
            QModelIndexList matches = testCaseTableView->model()->
                    match(index, Qt::DisplayRole, currentSelectedTestName, 1);
            if (!matches.empty())
            {
                selection.append(QItemSelectionRange(matches.first()));
                break;
            }
        }
    }

    if (!selection.isEmpty())
    {
        testCaseTableView->selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: saveSettings
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::saveSettings() const
{
    const QDir settingsDir(settingsPath());
    if (!settingsDir.exists())
    {
        (void) settingsDir.mkpath(".");
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
    options.insert("pipeAllTestOutput", pipeAllTestOutput_->isChecked());
    options.insert("autoUpdateTestListAction", autoUpdateTestListAction_->isChecked());
    options.insert("notifyOnFailure", notifyOnFailureAction->isChecked());
    options.insert("notifyOnSuccess", notifyOnSuccessAction->isChecked());
    options.insert("theme", themeActionGroup->checkedAction()->objectName());
    options.insert("currentRunEnvPath", currentRunEnvPath_);
    options.insert("testCaseFilterNotExecuted", testCaseFilterNotExecuted->isChecked());
    options.insert("testCaseFilterPassed", testCaseFilterPassed->isChecked());
    options.insert("testCaseFilterIgnored", testCaseFilterIgnored->isChecked());
    root.insert("options", options);

    const QJsonDocument document(root);
    QFile settingsFile(path);
    if (!settingsFile.open(QIODevice::WriteOnly))
    {
        return;
    }
    settingsFile.write(document.toJson());
    settingsFile.close();
}


QString MainWindowPrivate::pathToCurrenRunEnvSettings() const
{
    if (currentRunEnvPath_.isEmpty())
    {
        return "";
    }
    const QString hash = QCryptographicHash::hash(currentRunEnvPath_.toLatin1(), QCryptographicHash::Md5).toHex();
    return settingsPath() + "/" + hash + ".json";
}

void MainWindowPrivate::saveTestSettingsForCurrentRunEnv() const
{
    const QString path = pathToCurrenRunEnvSettings();
    if (path.isEmpty())
    {
        return;
    }
    saveTestSettings(path);
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
        test.insert("name", QJsonValue::fromVariant(index.data(QExecutableModel::NameRole)));
        test.insert("testDriver", QJsonValue::fromVariant(index.data(QExecutableModel::TestDriverRole)));
        test.insert("autorun", QJsonValue::fromVariant(index.data(QExecutableModel::AutorunRole)));
        test.insert("lastModified", index.data(QExecutableModel::LastModifiedRole).toDateTime().toString(DATE_FORMAT));
        test.insert("filter", QJsonValue::fromVariant(index.data(QExecutableModel::FilterRole)));
        test.insert("repeat", QJsonValue::fromVariant(index.data(QExecutableModel::RepeatTestsRole)));
        test.insert("runDisabled", QJsonValue::fromVariant(index.data(QExecutableModel::RunDisabledTestsRole)));
        test.insert("breakOnFailure", QJsonValue::fromVariant(index.data(QExecutableModel::BreakOnFailureRole)));
        test.insert("failFast", QJsonValue::fromVariant(index.data(QExecutableModel::FailFastRole)));
        test.insert("shuffle", QJsonValue::fromVariant(index.data(QExecutableModel::ShuffleRole)));
        test.insert("seed", QJsonValue::fromVariant(index.data(QExecutableModel::RandomSeedRole)));
        test.insert("args", QJsonValue::fromVariant(index.data(QExecutableModel::ArgsRole)));
        tests.append(test);
    }
    root.insert("tests", tests);

    const QJsonDocument document(root);
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

    const auto alreadyIndex = runEnvComboBox_->findText(currentRunEnvPath_);
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
    const QJsonDocument document = QJsonDocument::fromJson(settingsFile.readAll());
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
    pipeAllTestOutput_->setChecked(options["pipeAllTestOutput"].toBool());
    // Added in a later version
    auto iter = options.find("autoUpdateTestListAction");
    if (iter != options.end())
    {
        autoUpdateTestListAction_->setChecked(iter->toBool());
    }
    notifyOnFailureAction->setChecked(options["notifyOnFailure"].toBool());
    notifyOnSuccessAction->setChecked(options["notifyOnSuccess"].toBool());
    themeMenu->findChild<QAction *>(options["theme"].toString())->trigger();
    currentRunEnvPath_ = options["currentRunEnvPath"].toString();
    removeRunEnvAction_->setEnabled(!currentRunEnvPath_.isEmpty());
    iter = options.find("testCaseFilterNotExecuted");
    if (iter != options.end())
    {
        testCaseFilterNotExecuted->setChecked(iter->toBool());
    }
    iter = options.find("testCaseFilterPassed");
    if (iter != options.end())
    {
        testCaseFilterPassed->setChecked(iter->toBool());
    }
    iter = options.find("testCaseFilterIgnored");
    if (iter != options.end())
    {
        testCaseFilterIgnored->setChecked(iter->toBool());
    }
}

void MainWindowPrivate::loadTestSettingsForCurrentRunEnv()
{
    if (currentRunEnvPath_.isEmpty())
    {
        return;
    }
    const QString hash = QCryptographicHash::hash(currentRunEnvPath_.toLatin1(), QCryptographicHash::Md5).toHex();
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
    const QJsonDocument document = QJsonDocument::fromJson(settingsFile.readAll());
    settingsFile.close();

    QJsonObject root = document.object();
    QJsonArray tests = root["tests"].toArray();

    for (const auto& testRef : tests)
    {
        QJsonObject test = testRef.toObject();

        QString pathInner = test["path"].toString();
        QString name = test["name"].toString();
        QString testDriver = test["testDriver"].toString();
        const bool autorun = test["autorun"].toBool();
        const QDateTime lastModified = QDateTime::fromString(test["lastModified"].toString(), DATE_FORMAT);
        QString filter = test["filter"].toString();
        const int repeat = test["repeat"].toInt();
        const auto runDisabled = static_cast<Qt::CheckState>(test["runDisabled"].toInt());
        const auto breakOnFailure = static_cast<Qt::CheckState>(test["breakOnFailure"].toInt());
        const auto failFast = static_cast<Qt::CheckState>(test["failFast"].toInt());
        const auto shuffle = static_cast<Qt::CheckState>(test["shuffle"].toInt());
        const int seed = test["seed"].toInt();
        QString args = test["args"].toString();

        addTestExecutable(pathInner, name, testDriver, autorun, lastModified, filter, repeat, runDisabled,
                          breakOnFailure, failFast, shuffle, seed, args);
    }
}

void MainWindowPrivate::removeAllTest(const bool confirm)
{
    if (executableModel->rowCount() == 0)
    {
        // nothing to remove
        return;
    }

    if (confirm || QMessageBox::question(this->q_ptr, "Remove All Test?", "Do you want to remove all tests?",
                                         QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        for (int i = 0; i < executableModel->rowCount(); ++i)
        {
            auto index = executableModel->index(i, 0);
            QString path = index.data(QExecutableModel::PathRole).toString();

            // remove all data related to this test
            testsController_->removeTest(path);
            executablePaths.removeAll(path);
            fileWatcher->removePath(path);
        }

        const QAbstractItemModel* oldFailureModel = failureProxyModel->sourceModel();
        failureProxyModel->setSourceModel(new GTestFailureModel(nullptr));
        testCaseProxyModel->setSourceModel(testsController_->gTestModel(QString()).get());
        delete oldFailureModel;
        executableModel->removeRows(0, executableModel->rowCount());
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: removeTest
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::removeTest(const QModelIndex& index, const bool confirm)
{
    if (!index.isValid())
        return;

    const QString path = index.data(QExecutableModel::PathRole).toString();

    if (confirm || QMessageBox::question(this->q_ptr, QString("Remove Test?"),
                                         "Do you want to remove test " + index.data(QExecutableModel::NameRole).
                                         toString() + "?",
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        executableTreeView->setCurrentIndex(index);

        // remove all data related to this test
        testsController_->removeTest(path);
        executablePaths.removeAll(path);
        fileWatcher->removePath(path);

        const QAbstractItemModel* oldFailureModel = failureProxyModel->sourceModel();
        failureProxyModel->setSourceModel(new GTestFailureModel(nullptr));
        testCaseProxyModel->setSourceModel(testsController_->gTestModel(QString()).get());
        delete oldFailureModel;

        executableModel->removeRow(index.row(), index.parent());
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: clearData
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::clearData() const
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

void MainWindowPrivate::createToolBar()
{
    Q_Q(MainWindow);

    addRunEnvAction = new QAction(q->style()->standardIcon(QStyle::SP_DialogOpenButton),
                                  "Add Run Environment", q);
    addRunEnvAction->setToolTip("Add new Run Environment to automatically find & execute build tests");
    runEnvComboBox_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    runEnvComboBox_->setModel(runEnvModel_);
    runEnvComboBox_->setToolTip(
        "Select current Run Environment. You can start a separate instance of gtest-runner for each environment");
    removeRunEnvAction_ = new QAction(q->style()->standardIcon(QStyle::SP_TrashIcon),
                                      "Remove Run Environment", q);
    removeRunEnvAction_->setToolTip(
        "Removes current Run Environment (doesn't delete test data). If you want to remove test data, you have to do it manually.");

    runAllFailedTestsAction = new QAction(QIcon(":images/runFailedTests"), "Run All Failed Tests", toolBar_);
    runAllFailedTestsAction->setShortcut(QKeySequence(Qt::Key_F4));
    runAllTestsAction = new QAction(QIcon(":images/runTest"), "Run All Tests", toolBar_);
    runAllTestsAction->setShortcut(QKeySequence(Qt::Key_F6));
    killAllTestsAction_ = new QAction(q->style()->standardIcon(QStyle::SP_BrowserStop), "Kill All Tests", toolBar_);
    killAllTestsAction_->setShortcuts({QKeySequence(Qt::SHIFT | Qt::Key_F6), QKeySequence(Qt::SHIFT | Qt::Key_F4)});
    updateAllTestsListAction = new QAction(QIcon(":images/updateTestList"), "Update All Tests List", toolBar_);
    updateAllTestsListAction->setShortcut(QKeySequence(Qt::Key_F8));
    revealExplorerTestResultAction_ = new QAction(q->style()->standardIcon(QStyle::SP_DirOpenIcon),
                                                  "Reveal Test Result Dir", toolBar_);
    removeAllTestsAction_ = new QAction(q->style()->standardIcon(QStyle::SP_TrashIcon), "Remove All Tests", toolBar_);

    toolBar_->addWidget(new QLabel("Env:", toolBar_));
    toolBar_->addAction(addRunEnvAction);
    toolBar_->addWidget(runEnvComboBox_);
    toolBar_->addAction(removeRunEnvAction_);
    toolBar_->addSeparator();
    toolBar_->addWidget(new QLabel("All:", toolBar_));
    toolBar_->addAction(runAllFailedTestsAction);
    toolBar_->addAction(runAllTestsAction);
    toolBar_->addAction(killAllTestsAction_);
    toolBar_->addAction(updateAllTestsListAction);
    toolBar_->addAction(revealExplorerTestResultAction_);
    toolBar_->addAction(removeAllTestsAction_);

    connect(addRunEnvAction, &QAction::triggered, [this]()
    {
        const QString filter = "RunEnv (*.bat *.sh)";
        const QString caption = "Select RunEnv.bat or RunEnv.sh";
        const QString filename = QFileDialog::getOpenFileName(q_ptr, caption, currentRunEnvPath_, filter);

        if (filename.isEmpty())
        {
            return;
        }
        else
        {
            const QFileInfo info(filename);
            const QString runEnvPath = info.absoluteFilePath();
            const auto alreadyIndex = runEnvComboBox_->findText(runEnvPath);
            if (alreadyIndex != -1)
            {
                runEnvComboBox_->setCurrentIndex(alreadyIndex);
                return;
            }

            saveTestSettingsForCurrentRunEnv();
            removeAllTest(true);
            currentRunEnvPath_ = runEnvPath;
            removeRunEnvAction_->setEnabled(!currentRunEnvPath_.isEmpty());
            updateTestExecutables();

            if (runEnvModel_->insertRow(runEnvModel_->rowCount()))
            {
                const auto index = runEnvModel_->index(runEnvModel_->rowCount() - 1, 0);
                runEnvModel_->setData(index, runEnvPath);
            }
            runEnvComboBox_->setCurrentIndex(runEnvComboBox_->count() - 1);
        }
    });

    connect(runEnvComboBox_, &QComboBox::currentTextChanged, [this]
    (const QString& selectedRunEnv)
            {
                // only reload if changed and had one selected before
                if (currentRunEnvPath_ != selectedRunEnv && !currentRunEnvPath_.isEmpty() && !selectedRunEnv.isEmpty())
                {
                    saveTestSettingsForCurrentRunEnv();
                    removeAllTest(true);
                    currentRunEnvPath_ = selectedRunEnv;
                    removeRunEnvAction_->setEnabled(!currentRunEnvPath_.isEmpty());
                    loadTestSettingsForCurrentRunEnv();
                }
            });

    connect(removeRunEnvAction_, &QAction::triggered, [this]()
    {
        if (QMessageBox::question(this->q_ptr, "Remove Run Env", "Do you want to remove current Run Environment?",
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        {
            QFile(pathToCurrenRunEnvSettings()).remove();
            removeAllTest(true);
            currentRunEnvPath_ = "";
            runEnvModel_->removeRow(runEnvComboBox_->currentIndex());
            if (runEnvComboBox_->count() > 0)
            {
                currentRunEnvPath_ = runEnvComboBox_->currentText();
                loadTestSettingsForCurrentRunEnv();
            }
            removeRunEnvAction_->setEnabled(!currentRunEnvPath_.isEmpty());
        }
    });

    connect(runAllFailedTestsAction, &QAction::triggered, [this]
    {
        for (int i = 0; i < executableTreeView->model()->rowCount(); ++i)
        {
            QModelIndex index = executableTreeView->model()->index(i, 0);
            const QString path = index.data(QExecutableModel::PathRole).toString();
            const QString testFilter = testFilterForAllFailedTests(path);
            if (!testFilter.isEmpty())
            {
                runTestInThread(path, testFilter, false, RunTests);
            }
        }
    });

    connect(runAllTestsAction, &QAction::triggered, [this]
    {
        for (int i = 0; i < executableTreeView->model()->rowCount(); ++i)
        {
            QModelIndex index = executableTreeView->model()->index(i, 0);
            runTestInThread(index.data(QExecutableModel::PathRole).toString(), {}, false, RunTests);
        }
    });

    connect(killAllTestsAction_, &QAction::triggered, [this]
    {
        killAllTest();
    });

    connect(updateAllTestsListAction, &QAction::triggered, [this]
    {
        for (int i = 0; i < executableTreeView->model()->rowCount(); ++i)
        {
            QModelIndex index = executableTreeView->model()->index(i, 0);
            runTestInThread(index.data(QExecutableModel::PathRole).toString(), {}, false, ListTests);
        }
    });

    connect(revealExplorerTestResultAction_, &QAction::triggered, [this]
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dataPath()));
    });

    connect(removeAllTestsAction_, &QAction::triggered, [this]
    {
        removeAllTest();
    });
}

void MainWindowPrivate::killAllTest(const bool confirm)
{
    if (confirm || QMessageBox::question(this->q_ptr, "Kill All Test?", "Are you sure you want to kill all test?",
                                         QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        for (int i = 0; i < executableTreeView->model()->rowCount(); ++i)
        {
            QModelIndex index = executableTreeView->model()->index(i, 0);
            emitKillTest(index.data(QExecutableModel::PathRole).toString());
        }
    }
}

void MainWindowPrivate::killAllTestAndWait()
{
    killAllTest(true);
    // Busy waiting till all threads are terminated
    while (numberOfRunningTests_ != 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void MainWindowPrivate::updateTestExecutables()
{
    if (currentRunEnvPath_.isEmpty())
    {
        return;
    }

    QDir homeBase = QFileInfo(currentRunEnvPath_).dir();

    QProgressDialog progress("Your RunEnv.bat/sh dir is searched for all build tests.\n"
                             "Please wait till all tests are found.",
                             "No abort possible", 0, 100, q_ptr);
    progress.setWindowModality(Qt::WindowModal);

    // Remove all old and add again
    removeAllTest(true);

    homeBase.setNameFilters({TEST_DRIVER_NAME});
    QDirIterator it(homeBase, QDirIterator::Subdirectories);
    int i = 0;
    while (it.hasNext())
    {
        progress.setValue(i++);
        QFileInfo testDriverFileInfo(it.next());

        // Get test executables with list-test-exes option
        QProcess testProcess;
        QStringList arguments(testDriverFileInfo.absoluteFilePath());
        arguments << "--list-test-exes";
        testProcess.start(PYTHON, arguments);

        if (testProcess.waitForFinished(500))
        {
            QString output = testProcess.readAllStandardOutput();
            static const QRegularExpression sep("[\r\n]");
            QStringList executables = output.split(sep, Qt::SkipEmptyParts);
            for (const auto& testExe : executables)
            {
                QString name = QFileInfo(testExe).baseName();
                // Only add suffix if more than one build
                if (executables.size() != 1)
                {
                    if (testExe.contains("Debug"))
                        name.append(" (Debug)");
                    else if (testExe.contains("RelWithDebInfo"))
                        name.append(" (RelWithDebInfo)");
                    else if (testExe.contains("Release"))
                        name.append(" (Release)");
                    else if (testExe.contains("MinSizeRel"))
                        name.append(" (MinSizeRel)");
                }

                addTestExecutable(testExe.trimmed(), name, testDriverFileInfo.absoluteFilePath(), false, QDateTime());
            }
        }
        else
        {
            testProcess.kill();
        }
    }

    progress.setValue(progress.maximum());
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createExecutableContextMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createExecutableContextMenu()
{
    Q_Q(MainWindow);

    executableContextMenu = new QMenu(executableTreeView);
    executableContextMenu->setToolTipsVisible(true);

    runFailedTestAction = new QAction(QIcon(":images/runFailedTests"), "Run Failed Tests", executableTreeView);
    runFailedTestAction->setShortcut(QKeySequence(Qt::Key_F3));
    runTestAction = new QAction(QIcon(":images/runTest"), "Run Test", executableTreeView);
    runTestAction->setShortcut(QKeySequence(Qt::Key_F5));
    killTestAction = new QAction(q->style()->standardIcon(QStyle::SP_BrowserStop), "Kill Test", executableTreeView);
    killTestAction->setShortcuts({QKeySequence(Qt::SHIFT | Qt::Key_F5), QKeySequence(Qt::SHIFT | Qt::Key_F3)});
    updateTestListAction = new QAction(QIcon(":images/updateTestList"), "Update Test List", executableTreeView);
    updateTestListAction->setShortcut(QKeySequence(Qt::Key_F7));
    revealExplorerTestAction_ = new QAction(q->style()->standardIcon(QStyle::SP_DirOpenIcon), "Reveal Test Results",
                                            executableTreeView);

    removeTestAction = new QAction(q->style()->standardIcon(QStyle::SP_TrashIcon), "Remove Test", executableTreeView);

    // Also add here for shortcut
    executableTreeView->addAction(runFailedTestAction);
    executableTreeView->addAction(runTestAction);
    executableTreeView->addAction(killTestAction);
    executableTreeView->addAction(updateTestListAction);

    executableContextMenu->addAction(runFailedTestAction);
    executableContextMenu->addAction(runTestAction);
    executableContextMenu->addAction(killTestAction);
    executableContextMenu->addAction(updateTestListAction);
    executableContextMenu->addAction(revealExplorerTestAction_);
    executableContextMenu->addSeparator();
    executableContextMenu->addAction(removeTestAction);

    executableTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    auto setEnable = [this](const QModelIndex& index)
    {
        if (index.isValid())
        {
            const bool isTestRunning = testRunningHash[index.data(QExecutableModel::PathRole).toString()];
            bool anyTestFailed = false;
            if (testCaseProxyModel->sourceModel())
            {
                const auto failureData = testCaseProxyModel->sourceModel()->data(
                    testCaseProxyModel->sourceModel()->index(0, GTestModel::ResultAndTime),
                    GTestModel::FailureRole);
                if (failureData.isValid())
                {
                    anyTestFailed = failureData.toInt() != 0;
                }
            }
            runFailedTestAction->setEnabled(anyTestFailed);
            runTestAction->setEnabled(true);
            killTestAction->setEnabled(isTestRunning);
            updateTestListAction->setEnabled(!isTestRunning);
            revealExplorerTestAction_->setEnabled(
                QDir(xmlPath(index.data(QExecutableModel::PathRole).toString())).exists());
            removeTestAction->setEnabled(!isTestRunning);
        }
        else
        {
            runFailedTestAction->setEnabled(false);
            runTestAction->setEnabled(false);
            killTestAction->setEnabled(false);
            updateTestListAction->setEnabled(false);
            revealExplorerTestAction_->setEnabled(false);
            removeTestAction->setEnabled(false);
        }
    };

    connect(executableTreeView, &QExecutableTreeView::itemSelectionChanged, [this, setEnable]()
    {
        const QModelIndex index = executableTreeView->currentIndex();
        setEnable(index);
    });

    connect(executableTreeView, &QListView::customContextMenuRequested, [this, setEnable](const QPoint& pos)
    {
        const QModelIndex index = executableTreeView->indexAt(pos);
        setEnable(index);
        executableContextMenu->exec(executableTreeView->mapToGlobal(pos));
    });

    connect(runFailedTestAction, &QAction::triggered, [this]
    {
        const QModelIndex index = executableTreeView->currentIndex();
        const QString path = index.data(QExecutableModel::PathRole).toString();
        const QString testFilter = testFilterForAllFailedTests(path);
        if (!testFilter.isEmpty())
        {
            runTestInThread(path, testFilter, false, RunTests);
        }
    });

    connect(runTestAction, &QAction::triggered, [this]
    {
        const QModelIndex index = executableTreeView->currentIndex();
        const QString path = index.data(QExecutableModel::PathRole).toString();
        runTestInThread(path, {}, false, RunTests);
    });

    connect(killTestAction, &QAction::triggered, [this]
    {
        Q_Q(MainWindow);
        const QString path = executableTreeView->currentIndex().data(QExecutableModel::PathRole).toString();
        const QString name = executableTreeView->currentIndex().data(QExecutableModel::NameRole).toString();
        if (QMessageBox::question(q, "Kill Test?", "Are you sure you want to kill test: " + name + "?",
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
            emitKillTest(path);
    });

    connect(updateTestListAction, &QAction::triggered, [this]
    {
        const QModelIndex index = executableTreeView->currentIndex();
        const QString path = index.data(QExecutableModel::PathRole).toString();
        runTestInThread(path, {}, false, ListTests);
    });

    connect(revealExplorerTestAction_, &QAction::triggered, [this]
    {
        const QString path = executableTreeView->currentIndex().data(QExecutableModel::PathRole).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(xmlPath(path)));
    });

    connect(removeTestAction, &QAction::triggered, [this]
    {
        removeTest(executableTreeView->currentIndex());
    });
}

namespace
{
void testFilterForTestCase(const GTestModel& testModel, const QModelIndex& testIndex, QString& testFilter)
{
    const auto testCaseName = testIndex.data(Qt::DisplayRole).toString().trimmed();
    const FlatDomeItemPtr item = testModel.itemForIndex(testIndex);
    const auto testSuiteIndex = testModel.index(item->parentIndex(), GTestModel::Sections::Name);
    const auto testSuiteName = testSuiteIndex.data(Qt::DisplayRole).toString().trimmed();

    if (!testFilter.isEmpty())
    {
        testFilter += ":";
    }
    testFilter += testSuiteName + ".";
    if (testCaseName.contains(" ("))
    {
        // name contains (value_param)
        testFilter += testCaseName.split(" (")[0];
    }
    else
    {
        testFilter += testCaseName;
    }
}
}

QString MainWindowPrivate::testFilterForAllFailedTests(const QString& testPath) const
{
    QString testFilter;
    const auto& testModel = testsController_->gTestModel(testPath);
    for (int i = 0; i < testModel->rowCount(); ++i)
    {
        const auto& testCaseSourceIndex = testModel->index(i, GTestModel::ResultAndTime);
        const FlatDomeItemPtr& item = testModel->itemForIndex(testCaseSourceIndex);
        // Only check TestCases
        if (item &&
            item->level() == 2)
        {
            if (testModel->data(testCaseSourceIndex, GTestModel::FailureRole).toInt() != 0)
            {
                const auto& nameIndex = testModel->index(testCaseSourceIndex.row(), GTestModel::Name);
                testFilterForTestCase(*testModel, nameIndex, testFilter);
            }
        }
    }
    return testFilter;
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createTestCaseViewContextMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createTestCaseViewContextMenu()
{
    testCaseTableView->setContextMenuPolicy(Qt::CustomContextMenu);

    testCaseViewContextMenu = new QMenu(testCaseTableView);
    testCaseViewContextMenu->setToolTipsVisible(true);

    testCaseViewRunTestCaseAction_ = new QAction("", testCaseViewContextMenu);

    testCaseViewContextMenu->addAction(testCaseViewRunTestCaseAction_);
    testCaseViewContextMenu->addSeparator();

    enum SelectedLevel
    {
        Nothing,
        AllTests,
        TestSuite,
        TestCase
    };
    auto getSelectedIndexLevel = [this](const QModelIndex& testCaseTableViewIndex) -> SelectedLevel
    {
        const auto index = testCaseProxyModel->mapToSource(testCaseTableViewIndex);
        if (!index.isValid())
        {
            return Nothing;
        }
        const FlatDomeItemPtr item = static_cast<GTestModel *>(testCaseProxyModel->sourceModel())->itemForIndex(index);

        if (item && item->level() == 0)
        {
            return AllTests;
        }
        if (item && item->level() == 1)
        {
            return TestSuite;
        }
        return TestCase;
    };

    connect(testCaseTableView, &QListView::customContextMenuRequested, [this, getSelectedIndexLevel](const QPoint& pos)
    {
        const QModelIndex index = testCaseTableView->indexAt(pos);

        QString runActionText;
        switch (getSelectedIndexLevel(index))
        {
        case Nothing:
            // Nothing -> empty
            break;
        case AllTests:
        {
            runActionText = "Run All Tests";
            break;
        }
        case TestSuite:
        {
            runActionText = "Run Test Suite(s)";
            break;
        }
        case TestCase:
        {
            runActionText = "Run Test Case(s)";
            break;
        }
        default:
            break;
        }
        testCaseViewRunTestCaseAction_->setText(runActionText);
        testCaseViewRunTestCaseAction_->setEnabled(!runActionText.isEmpty());

        testCaseViewContextMenu->exec(testCaseTableView->mapToGlobal(pos));
    });

    connect(testCaseViewRunTestCaseAction_, &QAction::triggered, [this, getSelectedIndexLevel]()
    {
        const auto testCaseTableViewIndexes = testCaseTableView->selectionModel()->selectedRows(
            GTestModel::Sections::Name);
        QString testFilter;
        for (const auto& testCaseTableViewIndex : testCaseTableViewIndexes)
        {
            const auto testCaseSourceIndex = testCaseProxyModel->mapToSource(testCaseTableViewIndex);

            switch (getSelectedIndexLevel(testCaseTableViewIndex))
            {
            case Nothing:
                // Nothing -> run all
                break;
            case AllTests:
            {
                // all tests -> empty filter
                testFilter.clear();
                break;
            }
            case TestSuite:
            {
                const auto testSuiteName = testCaseSourceIndex.data(Qt::DisplayRole).toString().trimmed();
                if (!testFilter.isEmpty())
                {
                    testFilter += ":";
                }
                testFilter += testSuiteName + ".*";
                break;
            }
            case TestCase:
            {
                testFilterForTestCase(*static_cast<GTestModel *>(testCaseProxyModel->sourceModel()),
                                      testCaseSourceIndex, testFilter);
                break;
            }
            default:
                break;
            }
        }

        const QModelIndex execIndex = executableTreeView->selectionModel()->currentIndex();
        const QString path = execIndex.data(QExecutableModel::PathRole).toString();
        runTestInThread(path, testFilter, false, RunTests);
    });
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

    connect(consoleTextEdit, &QTextEdit::customContextMenuRequested, [this](const QPoint& pos)
    {
        const QScopedPointer<QMenu> consoleContextMenu(
            consoleTextEdit->createStandardContextMenu(consoleTextEdit->mapToGlobal(pos)));
        consoleContextMenu->addSeparator();
        consoleContextMenu->addAction(consoleFindAction);
        consoleContextMenu->addSeparator();
        consoleContextMenu->addAction(clearConsoleAction);
        consoleContextMenu->exec(consoleTextEdit->mapToGlobal(pos));
    });

    connect(clearConsoleAction, &QAction::triggered, consoleTextEdit, &QTextEdit::clear);
    connect(consoleFindShortcut, &QShortcut::activated, consoleFindAction, &QAction::trigger);
    connect(consoleFindAction, &QAction::triggered, [this]
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
    testMenu->setToolTipsVisible(true);

    // Actions already created by createToolBar() -> mirror functionality

    testMenu->addAction(addRunEnvAction);
    testMenu->addAction(removeRunEnvAction_);
    testMenu->addSeparator();
    testMenu->addAction(runAllFailedTestsAction);
    testMenu->addAction(runAllTestsAction);
    testMenu->addAction(killAllTestsAction_);
    testMenu->addAction(updateAllTestsListAction);
    testMenu->addAction(revealExplorerTestResultAction_);
    testMenu->addAction(removeAllTestsAction_);

    q->menuBar()->addMenu(testMenu);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: createOptionsMenu
//--------------------------------------------------------------------------------------------------
void MainWindowPrivate::createOptionsMenu()
{
    Q_Q(MainWindow);

    optionsMenu = new QMenu("Options", q);
    optionsMenu->setToolTipsVisible(true);

    runTestsSynchronousAction_ = new QAction("Run tests synchronous and not parallel", optionsMenu);
    pipeAllTestOutput_ = new QAction("Pipe all test output to Console Output", optionsMenu);
    autoUpdateTestListAction_ = new QAction("Auto-Update the Test List after Build", optionsMenu);
    notifyOnFailureAction = new QAction("Notify on auto-run Failure", optionsMenu);
    notifyOnSuccessAction = new QAction("Notify on auto-run Success", optionsMenu);


    runTestsSynchronousAction_->setCheckable(true);
    runTestsSynchronousAction_->setChecked(false);
    pipeAllTestOutput_->setCheckable(true);
    pipeAllTestOutput_->setChecked(false);
    autoUpdateTestListAction_->setCheckable(true);
    autoUpdateTestListAction_->setChecked(true);
    notifyOnFailureAction->setCheckable(true);
    notifyOnFailureAction->setChecked(true);
    notifyOnSuccessAction->setCheckable(true);
    notifyOnSuccessAction->setChecked(false);

    optionsMenu->addAction(runTestsSynchronousAction_);
    optionsMenu->addAction(pipeAllTestOutput_);
    optionsMenu->addAction(autoUpdateTestListAction_);
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
    windowMenu->setToolTipsVisible(true);
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
    themeMenu->setToolTipsVisible(true);

    defaultThemeAction = new QAction("Default Theme", themeMenu);
    defaultThemeAction->setObjectName("defaultThemeAction");
    defaultThemeAction->setCheckable(true);
    connect(defaultThemeAction, &QAction::triggered, [&]()
    {
        qApp->setStyleSheet(QString());
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
    helpMenu->setToolTipsVisible(true);

    aboutAction = new QAction("About...", helpMenu);

    helpMenu->addAction(aboutAction);

    connect(aboutAction, &QAction::triggered, []
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("About");
        msgBox.setIconPixmap(QPixmap(":images/logo").scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        msgBox.setTextFormat(Qt::RichText); //this is what makes the links clickable
        msgBox.setText("Application: " + APPINFO::name + "<br>version: " + APPINFO::version +
                       "<br>Developer: Oliver Karrenbauer" + "<br>Organization: " + APPINFO::organization +
                       "<br>Website: <a href='" + APPINFO::oranizationDomain + "'>" + APPINFO::oranizationDomain +
                       "</a>" + "<br><br>" +
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
void MainWindowPrivate::scrollToConsoleCursor() const
{
    const int cursorY = consoleTextEdit->cursorRect().top();
    QScrollBar* vbar = consoleTextEdit->verticalScrollBar();
    vbar->setValue(vbar->value() + cursorY - 0);
}

void MainWindowPrivate::emitKillTest(const QString& path)
{
    const auto findTest = testKillHandler_.find(path);
    if (findTest != testKillHandler_.end() && findTest->second.load())
    {
        findTest->second.load()->emitKillTest();
    }
}
