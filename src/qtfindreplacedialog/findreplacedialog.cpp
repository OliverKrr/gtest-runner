/*
 * Copyright (C) 2009  Lorenzo Bettini <http://www.lorenzobettini.it>
 * See COPYING file that comes with this distribution
 */

#include "findreplacedialog.h"
#include "ui_findreplacedialog.h"

FindReplaceDialog::FindReplaceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindReplaceDialog)
{
    ui->setupUi(this);
}

FindReplaceDialog::~FindReplaceDialog()
{
    delete ui;
}

void FindReplaceDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void FindReplaceDialog::setTextEdit(QTextEdit *textEdit) const
{
    ui->findReplaceForm->setTextEdit(textEdit);
}

void FindReplaceDialog::writeSettings(QJsonObject &settings, const QString &prefix) {
    ui->findReplaceForm->writeSettings(settings, prefix);
}

void FindReplaceDialog::readSettings(QJsonObject &settings, const QString &prefix) {
    ui->findReplaceForm->readSettings(settings, prefix);
}

void FindReplaceDialog::findNext() {
    ui->findReplaceForm->findNext();
}

void FindReplaceDialog::findPrev() {
    ui->findReplaceForm->findPrev();
}
