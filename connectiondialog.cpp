#include "connectiondialog.h"
#include "ui_connectiondialog.h"

ConnectionDialog::ConnectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);

    setWindowTitle("Подключение к БД");
}

ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

void ConnectionDialog::read_settings(SearchSettings& settings){
    ConnectionData conData=settings.read_connection_settings();
    ui->le_hostName->setText(conData.hostName);
    ui->le_dbName->setText(conData.dbName);
    ui->le_user->setText(conData.user);
    ui->le_password->setText(conData.password);
    ui->spB_port->setValue(conData.port);
    ui->chB_autoConnection->setChecked(settings.value("autoconnection").toBool());
}

void ConnectionDialog::write_settings(SearchSettings& settings){
    ConnectionData conData;
    conData.hostName=ui->le_hostName->text().trimmed();
    conData.dbName=ui->le_dbName->text().trimmed();
    conData.user=ui->le_user->text().trimmed();
    conData.password=ui->le_password->text();
    conData.port=ui->spB_port->value();
    settings.write_connection_settings(conData);
    settings.setValue("autoconnection",ui->chB_autoConnection->isChecked());
}

bool ConnectionDialog::is_autoconnect(){
    return ui->chB_autoConnection->isChecked();
}
