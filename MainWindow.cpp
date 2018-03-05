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
        if (!this->annotateArea->displayFrame(this->annotations->getRecord().getAnnotationById(id).FrameNumber))
            return;

    this->annotateArea->selectAnnotation(id);
}

void MainWindow::updateActionsAvailability()
{
    this->configMenuAct->setEnabled(!(this->annotations->isImageOpen() || this->annotations->isVideoOpen()));
    this->nextFrameAct->setEnabled(this->annotations->canReadNextFrame());
    this->prevFrameAct->setEnabled(this->annotations->canReadPrevFrame());
    this->closeFileAct->setEnabled(this->annotations->isImageOpen() || this->annotations->isVideoOpen());
    switch (this->annotateArea->getPenStyle())
    {
        case AA_CS_round: this->penStyleRoundAct->setChecked(true); break;
        case AA_CS_square: this->penStyleSquareAct->setChecked(true); break;
    }
}


void MainWindow::openImage()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open Image File"), QDir::currentPath());
        if (!fileName.isEmpty())
        {
            this->annotations->closeFile(false);    // don't save, it's supposed to have been done already
            this->annotateArea->openImage(fileName);
        }
    }
}


void MainWindow::openVideo()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open Video File"), QDir::currentPath());
        if (!fileName.isEmpty())
        {
            this->annotations->closeFile(false);    // don't save, it's supposed to have been done already
            this->annotateArea->openVideo(fileName);
        }
    }
}

void MainWindow::loadAnnotations()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open Annotations File"), QDir::currentPath(), tr("XML/YAML/JSON file (*.xml *.yaml *.json)"));
        if (!fileName.isEmpty())
        {
            this->annotations->closeFile(false);    // don't save, it's supposed to have been done already
            this->annotateArea->openAnnotations(fileName);
        }
    }
}

void MainWindow::saveAnnotationsAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Annotations File"),
                                                    QDir(QString::fromStdString(this->annotations->getDefaultAnnotationsSaveFileName())).absolutePath(),
                                                    tr("XML/YAML/JSON file (*.xml *.yaml *.json)"));
    if (!fileName.isEmpty())
    {
        this->annotations->saveCurrentState(fileName.toStdString());

        /*
        this->annotations->closeFile(false);    // don't save, it's supposed to have been done already
        this->annotateArea->openVideo(fileName);
        */
    }
}


void MainWindow::save()
{
    this->annotations->saveCurrentState();
}


void MainWindow::saveCurrentImage()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Current Annotation Image"),
                                                    QDir(QString::fromStdString(this->annotations->getDefaultCurrentImageSaveFileName())).absolutePath(),
                                                    tr("PNG file (*.png)"));
    if (!fileName.isEmpty())
        this->annotations->saveCurrentAnnotationImage(fileName.toStdString());
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

void MainWindow::setPenWidth()
{
    bool ok;
    int newWidth = QInputDialog::getInt(this, tr("Scribble"),
                                        tr("Select pen width:"),
                                        this->annotateArea->penWidth(),
                                        1, 62, 1, &ok);
    if (ok)
        this->annotateArea->setPenWidth(newWidth);
}

void MainWindow::increasePenWidth()
{
    int incr = 1;
    if (this->annotateArea->getScale()>1.)
        incr = (int)this->annotateArea->getScale();
    this->annotateArea->setPenWidth(QtCvUtils::getMin(this->annotateArea->penWidth()+incr, 62));
}

void MainWindow::decreasePenWidth()
{
    int decr = 1;
    if (this->annotateArea->getScale()>1.)
        decr = (int)this->annotateArea->getScale();
    this->annotateArea->setPenWidth(QtCvUtils::getMax(this->annotateArea->penWidth()-decr, 1));
}

void MainWindow::switchPenStyle()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == this->penStyleRoundAct)
        this->annotateArea->setPenStyle(AA_CS_round);
    else if (action == this->penStyleSquareAct)
        this->annotateArea->setPenStyle(AA_CS_square);
}


void MainWindow::setZoomToOne()
{
    this->annotateArea->setScale(1.);
}

void MainWindow::increaseZoom()
{
    float mulFactor, maxZoom;
    if (this->annotateArea->getScale()>=1.)
    {
        mulFactor = 2.; maxZoom = 16.;
    }
    else
    {
        mulFactor = 1.5; maxZoom = 1.;
    }

    this->annotateArea->setScale(QtCvUtils::getMin(this->annotateArea->getScale()*mulFactor, maxZoom));
}

void MainWindow::decreaseZoom()
{
    float divFactor, minZoom;
    if (this->annotateArea->getScale()>1.)
    {
        divFactor = 2.; minZoom = 1.;
    }
    else
    {
        divFactor = 1.5; minZoom = 0.1;
    }
    this->annotateArea->setScale(QtCvUtils::getMax(this->annotateArea->getScale()/divFactor, minZoom));
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
    connect(this->openImageAct, SIGNAL(triggered()), this, SLOT(openImage()));

    this->openVideoAct = new QAction(tr("Open &Video..."), this);
    connect(this->openVideoAct, SIGNAL(triggered()), this, SLOT(openVideo()));

    this->openAnnotationsAct = new QAction(tr("Open &Annotations..."), this);
    connect(this->openAnnotationsAct, SIGNAL(triggered()), this, SLOT(loadAnnotations()));

    this->openAnnotationsAct->setShortcuts(QKeySequence::Open);





    this->saveAct = new QAction(tr("&Save State"), this);
    connect(this->saveAct, SIGNAL(triggered()), this, SLOT(save()));

    this->saveAnnotationsAct = new QAction(tr("&Save Annotations As"), this);
    connect(this->saveAnnotationsAct, SIGNAL(triggered()), this, SLOT(saveAnnotationsAs()));

    this->saveCurrentImageAct = new QAction(tr("Save &Current Annotation (single frame)"), this);
    connect(this->saveCurrentImageAct, SIGNAL(triggered()), this, SLOT(saveCurrentImage()));

    this->printAct = new QAction(tr("&Print..."), this);
    connect(this->printAct, SIGNAL(triggered()), this->annotateArea, SLOT(print()));

    this->saveAct->setShortcuts(QKeySequence::Save);
    this->saveAnnotationsAct->setShortcuts(QKeySequence::SaveAs);
    this->printAct->setShortcuts(QKeySequence::Print);



    this->closeFileAct = new QAction(tr("&Close"), this);
    connect(this->closeFileAct, SIGNAL(triggered()), this, SLOT(closeFile()));


    this->exitAct = new QAction(tr("E&xit"), this);
    connect(this->exitAct, SIGNAL(triggered()), this, SLOT(close()));

    this->closeFileAct->setShortcuts(QKeySequence::Close);
    this->exitAct->setShortcuts(QKeySequence::Quit);




    this->configMenuAct = new QAction(tr("&Configure the classes"), this);



    this->nextFrameAct = new QAction(tr("&Next frame"), this);
    connect(this->nextFrameAct, SIGNAL(triggered()), this->annotateArea, SLOT(displayNextFrame()));
    this->prevFrameAct = new QAction(tr("&Previous frame"), this);
    connect(this->prevFrameAct, SIGNAL(triggered()), this->annotateArea, SLOT(displayPrevFrame()));



    this->penWidthAct = new QAction(tr("Pen &Width..."), this);
    connect(this->penWidthAct, SIGNAL(triggered()), this, SLOT(setPenWidth()));
    this->increasePenWidthAct = new QAction(tr("&Increase Pen Width..."), this);
    connect(this->increasePenWidthAct, SIGNAL(triggered()), this, SLOT(increasePenWidth()));
    this->decreasePenWidthAct = new QAction(tr("&Decrease Pen Width..."), this);
    connect(this->decreasePenWidthAct, SIGNAL(triggered()), this, SLOT(decreasePenWidth()));

    this->penStyleActGroup = new QActionGroup(this);
    this->penStyleRoundAct = new QAction(tr("Use a &Round Pen"), this);
    this->penStyleRoundAct->setCheckable(true);
    this->penStyleActGroup->addAction(this->penStyleRoundAct);
    connect(this->penStyleRoundAct, SIGNAL(triggered()), this, SLOT(switchPenStyle()));
    this->penStyleSquareAct = new QAction(tr("Use a Square Pen"), this);
    this->penStyleSquareAct->setCheckable(true);
    this->penStyleActGroup->addAction(this->penStyleSquareAct);
    connect(this->penStyleSquareAct, SIGNAL(triggered()), this, SLOT(switchPenStyle()));
    this->penStyleActGroup->setEnabled(true);


    this->scaleToOneAct = new QAction(tr("Set Zoom to &1:1"), this);
    connect(this->scaleToOneAct, SIGNAL(triggered()), this, SLOT(setZoomToOne()));
    this->increaseScaleAct = new QAction(tr("&Zoom in..."), this);
    connect(this->increaseScaleAct, SIGNAL(triggered()), this, SLOT(increaseZoom()));
    this->decreaseScaleAct = new QAction(tr("Zoom &back..."), this);
    connect(this->decreaseScaleAct, SIGNAL(triggered()), this, SLOT(decreaseZoom()));




    this->rubberModeAct = new QAction(tr("&Rubber Mode"), this);
    connect(this->rubberModeAct, SIGNAL(triggered()), this->annotateArea, SLOT(switchRubberMode()));
    this->rubberModeAct->setCheckable(true);


    this->clearScreenAct = new QAction(tr("&Clear Screen"), this);
    this->clearScreenAct->setShortcut(tr("Ctrl+L"));
    connect(this->clearScreenAct, SIGNAL(triggered()),
            this->annotateArea, SLOT(clearImage()));



    this->nextFrameAct->setShortcut(Qt::Key_Right);
    this->prevFrameAct->setShortcut(Qt::Key_Left);
    this->increasePenWidthAct->setShortcut(Qt::Key_Up);
    this->decreasePenWidthAct->setShortcut(Qt::Key_Down);
    this->scaleToOneAct->setShortcut(Qt::Key_0);
    this->increaseScaleAct->setShortcut(Qt::Key_Plus);
    this->decreaseScaleAct->setShortcut(Qt::Key_Minus);
    this->rubberModeAct->setShortcut(Qt::Key_Delete);





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
    this->fileMenu->addAction(this->saveAct);
    this->fileMenu->addAction(this->saveAnnotationsAct);
    this->fileMenu->addAction(this->saveCurrentImageAct);
    this->fileMenu->addSeparator();
    this->fileMenu->addAction(this->printAct);
    this->fileMenu->addSeparator();
    this->fileMenu->addAction(this->closeFileAct);
    this->fileMenu->addSeparator();
    this->fileMenu->addAction(this->exitAct);

    this->viewMenu = new QMenu(tr("&View"), this);
    this->viewMenu->addAction(this->nextFrameAct);
    this->viewMenu->addAction(this->prevFrameAct);
    this->viewMenu->addSeparator();
    this->viewMenu->addAction(this->scaleToOneAct);
    this->viewMenu->addAction(this->increaseScaleAct);
    this->viewMenu->addAction(this->decreaseScaleAct);


    this->optionMenu = new QMenu(tr("&Edition"), this);
    this->optionMenu->addAction(this->penWidthAct);
    this->optionMenu->addAction(this->increasePenWidthAct);
    this->optionMenu->addAction(this->decreasePenWidthAct);
    this->optionMenu->addAction(this->penStyleRoundAct);
    this->optionMenu->addAction(this->penStyleSquareAct);
    this->optionMenu->addSeparator();
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
            // return saveFile("png");
            return this->annotations->saveCurrentState();
        }
        else if (ret == QMessageBox::Cancel)
        {
            return false;
        }
    }
    return true;
}


/*
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
        return this->annotateArea->saveImage(fileName);
    }
}
*/

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
