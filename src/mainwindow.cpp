#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTableWidgetItem>
#include <QStringList>
#include <QString>
#include <QFont>
#include <QFileDialog>

Mainwindow::Mainwindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::Mainwindow)
{
    ui->setupUi(this);

    // Setting tree table head
    QTableWidgetItem *headerItem;
    QStringList headerText;
    headerText << "HG serial"
               << "HG id"
               << "LG serial"
               << "LG id"
               << "TDC serial"
               << "TDC ID";
    ui->tableTree->setColumnCount(headerText.count());

    for (int i = 0; i < ui->tableTree->columnCount(); i++)
    {
        headerItem = new QTableWidgetItem(headerText.at(i));
        QFont font = headerItem->font();
        font.setBold(true);
        font.setPointSize(12);
        headerItem->setFont(font);
        ui->tableTree->setHorizontalHeaderItem(i, headerItem);
    }
    ui->tableTree->verticalHeader()->setVisible(0);
    ui->tableTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Setting Leaf table
    QTableWidgetItem *leafHeader;
    QStringList leafHeaderText;
    leafHeaderText << "Channel"
                   << "Value";
    ui->tableLeaf->setColumnCount(leafHeaderText.count());

    for (int i = 0; i < ui->tableLeaf->columnCount(); i++)
    {
        leafHeader = new QTableWidgetItem(leafHeaderText.at(i));
        QFont font = leafHeader->font();
        font.setBold(true);
        font.setPointSize(12);
        leafHeader->setFont(font);
        ui->tableLeaf->setHorizontalHeaderItem(i, leafHeader);
    }
    ui->tableLeaf->verticalHeader()->setVisible(0);
    ui->tableLeaf->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Setting ui
    ui->btnCloseFile->setEnabled(0);
}

Mainwindow::~Mainwindow()
{
    delete ui;
}

#include <QFileDialog>
#include <iostream>
void Mainwindow::on_btnInFile_clicked()
{
    sInFileName = QFileDialog::getOpenFileName(this, "Choose one file to read.", sCurrentPath, "*.root");
    if (sInFileName == "")
        return;

    QStringList pl = sInFileName.split('/');
    QString sfilePath = "";
    for (int i = 0; i < pl.size() - 1; i++)
    {
        sfilePath += pl.at(i);
        sfilePath += '/';
    }
    ui->lblInFilePath->setText(sfilePath);
    sCurrentPath = sfilePath;

    QString sfileName = pl.at(pl.size() - 1);
    auto pl2 = sfileName.split('.');
    QString sfileName1 = "";
    for (int i = 0; i < pl2.size() - 1; i++)
    {
        sfileName1 += pl2.at(i);
        if (i < pl2.size() - 2)
            sfileName1 += '.';
    }
    ui->lblInFileName->setText(sfileName1);
}

void Mainwindow::on_btnOutFile_clicked()
{
}

#include <QMessageBox>
#include "AlignManager.h"
void Mainwindow::on_btnOpenFile_clicked()
{
    // if (fInFile && fInFile->IsOpen())
    //     return;
    // if (fInFile)
    //     if (!fInFile->IsOpen())
    //         delete fInFile;
    auto error = gInputFile->OpenFile(sInFileName.toStdString());

    // Open file and process exception
    // fInFile = new TFile(sInFileName.toStdString().c_str());
    // if (!fInFile)
    //     fInFile = NULL;
    if (error == FileNotOpen)
    {
        QString text = "Cannot open file: " + sInFileName;
        QMessageBox::information(this, "Error while open file.", text, QMessageBox::Ok, QMessageBox::Cancel);
        return;
    }

    if (error == TreeNotFound)
    {
        QString text = "Cannot find three trees.";
        QMessageBox::information(this, "Error while open file.", text, QMessageBox::Ok, QMessageBox::Cancel);
        return;
    }
    ui->btnOpenFile->setEnabled(0);
    ui->btnInFile->setEnabled(0);
    ui->btnCloseFile->setEnabled(1);

    // Show Tree Information
    fHGEntries = gInputFile->GetEntries(hgTree);
    fLGEntries = gInputFile->GetEntries(lgTree);
    fTDCEntries = gInputFile->GetEntries(tdcTree);
    ui->lblHGEntry->setText(QString::number(fHGEntries));
    ui->lblLGEntry->setText(QString::number(fLGEntries));
    ui->lblTDCEntry->setText(QString::number(fTDCEntries));

    // Tree setting
    gInputFile->GetEntry(0, hgTree);
    gInputFile->GetEntry(0, lgTree);
    gInputFile->GetEntry(0, tdcTree);
    fHGStartid = gInputFile->HGid();
    fLGStartid = gInputFile->LGid();
    fTDCStartid = gInputFile->TDCid();
    ui->lblHGid->setText(QString::number(fHGStartid));
    ui->lblLGid->setText(QString::number(fLGStartid));
    ui->lblTDCid->setText(QString::number(fTDCStartid));

    // Table:
    showPage(0);
}

void Mainwindow::showPage(int page)
{
    if (!gInputFile->IsOpen())
        return;
    // if (fCurrentPage == page)
    //     return;

    ui->tableTree->clearContents();
    ui->tableLeaf->clearContents();
    ui->tableTree->setRowCount(50);

    int largestEntry = TMath::Max(TMath::Max(fHGEntries, fLGEntries), fTDCEntries);
    if (page >= largestEntry / 50)
        page = largestEntry / 50;
    if (page < 0)
        page = 0;
    fCurrentPage = page;

    // ui setting
    ui->lineChoosePage->setText(QString::number(fCurrentPage));

    for (int i = 0; i < 50; i++)
    {
        int idx = page * 50 + i;
        int HGidx, LGidx, TDCidx;
        int HGid, LGid, TDCid;

        if (fHGEntries > idx)
        {
            HGidx = idx;
            gInputFile->GetEntry(idx, hgTree);
            HGid = gInputFile->HGid();
        }
        else
        {
            HGidx = -1;
            HGid = -1;
        }
        if (fLGEntries > idx)
        {
            LGidx = idx;
            gInputFile->GetEntry(idx, lgTree);
            LGid = gInputFile->LGid();
        }
        else
        {
            LGidx = -1;
            LGid = -1;
        }
        if (fTDCEntries > idx)
        {
            TDCidx = idx;
            // fTDCTree->GetEntry(idx);
            // TDCid = fTDCid;

            gInputFile->GetEntry(idx, tdcTree);
            TDCid = gInputFile->TDCid();
        }
        else
        {
            TDCidx = -1;
            TDCid = -1;
        }
        createItemsARow(i, HGidx, HGid, LGidx, LGid, TDCidx, TDCid);
    }
}

void Mainwindow::createItemsARow(int rowNo, int hgSerial, int hgID, int lgSerial, int lgID, int tdcSerial, int tdcID)
{
    QTableWidgetItem *item;

    item = new QTableWidgetItem(QString::number(hgSerial));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    if (hgSerial < 0)
        item->setText("");
    ui->tableTree->setItem(rowNo, 0, item);

    item = new QTableWidgetItem(QString::number(hgID));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    if (hgID < 0)
        item->setText("");
    ui->tableTree->setItem(rowNo, 1, item);

    item = new QTableWidgetItem(QString::number(lgSerial));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    if (lgSerial < 0)
        item->setText("");
    ui->tableTree->setItem(rowNo, 2, item);

    item = new QTableWidgetItem(QString::number(lgID));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    if (lgID < 0)
        item->setText("");
    ui->tableTree->setItem(rowNo, 3, item);

    item = new QTableWidgetItem(QString::number(tdcSerial));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    if (tdcSerial < 0)
        item->setText("");
    ui->tableTree->setItem(rowNo, 4, item);

    item = new QTableWidgetItem(QString::number(tdcID));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    if (tdcID < 0)
        item->setText("");
    ui->tableTree->setItem(rowNo, 5, item);
}

void Mainwindow::on_btnCloseFile_clicked()
{
    gInputFile->CloseFile();

    // setting UI
    ui->btnOpenFile->setEnabled(1);
    ui->btnInFile->setEnabled(1);
    ui->btnCloseFile->setEnabled(0);

    ui->lblHGEntry->setText(QString::number(0));
    ui->lblLGEntry->setText(QString::number(0));
    ui->lblTDCEntry->setText(QString::number(0));

    ui->lblHGid->setText(QString::number(0));
    ui->lblLGid->setText(QString::number(0));
    ui->lblTDCid->setText(QString::number(0));
}

void Mainwindow::on_tableTree_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if (!gInputFile->IsOpen())
        return;
    int entry = currentRow + 50 * fCurrentPage;
    showLeaf(entry, (TreeType)(currentColumn / 2));
}

void Mainwindow::showLeaf(int entry, TreeType tree)
{
    ui->tableLeaf->clearContents();
    if (tree < 2 && tree >= 0)
        ui->tableLeaf->setRowCount(33);
    else
        ui->tableLeaf->setRowCount(34);

    gInputFile->GetEntry(entry, tree);
    QTableWidgetItem *item;

    if (tree == hgTree)
        item = new QTableWidgetItem("HG id:");
    else if (tree == lgTree)
        item = new QTableWidgetItem("LG id:");
    else if (tree == tdcTree)
        item = new QTableWidgetItem("TDC id:");

    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    auto font = item->font();
    font.setBold(true);
    item->setFont(font);
    ui->tableLeaf->setItem(0, 0, item);

    uint32_t temp;
    if (tree == hgTree)
        temp = gInputFile->HGid();
    else if (tree == lgTree)
        temp = gInputFile->LGid();
    else if (tree == tdcTree)
        temp = gInputFile->TDCid();
    item = new QTableWidgetItem(QString::number(temp));

    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setFont(font);
    ui->tableLeaf->setItem(0, 1, item);

    for (int i = 0; i < N_BOARD_CHANNELS; i++)
    {
        item = new QTableWidgetItem(QString::number(i));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableLeaf->setItem(i + 1, 0, item);

        if (tree == hgTree)
            item = new QTableWidgetItem(QString::number(gInputFile->HGamp()[i]));
        else if (tree == lgTree)
            item = new QTableWidgetItem(QString::number(gInputFile->LGamp()[i]));
        else if (tree == tdcTree)
            item = new QTableWidgetItem(QString::number(gInputFile->TDCTime()[i]));

        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableLeaf->setItem(i + 1, 1, item);
    }
}

void Mainwindow::on_lineChoosePage_textChanged(const QString &arg1)
{
    auto page = arg1.toInt();
    showPage(page);
}

void Mainwindow::on_btnPrePage_clicked()
{
    showPage(fCurrentPage - 1);
}

void Mainwindow::on_btnNextPage_clicked()
{
    showPage(fCurrentPage + 1);
}
