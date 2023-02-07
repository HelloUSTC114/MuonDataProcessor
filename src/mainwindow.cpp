#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTableWidgetItem>
#include <QStringList>
#include <QString>
#include <QFont>
#include <QFileDialog>


Mainwindow::Mainwindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Mainwindow)
{
    ui->setupUi(this);

    // Setting tree table head
    QTableWidgetItem *headerItem;
    QStringList headerText;
    headerText << "HG serial" << "HG id" << "LG serial" << "LG id" << "TDC serial" << "TDC ID";
    ui->tableTree->setColumnCount(headerText.count());

    for(int i = 0; i < ui->tableTree->columnCount();i++)
    {
        headerItem = new QTableWidgetItem(headerText.at(i));
        QFont font = headerItem->font();
        font.setBold(true);
        font.setPointSize(12);
        headerItem->setFont(font);
        ui->tableTree->setHorizontalHeaderItem(i, headerItem);
    }
    
}

Mainwindow::~Mainwindow()
{
    delete ui;
}

void Mainwindow::on_btnInFile_clicked()
{

}


void Mainwindow::on_btnOutFile_clicked()
{

}


void Mainwindow::on_btnAnaFile_clicked()
{

}

