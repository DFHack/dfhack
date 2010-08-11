#include <QtGui/QApplication>
#include "dfedit.h"


int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    dfedit appGui;
    appGui.show();
    return app.exec();
}
