#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QFont>
#include <QIcon>
int main(int argc,char* argv[]) {
    QApplication app(argc,argv);
    app.setApplicationName("Наземная станция"); app.setOrganizationName("Practical03");
    app.setStyle("Fusion"); app.setFont(QFont("Arial",13));
    app.setWindowIcon(QIcon(":/assets/station.svg"));
    MainWindow window; window.show();
    return app.exec();
}
