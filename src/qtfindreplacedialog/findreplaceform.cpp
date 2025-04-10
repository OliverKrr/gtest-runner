/*
 * Copyright (C) 2009  Lorenzo Bettini <http://www.lorenzobettini.it>
 * See COPYING file that comes with this distribution
 */

#include <QtGui>
#include <QTextEdit>
#include <QRegExp>
#include <QJsonObject>

#include "findreplaceform.h"
#include "ui_findreplaceform.h"

#define TEXT_TO_FIND "textToFind"
#define TEXT_TO_REPLACE "textToReplace"
#define DOWN_RADIO "downRadio"
#define UP_RADIO "upRadio"
#define CASE_CHECK "caseCheck"
#define WHOLE_CHECK "wholeCheck"
#define REGEXP_CHECK "regexpCheck"

FindReplaceForm::FindReplaceForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FindReplaceForm), textEdit(0)
{
    ui->setupUi(this);

    ui->errorLabel->setText("");

    connect(ui->textToFind, SIGNAL(textChanged(QString)), this, SLOT(textToFindChanged()));
    connect(ui->textToFind, SIGNAL(textChanged(QString)), this, SLOT(validateRegExp(QString)));

    connect(ui->regexCheckBox, SIGNAL(toggled(bool)), this, SLOT(regexpSelected(bool)));

    connect(ui->findButton, SIGNAL(clicked()), this, SLOT(find()));
    connect(ui->closeButton, SIGNAL(clicked()), parent, SLOT(close()));

    connect(ui->replaceButton, SIGNAL(clicked()), this, SLOT(replace()));
    connect(ui->replaceAllButton, SIGNAL(clicked()), this, SLOT(replaceAll()));
}

FindReplaceForm::~FindReplaceForm()
{
    delete ui;
}

void FindReplaceForm::hideReplaceWidgets() {
    ui->replaceLabel->setVisible(false);
    ui->textToReplace->setVisible(false);
    ui->replaceButton->setVisible(false);
    ui->replaceAllButton->setVisible(false);
}

void FindReplaceForm::setTextEdit(QTextEdit *textEdit_) {
    textEdit = textEdit_;
    connect(textEdit, SIGNAL(copyAvailable(bool)), ui->replaceButton, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)), ui->replaceAllButton, SLOT(setEnabled(bool)));
}

void FindReplaceForm::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

bool FindReplaceForm::event(QEvent *e)
{
    switch (e->type()) 
	{
	case QEvent::Show:
		ui->textToFind->setSelection(0, ui->textToFind->text().length());
		qDebug() << "show event";
		break;
	default:
        break;
    }
	return QWidget::event(e);
}

void FindReplaceForm::textToFindChanged() {
    ui->findButton->setEnabled(ui->textToFind->text().size() > 0);
}

void FindReplaceForm::regexpSelected(bool sel) {
    if (sel)
        validateRegExp(ui->textToFind->text());
    else
        validateRegExp("");
}

void FindReplaceForm::validateRegExp(const QString &text) {
    if (!ui->regexCheckBox->isChecked() || text.size() == 0) {
        ui->errorLabel->setText("");
        return; // nothing to validate
    }

    QRegExp reg(text,
                (ui->caseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive));

    if (reg.isValid()) {
        showError("");
    } else {
        showError(reg.errorString());
    }
}

void FindReplaceForm::showError(const QString &error) {
    if (error == "") {
        ui->errorLabel->setText("");
    } else {
        ui->errorLabel->setText("<span style=\" font-weight:600; color:#ff0000;\">" +
                                error +
                                "</spam>");
    }
}

void FindReplaceForm::showMessage(const QString &message) {
    if (message == "") {
        ui->errorLabel->setText("");
    } else {
        ui->errorLabel->setText("<span style=\" font-weight:600; color:green;\">" +
                                message +
                                "</spam>");
    }
}

void FindReplaceForm::find() {
    find(ui->downRadioButton->isChecked());
}

void FindReplaceForm::find(bool next) {
    if (!textEdit)
        return; // TODO: show some warning?

    // backward search
    bool back = !next;

    const QString &toSearch = ui->textToFind->text();

    bool result = false;

    QTextDocument::FindFlags flags;

    if (back)
        flags |= QTextDocument::FindBackward;
    if (ui->caseCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (ui->wholeCheckBox->isChecked())
        flags |= QTextDocument::FindWholeWords;

    if (ui->regexCheckBox->isChecked()) 
	{
        QRegExp reg(toSearch,
                    (ui->caseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive));

        textCursor = textEdit->document()->find(reg, textCursor, flags);
        textEdit->setTextCursor(textCursor);
        result = (!textCursor.isNull());
    }
    else
	{
        result = textEdit->find(toSearch, flags);
    }

    if (result) {
        showError("");
    } else {
        showError(tr("no match found"));
        // move to the beginning of the document for the next find
        textCursor.setPosition(0);
        textEdit->setTextCursor(textCursor);
    }
}

void FindReplaceForm::replace() {
    if (!textEdit->textCursor().hasSelection()) {
        find();
    } else {
        textEdit->textCursor().insertText(ui->textToReplace->text());
        find();
    }
}

void FindReplaceForm::replaceAll() {
    int i=0;
    while (textEdit->textCursor().hasSelection()){
        textEdit->textCursor().insertText(ui->textToReplace->text());
        find();
        i++;
    }
    showMessage(tr("Replaced %1 occurrence(s)").arg(i));
}

void FindReplaceForm::writeSettings(QJsonObject &settings, const QString &prefix) {
    QJsonObject obj;
    obj.insert(TEXT_TO_FIND, ui->textToFind->text());
    obj.insert(TEXT_TO_REPLACE, ui->textToReplace->text());
    obj.insert(DOWN_RADIO, ui->downRadioButton->isChecked());
    obj.insert(UP_RADIO, ui->upRadioButton->isChecked());
    obj.insert(CASE_CHECK, ui->caseCheckBox->isChecked());
    obj.insert(WHOLE_CHECK, ui->wholeCheckBox->isChecked());
    obj.insert(REGEXP_CHECK, ui->regexCheckBox->isChecked());
    settings.insert(prefix, obj);
}

void FindReplaceForm::readSettings(QJsonObject &settings, const QString &prefix) {
    QJsonObject obj = settings[prefix].toObject();
    ui->textToFind->setText(obj[TEXT_TO_FIND].toString());
    ui->textToReplace->setText(obj[TEXT_TO_REPLACE].toString());
    ui->downRadioButton->setChecked(obj[DOWN_RADIO].toBool());
    ui->upRadioButton->setChecked(obj[UP_RADIO].toBool());
    ui->caseCheckBox->setChecked(obj[CASE_CHECK].toBool());
    ui->wholeCheckBox->setChecked(obj[WHOLE_CHECK].toBool());
    ui->regexCheckBox->setChecked(obj[REGEXP_CHECK].toBool());
}
