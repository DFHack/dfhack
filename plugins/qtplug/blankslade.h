/*
 * Copyright (c) 2010 Petr Mrázek (peterix)
 * See LICENSE for details.
 */

#ifndef blankslade_H
#define blankslade_H

#include <QtGui/QMainWindow>
#include "ui_main.h"

class blankslade : public QMainWindow
{
    Q_OBJECT
public:
    blankslade(QWidget *parent = 0);
    virtual ~blankslade();

private:
    Ui::MainWindow ui;
public slots:
    void slotOpen(bool);
    void slotQuit(bool);
    void slotSave(bool);
    void slotSaveAs(bool);
    void slotRunDF(bool);
    void slotSetupDFs(bool);
};
#endif // blankslade_H
