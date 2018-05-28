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


#ifndef AnnotateArea_H
#define AnnotateArea_H

#include <QColor>
#include <QBitmap>
#include <QImage>
#include <QPoint>
#include <QWidget>
#include <QDebug>



#include "AnnotationsSet.h"

#include "SuperPixelsAnnotate.h"
#include "OptFlowTracking.h"

#include <opencv2/opencv.hpp>

#include "QtCvUtils.h"






enum cursorShape { AA_CS_round, AA_CS_square };

// enum BBCornerSelected { _BBEM_TopLeft, _BBEM_Top, _BBEM_TopRight, _BBEM_Right, _BBEM_BottomRight, _BBEM_Bottom, _BBEM_BottomLeft, _BBEM_Left };

enum BBEditionStyle { _BBES_Corner, _BBES_Vertical, _BBES_Horizontal };


const int _AnnotateArea_BBSelection_GlueDist = 5;


const unsigned int _AA_CI_NoC = 0;      // no contours color index
const unsigned int _AA_CI_NotSelC = 1;  // non-selected contour color index
const unsigned int _AA_CI_SelC = 2;     // selected contour color index


class AnnotateArea : public QWidget
{
    Q_OBJECT

public:
    AnnotateArea(AnnotationsSet* annotsSet, SuperPixelsAnnotate* SPAnnotate, OptFlowTracking* OFTrack, QWidget *parent = 0);

    bool openImage(const QString &fileName);
    bool openVideo(const QString &fileName);
    bool openAnnotations(const QString &fileName);
    bool saveImage(const QString &fileName);
    // void setPenColor(const QColor &newColor);


    //void displayNextFrame();
    bool displayFrame(int id);
    void reload();


    bool isModified() const { return this->annotations->thereWereChangesPerformedUponCurrentAnnot(); }

    QColor penColor() const { return myPenColor; }

    int penWidth() const { return myPenWidth; }
    void setPenWidth(int newWidth);
    cursorShape getPenStyle() const { return this->myPenShape; }
    void setPenStyle(cursorShape sh) { this->myPenShape = sh; this->setPenWidth(this->penWidth()); }

    float getScale() const { return this->scaleFactor; }
    //void setScale(float newScale) { this->scaleFactor = newScale; this->setFixedSize(this->BackgroundImage.size() * newScale); this->update(); }
    void setScale(float newScale);


public slots:
    void displayNextFrame();
    void displayPrevFrame();

    void contentModified(QRect);

    void clearImageAnnotations();
    void print();
    void switchRubberMode();
    void selectClassId(int);
    void selectAnnotation(int);

    // image processing slots
    void computeSuperPixelsMap();
    void growAnnotationBySP();
    void clearSPMap();

    void OFTrackToNextFrame();
    void OFTrackMultipleFrames();
    void interpolateBBObjects();



signals:
    void selectedObject(int);
    void updateSignal();
    void sizeChangedByFactor(float);
    void newStatusBarMessage(const QString&);


protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    //void resizeEvent(QResizeEvent *event) override;

private:
    void updateStatusBar();

    void drawLineTo(const QPoint &endPoint);
    //void resizeImage(QImage *image, const QSize &newSize);

    //void updateContent();
    void updatePaintImages(const QRect& ROI=QRect(-3, -3, 0, 0), bool contoursOnly=false);


    QRect adaptToScaleMul(const QRect&) const;
    QPoint adaptToScaleDiv(const QPoint&) const;


    void setWidgetSize(const QSize&);

    void drawBoundingBoxes(const QRect& ROI=QRect(-3, -3, 0, 0));


    // bool modified;
    bool scribbling, rubberMode, BBClassOnly;
    // BBCornerSelected BBWhichCornerSelected;
    BBEditionStyle currentBBEditionStyle;
    QRect currentBBEditionFixedRect;

    int myPenWidth;
    QColor myPenColor;
    int selectedClass, selectedObjectId;

    cursorShape myPenShape;

    QImage PaintingImage, BackgroundImage, BoundingBoxesImage, ObjectImage, ContoursImage, SPContoursImage;
    // QRgb contourNone, contourOn, contourSelected;
    QRect ObjectROI;

    float scaleFactor;

    QBitmap CursorBitmap;
    QPoint lastPoint, firstAnnotPoint;

    AnnotationsSet *annotations;
    SuperPixelsAnnotate* SPAnnotate;
    OptFlowTracking* OFTracking;
    int framesComputedForInterpolation;
};

#endif
