#include "mainwindow_p.h"

#include <QScreen>
#include <QMimeData>
#include <QApplication>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>

//--------------------------------------------------------------------------------------------------
//	FUNCTION: MainWindow
//--------------------------------------------------------------------------------------------------
MainWindow::MainWindow(const QStringList& tests, const bool reset) : d_ptr(new MainWindowPrivate(tests, reset, this))
{
    Q_D(MainWindow);

    this->addToolBar(d->toolBar_);
    this->setStatusBar(d->statusBar);

    this->setCentralWidget(d->centralFrame);
    this->setWindowIcon(QIcon(":images/logo"));

    this->setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    this->setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    this->addDockWidget(Qt::LeftDockWidgetArea, d->executableDock);
    this->addDockWidget(Qt::BottomDockWidgetArea, d->consoleDock);
    this->addDockWidget(Qt::BottomDockWidgetArea, d->failureDock);
    this->tabifyDockWidget(d->failureDock, d->consoleDock);
    d->failureDock->raise();

    this->setDockNestingEnabled(true);

    // accept drops
    this->setAcceptDrops(true);

    // restore settings
    d->loadSettings();

    // add tests from the command line
    for (const auto& test : tests)
    {
        QFileInfo info(test);
        QString name = info.baseName();
        QString testDriver;
        d_ptr->addTestExecutable(info.absoluteFilePath(), name, testDriver, true, info.lastModified());
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: closeEvent
//--------------------------------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent* event)
{
    Q_D(MainWindow);
    d->killAllTestAndWait();
    d->saveSettings();
    QMainWindow::closeEvent(event);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: changeEvent
//--------------------------------------------------------------------------------------------------
void MainWindow::changeEvent(QEvent* e)
{
    QMainWindow::changeEvent(e);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: sizeHint
//--------------------------------------------------------------------------------------------------
QSize MainWindow::sizeHint() const
{
    return 0.5 * QApplication::primaryScreen()->size();
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: dragEnterEvent
//--------------------------------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls())
    {
        const QFileInfo info(e->mimeData()->urls().first().toLocalFile());
        if (info.isExecutable())
        {
            e->acceptProposedAction();
        }
    }
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: dropEvent
//--------------------------------------------------------------------------------------------------
void MainWindow::dropEvent(QDropEvent* e)
{
    if (e->mimeData()->hasUrls())
    {
        for (const auto& url : e->mimeData()->urls())
        {
            QFileInfo info(url.toLocalFile());
            QString name = info.baseName();
            QString testDriver;
            d_ptr->addTestExecutable(info.absoluteFilePath(), name, testDriver, true, info.lastModified());
        }
    }
}
