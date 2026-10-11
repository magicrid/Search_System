#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFutureWatcher>
#include "searchsettings.h"
#include "connectiondialog.h"
#include "database.h"
#include "indexingdialog.h"
#include "indexer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void show_warning(const QString& message);
    void show_error(const QString& message);
    void show_info(const QString& message);
    void start_indexing();

private:
    Ui::MainWindow *ui;
    SearchSettings* m_settings;
    ConnectionDialog* m_conDialog;
    IndexingDialog* m_indDialog;
    Database* m_database;
    QMessageBox* m_message;
    ConnectionData m_conData;
    QFutureWatcher<IndexingData*>* m_indexWatcher;
    QMenu* m_menuProg;
    QMenu* m_menuSettings;
    QTableWidget* m_tableDocs=nullptr;
    QTableWidget* m_tableWords=nullptr;

public slots:
    void slot_conDialog_accepted();
    void slot_conDialog_rejected();
    void slot_connect_to_database();
    void slot_receive_connection_status(bool);
    void slot_receive_request_status(QSqlError,RequestType);
    void slot_indDialog_accepted();
    void slot_no_dirs();
    void slot_no_extensions();
    void slot_dir_not_exists(QString);
    void slot_database_finished();
    void slot_indexing_finished();
    void set_busy(bool busy);
    void slot_receive_table(QTableWidget*,RequestType);
private slots:
    void on_pb_search_clicked();
};
#endif // MAINWINDOW_H
