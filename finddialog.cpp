#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FindDialog)
{
    ui->setupUi(this);
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::setText(const QString &text)
{
    ui->lineEdit->setText(text);
}

void FindDialog::setFindFocus()
{
    ui->lineEdit->setFocus();
}

void FindDialog::on_closeButton_clicked()
{
    emit findClosed();
}

void FindDialog::on_lineEdit_textChanged(const QString &text)
{
    emit textChanged(text);
}

void FindDialog::on_nextToolButton_clicked()
{
    emit findNext();
}

void FindDialog::on_prevToolButton_clicked()
{
    emit findPrev();
}

void FindDialog::on_lineEdit_editingFinished()
{
    emit findNext();
}
