#include "dfedit.h"
#include <QFileDialog>
#include <QDebug>

dfedit::dfedit(QWidget *parent): QMainWindow(parent)
{
    ui.setupUi(this);
    connect(ui.actionOpen,SIGNAL(triggered(bool)),this,SLOT(slotOpen(bool)));
    connect(ui.actionQuit,SIGNAL(triggered(bool)),this,SLOT(slotQuit(bool)));
    connect(ui.actionRun_DF,SIGNAL(triggered(bool)),this,SLOT(slotRunDF(bool)));
    connect(ui.actionSave,SIGNAL(triggered(bool)),this,SLOT(slotSave(bool)));
    connect(ui.actionSave_As,SIGNAL(triggered(bool)),this,SLOT(slotSaveAs(bool)));
    connect(ui.actionSetup_DF_executables,SIGNAL(triggered(bool)),this,SLOT(slotSetupDFs(bool)));
    ui.actionOpen->setIcon(QIcon::fromTheme("document-open"));
    ui.actionOpen->setIconText("Open");
    ui.actionSave->setIcon(QIcon::fromTheme("document-save"));
    ui.actionSave->setIconText("Save");
    ui.actionSave_As->setIcon(QIcon::fromTheme("document-save-as"));
    ui.actionSave_As->setIconText("Save As");
    ui.actionRun_DF->setIcon(QIcon::fromTheme("system-run"));
    ui.actionRun_DF->setIconText("Run DF");
    ui.actionQuit->setIcon(QIcon::fromTheme("application-exit"));
    ui.actionQuit->setIconText("Run DF");
}

dfedit::~dfedit()
{}

void dfedit::slotOpen(bool )
{
    QFileDialog fd(this,"Locate the Memoxy.xml file");
    fd.setNameFilter("Memory definition (Memory.xml)");
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    int result = fd.exec();
    if(result == QDialog::Accepted)
    {
        QStringList files = fd.selectedFiles();
        QString file = files[0];
        qDebug() << "File:" << file;
    }
}

void dfedit::slotQuit(bool )
{
    close();
}

void dfedit::slotSave(bool )
{
    // blah
}

void dfedit::slotRunDF(bool )
{
    // blah
}

void dfedit::slotSaveAs(bool )
{
    QFileDialog fd(this,"Choose file to save as...");
    fd.setNameFilter("Memory definition (*.xml)");
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

void dfedit::slotSetupDFs(bool )
{
    // dialog showing all the versions in Memory.xml that lets the user set up ways to run those versions...
    // currently unimplemented
}

#include "dfedit.moc"
