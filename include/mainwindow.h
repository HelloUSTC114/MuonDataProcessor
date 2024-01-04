#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QString>
#include <QThread>
#include <QVector>

// C++ STL
#include <string>
#include <vector>

// ROOT
#include <TFile.h>
#include <TTree.h>

// User
#include "AlignManager.h"

namespace Ui
{
    class Mainwindow;
}

class AlignRuning : public QObject
{
    Q_OBJECT
signals:
    // For Batch align (Inside board) tasks
    void stopAlignSignal(QString sFileName, int alignedEntry);
    void updateRowSignal(QString sInput, int hgEntry, int lgEntry, int tdcEntry);

    // For T0Match task
    void updateT0Row(int boardIndex, int T0TSCounter, double startT0, double endT0);
    void stopT0Signal(std::vector<int> *matchedEntries);
public slots:
    // For Batch align (Inside board) tasks
    void startAlign(QString sInput, QString sOutput);

    // For T0Match task
    void startT0Match(QVector<int> *boardArray, QString sInputTxtDir, QString sOutputROOTDir);

private:
    QString fsFileName;
};

#define gAlignWin (Mainwindow::Instance())
class Mainwindow : public QMainWindow
{
    Q_OBJECT

public:
    ~Mainwindow();

    static Mainwindow *Instance();
    void DisplayAlignedEntries(int entries);

private slots:
    void on_btnInFile_clicked();

    void on_btnOutFile_clicked();

    void on_btnOpenFile_clicked();

    void on_btnCloseFile_clicked();

    void on_tableTree_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_lineChoosePage_textChanged(const QString &arg1);

    void on_btnPrePage_clicked();

    void on_btnNextPage_clicked();

    void on_btnAlignOne_clicked();

    void on_btnOpenOutputFile_clicked();

    void on_btnCloseOutputFile_clicked();

    void on_btnAlignAll_clicked();

    void on_btnBatchInPath_clicked();

    void on_btnBatchOutPath_clicked();

    void on_btnStartBatch_clicked();

    void on_lineBatchInPath_textChanged(const QString &arg1);

    void on_lineBatchOutPath_textChanged(const QString &arg1);

    void handle_AlignDone(QString sInput, int alignedEntry);
    void handle_UpdateRow(QString sInput, int hgEntry, int lgEntry, int tdcEntry);
    void handle_UpdateT0Row(int boardIndex, int T0TSCounter, double startT0, double endT0);
    void handle_T0Done(std::vector<int> *matchedEntries);

    void on_btnGenerateFileList_clicked();

    void on_btnReadT0Files_clicked();

    void on_btnT0TSInputPath_clicked();

    void on_btnT0TSOutputPath_clicked();

    void on_btnStartT0Match_clicked();

    void on_btnT0TSFile_clicked();

    void on_btnBoardPath_clicked();

    void on_btnReadBoardData_clicked();

    void on_btnStartBoardMatch_clicked();

signals:
    void startAlignRequest(QString sInput, QString sOutput);
    void startT0Request(QVector<int> *boardArray, QString sInputTxtDir, QString sOutputROOTDir);

private:
    Ui::Mainwindow *ui;

    explicit Mainwindow(QWidget *parent = nullptr);

    // File string
    QString sInFileName, sOutFileName;
    QString sCurrentPath = "./";
    QString sOutCurrentPath = "";

    // // Read tree
    int fHGEntries = 0, fLGEntries = 0, fTDCEntries = 0;
    int fHGStartid = 0, fLGStartid = 0, fTDCStartid = 0;

    // Table setting
    uint64_t fCurrentPage = 0;
    void showPage(int page);
    void showLeaf(int entry, TreeType tree);
    void createItemsARow(int rowNo, int hgSerial, int hgID, int lgSerial, int lgID, int tdcSerial, int tdcID);

    // Batch Align
    QString sBatchInPath = "", sBatchOutPath = "";
    QStringList sInFileList, sOutFileList;

    // T0 Match
    QString sT0InPath = "E:\\Data\\CRTest-3XY-Detectors\\Logic-6&\\3\\Source", sT0OutPath = "";
    QStringList sT0InList, sT0OutList;
    QVector<int> fBoardArray;

    // Align Manager Thread
    AlignRuning *fAlignWorker;
    QThread fWorkThread;
};

#endif // MAINWINDOW_H
