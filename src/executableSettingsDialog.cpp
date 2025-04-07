#include "executableSettingsDialog.h"
#include "qexecutablemodel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QModelIndex>
#include <QLineEdit>
#include <QPushButton>
#include <QValidator>

class QExecutableSettingsDialogPrivate : public QObject
{

public:

	explicit QExecutableSettingsDialogPrivate(QExecutableSettingsDialog* q) : QObject(q),
		q_ptr(q),
		gtestFilterLabel(new QLabel("Filter:", q)),
		gtestFilterEdit(new QLineEdit(q)),
		gtestAlsoRunDisabledTestsLabel(new QLabel("Run disabled Tests:", q)),
		gtestAlsoRunDisabledTestsCheckbox(new QCheckBox(q)),
		gtestBreakOnFailureLabel(new QLabel("Break on Failure:", q)),
		gtestBreakOnFailureCheckbox(new QCheckBox(q)),
        gtestFailFastLabel(new QLabel("Fail fast:", q)),
        gtestFailFastCheckbox(new QCheckBox(q)),
		gtestRepeatLabel(new QLabel("Repeat Tests:", q)),
		gtestRepeatLineEdit(new QLineEdit(q)),
		gtestRepeatValidator(new QIntValidator(q)),
		gtestShuffleLabel(new QLabel("Shuffle Tests:", q)),
		gtestShuffleCheckbox(new QCheckBox(q)),
		gtestRandomSeedLabel(new QLabel("Random Seed:", q)),
		gtestRandomSeedLineEdit(new QLineEdit(q)),
		gtestRandomSeedValidator(new QIntValidator(q)),
		gtestOtherArgsLabel(new QLabel("Command line:", q)),
		gtestOtherArgsLineEdit(new QLineEdit(q)),
		buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, q))
	{}

	Q_DECLARE_PUBLIC(QExecutableSettingsDialog);

	QExecutableSettingsDialog*	q_ptr;
	QLabel*						gtestFilterLabel;
	QLineEdit*					gtestFilterEdit;
	QLabel*						gtestAlsoRunDisabledTestsLabel;
	QCheckBox*					gtestAlsoRunDisabledTestsCheckbox;
	QLabel*						gtestBreakOnFailureLabel;
	QCheckBox*					gtestBreakOnFailureCheckbox;
    QLabel*						gtestFailFastLabel;
    QCheckBox*					gtestFailFastCheckbox;
	QLabel*						gtestRepeatLabel;
	QLineEdit*					gtestRepeatLineEdit;
	QIntValidator*				gtestRepeatValidator;
	QLabel*						gtestShuffleLabel;
	QCheckBox*					gtestShuffleCheckbox;
	QLabel*						gtestRandomSeedLabel;
	QLineEdit*					gtestRandomSeedLineEdit;
	QIntValidator*				gtestRandomSeedValidator;
	QLabel*						gtestOtherArgsLabel;
	QLineEdit*					gtestOtherArgsLineEdit;

	QDialogButtonBox*			buttonBox;

	QPersistentModelIndex		index;

};

//--------------------------------------------------------------------------------------------------
//	FUNCTION: QExecutableSettingsDialog
//--------------------------------------------------------------------------------------------------
QExecutableSettingsDialog::QExecutableSettingsDialog(QWidget* parent /*= (QObject*)0*/) : QDialog(parent, Qt::FramelessWindowHint),
	d_ptr(new QExecutableSettingsDialogPrivate(this))
{
	Q_D(QExecutableSettingsDialog);

	this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
	this->setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);

	auto* layout = new QGridLayout(this);
	this->setLayout(layout);
	layout->addWidget(d->gtestFilterLabel, 0, 0);
	layout->addWidget(d->gtestFilterEdit, 0, 1);
	layout->addWidget(d->gtestRepeatLabel, 1, 0);
	layout->addWidget(d->gtestRepeatLineEdit, 1, 1);
	layout->addWidget(d->gtestAlsoRunDisabledTestsLabel, 2, 0);
	layout->addWidget(d->gtestAlsoRunDisabledTestsCheckbox, 2, 1);
	layout->addWidget(d->gtestBreakOnFailureLabel, 3, 0);
	layout->addWidget(d->gtestBreakOnFailureCheckbox, 3, 1);
    layout->addWidget(d->gtestFailFastLabel, 4, 0);
    layout->addWidget(d->gtestFailFastCheckbox, 4, 1);
	layout->addWidget(d->gtestShuffleLabel, 5, 0);
	layout->addWidget(d->gtestShuffleCheckbox, 5, 1);
	layout->addWidget(d->gtestRandomSeedLabel, 6, 0);
	layout->addWidget(d->gtestRandomSeedLineEdit, 6, 1);
	layout->addWidget(d->gtestOtherArgsLabel, 7, 0);
	layout->addWidget(d->gtestOtherArgsLineEdit, 7, 1);
	layout->addWidget(d->buttonBox, 8, 0, 2, 2);

	d->gtestFilterEdit->setPlaceholderText("Use * for wildcard");
	d->gtestFilterLabel->setToolTip("Sets the gtest_filter command line argument.");
	d->gtestAlsoRunDisabledTestsCheckbox->setToolTip("sets the gtest_also_run_disabled_tests command line argument.");
	d->gtestBreakOnFailureCheckbox->setToolTip("Drop application into interactive mode on failure when running attached to a debugger.");
	d->gtestFailFastCheckbox->setToolTip("Sets the gtest_fail_fast command line argument. Test execution will stop of first error.");
	d->gtestRepeatLineEdit->setToolTip("set the gtest_repeat command line argument. A value of -1 will cause the test to run forever.");
	d->gtestRepeatLineEdit->setText("1");
	d->gtestRepeatLineEdit->setValidator(d->gtestRepeatValidator);
	d->gtestRepeatValidator->setBottom(-1);
	d->gtestRepeatValidator->setTop(std::numeric_limits<int>::max());
	d->gtestShuffleCheckbox->setToolTip("Sets the gtest_shuffle command line argument.");
	d->gtestRandomSeedLineEdit->setToolTip("Sets the gtest_random_seed command line argument. If set to 0, the current time will be used as a seed.");
	d->gtestRandomSeedLineEdit->setText("0");
	d->gtestRandomSeedLineEdit->setEnabled(false);
	d->gtestRandomSeedLineEdit->setValidator(d->gtestRandomSeedValidator);
	d->gtestRandomSeedValidator->setBottom(0);
	d->gtestRandomSeedValidator->setTop(99999);
	d->gtestOtherArgsLineEdit->setPlaceholderText("other command line arguments");

	setTabOrder(d->gtestFilterEdit, d->gtestRepeatLineEdit);
	setTabOrder(d->gtestRepeatLineEdit, d->gtestAlsoRunDisabledTestsCheckbox);
    setTabOrder(d->gtestAlsoRunDisabledTestsCheckbox, d->gtestBreakOnFailureCheckbox);
	setTabOrder(d->gtestBreakOnFailureCheckbox, d->gtestFailFastCheckbox);
    setTabOrder(d->gtestFailFastCheckbox, d->gtestShuffleCheckbox);
	setTabOrder(d->gtestShuffleCheckbox, d->gtestRandomSeedLineEdit);
	setTabOrder(d->gtestRandomSeedLineEdit, d->gtestOtherArgsLineEdit);
	setTabOrder(d->gtestOtherArgsLineEdit, d->buttonBox->button(QDialogButtonBox::Ok));
	setTabOrder(d->buttonBox->button(QDialogButtonBox::Ok), d->buttonBox->button(QDialogButtonBox::Cancel));
	setTabOrder(d->buttonBox->button(QDialogButtonBox::Cancel), d->gtestFilterEdit);

	d->gtestFilterEdit->setFocus();

	connect(d->gtestShuffleCheckbox, &QCheckBox::checkStateChanged, d->gtestRandomSeedLineEdit, &QLineEdit::setEnabled);
	connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QExecutableSettingsDialog::accept);
	connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QExecutableSettingsDialog::reject);
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: setModelIndex
//--------------------------------------------------------------------------------------------------
void QExecutableSettingsDialog::setModelIndex(const QPersistentModelIndex& index)
{
	Q_D(QExecutableSettingsDialog);

	d->index = index;
	d->gtestFilterEdit->setText(index.data(QExecutableModel::FilterRole).toString());
	d->gtestRepeatLineEdit->setText(index.data(QExecutableModel::RepeatTestsRole).toString());
	d->gtestAlsoRunDisabledTestsCheckbox->setCheckState(static_cast<Qt::CheckState>(index.data(QExecutableModel::RunDisabledTestsRole).toInt()));
	d->gtestBreakOnFailureCheckbox->setCheckState(static_cast<Qt::CheckState>(index.data(QExecutableModel::BreakOnFailureRole).toInt()));
    d->gtestFailFastCheckbox->setCheckState(static_cast<Qt::CheckState>(index.data(QExecutableModel::FailFastRole).toInt()));
	d->gtestShuffleCheckbox->setCheckState(static_cast<Qt::CheckState>(index.data(QExecutableModel::ShuffleRole).toInt()));
	d->gtestRandomSeedLineEdit->setText(index.data(QExecutableModel::RandomSeedRole).toString());
	d->gtestOtherArgsLineEdit->setText(index.data(QExecutableModel::ArgsRole).toString());
}

//--------------------------------------------------------------------------------------------------
//	FUNCTION: accept
//--------------------------------------------------------------------------------------------------
void QExecutableSettingsDialog::accept()
{
	Q_D(QExecutableSettingsDialog);

	if (d->index.isValid())
	{
		auto* model(const_cast<QAbstractItemModel*>(d->index.model()));
		model->setData(model->index(d->index.row(), QExecutableModel::NameColumn), d->gtestFilterEdit->text(), QExecutableModel::FilterRole);
		model->setData(model->index(d->index.row(), QExecutableModel::NameColumn), d->gtestRepeatLineEdit->text(), QExecutableModel::RepeatTestsRole);
		model->setData(model->index(d->index.row(), QExecutableModel::NameColumn), d->gtestAlsoRunDisabledTestsCheckbox->checkState(), QExecutableModel::RunDisabledTestsRole);
		model->setData(model->index(d->index.row(), QExecutableModel::NameColumn), d->gtestBreakOnFailureCheckbox->checkState(), QExecutableModel::BreakOnFailureRole);
        model->setData(model->index(d->index.row(), QExecutableModel::NameColumn), d->gtestFailFastCheckbox->checkState(), QExecutableModel::FailFastRole);
		model->setData(model->index(d->index.row(), QExecutableModel::NameColumn), d->gtestShuffleCheckbox->checkState(), QExecutableModel::ShuffleRole);
		model->setData(model->index(d->index.row(), QExecutableModel::NameColumn), d->gtestRandomSeedLineEdit->text(), QExecutableModel::RandomSeedRole);
		model->setData(model->index(d->index.row(), QExecutableModel::NameColumn), d->gtestOtherArgsLineEdit->text(), QExecutableModel::ArgsRole);
	}

	QDialog::accept();
}
