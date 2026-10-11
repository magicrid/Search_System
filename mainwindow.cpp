#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QtConcurrent>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Поисковая система");

    QRegularExpression re(QStringLiteral("^[\\p{L}]{3,32}(?:[ \t]+[\\p{L}]{3,32}){0,3}$"));
    QRegularExpressionValidator* reValidator=new QRegularExpressionValidator(re,this);
    ui->le_search->setValidator(reValidator);

    m_settings=new SearchSettings(this);
    m_conDialog=new ConnectionDialog(this);
    m_indDialog=new IndexingDialog(this);
    m_database=new Database(this);
    m_message=new QMessageBox(this);
    m_indexWatcher = new QFutureWatcher<IndexingData*>(this);
    connect(m_indexWatcher, &QFutureWatcher<IndexingData*>::finished, this, &MainWindow::slot_indexing_finished);

    m_menuProg=new QMenu("Приложение",this);
    m_menuProg->addAction("Запустить индексацию",this,&MainWindow::start_indexing);
    m_menuProg->addAction("Выход",this,&MainWindow::close);
    m_menuSettings=new QMenu("Настройки",this);
    m_menuSettings->addAction("Подключение к БД",m_conDialog,&ConnectionDialog::open);
    m_menuSettings->addAction("Параметры индексации",m_indDialog,&IndexingDialog::open);
    menuBar()->addMenu(m_menuProg);
    menuBar()->addMenu(m_menuSettings);

    connect(m_conDialog,&ConnectionDialog::accepted,this,&MainWindow::slot_conDialog_accepted);
    connect(m_conDialog,&ConnectionDialog::rejected,this,&MainWindow::slot_conDialog_rejected);
    connect(m_database,&Database::sig_connection_status,this,&MainWindow::slot_receive_connection_status);
    connect(m_database,&Database::sig_request_status,this,&MainWindow::slot_receive_request_status);
    connect(m_indDialog,&IndexingDialog::accepted,this,&MainWindow::slot_indDialog_accepted);
    connect(m_database,&Database::sig_inserting_finished,this,&MainWindow::slot_database_finished);
    connect(m_database,&Database::sig_send_table,this,&MainWindow::slot_receive_table);
    connect(ui->le_search,&QLineEdit::returnPressed,this,&MainWindow::on_pb_search_clicked);

    m_indDialog->read_settings(*m_settings);

    m_conDialog->read_settings(*m_settings);
    if(m_conDialog->is_autoconnect()){
        slot_connect_to_database();
    }
    else{
        m_conDialog->open();
    }
}

MainWindow::~MainWindow()
{
    if (m_indexWatcher && m_indexWatcher->isRunning()) {
        m_indexWatcher->waitForFinished();
        delete m_indexWatcher->result();
    }

    delete ui;
}

void MainWindow::show_warning(const QString& message){
    m_message->setWindowTitle("Внимание!");
    m_message->setIcon(QMessageBox::Warning);
    m_message->setText(message);
    m_message->open();
}

void MainWindow::show_error(const QString& message){
    m_message->setWindowTitle("Ошибка!");
    m_message->setIcon(QMessageBox::Critical);
    m_message->setText(message);
    m_message->open();
}

void MainWindow::show_info(const QString& message){
    m_message->setWindowTitle("Информация");
    m_message->setIcon(QMessageBox::Information);
    m_message->setText(message);
    m_message->open();
}

void MainWindow::set_busy(bool busy)
{
    m_menuProg->setEnabled(!busy);
    m_menuSettings->setEnabled(!busy);
    ui->le_search->setEnabled(!busy);
    ui->pb_search->setEnabled(!busy);
    ui->tabWidget->setEnabled(!busy);
}

void MainWindow::start_indexing()
{
    if (!m_database->is_connected()) {
        show_error("Сначала подключитесь к базе данных PostgreSQL.");
        return;
    }

    const IndexingParameters params = m_settings->read_indexing_settings();
    if (params.dirs.isEmpty()) {
        show_warning("Не указаны индексируемые каталоги! Откройте настройки индексации.");
        return;
    }
    if (params.extensions.isEmpty()) {
        show_warning("Не указаны расширения файлов для индексации!");
        return;
    }
    bool hasExistingDir = false;
    for (const QString& path : params.dirs) {
        if (QDir(path.trimmed()).exists()) hasExistingDir = true;
        else show_warning(QString("Каталог не существует: %1").arg(path));
    }
    if (!hasExistingDir) return;

    if (!m_database->reset_tables()) {
        set_busy(false);
        return;
    }

    set_busy(true);
    statusBar()->showMessage("Выполняется индексация. Пожалуйста, ожидайте...");

    QFuture<IndexingData*> future = QtConcurrent::run([params]() -> IndexingData* {
        Indexer worker;
        std::unique_ptr<IndexingData> data = worker.start(params);
        return data.release();
    });
    m_indexWatcher->setFuture(future);
}

void MainWindow::slot_indexing_finished()
{
    IndexingData* rawData = m_indexWatcher->result();
    if (!rawData) {
        set_busy(false);
        show_error("Индексатор не вернул данные.");
        return;
    }
    std::unique_ptr<IndexingData> data(rawData);
    if (data->docs.isEmpty()) {
        set_busy(false);
        show_warning("Не найдено ни одного доступного файла с указанными расширениями. Существующий индекс не изменён.");
        return;
    }
    m_database->insert_data(std::move(data));
}

void MainWindow::slot_conDialog_accepted(){
    m_conDialog->write_settings(*m_settings);
    if(!m_database->is_connected()){
        slot_connect_to_database();
    }
}

void MainWindow::slot_conDialog_rejected(){
    if(!m_database->is_connected()){
        close();
    }
}

void MainWindow::slot_connect_to_database(){
    m_conData=m_settings->read_connection_settings();
    m_database->connect_to_database(m_conData);
}

void MainWindow::slot_receive_connection_status(bool status){
    if(status){
        start_indexing();
    }
    else{
        m_database->disconnect_from_database();
        m_conDialog->read_settings(*m_settings);
        m_conDialog->open();
        show_error(m_database->get_last_error().text());
    }
}

void MainWindow::slot_receive_request_status(QSqlError err,RequestType reqType){
    switch(reqType){
    case RequestType::CREATE_TABLES:
    case RequestType::INSERT_DATA:
        statusBar()->showMessage("Операция прервана из-за ошибки.",10000);
        set_busy(false);
        show_error(err.text());
        break;
    case RequestType::SELECT_WORDS:
        if(err.isValid()){
            statusBar()->showMessage("Индексация прервана из-за ошибки.",10000);
            set_busy(false);
            show_error(err.text());
        }
        else{
            m_database->read_data(reqType);
        }
        break;
    case RequestType::SELECT_DOCS:
        if(err.isValid()){
            show_error(err.text());
        }
        else{
            m_database->read_data(reqType);
        }
        break;
    }
}

void MainWindow::slot_indDialog_accepted(){
    m_indDialog->write_settings(*m_settings);
}

void MainWindow::slot_no_dirs(){
    show_warning("Не указаны индексируемые каталоги!\nИзмените параметры индексирования и запустите индексирование еще раз.");
}

void MainWindow::slot_no_extensions(){
    show_warning("Не указаны расширения индексируемых текстовых файлов!\nИзмените параметры индексирования и запустите индексирование еще раз.");
}

void MainWindow::slot_dir_not_exists(QString dirPath){
    show_warning(QString("Каталога %1 не существует!\nИзмените параметры индексирования и запустите индексирование еще раз.").arg(dirPath));
}

void MainWindow::slot_database_finished(){
    m_database->request_words();
}

void MainWindow::slot_receive_table(QTableWidget* table,RequestType reqType){
    switch(reqType){
    case RequestType::SELECT_WORDS:
        delete m_tableWords;
        m_tableWords=table;
        ui->vbox_tableWords->addWidget(m_tableWords);
        statusBar()->showMessage("Индексация завершена. Приложение готово к работе.",10000);
        m_menuProg->setEnabled(true);
        m_menuSettings->setEnabled(true);
        ui->le_search->setEnabled(true);
        ui->pb_search->setEnabled(true);
        ui->tabWidget->setEnabled(true);
        break;
    case RequestType::SELECT_DOCS:
        delete m_tableDocs;
        m_tableDocs = table;
        ui->vbox_tableResults->addWidget(m_tableDocs);
        if (m_tableDocs->rowCount() == 0) show_info("Ничего не найдено.");
        break;
    default:
        break;
    }
}

void MainWindow::on_pb_search_clicked()
{
    const QString text = ui->le_search->text().trimmed();
    if (!text.isEmpty()) {
        m_database->request_docs(text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts));
    }
}

