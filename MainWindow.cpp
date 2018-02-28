/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtWidgets>

#include "MainWindow.h"





MainWindow::MainWindow()
{
    // first, generate the "core" object that will handle the annotations
    this->annotations = new AnnotationsSet();



    // generate the interface, give the right pointer
    this->annotateArea = new AnnotateArea(this->annotations);

    // embed it into a ScrollArea, so that it can handle big images
    this->annotateScrollArea = new QScrollArea;

    // specify that the widget size can change...
    this->annotateScrollArea->setWidgetResizable(true);
    this->annotateScrollArea->setWidget(this->annotateArea);

    // this is for window-related stuff
    setCentralWidget(this->annotateScrollArea);

    // create the menu actions etc
    this->createActions();
    this->createMenus();

    this->updateActionsAvailability();


    // generate the object little window that will handle the class selection
    this->classSelection = new DialogClassSelection(this->annotations, this, Qt::Tool);
    this->classSelection->show();
    this->classSelection->setWindowTitle(tr("Class Selection"));
    this->classSelection->move(100, 100);

    // generate the browser, provide the right pointer
    this->annotsBrowser = new AnnotationsBrowser(this->annotations, this, Qt::Tool);
    this->annotsBrowser->show();
    this->annotsBrowser->setWindowTitle(tr("Objects Selection"));
    this->annotsBrowser->move(100, 400);


    // connect the class selection and the annotate area
    QObject::connect(this->classSelection, SIGNAL(classSelected(int)), this->annotateArea, SLOT(selectClassId(int)));

    QObject::connect(this->annotsBrowser, SIGNAL(annotationSelected(int)), this, SLOT(selectAnnot(int)));
    QObject::connect(this->annotateArea, SIGNAL(selectedObject(int)), this->annotsBrowser, SLOT(updateBrowser(int)));
    QObject::connect(this->annotateArea, SIGNAL(updateSignal()), this, SLOT(updateActionsAvailability()));



    setWindowTitle(tr("Annotate"));
    resize(500, 500);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::selectAnnot(int id)
{
    // update the selection browser
    this->annotsBrowser->updateBrowser(id);

    // update the selection area
    // but first, handle the frame thing in case we're looking at a video
    if (this->annotations->getRecord().getAnnotationById(id).FrameNumber != this->annotations->getCurrentFramePosition())
        this->annotateArea->displayFrame(this->annotations->getRecord().getAnnotationById(id).FrameNumber);

    this->annotateArea->selectAnnotation(id);
}

void MainWindow::updateActionsAvailability()
{
    this->configMenuAct->setEnabled(!(this->annotations->isImageOpen() || this->annotations->isVideoOpen()));
    this->nextFrameAct->setEnabled(this->annotations->canReadNextFrame());
    this->prevFrameAct->setEnabled(this->annotations->canReadPrevFrame());
    this->closeFileAct->setEnabled(this->annotations->isImageOpen() || this->annotations->isVideoOpen());
}


void MainWindow::openImage()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open Image File"), QDir::currentPath());
        if (!fileName.isEmpty())
            this->annotateArea->openImage(fileName);
    }
}


void MainWindow::openVideo()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open Video File"), QDir::currentPath());
        if (!fileName.isEmpty())
            this->annotateArea->openVideo(fileName);
    }
}

void MainWindow::loadAnnotations()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open Annotations File"), QDir::currentPath());
        if (!fileName.isEmpty())
            this->annotateArea->openAnnotations(fileName);
    }
}

void MainWindow::saveAnnotations()
{

}

void MainWindow::saveCurrentImage()
{

}

void MainWindow::closeFile()
{
    this->annotations->closeFile(maybeSave());

    this->annotsBrowser->updateBrowser(-1);
    this->annotateArea->reload();
}



void MainWindow::openConfiguration()
{
    // TO BE COMPLETED
}

/*
void MainWindow::save()
{
    QAction *action = qobject_cast<QAction *>(sender());
    QByteArray fileFormat = action->data().toByteArray();
    saveFile(fileFormat);
}
*/




/*
void MainWindow::penColor()
{
    QColor newColor = QColorDialog::getColor(this->annotateArea->penColor());
    if (newColor.isValid())
        this->annotateArea->setPenColor(newColor);
}
*/

void MainWindow::penWidth()
{
    bool ok;
    int newWidth = QInputDialog::getInt(this, tr("Scribble"),
                                        tr("Select pen width:"),
                                        this->annotateArea->penWidth(),
                                        1, 62, 1, &ok);
    if (ok)
        this->annotateArea->setPenWidth(newWidth);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Scribble"),
            tr("<p>The <b>Scribble</b> example shows how to use QMainWindow as the "
               "base widget for an application, and how to reimplement some of "
               "QWidget's event handlers to receive the events generated for "
               "the application's widgets:</p><p> We reimplement the mouse event "
               "handlers to facilitate drawing, the paint event handler to "
               "update the application and the resize event handler to optimize "
               "the application's appearance. In addition we reimplement the "
               "close event handler to intercept the close events before "
               "terminating the application.</p><p> The example also demonstrates "
               "how to use QPainter to draw an image in real time, as well as "
               "to repaint widgets.</p>"));
}

void MainWindow::createActions()
{
    this->openImageAct = new QAction(tr("&Open Image..."), this);
    this->openImageAct->setShortcuts(QKeySequence::Open);
    connect(this->openImageAct, SIGNAL(triggered()), this, SLOT(openImage()));

    this->openVideoAct = new QAction(tr("Open &Video..."), this);
    //this->openAct->setShortcuts(QKeySequence::Open);
    connect(this->openVideoAct, SIGNAL(triggered()), this, SLOT(openVideo()));

    this->openAnnotationsAct = new QAction(tr("Open &Annotations..."), this);
    //this->openAct->setShortcuts(QKeySequence::Open);
    connect(this->openAnnotationsAct, SIGNAL(triggered()), this, SLOT(loadAnnotations()));




    this->saveAnnotationsAct = new QAction(tr("&Save Annotations"), this);
    connect(this->saveAnnotationsAct, SIGNAL(triggered()), this, SLOT(saveAnnotations()));

    this->saveCurrentImageAct = new QAction(tr("Save &Current Annotation (single frame)"), this);
    connect(this->saveCurrentImageAct, SIGNAL(triggered()), this, SLOT(saveCurrentImage()));

    this->printAct = new QAction(tr("&Print..."), this);
    connect(this->printAct, SIGNAL(triggered()), this->annotateArea, SLOT(print()));



    this->closeFileAct = new QAction(tr("&Close"), this);
    this->closeFileAct->setShortcuts(QKeySequence::Close);
    connect(this->closeFileAct, SIGNAL(triggered()), this, SLOT(closeFile()));


    this->exitAct = new QAction(tr("E&xit"), this);
    this->exitAct->setShortcuts(QKeySequence::Quit);
    connect(this->exitAct, SIGNAL(triggered()), this, SLOT(close()));



    this->configMenuAct = new QAction(tr("&Configure the classes"), this);




    this->nextFrameAct = new QAction(tr("&Next frame"), this);
    this->nextFrameAct->setShortcut(Qt::Key_Right);
    connect(this->nextFrameAct, SIGNAL(triggered()), this->annotateArea, SLOT(displayNextFrame()));

    this->prevFrameAct = new QAction(tr("&Previous frame"), this);
    this->prevFrameAct->setShortcut(Qt::Key_Left);
    connect(this->prevFrameAct, SIGNAL(triggered()), this->annotateArea, SLOT(displayPrevFrame()));
    /*
    this->penColorAct = new QAction(tr("&Pen Color..."), this);
    connect(this->penColorAct, SIGNAL(triggered()), this, SLOT(penColor()));
    */

    this->penWidthAct = new QAction(tr("Pen &Width..."), this);
    connect(this->penWidthAct, SIGNAL(triggered()), this, SLOT(penWidth()));

    this->rubberModeAct = new QAction(tr("&Rubber Mode"), this);
    connect(this->rubberModeAct, SIGNAL(triggered()), this->annotateArea, SLOT(switchRubberMode()));
    this->rubberModeAct->setCheckable(true);
    this->rubberModeAct->setShortcut(Qt::Key_Delete);

    this->clearScreenAct = new QAction(tr("&Clear Screen"), this);
    this->clearScreenAct->setShortcut(tr("Ctrl+L"));
    connect(this->clearScreenAct, SIGNAL(triggered()),
            this->annotateArea, SLOT(clearImage()));

    this->aboutAct = new QAction(tr("&About"), this);
    connect(this->aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    /*
    this->aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(this->aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    */
}

void MainWindow::createMenus()
{
    this->fileMenu = new QMenu(tr("&File"), this);
    this->fileMenu->addAction(this->openImageAct);
    this->fileMenu->addAction(this->openVideoAct);
    this->fileMenu->addAction(this->openAnnotationsAct);
    this->fileMenu->addSeparator();
    this->fileMenu->addAction(this->saveCurrentImageAct);
    this->fileMenu->addAction(this->saveAnnotationsAct);
    this->fileMenu->addSeparator();
    this->fileMenu->addAction(this->printAct);
    this->fileMenu->addSeparator();
    this->fileMenu->addAction(this->closeFileAct);
    this->fileMenu->addSeparator();
    this->fileMenu->addAction(this->exitAct);

    this->viewMenu = new QMenu(tr("&View"), this);
    this->viewMenu->addAction(this->nextFrameAct);
    this->viewMenu->addAction(this->prevFrameAct);

    this->optionMenu = new QMenu(tr("&Edition"), this);
    this->optionMenu->addAction(this->penWidthAct);
    this->optionMenu->addAction(this->rubberModeAct);
    this->optionMenu->addSeparator();
    this->optionMenu->addAction(this->clearScreenAct);
    this->optionMenu->addSeparator();
    this->optionMenu->addAction(this->configMenuAct);

    this->helpMenu = new QMenu(tr("&Help"), this);
    this->helpMenu->addAction(this->aboutAct);
    //this->helpMenu->addAction(this->aboutQtAct);

    menuBar()->addMenu(this->fileMenu);
    menuBar()->addMenu(this->viewMenu);
    menuBar()->addMenu(this->optionMenu);
    menuBar()->addMenu(this->helpMenu);
}



bool MainWindow::maybeSave()
{
    if (this->annotateArea->isModified())
    {
       QMessageBox::StandardButton ret;
       ret = QMessageBox::warning(this, tr("Scribble"),
                          tr("The image has been modified.\n"
                             "Do you want to save your changes?"),
                          QMessageBox::Save | QMessageBox::Discard
                          | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
        {
            return saveFile("png");
        }
        else if (ret == QMessageBox::Cancel)
        {
            return false;
        }
    }
    return true;
}



bool MainWindow::saveFile(const QByteArray &fileFormat)
{
    QString initialPath = QDir::currentPath() + "/untitled." + fileFormat;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                               initialPath,
                               tr("%1 Files (*.%2);;All Files (*)")
                               .arg(QString::fromLatin1(fileFormat.toUpper()))
                               .arg(QString::fromLatin1(fileFormat)));
    if (fileName.isEmpty())
    {
        return false;
    }
    else
    {
        return this->annotateArea->saveImage(fileName, fileFormat.constData());
    }
}

/*
void MainWindow::checkSave()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open File"), QDir::currentPath());
        if (!fileName.isEmpty())
            this->annotateArea->openImage(fileName);
    }
}
*/
