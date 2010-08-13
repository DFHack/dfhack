#ifndef dfedit_H
#define dfedit_H

#include <QtGui/QMainWindow>
#include "ui_main.h"
#include "memxmlModel.h"

class dfedit : public QMainWindow
{
    Q_OBJECT
public:
    dfedit(QWidget *parent = 0);
    virtual ~dfedit();

private:
    Ui::MainWindow ui;
    MemXMLModel * mod;
public slots:
    void slotOpen(bool);
    void slotQuit(bool);
    void slotSave(bool);
    void slotSaveAs(bool);
    void slotRunDF(bool);
    void slotSetupDFs(bool);
};
#endif // dfedit_H
