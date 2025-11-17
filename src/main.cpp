#include <QApplication>
#include "backend.h"
#include "mainwindow.h"


int main(int argc, char *argv[])
{
QApplication app(argc, argv);

QFont appFont("Roboto", 18);          // or "Roboto", "Inter", "Helvetica", "Arial" …
appFont.setWeight(QFont::Medium);       // or Bold if you prefer
// appFont.setStyleStrategy(QFont::PreferAntialias); // optional, usually default
QApplication::setFont(appFont);

Backend backend;
MainWindow win(&backend);
win.show();


return app.exec();
}
