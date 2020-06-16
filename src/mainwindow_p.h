//--------------------------------------------------------------------------------------------------
// 
///	@PROJECT	project
///	@BRIEF		brief
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
// Copyright (c) 2016 Nic Holthaus
// Copyright (c) 2020 Oliver Karrenbauer
// 
//--------------------------------------------------------------------------------------------------

#ifndef mainwindow_p_h__
#define mainwindow_p_h__

//------------------------------
//	INCLUDE
//------------------------------
#include "mainwindow.h"
#include "qexecutablemodel.h"
#include "QBottomUpSortFilterProxy.h"
#include "qexecutabletreeview.h"
#include "QStdOutSyntaxHighlighter.h"
#include "appinfo.h"
#include "gtestModel.h"
#include "killTestWrapper.h"

#include <finddialog.h>

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDockWidget>
#include <QDomDocument>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QFrame>
#include <QHash>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPersistentModelIndex>
#include <QProcess>
#include <QPushButton>
#include <QShortcut>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTextEdit>
#include <QTreeView>
#include <QToolBar>
#include <QStringListModel>
#include <QComboBox>
#include <QSemaphore>

#include <qglobal.h>

//--------------------------------------------------------------------------------------------------
//	CLASS: MainWindowPrivate
//--------------------------------------------------------------------------------------------------
/// @brief		Private members of MainWindow
/// @details	
//--------------------------------------------------------------------------------------------------
class MainWindowPrivate : public QObject
{
	Q_OBJECT

public:

	Q_DECLARE_PUBLIC(MainWindow);

	// GUI components

        MainWindow*								q_ptr;
        QToolBar* toolBar_;
        // ComboBox and model to choose RunEnv
        QComboBox* runEnvComboBox_;
        QStringListModel* runEnvModel_;
	QDockWidget*							executableDock;							///< Dock Widget for the gtest executable selector.
	QFrame*									executableDockFrame;					///< Frame for containing the dock's sub-widgets
	QExecutableTreeView*					executableTreeView;						///< Widget to display and select gtest executables	
	QExecutableModel*						executableModel;						///< Item model for test executables.
        QPushButton*							updateTestsButton;							///< Button to update tests and select a _RunEnv if not selected before
	QFileSystemWatcher*						fileWatcher;							///< Hash table to store the file system watchers.
	QStringList								executablePaths;						///< String list of all the paths, which can be used to re-constitute the filewatcher after an executable rebuild.

	QFrame*									centralFrame;							///< Central widget frame.
	QLineEdit*								testCaseFilterEdit;						///< Line edit for filtering test cases.
	QTreeView*								testCaseTreeView;						///< Tree view where the test results are displayed.
	QBottomUpSortFilterProxy*				testCaseProxyModel;						///< Sort/filter proxy for the test-case mode.

	QDockWidget*							failureDock;							///< Dock widget for reporting failures.
	QTreeView*								failureTreeView;						///< Tree view for failures.
	QBottomUpSortFilterProxy*				failureProxyModel;						///< Proxy model for sorting failures.

	QStatusBar*								statusBar;								///< status

	QDockWidget*							consoleDock;							///< Console emulator
	QFrame*									consoleFrame;							///< Console Dock frame.
	QVBoxLayout*							consoleButtonLayout;					///< Layout for the console dock buttons.
	QHBoxLayout*							consoleLayout;							///< Console Dock Layout.
	QPushButton*							consolePrevFailureButton;						///< Jumps the the previous failure.
	QPushButton*							consoleNextFailureButton;						///< Jumps the the next failure.
	QTextEdit*								consoleTextEdit;						///< Console emulator text edit
	QStdOutSyntaxHighlighter*				consoleHighlighter;						///< Console syntax highlighter.
	FindDialog*								consoleFindDialog;						///< Dialog to find stuff in the console.
	QSystemTrayIcon*						systemTrayIcon;							///< System Tray Icon.

	// Menus
	QMenu*									executableContextMenu;					///< context menu for the executable list view.
	QAction*								killTestAction;							///< Kills a running test
        // Reveal Test Results in Explorer
        QAction* revealExplorerTestAction_;
        QAction* selectAndRevealExplorerTestAction_;
        QAction* revealExplorerTestResultAction_;
	QAction*								runTestAction;							///< Manually forces a test-run.
	QAction*								removeTestAction;						///< Removes a test from being watched.
        // Remove all tests
        QAction* removeAllTestsAction_;
	
	QMenu*									optionsMenu;							///< Menu to set system options
        // Enable synchronous test execution
        QAction* runTestsSynchronousAction_;
        // Enable to pipe all test output to console
        QAction* pipeAllTestOutput_;
	QAction*								notifyOnFailureAction;					///< Enable failure notifications
	QAction*								notifyOnSuccessAction;					///< Enable success notifications

	QMenu*									windowMenu;								///< Menu to display/change dock visibility.
	QMenu*									testMenu;								///< Menu for test-related actions

        QAction*								addRunEnvAction;							///< Opens a dialog to add a new _RunEnv
	QAction*								selectAndKillTest;						///< Selects and kills a running test.
        // Kills all running test
        QAction* killAllTestsAction_;
	QAction*								selectAndRemoveTestAction;				///< Remove a test after choosing it from a list.
	QAction*								selectAndRunTest;						///< Run a test after selecting it from a list.																	///< program options.
        QAction*								runAllTestsAction;					///< Run all tests in the list.

	QMenu*									testCaseViewContextMenu;				///< Context menu for the test case tree view
	QAction*								testCaseViewExpandAllAction;			///< expands all nodes in the test case tree view
	QAction*								testCaseViewCollapseAllAction;			///< Collapses all nodes in the test case tree view.

	QMenu*									consoleContextMenu;						///< Context menu for the console dock;
	QShortcut*								consoleFindShortcut;					///< Global Ctrl-F to activate the find dialog.
	QAction*								consoleFindAction;						///< Finds text in the console.
	QAction*								clearConsoleAction;						///< Clears the console window.
	
	QMenu*									themeMenu;								///< Menu for selecting themes.
	QActionGroup*							themeActionGroup;						///< Action group for selecting a theme option.
	QAction*								defaultThemeAction;						///< System default theme.
	QAction*								darkThemeAction;						///< Dark theme.

	QMenu*									helpMenu;								///< Help menu
	QAction*								aboutAction;							///< Shows the programs 'about' window

	// state variables
	QString									mostRecentFailurePath;					///< Stores the path [key] of the most recently failed test.
	QHash<QString, bool>					executableCheckedStateHash;				///< Hash of the previous state of the checkboxes.
	QHash<QString, QDomDocument>			testResultsHash;						///< Hash table storing the xml test results for each test path.
	std::map<QString, std::atomic<bool>>	testRunningHash;						///< Stores whether the given test is actively running.
        // Mapping test to latest test result dir
        std::map<QString, QString> testLatestTestRun_;

	// synchronization
	std::mutex								threadKillMutex;						
	std::condition_variable					threadKillCv;							///< Condition variable that is notified when a thread is killed.
        // Sempaphore to control synchronous test running
        QSemaphore runTestParallelSemaphore_;
        // Wrapper of signal to kill a specific test
        std::map<QString, std::atomic<KillTestWrapper*>> testKillHandler_;

signals:

	void testResultsReady(QString path, bool notify);								///< Signal emitted when new test results are ready
	void setStatus(QString);
	void showMessage(QString msg, int timeout = 0);
	void testOutputReady(QString);
	void testProgress(QString path, int complete, int total);
	void runTest(QString path, bool notify);

public:

	explicit MainWindowPrivate(QStringList tests, bool reset, MainWindow* q);

	QString xmlPath(const QString& testPath, const bool create = false) const;
        QString latestGtestResultPath(const QString& testPath);
        void addTestExecutable(const QString& path, const QString& testDriver, bool autorun, QDateTime lastModified, QString filter = "", int repeat = 0, Qt::CheckState runDisabled = Qt::Unchecked, Qt::CheckState shuffle = Qt::Unchecked, int randomSeed = 0, QString otherArgs = "");
	void runTestInThread(const QString& pathToTest, bool notify);
	bool loadTestResults(const QString& testPath, bool notify);
	void selectTest(const QString& testPath);
	void saveSettings() const;
	void loadSettings();
        void removeAllTest(const bool confirm = false);
	void removeTest(const QModelIndex &index, const bool confirm = false);
        void killAllTestAndWait();
	void clearData();
	void clearSettings();
        void updateTestExecutables();

protected:

        void createToolBar();

        void createTestMenu();
	void createOptionsMenu();
	void createWindowMenu();
	void createHelpMenu();
	void createThemeMenu();

	void createExecutableContextMenu();
	void createTestCaseViewContextMenu();
	void createConsoleContextMenu();

	QModelIndex getTestIndexDialog(const QString& label, bool running = false);

	void scrollToConsoleCursor();

        ///< kills the test if it's currently running
        void emitKillTest(const QString& path);
        void killAllTest(const bool confirm = false);

        void saveCommonSettings(const QString& path) const;
        void saveTestSettingsForCurrentRunEnv() const;
        void saveTestSettings(const QString& path) const;
        void loadCommonSettings(const QString& path);
        void loadTestSettingsForCurrentRunEnv();
        void loadTestSettings(const QString& path);


        QString settingsPath() const;
        QString dataPath() const;
	
private:

	QString currentRunEnvPath_;

};	// CLASS: MainWindowPrivate


#endif // mainwindow_p_h__