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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QScrollArea>
#include <QActionGroup>


#include "AnnotationsSet.h"
#include "AnnotateArea.h"
#include "DialogClassSelection.h"
#include "AnnotationsBrowser.h"
#include "ParamsQEditorWindow.h"

#include "SuperPixelsAnnotate.h"
#include "OptFlowTracking.h"





class AnnotateArea;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

public slots:
    void selectAnnot(int);
    void updateActionsAvailability();



private slots:
    void openImage();
    void openVideo();
    void loadAnnotations();
    void saveAnnotationsAs();
    void save();
    void saveCurrentImage();
    void closeFile();

    void loadConfiguration();
    void configureSuperPixels();
    void configureOFTracking();

    void setPenWidth();
    void increasePenWidth();
    void decreasePenWidth();

    void setZoomToOne();
    void increaseZoom();
    void decreaseZoom();

    void switchPenStyle();



    void about();

private:
    void createActions();
    void createMenus();
    bool maybeSave();
    bool saveFile(const QByteArray &fileFormat);
    void resetClassSelection();

    void imageSizeAdjustedByFactor(float);


    AnnotateArea *annotateArea;
    QScrollArea *annotateScrollArea;

    int knownHScrollValue, knownVScrollValue;

    DialogClassSelection *classSelection;
    AnnotationsBrowser *annotsBrowser;



    AnnotationsSet *annotations;
    SuperPixelsAnnotate *SPAnnotate;
    OptFlowTracking *OFTracking;






    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *optionMenu;
    QMenu *imageProcessingMenu;
    QMenu *helpMenu;
    QMenu *settingsMenu;




    QAction *openImageAct;
    QAction *openVideoAct;
    QAction *openAnnotationsAct;
    QAction *closeFileAct;

    QAction *loadClassesConfigAct;

    QAction *saveAct;
    QAction *saveAnnotationsAct;
    QAction *saveCurrentImageAct;
    QAction *exitAct;

    QAction *nextFrameAct;
    QAction *prevFrameAct;

    // pen related actions
    QAction *penWidthAct, *increasePenWidthAct, *decreasePenWidthAct, *penStyleRoundAct, *penStyleSquareAct;
    QActionGroup *penStyleActGroup;
    QAction *rubberModeAct;

    // zoom related actions
    QAction *scaleToOneAct, *increaseScaleAct, *decreaseScaleAct;

    // browser and selection related actionss
    QAction *checkSelectedAct, *uncheckSelectedAct, *uncheckAllAct;


    // superpixels related stuff
    QAction *configureSuperPixelsAct, *computeSuperPixelsAct, *expandSelectedToSuperPixelAct;

    // optical flow tracking related stuff
    QAction *configureOFTrackingAct, *OFTrackToNextFrameAct;



    QAction *printAct;
    QAction *clearScreenAct;
    QAction *aboutAct;
    //QAction *aboutQtAct;




};

#endif
