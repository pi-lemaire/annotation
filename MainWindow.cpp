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
    this->SPAnnotate = new SuperPixelsAnnotate(this->annotations);
    this->OFTracking = new OptFlowTracking(this->annotations);


    // generate the interface, give the right pointer
    this->annotateArea = new AnnotateArea(this->annotations, this->SPAnnotate, this->OFTracking);

    // embed it into a ScrollArea, so that it can handle big images
    this->annotateScrollArea = new QScrollArea;

    // specify that the widget size can change...
    this->annotateScrollArea->setWidgetResizable(true);
    // this->annotateScrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    this->annotateScrollArea->setWidget(this->annotateArea);



    // this is for window-related stuff
    setCentralWidget(this->annotateScrollArea);



    // generate the object little window that will handle the class selection
    this->classSelection = new DialogClassSelection(this->annotations, this, Qt::Tool);
    this->classSelection->show();
    this->classSelection->setWindowTitle(tr("Class Selection"));
    this->classSelection->move(100, 100);

    // generate the browser, provide the right pointer
    this->annotsBrowser = new AnnotationsBrowser(this->annotations, this, Qt::Tool);
    this->annotsBrowser->resize(350, 550);
    this->annotsBrowser->show();
    this->annotsBrowser->setWindowTitle(tr("Objects Selection"));
    this->annotsBrowser->move(100, 300);


    // create the menu actions etc
    this->createActions();
    this->createMenus();

    this->updateActionsAvailability();



    // connect the class selection and the annotate area
    QObject::connect(this->classSelection, SIGNAL(classSelected(int)), this->annotateArea, SLOT(selectClassId(int)));
    QObject::connect(this->classSelection, SIGNAL(classSelected(int)), this->annotsBrowser, SLOT(setClassSelected(int)));

    QObject::connect(this->annotsBrowser, SIGNAL(annotationSelected(int)), this, SLOT(selectAnnot(int)));
    QObject::connect(this->annotsBrowser, SIGNAL(changesCausedByTheBrowser()), this->annotateArea, SLOT(contentModified()));

    QObject::connect(this->annotateArea, SIGNAL(selectedObject(int)), this->annotsBrowser, SLOT(updateBrowser(int)));
    QObject::connect(this->annotateArea, SIGNAL(updateSignal()), this, SLOT(updateActionsAvailability()));



    setWindowTitle(tr("Annotate"));
    resize(800, 600);
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
    if ((id!=-1) && (this->annotations->getRecord().getAnnotationById(id).FrameNumber != this->annotations->getCurrentFramePosition()))
        if (!this->annotateArea->displayFrame(this->annotations->getRecord().getAnnotationById(id).FrameNumber))
            return;

    this->annotateArea->selectAnnotation(id);
}

void MainWindow::updateActionsAvailability()
{
    this->loadClassesConfigAct->setEnabled(!(this->annotations->isImageOpen() || this->annotations->isVideoOpen()));
    this->nextFrameAct->setEnabled(this->annotations->canReadNextFrame());
    this->prevFrameAct->setEnabled(this->annotations->canReadPrevFrame());
    this->closeFileAct->setEnabled(this->annotations->isImageOpen() || this->annotations->isVideoOpen());
    switch (this->annotateArea->getPenStyle())
    {
        case AA_CS_round: this->penStyleRoundAct->setChecked(true); break;
        case AA_CS_square: this->penStyleSquareAct->setChecked(true); break;
    }
}




void MainWindow::imageSizeAdjustedByFactor(float factor)
{
    /*
    int hScrollValue = this->annotateScrollArea->horizontalScrollBar()->value();
    int vScrollValue = this->annotateScrollArea->verticalScrollBar()->value();
    */
    //int prevHMiddlePos = this->annotateArea->size().width() / (factor * 2.);
    //int prevVMiddlePos = this->annotateArea->size().height() / (factor * 2.);

    // the theoretical position of the scroll area center within the image
    int prevHMiddlePos = this->knownHScrollValue + (this->annotateScrollArea->size().width() / 2);
    int prevVMiddlePos = this->knownVScrollValue + (this->annotateScrollArea->size().height() / 2);

    // the new position of the center
    int newHMiddlePos = factor * prevHMiddlePos;
    int newVMiddlePos = factor * prevVMiddlePos;

    // set the scrollbars to the position of the top left corner according to this theoretical center pos :
    this->annotateScrollArea->horizontalScrollBar()->setValue(newHMiddlePos - (this->annotateScrollArea->size().width() / 2));
    this->annotateScrollArea->verticalScrollBar()->setValue(newVMiddlePos - (this->annotateScrollArea->size().height() / 2));

    /*
    QSize newWidgetSizeValue = this->annotateArea->size();
    QSize scrollAreaSizeValue = this->annotateScrollArea->size();



    qDebug() << "resize called : factor=" << factor << " ; hscroll=" << hScrollValue << " ; vscroll=" << vScrollValue << " ; newsize=" << newWidgetSizeValue << " ; scrollSize=" << scrollAreaSizeValue;
    */
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

            this->resetClassSelection();
        }
    }
}


void MainWindow::jumpToLast()
{
    if (this->annotations->isImageOpen() || this->annotations->isVideoOpen())
    {
        int maxFrame = this->annotations->getRecord().getRecordedFramesNumber() - 1;
        this->annotateArea->displayFrame(maxFrame);
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



void MainWindow::loadConfiguration()
{
    // TO BE COMPLETED
    QString fileName = QFileDialog::getOpenFileName(this,
                               tr("Open Annotations or Configuration File"), QDir::currentPath(), tr("XML/YAML/JSON file (*.xml *.yaml *.json)"));
    if (!fileName.isEmpty())
    {
        if (this->annotations->loadConfiguration(fileName.toStdString()))
        {
            // don't be shy - just kill the thing and load it back
            this->resetClassSelection();
        }
    }

}

void MainWindow::resetClassSelection()
{
    delete this->classSelection;

    this->classSelection = new DialogClassSelection(this->annotations, this, Qt::Tool);
    this->classSelection->show();
    this->classSelection->setWindowTitle(tr("Class Selection"));
    this->classSelection->move(100, 100);

    QObject::connect(this->classSelection, SIGNAL(classSelected(int)), this->annotateArea, SLOT(selectClassId(int)));
    QObject::connect(this->classSelection, SIGNAL(classSelected(int)), this->annotsBrowser, SLOT(setClassSelected(int)));

    this->annotateArea->selectClassId(1);
}


void MainWindow::configureSuperPixels()
{
    ParamsQEditorWindow *configWindow = new ParamsQEditorWindow(this->SPAnnotate);
    configWindow->show();
}


void MainWindow::configureOFTracking()
{
    ParamsQEditorWindow *configWindow = new ParamsQEditorWindow(this->OFTracking);
    configWindow->show();
}


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
    float prevScale = this->annotateArea->getScale();
    if (prevScale>=1.)
    {
        mulFactor = 2.; maxZoom = 16.;
    }
    else
    {
        mulFactor = 1.5; maxZoom = 1.;
    }

    float newScale = QtCvUtils::getMin(this->annotateArea->getScale()*mulFactor, maxZoom);

    if (newScale == prevScale)
        return;

    // we store the H scroll values and the V scroll values for smoother scrollarea behaviour
    // (changing the size of the object may affect them)
    this->knownHScrollValue = this->annotateScrollArea->horizontalScrollBar()->value();
    this->knownVScrollValue = this->annotateScrollArea->verticalScrollBar()->value();

    this->annotateArea->setScale(newScale);

    this->imageSizeAdjustedByFactor(newScale / prevScale);
}



void MainWindow::decreaseZoom()
{
    float divFactor, minZoom;
    float prevScale = this->annotateArea->getScale();
    if (prevScale>1.)
    {
        divFactor = 2.; minZoom = 1.;
    }
    else
    {
        divFactor = 1.5; minZoom = 0.1;
    }

    float newScale = QtCvUtils::getMax(this->annotateArea->getScale()/divFactor, minZoom);

    if (newScale == prevScale)
        return;

    // we store the H scroll values and the V scroll values for smoother scrollarea behaviour
    // (changing the size of the object may affect them)
    this->knownHScrollValue = this->annotateScrollArea->horizontalScrollBar()->value();
    this->knownVScrollValue = this->annotateScrollArea->verticalScrollBar()->value();

    this->annotateArea->setScale(newScale);

    this->imageSizeAdjustedByFactor(newScale / prevScale);
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




    this->loadClassesConfigAct = new QAction(tr("&Load classes"), this);
    connect(this->loadClassesConfigAct, SIGNAL(triggered()), this, SLOT(loadConfiguration()));



    this->nextFrameAct = new QAction(tr("&Next frame"), this);
    connect(this->nextFrameAct, SIGNAL(triggered()), this->annotateArea, SLOT(displayNextFrame()));
    this->prevFrameAct = new QAction(tr("&Previous frame"), this);
    connect(this->prevFrameAct, SIGNAL(triggered()), this->annotateArea, SLOT(displayPrevFrame()));
    this->jumpToLastAnnotatedFrameAct = new QAction(tr("&Jump to Last annotated Frame"), this);
    connect(this->jumpToLastAnnotatedFrameAct, SIGNAL(triggered()), this, SLOT(jumpToLast()));



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


    this->clearScreenAct = new QAction(tr("&Clear Annotations"), this);
    //this->clearScreenAct->setShortcut(tr("Ctrl+L"));
    connect(this->clearScreenAct, SIGNAL(triggered()), this->annotateArea, SLOT(clearImageAnnotations()));


    this->checkSelectedAct = new QAction(tr("Check the Selected Annotation"), this);
    connect(this->checkSelectedAct, SIGNAL(triggered()), this->annotsBrowser, SLOT(checkSelected()));
    this->uncheckSelectedAct = new QAction(tr("Un-Check the Selected Annotation"), this);
    connect(this->uncheckSelectedAct, SIGNAL(triggered()), this->annotsBrowser, SLOT(uncheckSelected()));
    this->uncheckAllAct = new QAction(tr("Uncheck All annotations"), this);
    connect(this->uncheckAllAct, SIGNAL(triggered()), this->annotsBrowser, SLOT(uncheckAll()));
    this->deleteCheckedAct = new QAction(tr("Delete the Checked Annotation(s)"), this);
    connect(this->deleteCheckedAct, SIGNAL(triggered()), this->annotsBrowser, SLOT(DeleteAnnotationsClicked()));
    this->groupCheckedAct = new QAction(tr("Group the Checked Annotations"), this);
    connect(this->groupCheckedAct, SIGNAL(triggered()), this->annotsBrowser, SLOT(GroupAnnotationsClicked()));
    this->lockCheckedAct = new QAction(tr("Lock the checked annotation(s)"), this);
    connect(this->lockCheckedAct, SIGNAL(triggered()), this->annotsBrowser, SLOT(LockAnnotationsClicked()));
    this->unlockCheckedAct = new QAction(tr("Unlock the checked annotation(s)"), this);
    connect(this->unlockCheckedAct, SIGNAL(triggered()), this->annotsBrowser, SLOT(UnlockAnnotationsClicked()));


    this->configureSuperPixelsAct = new QAction(tr("SuperPixels Settings"), this);
    connect(this->configureSuperPixelsAct, SIGNAL(triggered()), this, SLOT(configureSuperPixels()));
    this->computeSuperPixelsAct = new QAction(tr("Compute the Super Pixels Map"), this);
    connect(this->computeSuperPixelsAct, SIGNAL(triggered()), this->annotateArea, SLOT(computeSuperPixelsMap()));
    this->expandSelectedToSuperPixelAct = new QAction(tr("Expand the Selected Annotation"), this);
    connect(this->expandSelectedToSuperPixelAct, SIGNAL(triggered()), this->annotateArea, SLOT(growAnnotationBySP()));


    this->configureOFTrackingAct = new QAction(tr("Optical Flow Tracking Settings"), this);
    connect(this->configureOFTrackingAct, SIGNAL(triggered()), this, SLOT(configureOFTracking()));
    this->OFTrackToNextFrameAct = new QAction(tr("Track to next frame using Optical Flow"), this);
    connect(this->OFTrackToNextFrameAct, SIGNAL(triggered()), this->annotateArea, SLOT(OFTrackToNextFrame()));
    this->OFTrackMultipleFramesAct = new QAction(tr("Track Multiples frames using Optical flow"), this);
    connect(this->OFTrackMultipleFramesAct, SIGNAL(triggered()), this->annotateArea, SLOT(OFTrackMultipleFrames()));
    this->interpolateLastBBsAct = new QAction(tr("Interpolate the last frames on Bounding Boxes"), this);
    connect(this->interpolateLastBBsAct, SIGNAL(triggered()), this->annotateArea, SLOT(interpolateBBObjects()));



    this->nextFrameAct->setShortcut(Qt::Key_Right);
    this->prevFrameAct->setShortcut(Qt::Key_Left);

    this->increasePenWidthAct->setShortcut(Qt::Key_Up);
    this->decreasePenWidthAct->setShortcut(Qt::Key_Down);
    this->rubberModeAct->setShortcut(Qt::Key_Backspace);

    this->scaleToOneAct->setShortcut(Qt::Key_0);
    this->increaseScaleAct->setShortcut(Qt::Key_Plus);
    this->decreaseScaleAct->setShortcut(Qt::Key_Minus);

    this->checkSelectedAct->setShortcut(Qt::Key_C);
    this->uncheckSelectedAct->setShortcut(Qt::Key_U);
    this->uncheckAllAct->setShortcut(Qt::ALT + Qt::Key_U);
    this->deleteCheckedAct->setShortcut(Qt::ALT + Qt::Key_D);
    this->groupCheckedAct->setShortcut(Qt::ALT + Qt::Key_G);
    this->lockCheckedAct->setShortcut(Qt::ALT + Qt::Key_L);
    this->unlockCheckedAct->setShortcut(Qt::ALT + Qt::Key_U);

    this->computeSuperPixelsAct->setShortcut(Qt::ALT + Qt::Key_P);
    this->expandSelectedToSuperPixelAct->setShortcut(Qt::SHIFT + Qt::Key_E);

    this->OFTrackToNextFrameAct->setShortcut(Qt::SHIFT + Qt::Key_T);
    this->OFTrackMultipleFramesAct->setShortcut(Qt::SHIFT + Qt::Key_Y);
    this->interpolateLastBBsAct->setShortcut(Qt::SHIFT + Qt::Key_I);


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
    this->viewMenu->addAction(this->jumpToLastAnnotatedFrameAct);
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
    this->optionMenu->addAction(this->checkSelectedAct);
    this->optionMenu->addAction(this->uncheckSelectedAct);
    this->optionMenu->addAction(this->uncheckAllAct);
    this->optionMenu->addAction(this->deleteCheckedAct);
    this->optionMenu->addAction(this->groupCheckedAct);
    this->optionMenu->addAction(this->lockCheckedAct);
    this->optionMenu->addAction(this->unlockCheckedAct);
    this->optionMenu->addSeparator();


    this->imageProcessingMenu = new QMenu(tr("&Image Processing"), this);
    this->imageProcessingMenu->addAction(this->computeSuperPixelsAct);
    this->imageProcessingMenu->addAction(this->expandSelectedToSuperPixelAct);
    this->imageProcessingMenu->addSeparator();
    this->imageProcessingMenu->addAction(this->OFTrackToNextFrameAct);
    this->imageProcessingMenu->addAction(this->OFTrackMultipleFramesAct);
    this->imageProcessingMenu->addAction(this->interpolateLastBBsAct);



    // this->settingsMenu = this->optionMenu->addMenu(tr("&Settings..."));
    this->settingsMenu = new QMenu(tr("&Settings"), this);
    this->settingsMenu->addAction(this->loadClassesConfigAct);
    this->settingsMenu->addAction(this->configureSuperPixelsAct);
    this->settingsMenu->addAction(this->configureOFTrackingAct);




    this->helpMenu = new QMenu(tr("&Help"), this);
    this->helpMenu->addAction(this->aboutAct);
    //this->helpMenu->addAction(this->aboutQtAct);



    menuBar()->addMenu(this->fileMenu);
    menuBar()->addMenu(this->viewMenu);
    menuBar()->addMenu(this->optionMenu);
    menuBar()->addMenu(this->imageProcessingMenu);
    menuBar()->addMenu(this->settingsMenu);
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
