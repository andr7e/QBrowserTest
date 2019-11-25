#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QWidget>

namespace Ui {
class FindDialog;
}

class FindDialog : public QWidget
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = 0);
    ~FindDialog();

    void setText(const QString &text);

    void setFindFocus();

    bool useCaseSensitively();

private slots:
    void on_lineEdit_textChanged(const QString &text);
    void on_closeButton_clicked();

    void on_nextToolButton_clicked();

    void on_prevToolButton_clicked();

    void on_lineEdit_editingFinished();

    void on_caseSensitivelyCheckBox_clicked();

signals:
    void textChanged(QString);
    void findClosed();
    void findNext();
    void findPrev();
    void caseSensitivelyChanged();

private:
    Ui::FindDialog *ui;
};

#endif // FINDDIALOG_H
