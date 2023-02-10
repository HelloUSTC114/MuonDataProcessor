#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QString>

// C++ STL
#include <string>

// ROOT
#include <TFile.h>
#include <TTree.h>

// User
#include "AlignManager.h"

namespace Ui
{
    class Mainwindow;
}

class Mainwindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit Mainwindow(QWidget *parent = nullptr);
    ~Mainwindow();

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

    void on_btnBatchAlign_clicked();

private:
    Ui::Mainwindow *ui;

    // File string
    QString sInFileName, sOutFileName;
    QString sCurrentPath = "./";

    // // Read tree
    int fHGEntries = 0, fLGEntries = 0, fTDCEntries = 0;
    int fHGStartid = 0, fLGStartid = 0, fTDCStartid = 0;

    // Table setting
    uint64_t fCurrentPage = 0;
    void showPage(int page);
    void showLeaf(int entry, TreeType tree);
    void showHGLeaf(int entry);
    void showLGLeaf(int entry);
    void showTDCLeaf(int entry);
    void createItemsARow(int rowNo, int hgSerial, int hgID, int lgSerial, int lgID, int tdcSerial, int tdcID);
};

#endif // MAINWINDOW_H
