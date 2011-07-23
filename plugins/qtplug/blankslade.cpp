/*
 * Copyright (c) 2010 Petr Mrázek (peterix)
 * See LICENSE for details.
 */

#include "blankslade.h"
#include <QFileDialog>
#include <QDebug>
#include "glwidget.h"

blankslade::blankslade(QWidget *parent): QMainWindow(parent)
{
    ui.setupUi(this);
    GLWidget * glw = new GLWidget();
    ui.gridding->addWidget(glw);
    connect(ui.actionOpen,SIGNAL(triggered(bool)),this,SLOT(slotOpen(bool)));
    connect(ui.actionQuit,SIGNAL(triggered(bool)),this,SLOT(slotQuit(bool)));
    connect(ui.actionSave,SIGNAL(triggered(bool)),this,SLOT(slotSave(bool)));
    connect(ui.actionSave_As,SIGNAL(triggered(bool)),this,SLOT(slotSaveAs(bool)));
    ui.actionOpen->setIcon(QIcon::fromTheme("document-open"));
    ui.actionOpen->setIconText(tr("Open"));
    ui.actionSave->setIcon(QIcon::fromTheme("document-save"));
    ui.actionSave->setIconText(tr("Save"));
    ui.actionSave_As->setIcon(QIcon::fromTheme("document-save-as"));
    ui.actionSave_As->setIconText(tr("Save As"));
    ui.actionQuit->setIcon(QIcon::fromTheme("application-exit"));
    ui.actionQuit->setIconText(tr("Run DF"));
}

blankslade::~blankslade()
{}

void blankslade::slotOpen(bool )
{
    /*
    QFileDialog fd(this,tr("Locate the Memoxy.xml file"));
    fd.setNameFilter(tr("Memory definition (*.xml)"));
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    int result = fd.exec();
    if(result == QDialog::Accepted)
    {
        QStringList files = fd.selectedFiles();
        QString fileName = files[0];
        QDomDocument doc("memxml");
        QFile file(fileName);
        if(!file.open(QIODevice::ReadOnly))
        {
            return;
        }
        if(!doc.setContent(&file))
        {
            file.close();
            return;
        }
        mod = new MemXMLModel(doc,this);
        ui.entryView->setModel(mod);
        file.close();
    }
    */
}

void blankslade::slotQuit(bool )
{
    close();
}

void blankslade::slotSave(bool )
{
    // blah
}

void blankslade::slotRunDF(bool )
{
    // blah
}

void blankslade::slotSaveAs(bool )
{
    QFileDialog fd(this,tr("Choose file to save as..."));
    fd.setNameFilter(tr("Memory definition (*.xml)"));
    fd.setFileMode(QFileDialog::AnyFile);
    fd.selectFile("Memory.xml");
    fd.setAcceptMode(QFileDialog::AcceptSave);
    int result = fd.exec();
    if(result == QDialog::Accepted)
    {
        QStringList files = fd.selectedFiles();
        QString file = files[0];
        qDebug() << "File:" << file;
    }
}

void blankslade::slotSetupDFs(bool )
{
}

#include "blankslade.moc"
