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
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QPrinter>
#include <QPrintDialog>
#endif
#endif

#include "AnnotateArea.h"



// public region

AnnotateArea::AnnotateArea(AnnotationsSet* annotsSet, SuperPixelsAnnotate* SPAnnot, OptFlowTracking* OFTrack, QWidget *parent)
    : QWidget(parent)
{
    this->annotations = annotsSet;

    this->SPAnnotate = SPAnnot;

    this->OFTracking = OFTrack;

    this->framesComputedForInterpolation = 0;

    // ensuring that the widget is always referenced to the top left corner
    setAttribute(Qt::WA_StaticContents);

    // initialization stuff...
    //this->modified = false;
    this->scribbling = false;
    this->rubberMode = false;
    this->BBClassOnly = false;

    this->scaleFactor = 1.0;

    this->myPenShape = AA_CS_round;

    this->setPenWidth(10);

    // selecting the first available class
    this->selectClassId(1);
    this->selectedObjectId = -1;
}



bool AnnotateArea::openImage(const QString &fileName)
{
    // load the annotation into the system
    if (!this->annotations->loadOriginalImage(fileName.toStdString()))
        return false;

    this->reload();

    this->updatePaintImages();

    this->selectAnnotation(-1);

    return true;
}


bool AnnotateArea::openVideo(const QString &fileName)
{
    // load the annotation into the system
    if (!this->annotations->loadOriginalVideo(fileName.toStdString()))
        return false;

    this->reload();

    this->updatePaintImages();

    this->selectAnnotation(-1);

    return true;
}


bool AnnotateArea::openAnnotations(const QString &fileName)
{
    // first close the current action
    // this->annotations->closeFile();

    // load the annotation into the system
    if (!this->annotations->loadAnnotations(fileName.toStdString()))
        return false;

    this->reload();

    this->updatePaintImages();

    this->selectAnnotation(-1);

    return true;
}


bool AnnotateArea::saveImage(const QString &fileName)
{
    /*
    QImage visibleImage = this->BackgroundImage;
    // resizeImage(&visibleImage, size());

    if (visibleImage.save(fileName, fileFormat))
    {
        this->modified = false;
        return true;
    }
    else
    {
        return false;
    }
    */
    return this->annotations->saveCurrentAnnotationImage(fileName.toStdString().c_str());
}


void AnnotateArea::setPenWidth(int newWidth)
{
    // set the pen width and generate a nice fancy cursor
    this->myPenWidth = newWidth;

    // check that it's compatible with the scale factor thing
    if (this->scaleFactor>1.)
    {
        int scaleFactInt = this->scaleFactor;

        // make it a multiple of scaleFactor
        this->myPenWidth = (newWidth / scaleFactInt) * scaleFactInt;

        // check that we're editing at least one pixel
        if (this->myPenWidth < this->scaleFactor)
            this->myPenWidth = this->scaleFactor;
    }


    // 64,64 is seemingly the max possible size. I'm not sure whether it's supported on all systems
    this->CursorBitmap = QBitmap(64,64);
    // draw with blank values, then paint it
    this->CursorBitmap.fill();

    QBitmap originalBitMapCopy = this->CursorBitmap.copy();

    QPainter cursorPainter(&(this->CursorBitmap));
    cursorPainter.setPen(Qt::black);

    // 32, 32 is supposed to be the middle of the area?
    if (this->myPenShape == AA_CS_round)
        cursorPainter.drawEllipse(QPointF(32,32), this->myPenWidth/2., this->myPenWidth/2.);
    else
        cursorPainter.drawRect(32-this->myPenWidth/2., 32-this->myPenWidth/2., this->myPenWidth, this->myPenWidth);
    cursorPainter.drawPoint(32,32);
    cursorPainter.end();

    if (this->rubberMode)
        this->setCursor(QCursor(originalBitMapCopy, this->CursorBitmap, 32, 32));
        // use this as a cursor
    else
        this->setCursor(QCursor(this->CursorBitmap, this->CursorBitmap, 32, 32));
}


void AnnotateArea::setScale(float newScale)
{
    this->scaleFactor = newScale;
    this->setWidgetSize(this->BackgroundImage.size() * newScale);

    if (newScale>1.)
    {
        // maybe we'll need to adapt the cursor size
        this->setPenWidth(this->myPenWidth);
    }

    this->update();
}



void AnnotateArea::reload()
{
    // convert the original image as background image
    this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

    // format the other layers correctly
    this->PaintingImage = QImage(this->BackgroundImage.size(), QImage::Format_ARGB32);
    this->PaintingImage.fill(qRgba(255, 255, 255, 0));

    this->ContoursImage = QImage(this->BackgroundImage.size(), QImage::Format_Indexed8);
    this->ContoursImage.setColor(_AA_CI_NoC, qRgba(255, 255, 255, 0));
    this->ContoursImage.setColor(_AA_CI_NotSelC, qRgba(0, 0, 0, 255));
    this->ContoursImage.setColor(_AA_CI_SelC, qRgba(255, 255, 255, 255));
    this->ContoursImage.fill(_AA_CI_NoC);

    this->SPContoursImage = QImage(this->BackgroundImage.size(), QImage::Format_Indexed8);
    this->SPContoursImage.setColor(_AA_CI_NoC, qRgba(255, 255, 255, 0));
    this->SPContoursImage.setColor(_AA_CI_NotSelC, qRgba(255, 255, 0, 255));
    // this->ContoursImage.setColor(_AA_CI_SelC, qRgba(255, 255, 255, 255));
    this->SPContoursImage.fill(_AA_CI_NoC);

    this->ObjectImage = QImage(this->BackgroundImage.size(), QImage::Format_Mono);


    // set the widget to the right size - the QScrollArea should do the rest
    this->setWidgetSize(this->BackgroundImage.size() * this->scaleFactor);

    this->selectedObjectId = -1;
    // this->selectedClass = 0;

    // prepare for the annotation stuff, and display the result
    //this->modified = false;
    update();
}




void AnnotateArea::setWidgetSize(const QSize& newSize)
{
    this->setFixedSize(newSize);
    this->BoundingBoxesImage = QImage(newSize, QImage::Format_ARGB32);
    this->BoundingBoxesImage.fill(qRgba(255, 255, 255, 0));
}



// public slots region

void AnnotateArea::displayNextFrame()
{
    // store data about the previously selected object
    // if none was selected, the record should return some impossible filled object?
    AnnotationObject previouslySelectedObj = this->annotations->getRecord().getAnnotationById(this->selectedObjectId);

    if (this->annotations->loadNextFrame())
    {
        // preselect the same object than what was previously selected
        this->selectedObjectId = this->annotations->getRecord().searchAnnotation( this->annotations->getCurrentFramePosition(),
                                                                                  previouslySelectedObj.ClassId,
                                                                                  previouslySelectedObj.ObjectId );
        // if no object corresponds, it will be -1 - exactly what we need

        this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

        this->updatePaintImages();

        this->selectAnnotation(this->selectedObjectId);

        // nothing should have changed, so we don't need to update PaintingImage or ObjectImage

        update();
    }
}


void AnnotateArea::displayPrevFrame()
{
    this->displayFrame( this->annotations->getCurrentFramePosition()-1 );
}


void AnnotateArea::contentModified()
{
    this->updatePaintImages();
    // this->selectAnnotation(this->selectedObjectId);
}


bool AnnotateArea::displayFrame(int id)
{
    // store data about the previously selected object
    // if none was selected, the record should return some impossible filled object?
    AnnotationObject previouslySelectedObj = this->annotations->getRecord().getAnnotationById(this->selectedObjectId);

    if (this->annotations->loadFrame(id))
    {

        // preselect the same object than what was previously selected
        this->selectedObjectId = this->annotations->getRecord().searchAnnotation( this->annotations->getCurrentFramePosition(),
                                                                                  previouslySelectedObj.ClassId,
                                                                                  previouslySelectedObj.ObjectId );

        this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

        this->updatePaintImages();

        this->selectAnnotation(this->selectedObjectId);

        // nothing should have changed, so we don't need to update PaintingImage or ObjectImage

        update();

        return true;
    }

    return false;
}



void AnnotateArea::clearImageAnnotations()
{
    this->PaintingImage.fill(qRgba(255, 255, 255, 0));
    this->ContoursImage.fill(_AA_CI_NoC);
    //this->SPContoursImage.fill(_AA_CI_NoC);

    this->annotations->clearCurrentFrame();

    this->selectAnnotation(-1);

    this->update();
}


void AnnotateArea::print()
{
#if QT_CONFIG(printdialog)
    QPrinter printer(QPrinter::HighResolution);

    /*
    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        QSize size = image.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(image.rect());
        painter.drawImage(0, 0, image);
    }
    */
#endif // QT_CONFIG(printdialog)
}





/*
void AnnotateArea::updateContent()
{
    VisibleImage = BackgroundImage.copy();

    QPainter p(&VisibleImage);
    p.setOpacity(0.5);
    p.drawImage(0,0,PaintingImage);
    p.end();

    update();
}
*/

/*
void AnnotateArea::setPenColor(const QColor &newColor)
{
    this->myPenColor = newColor;
}
*/


void AnnotateArea::switchRubberMode()
{
    this->rubberMode = !this->rubberMode;
    this->setPenWidth(this->myPenWidth);
}


void AnnotateArea::selectClassId(int classId)
{
    this->selectedClass = classId;
    // cv::Vec3b classColor = this->annotations->getConfig().getProperty(classId).displayRGBColor;
    this->myPenColor = QtCvUtils::cvVec3bToQColor( this->annotations->getConfig().getProperty(classId).displayRGBColor );

    this->BBClassOnly = (this->annotations->getConfig().getProperty(classId).classType == _ACT_BoundingBoxOnly);
}


void AnnotateArea::selectAnnotation(int annotId)
{
    this->selectedObjectId = annotId;

    // try to update only what's necessary

    // start with the old objectROI
    QRect updateRoi = this->ObjectROI;
    if (annotId != -1)
    {
        QRect newROI = QRect(QtCvUtils::cvRect2iToQRect( this->annotations->getRecord().getAnnotationById(annotId).BoundingBox) );
        updateRoi |= newROI;
        this->ObjectROI = newROI;
    }

    // update the contour image
    this->updatePaintImages(updateRoi, true);

    // update the part containing both the old and the new ROI
    this->update( this->adaptToScaleMul(updateRoi.adjusted(-2,-2,2,2)) );

    emit selectedObject(annotId);
}




void AnnotateArea::computeSuperPixelsMap()
{
    this->SPAnnotate->buildSPMap();

    // force an update of the whole image
    QRect updateArea = QRect(QPoint(0,0), this->BackgroundImage.size());
    this->updatePaintImages(updateArea, true);
    this->update();
}



void AnnotateArea::growAnnotationBySP()
{
    if (this->selectedObjectId != -1)
    {
        this->SPAnnotate->expandAnnotation(this->selectedObjectId);

        QRect updateArea = QtCvUtils::cvRect2iToQRect(this->annotations->getRecord().getAnnotationById(this->selectedObjectId).BoundingBox);

        this->updatePaintImages(updateArea.adjusted(-1,-1,1,1));    // use adjusted to take into account possible contours modifications
        this->selectAnnotation(this->selectedObjectId);
    }
}




void AnnotateArea::OFTrackToNextFrame()
{
    // this is almost the same as displayNextFrame
    AnnotationObject previouslySelectedObj = this->annotations->getRecord().getAnnotationById(this->selectedObjectId);

    if (this->annotations->loadNextFrame())
    {
        // trackAnnotations should do everything that's needed?
        this->OFTracking->trackAnnotations();

        // preselect the same object than what was previously selected
        this->selectedObjectId = this->annotations->getRecord().searchAnnotation( this->annotations->getCurrentFramePosition(),
                                                                                  previouslySelectedObj.ClassId,
                                                                                  previouslySelectedObj.ObjectId );
        // if no object corresponds, it will be -1 - exactly what we need

        this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

        this->updatePaintImages();

        this->selectAnnotation(this->selectedObjectId);

        // nothing should have changed, so we don't need to update PaintingImage or ObjectImage

        update();
    }
}



void AnnotateArea::OFTrackMultipleFrames()
{
    // this is almost the same as displayNextFrame
    AnnotationObject previouslySelectedObj = this->annotations->getRecord().getAnnotationById(this->selectedObjectId);

    this->framesComputedForInterpolation = 0;

    while (this->framesComputedForInterpolation<this->OFTracking->getInterpolateLength())
    {
        if (!this->annotations->loadNextFrame())
            break;

        this->OFTracking->trackAnnotations();

        this->framesComputedForInterpolation++;
    }

    this->selectedObjectId = this->annotations->getRecord().searchAnnotation( this->annotations->getCurrentFramePosition(),
                                                                              previouslySelectedObj.ClassId,
                                                                              previouslySelectedObj.ObjectId );
    // if no object corresponds, it will be -1 - exactly what we need

    this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

    this->updatePaintImages();

    this->selectAnnotation(this->selectedObjectId);

    // nothing should have changed, so we don't need to update PaintingImage or ObjectImage

    update();
}


void AnnotateArea::interpolateBBObjects()
{
    if (this->framesComputedForInterpolation>1)
        this->annotations->interpolateLastBoundingBoxes(this->framesComputedForInterpolation);

    this->framesComputedForInterpolation = 0;
}




void AnnotateArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (!this->BBClassOnly)
        {
            // the user starts drawing something with his left button.. store the position and record the status
            this->lastPoint = this->adaptToScaleDiv(event->pos());
            this->firstAnnotPoint = this->lastPoint;


            // filling the image ObjectImage. This image will record the pixels that were specifically annotated
            // we fill it with color1, that sets all the pixels to 1. Then we will paint it black, and finally extract the darkest pixels
            this->ObjectImage.fill(Qt::color1);


            // also, in order to go faster, we record ROI data, so that we don't have to run through the whole image to know where something was painted
            // .. but this implies some fiddling with ROIs

            // remove the displayed part corresponding to the previous ROI
            // also specify that nothing is selected anymore (it's useful for the bounding boxes display thing)
            this->selectedObjectId = -1;
            QRect previousROI = this->ObjectROI;

            // update the contours because if an object was selected, it's likely to interfere
            this->updatePaintImages(previousROI, true);
            this->update( this->adaptToScaleMul(previousROI.adjusted(-2, -2, 2, 2)) );


            // at the (almost) last time, specify that we've entered in scribbling mode
            this->scribbling = true;

            // start recording the right ROI
            this->ObjectROI = QRect(this->firstAnnotPoint, this->firstAnnotPoint);
        }
        else
        {
            QPoint actualInImgPos = this->adaptToScaleDiv(event->pos());

            // 2 cases (actually, just one but slightly different) : new annotation or edition
            int selectedAnnot = this->annotations->getClosestBBFromPosition(actualInImgPos.x(), actualInImgPos.y(), (int)(ceil((float)_AnnotateArea_BBSelection_GlueDist / this->scaleFactor)));

            if (selectedAnnot != -1)
            {
                // we're in edition mode - find which one : TL, T, TR, R..., L?

                const cv::Rect2i& currBB = this->annotations->getRecord().getAnnotationById(selectedAnnot).BoundingBox;

                int distToTop     = abs(currBB.tl().y -     actualInImgPos.y());
                int distToLeft    = abs(currBB.tl().x -     actualInImgPos.x());
                int distToBottom  = abs(currBB.br().y - 1 - actualInImgPos.y());
                int distToRight   = abs(currBB.br().x - 1 - actualInImgPos.x());

                // we must be under the selection area + in case both are under this area, we select the closer with some preference over the TL corner
                bool topActive    = (distToTop   <=_AnnotateArea_BBSelection_GlueDist && distToTop<=distToBottom);
                bool bottomActive = (distToBottom<=_AnnotateArea_BBSelection_GlueDist && distToTop >distToBottom);
                bool leftActive   = (distToLeft  <=_AnnotateArea_BBSelection_GlueDist && distToLeft<=distToRight);
                bool rightActive  = (distToRight <=_AnnotateArea_BBSelection_GlueDist && distToLeft >distToRight);

                // simply fill the right position... it might have been possible to encode this kind of information but I'm a bit lazy there
                if (topActive && leftActive)
                {
                    this->BBWhichCornerSelected = _BBEM_TopLeft;
                    this->currentBBEditionStyle = _BBES_Corner;
                    this->currentBBEditionFixedRect = QRect(currBB.br().x, currBB.br().y, 1, 1);
                }
                else if (topActive && !leftActive && !rightActive)
                {
                    this->BBWhichCornerSelected = _BBEM_Top;
                    this->currentBBEditionStyle = _BBES_Vertical;
                    this->currentBBEditionFixedRect = QRect(currBB.tl().x, currBB.br().y, currBB.width+1, 1);
                }
                else if (topActive && rightActive)
                {
                    this->BBWhichCornerSelected = _BBEM_TopRight;
                    this->currentBBEditionStyle = _BBES_Corner;
                    this->currentBBEditionFixedRect = QRect(currBB.tl().x, currBB.br().y, 1, 1);
                }
                else if (rightActive && !bottomActive)
                {
                    this->BBWhichCornerSelected = _BBEM_Right;
                    this->currentBBEditionStyle = _BBES_Horizontal;
                    this->currentBBEditionFixedRect = QRect(currBB.tl().x, currBB.tl().y, 1, currBB.height+1);
                }
                else if (rightActive && bottomActive)
                {
                    this->BBWhichCornerSelected = _BBEM_BottomRight;
                    this->currentBBEditionStyle = _BBES_Corner;
                    this->currentBBEditionFixedRect = QRect(currBB.tl().x, currBB.tl().y, 1, 1);
                }
                else if (bottomActive && !leftActive && !rightActive)
                {
                    this->BBWhichCornerSelected = _BBEM_Bottom;
                    this->currentBBEditionStyle = _BBES_Vertical;
                    this->currentBBEditionFixedRect = QRect(currBB.tl().x, currBB.tl().y, currBB.width+1, 1);
                }
                else if (bottomActive && leftActive)
                {
                    this->BBWhichCornerSelected = _BBEM_BottomLeft;
                    this->currentBBEditionStyle = _BBES_Corner;
                    this->currentBBEditionFixedRect = QRect(currBB.br().x, currBB.tl().y, 1, 1);
                }
                else
                {
                    this->BBWhichCornerSelected = _BBEM_Left;
                    this->currentBBEditionStyle = _BBES_Horizontal;
                    this->currentBBEditionFixedRect = QRect(currBB.br().x, currBB.tl().y, 1, currBB.height+1);
                }

                this->selectedObjectId = selectedAnnot;

                this->ObjectROI = QRect(this->firstAnnotPoint, this->firstAnnotPoint);
            }
            else
            {
                // generate a new object
                this->selectedObjectId = this->annotations->addAnnotation( cv::Point2i(actualInImgPos.x(),   actualInImgPos.y()),
                                                                           cv::Point2i(actualInImgPos.x()+1, actualInImgPos.y()+1), this->selectedClass );

                // by default, work in BottomRight edition mode
                this->BBWhichCornerSelected = _BBEM_BottomRight;

                this->currentBBEditionStyle = _BBES_Corner;
                this->currentBBEditionFixedRect = QRect(actualInImgPos, QSize(1,1));

                QRect prevObjectROI = this->ObjectROI;
                this->ObjectROI = QRect(event->pos(), event->pos());

                this->update( (prevObjectROI | this->ObjectROI).adjusted(-2,-2,2,2) );
            }
        }
    }
}



void AnnotateArea::mouseMoveEvent(QMouseEvent *event)
{
    // the mouse has moved, we record it and paint the content straight away
    if ((event->buttons() & Qt::LeftButton) && this->scribbling)
        this->drawLineTo(this->adaptToScaleDiv(event->pos()));
    else if ((event->buttons() & Qt::LeftButton) && this->BBClassOnly)
    {
        // alright, we must edit the currently selected bounding box
        /*
        switch(this->BBWhichCornerSelected)
        {
        case _BBEM_TopLeft:
            // update the boundary - be cautious about the cases when one of the coordinates makes us fall in a case when we're not editing the TL corner anymore

            // TO BE CONTINUED
        }
        */
        QPoint actualInImgPos = this->adaptToScaleDiv(event->pos());

        QRect currPointRect;

        switch(this->currentBBEditionStyle)
        {
        case _BBES_Corner:
            currPointRect = QRect(actualInImgPos, QSize(1,1));
            break;
        case _BBES_Horizontal:
            currPointRect = QRect(actualInImgPos.x(), this->currentBBEditionFixedRect.top(), 1, 1);
            break;
        case _BBES_Vertical:
            currPointRect = QRect(this->currentBBEditionFixedRect.left(), actualInImgPos.y(), 1, 1);
            break;
        }

        QRect newArea = this->currentBBEditionFixedRect | currPointRect;

        this->annotations->editAnnotationBoundingBox( this->selectedObjectId, cv::Rect2i(cv::Point2i(newArea.left(), newArea.top()),
                                                                                         cv::Point2i(newArea.right(), newArea.bottom())) );


        QRect prevObjectROI = this->ObjectROI;
        this->ObjectROI = this->adaptToScaleMul(newArea);

        this->update( (prevObjectROI | this->ObjectROI).adjusted(-2,-2,2,2) );
    }
}



void AnnotateArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && this->scribbling)
    {
        // left mouse button released : end annotating, draw the last bits
        this->drawLineTo( this->adaptToScaleDiv(event->pos()) );
        this->scribbling = false;

        // compute the ROI
        int rad = (this->myPenWidth / 2) + 1;
        this->ObjectROI = this->ObjectROI.adjusted(-rad, -rad, rad, rad);

        // ensure that the ROI is within the boundaries of the image... it can happen that it's beyond them
        this->ObjectROI = this->ObjectROI.intersected( QRect(QPoint(0,0), this->BackgroundImage.size()) );
        //qDebug() << "ROI : " << ObjectROI;



        // now generating the cv::Mat object that will be passed into the annotations set
        cv::Mat currentAnnotationMat = cv::Mat::zeros(this->ObjectROI.height(), this->ObjectROI.width(), CV_8UC1);
        for (int i=0; i<this->ObjectROI.height(); i++)
        {
            for (int j=0; j<this->ObjectROI.width(); j++)
            {
                // qDebug() << this->ObjectImage.pixelColor(j+this->ObjectROI.left(), i+this->ObjectROI.top());

                if (this->ObjectImage.pixelColor(j+this->ObjectROI.left(), i+this->ObjectROI.top()).black() > 127)
                //if (this->ObjectImage.pixel(j+this->ObjectROI.left(), i+this->ObjectROI.top())==Qt::color0)
                {
                    currentAnnotationMat.at<uchar>(i,j) = 255;
                }
            }
        }

        if (!this->rubberMode)
        {
            this->selectedObjectId = this->annotations->addAnnotation( currentAnnotationMat,
                                                                       cv::Point2i(this->ObjectROI.left(), this->ObjectROI.top()),
                                                                       this->selectedClass,
                                                                       cv::Point2i(this->firstAnnotPoint.x(), this->firstAnnotPoint.y()) );

            // update the ObjectROI in case the new annotation is an increment to a previous one
            this->ObjectROI |= QtCvUtils::cvRect2iToQRect( this->annotations->getRecord().getAnnotationById(this->selectedObjectId).BoundingBox );
        }
        else
        {
            this->annotations->removePixelsFromAnnotations(currentAnnotationMat, cv::Point2i(this->ObjectROI.left(), this->ObjectROI.top()));
            this->selectedObjectId = -1;
        }


        // specify that we have to update the contours image
        // - update with a 2 pixel wide surrounding because it may have affected surrounding objects
        this->updatePaintImages(this->ObjectROI.adjusted(-2,-2,2,2));



        emit selectedObject(this->selectedObjectId);


        // update the corresponding image content
        this->update( this->adaptToScaleMul(this->ObjectROI.adjusted(-2, -2, 2, 2)) );
    }
    else if (event->button() == Qt::LeftButton && this->BBClassOnly)
    {
        // update the whole image, we may have messed-up with the objects numbering display
        this->update();

        // update the selection browser as well
        this->selectAnnotation(this->selectedObjectId);
    }
    // there's nothing to do if we were in BB only update mode

    else if (event->button() == Qt::RightButton)
    {
        // no annotation, but an object selection
        QPoint actualPos = this->adaptToScaleDiv(event->pos());

        // find the right annotation at the place where we clicked
        if (this->BBClassOnly)
            this->selectedObjectId = this->annotations->getClosestBBFromPosition( actualPos.x(), actualPos.y(), (int)(ceil((float)_AnnotateArea_BBSelection_GlueDist / this->scaleFactor)) );
        else
            this->selectedObjectId = this->annotations->getObjectIdAtPosition(actualPos.x(), actualPos.y());

        // update the display and emit the signal
        this->selectAnnotation(this->selectedObjectId);

        // emit the information
        //emit selectedObject(this->selectedObjectId);

        // ok i'm being lazy there - update the whole image...
        //this->update();
    }
}


void AnnotateArea::paintEvent(QPaintEvent *event)
{
    // draw the content of the widget - we draw directly on the widget itself
    QPainter painter(this);


    // dirtyRect corresponds to the area which we want to update in the widget
    // either it's external, thus it's related to the scaled version of the image (what's visible of the widget),
    // either it's internal, but we had to adapt to the external version, thus all the systematic calls to adaptToScale()
    QRect dirtyRect = event->rect();




    if (this->BackgroundImage.size()==QSize(0,0))
    {
        // no need to spend some time and get some warnings if there isn't anything to display
        // we just paint an empty image and return then
        painter.setOpacity(1.);
        painter.drawImage(dirtyRect.topLeft(), this->BackgroundImage);
        painter.end();
        return;
    }



    // use ceil and floor to ensure that the desired area is well included
    // first register to the original image dimensions
    dirtyRect.setTop( floor((float)dirtyRect.top() / this->scaleFactor) );
    dirtyRect.setLeft( floor((float)dirtyRect.left() / this->scaleFactor) );
    dirtyRect.setRight( ceil((float)dirtyRect.right() / this->scaleFactor) );
    dirtyRect.setBottom( ceil((float)dirtyRect.bottom() / this->scaleFactor) );

    // this is the concerned area in the original image
    QRect origImgArea = dirtyRect;

    // setting "dirtyRect", aka the region where we want to modify stuff, to the area concerned within the widget
    dirtyRect = this->adaptToScaleMul(dirtyRect);


    // first draw the BG image, use only the visible part
    painter.setOpacity(1.);
    painter.drawImage(dirtyRect.topLeft(), this->BackgroundImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding));


    // is there a super pixels map to display?
    if (this->SPAnnotate->getContoursMask().data)
    {
        painter.setOpacity(0.5);
        painter.drawImage(dirtyRect.topLeft(), this->SPContoursImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding));
    }



    if (this->scribbling)
    {
        // scribbling mode : paint contours THEN objects
        // and draw the contours of objects
        painter.setOpacity(0.5);
        painter.drawImage(dirtyRect.topLeft(), this->ContoursImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding));

        // now draw the annotations... remove some opacity so that the background is always visible
        painter.setOpacity(0.6);
        painter.drawImage(dirtyRect.topLeft(), this->PaintingImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding));
    }
    else
    {
        // normal mode : paint objects THEN contours

        // now draw the annotations... remove some opacity so that the background is always visible
        painter.setOpacity(0.6);
        painter.drawImage(dirtyRect.topLeft(), this->PaintingImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding));

        // and draw the contours of objects
        painter.setOpacity(0.5);
        painter.drawImage(dirtyRect.topLeft(), this->ContoursImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding));
    }
    /*
    qDebug() << "background image subset size : " << this->BackgroundImage.copy(origImgArea).size()
             << "after painting it : " << this->BackgroundImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding).size()
             << "area where we paint it : " << dirtyRect;
             */
    // now displaying the bounding boxes - only if we're not in scribble mode
    this->drawBoundingBoxes(dirtyRect);
    painter.setOpacity(0.9);
    painter.drawImage(dirtyRect.topLeft(), this->BoundingBoxesImage.copy(dirtyRect));


    /*
    // display the selected object
    if (!this->scribbling && this->selectedObjectId != -1)
    {
        // grab the object ROI within the record
        this->ObjectROI = QtCvUtils::cvRect2iToQRect(this->annotations->getRecord().getAnnotationById(this->selectedObjectId).BoundingBox);

        QRect modifiedROI = this->adaptToScaleMul(this->ObjectROI);

        // first, the bounding box
        painter.setOpacity(0.8);
        painter.setPen(QPen(Qt::red, 2, Qt::DashDotDotLine));
        painter.drawRect(modifiedROI);
    }
    */

    // that's all... at least before we add some additional layers
    painter.end();

    // signal that something has changed
    emit updateSignal();
}







void AnnotateArea::drawBoundingBoxes(const QRect& ROI)
{
    // avoid some annoying warnings
    if (this->BoundingBoxesImage.size()==QSize(0,0))
        return;


    // taking account of lines width to create a local ROI
    int adj = 2;
    if (this->scaleFactor>1.)
        adj *= this->scaleFactor;

    QRect localROI = ROI.adjusted(-adj, -adj, adj, adj);

    // defining the original area covered by the paint event, so that we can avoid re-drawing objects that are out of scope
    QRect origImgRect = QRect(localROI.topLeft()/this->scaleFactor, localROI.size()/this->scaleFactor);


    // initiating the QPainter
    QPainter painter(&(this->BoundingBoxesImage));
    painter.setCompositionMode(QPainter::CompositionMode_Clear);    // ensure that filling with transparent data will indeed fill with transparent

    // erase what's inside the ROI
    painter.fillRect(localROI, Qt::transparent);

    // now ensure that what we're painting now will add something
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);


    painter.setRenderHint(QPainter::Antialiasing);


    // setting the font
    QFont numberFont("Helvetica [Cronyx]", 16, QFont::Bold);
    numberFont.setStyleStrategy(QFont::PreferAntialias);
    QPen textOutline;
    textOutline.setWidth(1);
    //textOutline.setColor(Qt::white);





    // now displaying the bounding boxes
    const std::vector<int>& currFrameObjects = this->annotations->getObjectsListOnCurrentFrame();

    for (size_t k=0; k<currFrameObjects.size(); k++)
    {
        QRect currBB = QtCvUtils::cvRect2iToQRect(this->annotations->getRecord().getAnnotationById(currFrameObjects[k]).BoundingBox);

        if (origImgRect.intersects(currBB))
        {
            int currClass = this->annotations->getRecord().getAnnotationById(currFrameObjects[k]).ClassId;

            QColor rightColor = QtCvUtils::cvVec3bToQColor(this->annotations->getConfig().getProperty(currClass).displayRGBColor, 255);
            // setting the right color
            // we divide the color by 2 (darken it) so that it contrasts better

            if (currFrameObjects[k] == this->selectedObjectId)
            {
                painter.setPen(QPen(rightColor, 2, Qt::DashDotDotLine));
                painter.setOpacity(1.);
            }
            else
            {
                painter.setPen(QPen(rightColor, 1, Qt::DashLine));
                painter.setOpacity(0.8);
            }

            painter.drawRect(this->adaptToScaleMul(currBB));



            // now handling the text (if needed)
            if (this->annotations->getConfig().getProperty(currClass).classType != _ACT_Uniform)
            {
                // for drawing the objects numbering, we use a textPath object
                // this allows to draw the outline and the inside of characters with a different color
                QPainterPath textPath;
                textOutline.setColor(qRgb(rightColor.red()/2, rightColor.green()/2, rightColor.blue()/2));  // darken the color to increase the contrast!
                painter.setPen(textOutline);

                // try to set the text inside the bounding box
                QPoint textPos = this->adaptToScaleMul(currBB).topLeft();
                textPos.setY(textPos.y() + 13);

                textPath.addText(textPos, numberFont, QString::number(this->annotations->getRecord().getAnnotationById(currFrameObjects[k]).ObjectId));
                painter.setBrush(Qt::white);    // filling the characters with white
                painter.drawPath(textPath);
                painter.setBrush(Qt::transparent);  // don't forget to remove the filling


                // we draw the text then
                /*
                painter.drawText(this->adaptToScaleMul(currBB),
                                 QString::number(this->annotations->getRecord().getAnnotationById(currFrameObjects[k]).ObjectId),
                                 Qt::AlignBottom | Qt::AlignRight);
                                 */

                //painter.setPen(QPen());
            }
        }
    }

    painter.end();
}




/*
void AnnotateArea::resizeEvent(QResizeEvent *event)
{*/
    /*
    if (width() > PaintingImage.width() || height() > PaintingImage.height()) {
        int newWidth = qMax(width() + 128, PaintingImage.width());
        int newHeight = qMax(height() + 128, PaintingImage.height());
        resizeImage(&PaintingImage, QSize(newWidth, newHeight));
        update();
    }*/
/*
    QWidget::resizeEvent(event);
}
*/

void AnnotateArea::drawLineTo(const QPoint &endPoint)
{
    // draw a line to the image

    // we have 2 painters to fill : one for the visible image
    // and one for the share of informations with opencv. The objectPainter is not visible but this is the one that will be used
    // to determine which pixels were annotated.
    Qt::PenCapStyle capStyle = (this->myPenShape==AA_CS_round) ? Qt::RoundCap : Qt::SquareCap;
    Qt::PenJoinStyle joinStyle = (this->myPenShape==AA_CS_round) ? Qt::RoundJoin : Qt::MiterJoin;

    QPainter painter(&(this->PaintingImage));
    if (this->rubberMode)
    {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);    // this mode erases what was before - this is exactly what we are looking for
        painter.setPen(QPen(Qt::transparent, (int)this->myPenWidth / this->scaleFactor, Qt::SolidLine, capStyle, joinStyle));
    }
    else
    {
        painter.setPen(QPen(this->myPenColor, (int)this->myPenWidth / this->scaleFactor, Qt::SolidLine, capStyle, joinStyle));
    }

    QPainter objectPainter(&(this->ObjectImage));
    objectPainter.setPen(QPen(Qt::black, (int)this->myPenWidth / this->scaleFactor, Qt::SolidLine, capStyle, joinStyle));


    // Qt doesn't draw lines when the starting point is identical to the end point
    // so if we want to draw spots, we have to add this kind of condition...
    if (this->lastPoint!=endPoint)
    {
        painter.drawLine(this->lastPoint, endPoint);
        objectPainter.drawLine(this->lastPoint, endPoint);
    }
    else
    {
        painter.drawPoint(this->lastPoint);
        objectPainter.drawPoint(this->lastPoint);
    }
    //this->modified = true;

    this->ObjectROI |= QRect(endPoint, endPoint);

    //update();

    // updating only the right region
    int rad = (this->myPenWidth / (2 * this->scaleFactor)) + 2;
    this->update( this->adaptToScaleMul(QRect(this->lastPoint, endPoint).normalized()
                                                .adjusted(-rad, -rad, +rad, +rad)) );
    this->lastPoint = endPoint;
}



void AnnotateArea::updatePaintImages(const QRect& ROI, bool contoursOnly)
{
    QRect localROI = ROI;

    // update the paint image according to the annotations available
    if (ROI == QRect(-3, -3, 0, 0))
    {
        if (!contoursOnly)
        {
            // no ROI was specified, fill the whole image. Intialize it properly beforehand
            this->PaintingImage = QImage(this->BackgroundImage.size(), QImage::Format_ARGB32);
            this->PaintingImage.fill(qRgba(255, 255, 255, 0));
        }

        this->ContoursImage.fill(_AA_CI_NoC);
        this->SPContoursImage.fill(_AA_CI_NoC);

        // perhaps it's enough already? use the annotations index to know if we need to go any further
        const std::vector<int>& currentFrameAnnots = this->annotations->getRecord().getFrameContentIds(this->annotations->getCurrentFramePosition());

        // answer : no, we have some annotations to look at
        if (currentFrameAnnots.size()>0)
        {
            localROI = QtCvUtils::cvRect2iToQRect(this->annotations->getRecord().getAnnotationById(currentFrameAnnots[0]).BoundingBox);

            for (int i=1; i<(int)currentFrameAnnots.size(); i++)
            {
                localROI |= QtCvUtils::cvRect2iToQRect(this->annotations->getRecord().getAnnotationById(currentFrameAnnots[i]).BoundingBox);
            }
        }
    }

    if (localROI.isEmpty())
        return; // we have done enough already!


    // if we're there, that means that we have some job left to perform

    // first, generate the set of color lists that we will use to make things a bit faster
    std::vector<QColor> classesColorList;
    classesColorList.push_back(QColor(255,255,255,0));   // start to fill it with the "none" class (0 opacity value)

    for (int k=1; k<=this->annotations->getConfig().getPropsNumber(); k++)
    {
        cv::Vec3b classCol = this->annotations->getConfig().getProperty(k).displayRGBColor;
        classesColorList.push_back(QColor(classCol[0], classCol[1], classCol[2], 255));
    }


    // ensure that the ROI is not outside of the image boundaries
    localROI &= QRect(QPoint(0,0), this->BackgroundImage.size());


    // prepare for the contours displaying thing
    int selectedObjClass=0, selectedObjId=-1;
    if (this->selectedObjectId != -1)
    {
        selectedObjClass = this->annotations->getRecord().getAnnotationById(this->selectedObjectId).ClassId;
        selectedObjId = this->annotations->getRecord().getAnnotationById(this->selectedObjectId).ObjectId;
    }


    // now run through the ROI of the image and fill the pixels
    // for some reason, Qt's BR corner is inclusive - it means that unlike the rest of the whole framework, we need <= comparisons
    for (int i=localROI.top(); i<=localROI.bottom(); i++)
    {
        for (int j=localROI.left(); j<=localROI.right(); j++)
        {
            if (!contoursOnly)
                this->PaintingImage.setPixelColor(j,i, classesColorList[this->annotations->getCurrentAnnotationsClasses().at<int16_t>(i,j)]);

            unsigned int currContourColor = _AA_CI_NoC;
            if (this->annotations->getCurrentContours().at<uchar>(i,j) > 0)
            {
                // try to know if the object was selected or not
                if ( (this->annotations->getCurrentAnnotationsClasses().at<int16_t>(i,j)==selectedObjClass) &&
                     (this->annotations->getCurrentAnnotationsIds().at<int32_t>(i,j)==selectedObjId) )
                    currContourColor = _AA_CI_SelC;
                else
                    currContourColor = _AA_CI_NotSelC;
            }

            this->ContoursImage.setPixel(j,i,currContourColor);
        }
    }





    if (this->SPAnnotate->getContoursMask().data)
    {
        for (int i=localROI.top(); i<=localROI.bottom(); i++)
        {
            for (int j=localROI.left(); j<=localROI.right(); j++)
            {
                unsigned int currContourColor = _AA_CI_NoC;

                if (this->SPAnnotate->getContoursMask().at<uchar>(i,j) > 0)
                    currContourColor = _AA_CI_NotSelC;

                this->SPContoursImage.setPixel(j,i,currContourColor);
            }
        }
    }

    // this->ObjectImage = QImage(this->BackgroundImage.size(), QImage::Format_Mono);
}



// just a small function to translate from original size QRect objects to scaled (adapted to the zoom) QRect objects
QRect AnnotateArea::adaptToScaleMul(const QRect& rect) const
{
    // a small notice about this function
    // at first, I had

    if (this->scaleFactor>1.)
    {
        // if we're in "zoom" mode, we need to switch to integers. Rounding tends to cause problems
        int iScaleFactor = round(this->scaleFactor);
        return QRect(rect.topLeft()*iScaleFactor, rect.size()*iScaleFactor);
    }

    return QRect(rect.topLeft()*this->scaleFactor, rect.size()*this->scaleFactor);
}


QPoint AnnotateArea::adaptToScaleDiv(const QPoint& point) const
{
    if (this->scaleFactor>1.)
    {
        // if we're in "zoom" mode, we need to switch to integers. Rounding tends to cause problems
        int iScaleFactor = round(this->scaleFactor);
        return QPoint( (int)(point.x() / iScaleFactor), (int)(point.y() / iScaleFactor) );
    }

    return (point / this->scaleFactor);
}





/*
void AnnotateArea::resizeImage(QImage *image, const QSize &newSize)
{
*/
    /*
    if (image->size() == newSize)
        return;

    QImage newImage(newSize, QImage::Format_RGB32);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), *image);
    *image = newImage;
    * */
//}

