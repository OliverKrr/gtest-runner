#include "qexecutabletreeview.h"

#include <QPushButton>
#include <QFocusEvent>
#include "executableSettingsDialog.h"

class QExecutableTreeViewPrivate
{
public:
	QExecutableTreeViewPrivate(QExecutableTreeView* q) :
		settingsDialog(new QExecutableSettingsDialog(q))
	{
		settingsDialog->setModal(false);
	}

	QExecutableSettingsDialog*		settingsDialog;		///< Dialog to display/select advanced command-line settings.
};

//--------------------------------------------------------------------------------------------------
//	FUNCTION: QExecutableTreeView
//--------------------------------------------------------------------------------------------------
QExecutableTreeView::QExecutableTreeView(QWidget* parent /*= (QWidget*)0*/) : QTreeView(parent), d_ptr(new QExecutableTreeViewPrivate(this))
{

}

void QExecutableTreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QTreeView::selectionChanged(selected, deselected);
    emit itemSelectionChanged();
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: rowsInserted
//--------------------------------------------------------------------------------------------------
void QExecutableTreeView::rowsInserted(const QModelIndex &parent, int start, int end)
{
	Q_D(QExecutableTreeView);

	QTreeView::rowsInserted(parent, start, end);

	for (int i = start; i <= end; ++i)
	{
		QModelIndex newRow = this->model()->index(i, 0, parent);

		QPushButton* advButton = new QPushButton();
		advButton->setIcon(QIcon(":/images/hamburger"));
		advButton->setToolTip("Advanced...");
		advButton->setFixedSize(18, 18);
                advButton->installEventFilter(this);
		this->setIndexWidget(newRow, advButton);

		connect(advButton, &QPushButton::clicked, [this, d, advButton]
		{
			if (!d->settingsDialog->isVisible())
			{
				QModelIndex index = this->indexAt(this->mapFromGlobal(QCursor::pos()));
				auto pos = advButton->mapToGlobal(advButton->rect().bottomLeft());
				d->settingsDialog->move(pos);
				d->settingsDialog->setModelIndex(index);
				d->settingsDialog->show();
			}
			else
			{
				d->settingsDialog->reject();
			}
		});
	}
}

bool QExecutableTreeView::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::FocusIn)
    {
        QPushButton* advButton = static_cast<QPushButton*>(obj);
        auto pos = advButton->mapToGlobal(advButton->rect().center());
        QModelIndex index = this->indexAt(this->mapFromGlobal(pos));
        if (index.isValid())
        {
            this->setCurrentIndex(index);
        }
    }
    return QObject::eventFilter(obj, event);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: rowsAboutToBeRemoved
//--------------------------------------------------------------------------------------------------
void QExecutableTreeView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
	for (int i = start; i <= end; i++)
	{
		delete this->indexWidget(this->model()->index(i, 0 , parent));
	}

	QTreeView::rowsAboutToBeRemoved(parent, start, end);
}

