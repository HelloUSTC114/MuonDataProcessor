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
void Mainwindow::on_btnOpenFile_clicked()
{
    if (fInFile && fInFile->IsOpen())
        return;
    if (fInFile)
        if (!fInFile->IsOpen())
            delete fInFile;

    // Open file and process exception
    fInFile = new TFile(sInFileName.toStdString().c_str());
    if (!fInFile)
        fInFile = NULL;
    if (!fInFile->IsOpen())
    {
        QString text = "Cannot open file: " + sInFileName;
        QMessageBox::information(this, "Error while open file.", text, QMessageBox::Ok, QMessageBox::Cancel);
        delete fInFile;
        fInFile = NULL;
        return;
    }

    // Get Trees
    fHGTree = (TTree *)fInFile->Get("HGTree");
    fLGTree = (TTree *)fInFile->Get("LGTree");
    fTDCTree = (TTree *)fInFile->Get("TDCTree");
    if (!fHGTree || !fLGTree || !fTDCTree)
    {
        QString text = "Cannot find three trees.";
        QMessageBox::information(this, "Error while open file.", text, QMessageBox::Ok, QMessageBox::Cancel);
        CloseFile();
        return;
    }
    ui->btnOpenFile->setEnabled(0);
    ui->btnInFile->setEnabled(0);
    ui->btnCloseFile->setEnabled(1);
    fIsOpen = 1;

    // Show Tree Information
    fHGEntries = fHGTree->GetEntries();
    fLGEntries = fHGTree->GetEntries();
    fTDCEntries = fTDCTree->GetEntries();
    ui->lblHGEntry->setText(QString::number(fHGEntries));
    ui->lblLGEntry->setText(QString::number(fLGEntries));
    ui->lblTDCEntry->setText(QString::number(fTDCEntries));

    // Tree setting
    fHGTree->SetBranchAddress("chHGid", &fHGid);
    fLGTree->SetBranchAddress("chLGid", &fLGid);
    fTDCTree->SetBranchAddress("chTDCid", &fTDCid);

    fHGTree->SetBranchAddress("chHG", fHGamp);
    fLGTree->SetBranchAddress("chLG", fLGamp);
    fTDCTree->SetBranchAddress("chTDC", fTDCTime);

    fHGTree->GetEntry(0);
    fLGTree->GetEntry(0);
    fTDCTree->GetEntry(0);
    ui->lblHGid->setText(QString::number(fHGid));
    ui->lblLGid->setText(QString::number(fLGid));
    ui->lblTDCid->setText(QString::number(fTDCid));

    // Table:
    showPage(0);
}

void Mainwindow::showPage(int page)
{
    if (!fIsOpen)
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
            fHGTree->GetEntry(idx);
            HGid = fHGid;
        }
        else
        {
            HGidx = -1;
            HGid = -1;
        }
        if (fLGEntries > idx)
        {
            LGidx = idx;
            fLGTree->GetEntry(idx);
            LGid = fLGid;
        }
        else
        {
            LGidx = -1;
            LGid = -1;
        }
        if (fTDCEntries > idx)
        {
            TDCidx = idx;
            fTDCTree->GetEntry(idx);
            TDCid = fTDCid;
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
    CloseFile();

    // setting UI
    ui->btnOpenFile->setEnabled(1);
    ui->btnInFile->setEnabled(1);
    ui->btnCloseFile->setEnabled(0);

    ui->lblHGEntry->setText(QString::number(0));
    ui->lblLGEntry->setText(QString::number(0));
    ui->lblTDCEntry->setText(QString::number(0));
}

void Mainwindow::CloseFile()
{
    fIsOpen = 0;

    if (fInFile)
    {
        fInFile->Close();
        delete fInFile;
    }
    fInFile = NULL;
    fHGTree = NULL;
    fLGTree = NULL;
    fTDCTree = NULL;
    ui->tableTree->clearContents();
    ui->tableLeaf->clearContents();
}

void Mainwindow::on_tableTree_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if (!fIsOpen)
        return;
    int entry = currentRow + 50 * fCurrentPage;
    if (currentColumn / 2 == 0)
        showHGLeaf(entry);
    if (currentColumn / 2 == 1)
        showLGLeaf(entry);
    if (currentColumn / 2 == 2)
        showTDCLeaf(entry);
}

void Mainwindow::showHGLeaf(int entry)
{
    ui->tableLeaf->clearContents();
    ui->tableLeaf->setRowCount(33);
    fHGTree->GetEntry(entry);
    QTableWidgetItem *item;

    item = new QTableWidgetItem("HG id:");
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    auto font = item->font();
    font.setBold(true);
    item->setFont(font);
    ui->tableLeaf->setItem(0, 0, item);

    item = new QTableWidgetItem(QString::number(fHGid));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setFont(font);
    ui->tableLeaf->setItem(0, 1, item);

    for (int i = 0; i < N_BOARD_CHANNELS; i++)
    {
        item = new QTableWidgetItem(QString::number(i));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableLeaf->setItem(i + 1, 0, item);

        item = new QTableWidgetItem(QString::number(fHGamp[i]));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableLeaf->setItem(i + 1, 1, item);
    }
}

void Mainwindow::showLGLeaf(int entry)
{
    ui->tableLeaf->clearContents();
    ui->tableLeaf->setRowCount(33);
    fLGTree->GetEntry(entry);
    QTableWidgetItem *item;

    item = new QTableWidgetItem("LG id:");
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    auto font = item->font();
    font.setBold(true);
    item->setFont(font);
    ui->tableLeaf->setItem(0, 0, item);

    item = new QTableWidgetItem(QString::number(fLGid));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setFont(font);
    ui->tableLeaf->setItem(0, 1, item);

    for (int i = 0; i < N_BOARD_CHANNELS; i++)
    {
        item = new QTableWidgetItem(QString::number(i));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableLeaf->setItem(i + 1, 0, item);

        item = new QTableWidgetItem(QString::number(fLGamp[i]));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableLeaf->setItem(i + 1, 1, item);
    }
}

void Mainwindow::showTDCLeaf(int entry)
{
    ui->tableLeaf->clearContents();
    ui->tableLeaf->setRowCount(34);
    fTDCTree->GetEntry(entry);
    QTableWidgetItem *item;

    item = new QTableWidgetItem("TDC id:");
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    auto font = item->font();
    font.setBold(true);
    item->setFont(font);
    ui->tableLeaf->setItem(0, 0, item);

    item = new QTableWidgetItem(QString::number(fTDCid));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setFont(font);
    ui->tableLeaf->setItem(0, 1, item);

    for (int i = 0; i < N_BOARD_CHANNELS + 1; i++)
    {
        item = new QTableWidgetItem(QString::number(i));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableLeaf->setItem(i + 1, 0, item);

        uint64_t tdc = fTDCTime[i];

        double fgFreq = 433.995; // in unit of MHz, board TDC frequency
        double coarseTime = (tdc >> 16) / fgFreq * 1e3;
        double fineTime = (tdc & 0xffff) / 65536.0 / fgFreq * 1e3;
        item = new QTableWidgetItem(QString::number(fineTime - coarseTime));
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
