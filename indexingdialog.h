#ifndef INDEXINGDIALOG_H
#define INDEXINGDIALOG_H

#include <QDialog>
#include "searchsettings.h"

namespace Ui {
class IndexingDialog;
}

class IndexingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IndexingDialog(QWidget *parent = nullptr);
    ~IndexingDialog();

    void read_settings(SearchSettings& settings);
    void write_settings(SearchSettings& settings);

private slots:
    void on_pb_browse_clicked();

    void on_pb_addDir_clicked();

    void on_pb_deleteDir_clicked();

    void on_pb_addExt_clicked();

    void on_pb_deleteExt_clicked();

private:
    Ui::IndexingDialog *ui;
};

#endif // INDEXINGDIALOG_H
