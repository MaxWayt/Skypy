#include "widget.h"
#include <QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    std::cout << "STARTING SKYPY" << std::endl;
    QApplication a(argc, argv);
    Widget w;
    w.show();

    return a.exec();
}
