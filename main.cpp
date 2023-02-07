// Qt
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QTime>

// C++ STL
#include <fstream>
#include <iostream>
#include <time.h>

// ROOT
#include <TApplication.h>

// User
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication qapp(argc, argv);
    new TApplication("QTCanvas Demo", &argc, argv);

    auto win = new Mainwindow();
    win->show();
    return qapp.exec();

    return 1;
}