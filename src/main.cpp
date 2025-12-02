// main.cpp
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>

#include "backend.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QFont appFont("Roboto", 18);
    appFont.setWeight(QFont::Medium);
    QApplication::setFont(appFont);

    Backend backend;
    MainWindow win(&backend);

    win.resize(1024, 768);
    win.show();  // required for proper layout before embedding

    QGraphicsScene scene;
    QGraphicsProxyWidget* proxy = scene.addWidget(&win);

    proxy->setTransform(QTransform().rotate(90));
    proxy->setPos(768, 0);                         // magic line stays

    scene.setSceneRect(0, 0, 768, 1024);

    QGraphicsView view(&scene);
    view.setOptimizationFlags(QGraphicsView::DontSavePainterState);
    view.setCacheMode(QGraphicsView::CacheNone);   // crucial
    view.setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    // NEW: these three lines kill the giant backing store
    view.setAttribute(Qt::WA_OpaquePaintEvent);
    view.setAttribute(Qt::WA_NoSystemBackground);
    view.setAttribute(Qt::WA_DontCreateNativeAncestors);

    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFrameShape(QFrame::NoFrame);
    view.setBackgroundBrush(Qt::black);

    view.resize(768, 1024);
    view.setFixedSize(768, 1024);                  // lock it
    view.setWindowTitle("Toon Dashboard – Portrait");
    view.show();

    return app.exec();
}
