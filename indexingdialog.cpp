#include <QFileDialog>
#include <QDir>
#include <QMessageBox>

#include "indexingdialog.h"
#include "ui_indexingdialog.h"

IndexingDialog::IndexingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IndexingDialog)
{
    ui->setupUi(this);

    setWindowTitle("Параметры индексации");

    QRegularExpression re(QStringLiteral("^[\\p{L}0-9_+-]{1,16}$"));
    QRegularExpressionValidator* reValidator=new QRegularExpressionValidator(re,this);
    ui->le_ext->setValidator(reValidator);

}

IndexingDialog::~IndexingDialog()
{
    delete ui;
}

void IndexingDialog::read_settings(SearchSettings& settings){
    IndexingParameters indParams=settings.read_indexing_settings();
    ui->listW_dirs->clear();
    ui->listW_exts->clear();
    ui->listW_dirs->addItems(indParams.dirs);
    ui->listW_exts->addItems(indParams.extensions);
}

void IndexingDialog::write_settings(SearchSettings& settings){
    IndexingParameters indParams;
    for(int i=0; i<ui->listW_dirs->count(); i++){
        indParams.dirs.append(ui->listW_dirs->item(i)->text());
    }
    for(int i=0; i<ui->listW_exts->count(); i++){
        indParams.extensions.append(ui->listW_exts->item(i)->text());
    }
    settings.write_indexing_settings(indParams);
}

void IndexingDialog::on_pb_browse_clicked()
{
    QString dir=QFileDialog::getExistingDirectory(this,tr("Выбрать каталог"));
    ui->le_dir->setText(dir);
}


void IndexingDialog::on_pb_addDir_clicked()
{
    const QString path = ui->le_dir->text().trimmed();
    if (!path.isEmpty() && QDir(path).exists()) {
        for (int i = 0; i < ui->listW_dirs->count(); ++i)
            if (ui->listW_dirs->item(i)->text() == QDir(path).absolutePath()) return;
        ui->listW_dirs->addItem(QDir(path).absolutePath());
        ui->le_dir->clear();
    } else if (!path.isEmpty()) {
        QMessageBox::warning(this, "Каталог не найден", "Указанный каталог не существует.");
    }    
}


void IndexingDialog::on_pb_deleteDir_clicked()
{
    const int row = ui->listW_dirs->currentRow();
    if (row >= 0) delete ui->listW_dirs->takeItem(row);
}


void IndexingDialog::on_pb_addExt_clicked()
{
    QString ext = ui->le_ext->text().trimmed().toLower();
    if (ext.startsWith('.')) ext.remove(0, 1);
    if (!ext.isEmpty() && ui->listW_exts->findItems(ext, Qt::MatchFixedString).isEmpty()) {
        ui->listW_exts->addItem(ext);
        ui->le_ext->clear();
    }
}


void IndexingDialog::on_pb_deleteExt_clicked()
{
    const int row = ui->listW_exts->currentRow();
    if (row >= 0) delete ui->listW_exts->takeItem(row);
}

