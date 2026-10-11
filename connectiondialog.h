#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include "searchsettings.h"

namespace Ui {
class ConnectionDialog;
}

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr);
    ~ConnectionDialog();

    void read_settings(SearchSettings& settings);
    void write_settings(SearchSettings& settings);
    bool is_autoconnect();

private:
    Ui::ConnectionDialog *ui;
};

#endif // CONNECTIONDIALOG_H
