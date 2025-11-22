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

    // Original logical size (before rotation)
    win.resize(1024, 768);
    win.show();                    // optional – helps layout calculate properly
    win.setWindowTitle("Toon Dashboard – Portrait");

    // ------------------------------------------------------------------
    // Rotate 90° clockwise – perfect pixel-for-pixel result
    // ------------------------------------------------------------------
    QGraphicsScene scene;

    QGraphicsProxyWidget* proxy = scene.addWidget(&win);

    // 1. Rotate 90° clockwise
    proxy->setTransform(QTransform().rotate(90));

    // 2. Critical fix: reposition so the rotated widget is fully visible
    //     After +90° rotation the original top-left corner moves to top-right.
    //     We want the new top-left to be at (0,0) in the portrait scene.
    proxy->setPos(768, 0);          // <-- this is the magic line

    // 3. Scene must exactly match the new portrait dimensions
    scene.setSceneRect(0, 0, 768, 1024);

    QGraphicsView view(&scene);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setFrameShape(QFrame::NoFrame);
    view.setBackgroundBrush(Qt::black);   // matches your dark theme

    // Make sure the view exactly fits the rotated content
    view.resize(768, 1024);
    view.setWindowTitle("Toon-like Dashboard (Rotated 90° CW)");
    view.show();

    return app.exec();
}
