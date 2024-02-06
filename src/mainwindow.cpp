
#include <QTableWidgetItem>
#include <QStringList>
#include <QString>
#include <QFont>
#include <QFileDialog>

#include <iostream>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "T0TS.h"

void AlignRuning::startAlign(QString sInput, QString sOutput)
{
    if (gInputFile->IsOpen())
    {
        gInputFile->CloseFile();
    }
    if (gAlignOutput->IsOpen())
    {
        gAlignOutput->CloseFile();
    }
    auto error = gInputFile->OpenFile(sInput.toStdString());
    if (error < 0)
        return;
    error = gAlignOutput->OpenFile(sOutput.toStdString());

    if (error < 0)
        return;
    emit updateRowSignal(sInput, gInputFile->GetEntries(hgTree), gInputFile->GetEntries(lgTree), gInputFile->GetEntries(tdcTree));
    auto rtn = gAlignOutput->AlignAllEntries();
    emit stopAlignSignal(sInput, rtn);
}

void AlignRuning::startT0Match(QVector<int> *boardArray, QString sInputTxtDir, QString sOutputROOTDir)
{
    QDir dir;
    std::string txtInPath = sInputTxtDir.toStdString();
    std::string sT0FinalROOTPath = sOutputROOTDir.toStdString();
    std::string sROOTOutPath = txtInPath + "/T0TS/";
    dir.mkdir(QString::fromStdString(sROOTOutPath));

    gT0Manager->ForceClear();
    gT0Manager->InitBoardArray(std::vector<int>(boardArray->begin(), boardArray->end()), txtInPath, sROOTOutPath);
    for (int i = 0; i < boardArray->size(); i++)
    {
        int entries;
        double startT0, endT0;
        auto counter = gT0Manager->ConvertTS(i, entries, startT0, endT0);
        emit updateT0Row(i, entries, startT0 / 1e9, endT0 / 1e9);
    }
    gT0Manager->ConvertAllTS();

    std::string sT0FinalROOTFile = txtInPath + "./TS.root";
    gT0Manager->InitT0Matching(sT0FinalROOTFile);
    gT0Manager->FindBegining();
    int counter = 0;
    for (;; counter++)
    {
        auto flag = gT0Manager->SaveNextTS();
        if (!flag)
            break;
    }
    // gT0Manager->MatchT0(boardArray, sInputTxtDir.toStdString(), sOutputROOTDir.toStdString());
    emit stopT0Signal(&gT0Manager->GetInsideMatchedCounter());
    gT0Manager->CloseT0ROOTFile();
}

void AlignRuning::startBoardMatch()
{
    auto entries = gDataMatchManager->DoMatch();
    emit stopBoardMatchSignal(entries);
    gDataMatchManager->CloseFile();
}

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
    ui->tableLeaf->setColumnWidth(1, 150);

    // Setting Batch Test Table
    QTableWidgetItem *batchHeader;
    QStringList batchHeaderText;
    batchHeaderText << "Input File"
                    << "HG"
                    << "LG"
                    << "TDC"
                    << "Output File"
                    << "Aligned";
    ui->tableBatchAlign->setColumnCount(batchHeaderText.count());

    for (int i = 0; i < ui->tableBatchAlign->columnCount(); i++)
    {
        batchHeader = new QTableWidgetItem(batchHeaderText.at(i));
        QFont font = batchHeader->font();
        font.setBold(true);
        font.setPointSize(12);
        batchHeader->setFont(font);
        ui->tableBatchAlign->setHorizontalHeaderItem(i, batchHeader);
    }
    ui->tableBatchAlign->verticalHeader()->setVisible(0);
    ui->tableBatchAlign->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableBatchAlign->setColumnWidth(0, 200);
    ui->tableBatchAlign->setColumnWidth(1, 50);
    ui->tableBatchAlign->setColumnWidth(2, 50);
    ui->tableBatchAlign->setColumnWidth(3, 50);
    ui->tableBatchAlign->setColumnWidth(4, 200);
    ui->tableBatchAlign->setColumnWidth(5, 80);

    // Setting T0 Match Table
    QTableWidgetItem *t0MatchHeader;
    QStringList t0MatchText;
    t0MatchText
        << "Board No"
        << "Input File"
        << "Output File"
        << "Entries"
        << "First"
        << "Last"
        << "Matched Entries";
    ui->tableT0Match->setColumnCount(t0MatchText.count());

    for (int i = 0; i < ui->tableT0Match->columnCount(); i++)
    {
        t0MatchHeader = new QTableWidgetItem(t0MatchText.at(i));
        QFont font = t0MatchHeader->font();
        font.setBold(true);
        font.setPointSize(12);
        t0MatchHeader->setFont(font);
        ui->tableT0Match->setHorizontalHeaderItem(i, t0MatchHeader);
    }
    ui->tableT0Match->verticalHeader()->setVisible(0);
    ui->tableT0Match->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableT0Match->setColumnWidth(0, 50);
    ui->tableT0Match->setColumnWidth(1, 200);
    ui->tableT0Match->setColumnWidth(2, 200);
    ui->tableT0Match->setColumnWidth(3, 75);
    ui->tableT0Match->setColumnWidth(4, 75);
    ui->tableT0Match->setColumnWidth(5, 75);
    ui->tableT0Match->setColumnWidth(6, 75);

    // Setting Board Data Table
    on_cbxDefaultMatchFile_stateChanged(Qt::CheckState::Checked);
    ui->btnStartBoardMatch->setEnabled(0);

    QTableWidgetItem *boardDataHeader;
    QStringList boardDataText;
    boardDataText
        << "Board No"
        << "File Name"
        << "Entries"
        << "First"
        << "Last"
        << "Matched Entries";
    ui->tableBoardData->setColumnCount(boardDataText.count());

    for (int i = 0; i < ui->tableBoardData->columnCount(); i++)
    {
        boardDataHeader = new QTableWidgetItem(boardDataText.at(i));
        QFont font = boardDataHeader->font();
        font.setBold(true);
        font.setPointSize(12);
        boardDataHeader->setFont(font);
        ui->tableBoardData->setHorizontalHeaderItem(i, boardDataHeader);
    }
    ui->tableBoardData->verticalHeader()->setVisible(0);
    ui->tableBoardData->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableBoardData->setColumnWidth(0, 50);
    ui->tableBoardData->setColumnWidth(1, 200);
    ui->tableBoardData->setColumnWidth(2, 75);
    ui->tableBoardData->setColumnWidth(3, 75);
    ui->tableBoardData->setColumnWidth(4, 75);
    ui->tableBoardData->setColumnWidth(5, 75);
    ui->tableBoardData->setColumnWidth(6, 75);

    // Setting ui
    ui->btnCloseFile->setEnabled(0);
    ui->btnCloseOutputFile->setEnabled(0);
    ui->btnAlignOne->setEnabled(0);
    ui->btnAlignAll->setEnabled(0);

    // Setting Concurrent threads
    fAlignWorker = new AlignRuning;
    fAlignWorker->moveToThread(&fWorkThread);
    connect(this, &Mainwindow::startAlignRequest, fAlignWorker, &AlignRuning::startAlign);
    connect(this, &Mainwindow::startT0Request, fAlignWorker, &AlignRuning::startT0Match);
    connect(this, &Mainwindow::startBoardMatchRequest, fAlignWorker, &AlignRuning::startBoardMatch);
    connect(fAlignWorker, &AlignRuning::stopAlignSignal, this, &Mainwindow::handle_AlignDone);
    connect(fAlignWorker, &AlignRuning::stopT0Signal, this, &Mainwindow::handle_T0Done);
    connect(fAlignWorker, &AlignRuning::stopBoardMatchSignal, this, &Mainwindow::handle_BoardMatchDone);
    connect(fAlignWorker, &AlignRuning::updateRowSignal, this, &Mainwindow::handle_UpdateRow);
    connect(fAlignWorker, &AlignRuning::updateT0Row, this, &Mainwindow::handle_UpdateT0Row);
    connect(&fWorkThread, &QThread::finished, fAlignWorker, &QObject::deleteLater);
    fWorkThread.start();
}

Mainwindow::~Mainwindow()
{
    delete ui;
}

void Mainwindow::DisplayAlignedEntries(int entries)
{
    ui->lineAlignedEntries->setText(QString::number(entries));
}

Mainwindow *Mainwindow::Instance()
{
    static Mainwindow *instance = new Mainwindow(nullptr);
    return instance;
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

#include <QDateTime>
void Mainwindow::on_btnOutFile_clicked()
{
    if (sOutCurrentPath == "")
        sOutCurrentPath = sCurrentPath;
    sOutCurrentPath = QFileDialog::getExistingDirectory(this, "Choose output path.", sOutCurrentPath);
    ui->lblOutFilePath->setText(sOutCurrentPath);
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
    sOutFileName = sOutCurrentPath + "/" + ui->lblOutFileName->text() + QDateTime::currentDateTime().toString("-yyyy-MM-dd-hh-mm-ss") + ".root";
    std::cout << "Output file choosing: " << sOutFileName.toStdString() << std::endl;

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

    // Out ui Setting:
    if (gAlignOutput->IsOpen())
    {
        ui->btnAlignOne->setEnabled(1);
        ui->btnAlignAll->setEnabled(1);
    }

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

    ui->lineAlignedEntries->setText("");

    ui->lblHGid->setText(QString::number(0));
    ui->lblLGid->setText(QString::number(0));
    ui->lblTDCid->setText(QString::number(0));

    ui->btnAlignOne->setEnabled(0);
    ui->btnAlignAll->setEnabled(0);
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
            item = new QTableWidgetItem(QString::number((uint64_t)ConvertTDC2Time(gInputFile->TDCTime()[i])));

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

void Mainwindow::on_btnAlignOne_clicked()
{
    auto rtn = gAlignOutput->AlignOneEntry();

    QString text;
    if (rtn == FileNotOpen)
    {
        text = "Output file not open: " + sInFileName;
        QMessageBox::information(this, "Error while open file.", text, QMessageBox::Ok, QMessageBox::Cancel);
    }
    if (rtn == InputFileNotOpen)
    {
        text = "Input file not open: " + sInFileName;
        QMessageBox::information(this, "Error while open file.", text, QMessageBox::Ok, QMessageBox::Cancel);
    }
    if (rtn == EndOfInputFile)
    {
        text = "End of file: " + sInFileName;
        QMessageBox::information(this, "Read until the end of file.", text, QMessageBox::Ok, QMessageBox::Cancel);
    }
}

void Mainwindow::on_btnOpenOutputFile_clicked()
{
    // if (fInFile && fInFile->IsOpen())
    //     return;
    // if (fInFile)
    //     if (!fInFile->IsOpen())
    //         delete fInFile;

    auto error = gAlignOutput->OpenFile(sOutFileName.toStdString());

    // Open file and process exception
    if (error == FileAlreadyOpen)
        return;
    if (error == FileNotOpen)
    {
        QString text = "Cannot open file: " + sInFileName;
        QMessageBox::information(this, "Error while open file.", text, QMessageBox::Ok, QMessageBox::Cancel);
        return;
    }

    ui->btnOpenOutputFile->setEnabled(0);
    ui->btnOutFile->setEnabled(0);
    ui->btnCloseOutputFile->setEnabled(1);
    if (gInputFile->IsOpen())
    {
        ui->btnAlignOne->setEnabled(1);
        ui->btnAlignAll->setEnabled(1);
    }
}

void Mainwindow::on_btnCloseOutputFile_clicked()
{
    gAlignOutput->CloseFile();

    // setting UI
    ui->btnOpenOutputFile->setEnabled(1);
    ui->btnOutFile->setEnabled(1);
    ui->btnCloseOutputFile->setEnabled(0);

    ui->btnAlignOne->setEnabled(0);
    ui->btnAlignAll->setEnabled(0);
}

#include <QtConcurrent/QtConcurrent>
void TempFunc()
{
    gAlignOutput->AlignAllEntries();
}
void Mainwindow::on_btnAlignAll_clicked()
{
    QtConcurrent::run(TempFunc);
    // gAlignOutput->AlignAllEntries();
    ui->btnAlignOne->setEnabled(0);
    ui->btnAlignAll->setEnabled(0);
}

void Mainwindow::on_btnBatchInPath_clicked()
{
    if (sBatchInPath == "")
        sBatchInPath = sCurrentPath;
    auto tempString = QFileDialog::getExistingDirectory(this, "Choose Batch Align Input Path.", sBatchInPath);
    if (tempString == "")
    {
        fBatchInFlag = 0;
        ui->lineBatchInPath->setText("");
        return;
    }
    sBatchInPath = tempString;
    fBatchInFlag = 1;
    ui->lineBatchInPath->setText(sBatchInPath);
}

void Mainwindow::on_btnBatchOutPath_clicked()
{
    if (sBatchOutPath == "")
        sBatchOutPath = sBatchInPath;
    auto tempString = QFileDialog::getExistingDirectory(this, "Choose Batch Align Output Path.", sBatchOutPath);
    if (tempString == "")
    {
        fBatchOutFlag = 0;
        ui->lineBatchOutPath->setText("");
        return;
    }
    sBatchOutPath = tempString;
    fBatchOutFlag = 1;
    ui->lineBatchOutPath->setText(sBatchOutPath);
}

void Mainwindow::on_lineBatchInPath_textChanged(const QString &arg1)
{
    sBatchInPath = arg1;
    // std::cout << sBatchInPath.toStdString() << std::endl;
}

void Mainwindow::on_lineBatchOutPath_textChanged(const QString &arg1)
{
    sBatchOutPath = arg1;
    // std::cout << sBatchOutPath.toStdString() << std::endl;
}

#include <QDir>
#include <QStringList>
QString ProcessRAWDataOutputName(QString sInput)
{
    auto filename = sInput.mid(0, sInput.size() - 5);
    QString sOutPre = "";

    bool fTimeStampFlag = 0;
    auto list = filename.split('-');
    if (list.size() >= 6)
    {
        QString sdatetime = "";
        for (int i = 0; i < 6; i++)
        {
            sdatetime += list.at(list.size() - (6 - i));
            if (i < 5)
                sdatetime += "-";
        }
        // sdatetime = sdatetime.mid(0, sdatetime.size() - 5);
        // std::cout << sdatetime.toStdString() << std::endl;
        QDateTime dateTime0, dateTime1;
        dateTime0 = dateTime0.fromString(sdatetime, "yyyy-MM-dd-hh-mm-ss");
        // std::cout << dateTime0.toString("yyyy MM dd hh mm ss").toStdString() << std::endl;
        if (dateTime0 != dateTime1)
            fTimeStampFlag = 1;
    }

    if (fTimeStampFlag)
    {
        for (int i = 0; i < list.size() - 6; i++)
        {
            sOutPre += list.at(i);
            sOutPre += '-';
        }
    }
    else
    {
        sOutPre = filename + "-";
    }
    auto sOutName = sOutPre + "Aligned";
    //  + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".root";
    return sOutName;
}

void Mainwindow::on_btnStartBatch_clicked()
{
    // std::cout << sInFileList.at(0).toStdString() << '\t' << sOutFileList.at(0).toStdString() << std::endl;
    // Add TimeStamp before start align, update ui table
    if (ui->ckbTimeStamp->isChecked())
        sOutFileList[0] = sOutFileList[0] + QDateTime::currentDateTime().toString("-yyyy-MM-dd-hh-mm-ss-zzz") + ".root";
    else
        sOutFileList[0] = sOutFileList[0] + ".root";

    auto list = sOutFileList[0].split('/');
    ui->tableBatchAlign->item(0, 4)->setText(list.at(list.size() - 1));
    emit startAlignRequest(sInFileList.at(0), sOutFileList.at(0));

    // Disable UI
    ui->tabFilePreparation->setEnabled(0);
    ui->btnGenerateFileList->setEnabled(0);
    ui->btnStartBatch->setEnabled(0);
}

void Mainwindow::handle_AlignDone(QString sInput, int alignedEntry)
{
    int handle = sInFileList.indexOf(sInput);
    auto item = new QTableWidgetItem(QString::number(alignedEntry));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableBatchAlign->setItem(handle, 5, item);
    gInputFile->CloseFile();
    gAlignOutput->CloseFile();

    if (handle >= 0 && handle < sInFileList.size() - 1)
    {
        // Add TimeStamp before start align, update ui table
        if (ui->ckbTimeStamp->isChecked())
            sOutFileList[handle + 1] = sOutFileList[handle + 1] + QDateTime::currentDateTime().toString("-yyyy-MM-dd-hh-mm-ss-zzz") + ".root";
        else
            sOutFileList[handle + 1] = sOutFileList[handle + 1] + ".root";
        auto list = sOutFileList[handle + 1].split('/');
        ui->tableBatchAlign->item(handle + 1, 4)->setText(list.at(list.size() - 1));
        emit startAlignRequest(sInFileList.at(handle + 1), sOutFileList.at(handle + 1));
    }
    else
    {
        // Enable UI
        ui->tabFilePreparation->setEnabled(1);
        ui->btnGenerateFileList->setEnabled(1);
        ui->btnStartBatch->setEnabled(1);
    }
}
void Mainwindow::handle_UpdateRow(QString sInput, int hgEntry, int lgEntry, int tdcEntry)
{
    int handle = sInFileList.indexOf(sInput);
    auto item = new QTableWidgetItem(QString::number(hgEntry));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableBatchAlign->setItem(handle, 1, item);

    item = new QTableWidgetItem(QString::number(lgEntry));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableBatchAlign->setItem(handle, 2, item);

    item = new QTableWidgetItem(QString::number(tdcEntry));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableBatchAlign->setItem(handle, 3, item);
}

void Mainwindow::handle_UpdateT0Row(int boardIndex, int T0TSCounter, double startT0, double endT0)
{
    // std::cout << "Signal in updating" << '\t' << boardIndex << '\t' << T0TSCounter << std::endl;
    int handle = boardIndex;
    auto item = new QTableWidgetItem(QString::number(T0TSCounter));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableT0Match->setItem(handle, 3, item);

    item = new QTableWidgetItem(QString::number(startT0));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableT0Match->setItem(handle, 4, item);

    item = new QTableWidgetItem(QString::number(endT0));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableT0Match->setItem(handle, 5, item);
}

void Mainwindow::handle_T0Done(std::vector<int> *matchedEntries)
{
    int handle;
    QTableWidgetItem *item;

    for (int i = 0; i < matchedEntries->size(); i++)
    {
        handle = i;
        item = new QTableWidgetItem(QString::number(matchedEntries->at(i)));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableT0Match->setItem(handle, 6, item);
    }
    ui->btnStartT0Match->setEnabled(1);
}

void Mainwindow::handle_BoardMatchDone(int entries)
{
    // ui->btnStartBoardMatch->setEnabled(1);
    ui->btnReadBoardData->setEnabled(1);

    QTableWidgetItem *item;
    for (int i = 0; i < sBoardDataList.size(); i++)
    {
        item = new QTableWidgetItem(QString::number(entries));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableBoardData->setItem(i, 5, item);
    }
}

void Mainwindow::on_btnGenerateFileList_clicked()
{
    // Verify path
    if (!fBatchInFlag || !fBatchOutFlag)
        return;

    QDir dir(sBatchInPath);
    QStringList nameFilters;
    nameFilters << "*.root";

    sInFileList = dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);
    sOutFileList.clear();
    if (sInFileList.size() == 0)
    {
        QString text = "Find no file in " + sBatchInPath;
        QMessageBox::information(this, "Error while open File.", text, QMessageBox::Ok, QMessageBox::Cancel);
        return;
    }

    dir.setPath(sBatchOutPath);
    if (!dir.exists())
    {
        QString text = "Folder " + sBatchInPath + " Not Exists.";
        QMessageBox::information(this, "Error while open File.", text, QMessageBox::Ok, QMessageBox::Cancel);
        return;
    }

    // Close all file in test align mode
    on_btnCloseFile_clicked();
    on_btnCloseOutputFile_clicked();

    QTableWidgetItem *item;
    ui->tableBatchAlign->clearContents();
    ui->tableBatchAlign->setRowCount(sInFileList.size());
    for (int i = 0; i < sInFileList.size(); i++)
    {
        auto filename = sInFileList.at(i);

        item = new QTableWidgetItem(filename);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableBatchAlign->setItem(i, 0, item);
        // std::cout << sInFileList.at(i).toStdString() << std::endl;

        auto outFileName = ProcessRAWDataOutputName(filename);
        if (ui->ckbTimeStamp->isChecked())
            item = new QTableWidgetItem(outFileName + "-(Time Stamp).root");
        else
            item = new QTableWidgetItem(outFileName + ".root");

        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableBatchAlign->setItem(i, 4, item);

        // Add Full Path in Inside List
        sInFileList[i] = sBatchInPath + "/" + sInFileList[i];
        sOutFileList.push_back(sBatchOutPath + "/" + outFileName);
        // std::cout << ProcessRAWDataOutputName(filename).toStdString() << std::endl;
    }
}

QString ProcessT0OutputName(QString sInput, int &boardNo)
{
    auto filename = sInput.mid(0, sInput.size() - 4);
    QString sOutPre = "";

    bool fTimeStampFlag = 0;
    sOutPre = filename;
    auto sOutName = sOutPre;
    //  + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".root";

    auto sBoardNo = filename.mid(5, filename.size() - 7);
    boardNo = sBoardNo.toInt();
    return sOutName;
}

void Mainwindow::on_btnReadT0Files_clicked()
{
    // Verify path
    if (!fT0InFlag || !fT0OutFlag)
        return;

    QDir dir(sT0InPath);
    QStringList nameFilters;
    nameFilters << "Board*TS.txt";

    sT0InList = dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);
    sT0OutList.clear();
    if (sT0InList.size() == 0)
    {
        QString text = "Find no file in " + sT0InPath;
        QMessageBox::information(this, "Error while open File.", text, QMessageBox::Ok, QMessageBox::Cancel);
        return;
    }

    dir.setPath(sT0OutPath);
    if (!dir.exists())
    {
        QString text = "Folder " + sT0InPath + " Not Exists.";
        QMessageBox::information(this, "Error while open File.", text, QMessageBox::Ok, QMessageBox::Cancel);
        return;
    }

    QTableWidgetItem *item;
    ui->tableT0Match->clearContents();
    ui->tableT0Match->setRowCount(sT0InList.size());
    fT0BoardArray.clear();
    for (int i = 0; i < sT0InList.size(); i++)
    {
        auto filename = sT0InList.at(i);

        item = new QTableWidgetItem(filename);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableT0Match->setItem(i, 1, item);
        // std::cout << sT0InList.at(i).toStdString() << std::endl;

        int boardNo;
        auto outFileName = ProcessT0OutputName(filename, boardNo);

        item = new QTableWidgetItem(outFileName + ".root");
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableT0Match->setItem(i, 2, item);

        item = new QTableWidgetItem(QString::number(boardNo));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableT0Match->setItem(i, 0, item);

        fT0BoardArray.push_back(boardNo);

        // Add Full Path in Inside List
        sT0InList[i] = sT0InPath + "/" + sT0InList[i];
        sT0OutList.push_back(sT0OutPath + "/" + outFileName);
        // std::cout << ProcessRAWDataOutputName(filename).toStdString() << std::endl;
    }
}

void Mainwindow::on_btnT0TSInputPath_clicked()
{
    if (sT0InPath == "")
        sT0InPath = sCurrentPath;
    auto tempString = QFileDialog::getExistingDirectory(this, "Choose Batch Align Input Path.", sT0InPath);
    if (tempString == "")
    {
        fT0InFlag = 0;
        ui->lineT0InPath->setText("");
        return;
    }
    sT0InPath = tempString;
    if (sT0InPath != "" && sT0OutPath == "")
        sT0OutPath = sT0InPath;
    fT0InFlag = 1;
    ui->lineT0InPath->setText(sT0InPath);
}

void Mainwindow::on_btnT0TSOutputPath_clicked()
{
    if (sT0OutPath == "")
        sT0OutPath = sT0InPath;
    auto tempString = QFileDialog::getExistingDirectory(this, "Choose Batch Align Output Path.", sT0OutPath);
    if (tempString == "")
    {
        fT0OutFlag = 0;
        ui->lineT0OutPath->setText("");
        return;
    }
    sT0OutPath = tempString;
    ui->lineT0OutPath->setText(sT0OutPath);
    fT0OutFlag = 1;
}

void Mainwindow::on_btnStartT0Match_clicked()
{
    ui->btnStartT0Match->setEnabled(0);
    emit startT0Request(&fT0BoardArray, sT0InPath, sT0OutPath);
}

void Mainwindow::on_btnT0TSFile_clicked()
{
    if (sT0ROOTFile == "")
        sT0ROOTFile = sCurrentPath;
    auto tempString = QFileDialog::getOpenFileName(this, "Choose Batch Align Input Path.", sT0ROOTFile, "TS*.root");
    if (tempString == "")
    {
        fT0ROOTFlag = 0;
        ui->lineTSFile->setText("");
        return;
    }
    sT0ROOTFile = tempString;
    sBoardDataPath = sT0ROOTFile + "/../../Processed/";
    ui->lineTSFile->setText(sT0ROOTFile);
    fT0ROOTFlag = 1;
}

void Mainwindow::on_btnBoardPath_clicked()
{
    if (sBoardDataPath == "")
        sBoardDataPath = sT0ROOTFile + "/../../Processed/";
    auto tempString = QFileDialog::getExistingDirectory(this, "Choose Batch Align Output Path.", sBoardDataPath);
    if (tempString == "")
    {
        fBoardDataFlag = 0;
        ui->lineBoardPath->setText("");
        return;
    }
    sBoardDataPath = tempString;
    ui->lineBoardPath->setText(sBoardDataPath);
    fBoardDataFlag = 1;

    if (fDefaultMatchFileFlag)
    {
        sMatchPath = sBoardDataPath;
        sMatchFile = "MatchEntries.root";
        ui->lineMatchFilePath->setText(sMatchPath);
        ui->lblMatchFile->setText(sMatchFile);
    }
}

int ProcessBoardDataFileName(QString sInput, int &boardNo, QString sPath = "./")
{
    auto filename = sInput.mid(0, sInput.size() - QString(".root").size());

    bool fTimeStampFlag = 0;
    //  + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".root";

    auto sBoardNo = filename.mid(QString("Board").size(), filename.size() - QString("Board-Aligned").size());
    boardNo = sBoardNo.toInt();

    int entries = 0;
    TFile f((sPath + "/" + sInput).toLocal8Bit().data());
    auto tree = (TTree *)f.Get("board");
    if (tree)
        entries = tree->GetEntries();
    return entries;
}

void Mainwindow::on_btnReadBoardData_clicked()
{
    // Verify path
    if (!fT0ROOTFlag || !fBoardDataFlag)
        return;
    if (!fDefaultMatchFileFlag)
        sMatchFile = ui->lblMatchFile->text();

    QDir dir(sBoardDataPath);
    QStringList nameFilters;
    nameFilters << "Board*-Aligned.root";

    sBoardDataList = dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);
    if (sBoardDataList.size() == 0)
    {
        QString text = "Find no file in " + sBoardDataPath;
        QMessageBox::information(this, "Error while open File.", text, QMessageBox::Ok, QMessageBox::Cancel);
        return;
    }

    // Set table for Board Data
    QTableWidgetItem *item;
    ui->tableBoardData->clearContents();
    ui->tableBoardData->setRowCount(sBoardDataList.size());
    fDataBoardArray.clear();
    for (int i = 0; i < sBoardDataList.size(); i++)
    {
        auto filename = sBoardDataList.at(i);

        item = new QTableWidgetItem(filename);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableBoardData->setItem(i, 1, item);
        // std::cout << sBoardDataList.at(i).toStdString() << std::endl;

        int boardNo;
        int entries = ProcessBoardDataFileName(filename, boardNo, sBoardDataPath);

        item = new QTableWidgetItem(QString::number(boardNo));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableBoardData->setItem(i, 0, item);

        item = new QTableWidgetItem(QString::number(entries));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableBoardData->setItem(i, 2, item);

        fDataBoardArray.push_back(boardNo);

        // Add Full Path in Inside List
        sBoardDataList[i] = sBoardDataPath + "/" + sBoardDataList[i];
        // std::cout << ProcessRAWDataOutputName(filename).toStdString() << std::endl;
    }

    gDataMatchManager->DoInitiate(fDataBoardArray.toStdVector(), sT0ROOTFile.toStdString(), sBoardDataPath.toStdString(), (sMatchPath + "/" + sMatchFile).toStdString());
    if (gDataMatchManager->ReadyForMatch())
    {
        ui->btnReadBoardData->setEnabled(0);
        ui->btnStartBoardMatch->setEnabled(1);
    }
}

void Mainwindow::on_btnStartBoardMatch_clicked()
{
    ui->btnStartBoardMatch->setEnabled(0);
    emit startBoardMatchRequest();
}

void Mainwindow::on_btnMatchFileDir_clicked()
{
    if (sMatchPath == "")
        sMatchPath = sBoardDataPath;
    auto tempString = QFileDialog::getExistingDirectory(this, "Choose Batch Align Output Path.", sMatchPath);
    if (tempString == "")
    {
        ui->lineBoardPath->setText("");
        return;
    }
    sMatchPath = tempString;
    ui->lineMatchFilePath->setText(sMatchPath);
}

void Mainwindow::on_cbxDefaultMatchFile_stateChanged(int arg1)
{
    if (arg1 == Qt::CheckState::Checked)
    {
        sMatchPath = sBoardDataPath;
        sMatchFile = "MatchEntries.root";
        ui->lineMatchFilePath->setText(sMatchPath);
        ui->lblMatchFile->setText(sMatchFile);
        fDefaultMatchFileFlag = 1;

        ui->lineMatchFilePath->setEnabled(0);
        ui->lblMatchFile->setEnabled(0);
        ui->btnMatchFileDir->setEnabled(0);

        ui->cbxDefaultMatchFile->setCheckState(Qt::CheckState::Checked);
    }
    else if (arg1 == Qt::CheckState::Unchecked)
    {
        fDefaultMatchFileFlag = 0;

        ui->lineMatchFilePath->setEnabled(1);
        ui->lblMatchFile->setEnabled(1);
        ui->btnMatchFileDir->setEnabled(1);

        ui->cbxDefaultMatchFile->setCheckState(Qt::CheckState::Unchecked);
    }
}
