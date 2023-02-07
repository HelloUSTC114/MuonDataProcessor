#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QString>

// C++ STL
#include <string>

namespace Ui {
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

    void on_btnAnaFile_clicked();

private:
    Ui::Mainwindow *ui;

    QString 

};

#endif // MAINWINDOW_H
