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

AnnotateArea::AnnotateArea(AnnotationsSet* annotsSet, QWidget *parent)
    : QWidget(parent)
{
    this->annotations = annotsSet;

    // ensuring that the widget is always referenced to the top left corner
    setAttribute(Qt::WA_StaticContents);

    // initialization stuff...
    this->modified = false;
    this->scribbling = false;
    this->rubberMode = false;

    this->setPenWidth(10);
    //this->myPenColor = Qt::blue;
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

    return true;
}


bool AnnotateArea::openVideo(const QString &fileName)
{
    // load the annotation into the system
    if (!this->annotations->loadOriginalVideo(fileName.toStdString()))
        return false;

    this->reload();

    return true;
}


bool AnnotateArea::openAnnotations(const QString &fileName)
{
    // load the annotation into the system
    if (!this->annotations->loadAnnotations(fileName.toStdString()))
        return false;

    this->reload();

    return true;
}


bool AnnotateArea::saveImage(const QString &fileName, const char *fileFormat)
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

    // 64,64 is seemingly the max possible size. I'm not sure whether it's supported on all systems
    this->CursorBitmap = QBitmap(64,64);
    // draw with blank values, then paint it
    this->CursorBitmap.fill();
    QPainter cursorPainter(&(this->CursorBitmap));
    cursorPainter.setPen(Qt::black);
    // 32, 32 is supposed to be the middle of the area.
    cursorPainter.drawEllipse(QPointF(32,32), this->myPenWidth/2., this->myPenWidth/2.);
    cursorPainter.end();

    // use this as a cursor
    this->setCursor(QCursor(this->CursorBitmap, this->CursorBitmap));
}


void AnnotateArea::displayFrame(int id)
{
    if (this->annotations->loadFrame(id))
    {
        this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

        this->updatePaintImage();

        this->selectAnnotation(-1);

        // nothing should have changed, so we don't need to update PaintingImage or ObjectImage

        update();
    }
}


void AnnotateArea::reload()
{
    // convert the original image as background image
    this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

    // format the other layers correctly
    this->PaintingImage = QImage(this->BackgroundImage.size(), QImage::Format_ARGB32);
    this->PaintingImage.fill(qRgba(255, 255, 255, 0));
    this->ObjectImage = QImage(this->BackgroundImage.size(), QImage::Format_Mono);


    // set the widget to the right size - the QScrollArea should do the rest
    this->setFixedSize(this->BackgroundImage.size());

    this->selectedObjectId = -1;
    // this->selectedClass = 0;

    // prepare for the annotation stuff, and display the result
    this->modified = false;
    update();
}



// public slots region

void AnnotateArea::displayNextFrame()
{
    if (this->annotations->loadNextFrame())
    {
        this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

        this->updatePaintImage();

        this->selectAnnotation(-1);

        // nothing should have changed, so we don't need to update PaintingImage or ObjectImage

        update();
    }
}


void AnnotateArea::displayPrevFrame()
{
    this->displayFrame( this->annotations->getCurrentFramePosition()-1 );
}



void AnnotateArea::clearImage()
{
    this->PaintingImage.fill(qRgba(255, 255, 255, 0));
    this->modified = true;
    // updateContent();
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
}


void AnnotateArea::selectClassId(int classId)
{
    this->selectedClass = classId;
    // cv::Vec3b classColor = this->annotations->getConfig().getProperty(classId).displayRGBColor;
    this->myPenColor = QtCvUtils::cvVec3bToQColor( this->annotations->getConfig().getProperty(classId).displayRGBColor );
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

    // update the part containing both the old and the new ROI
    this->update(updateRoi.adjusted(-2,-2,2,2));

    emit selectedObject(annotId);
}





void AnnotateArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // the user starts drawing something with his left button.. store the position and record the status
        this->lastPoint = event->pos();
        this->firstAnnotPoint = event->pos();
        this->scribbling = true;

        // filling the image ObjectImage. This image will record the pixels that were specifically annotated
        // we fill it with color1, that sets all the pixels to 1. Then we will paint it black, and finally extract the darkest pixels
        this->ObjectImage.fill(Qt::color1);


        // also, in order to go faster, we record ROI data, so that we don't have to run through the whole image to know where something was painted
        // .. but this implies some fiddling with ROIs

        // remove the displayed part corresponding to the previous ROI
        QRect previousROI = this->ObjectROI;
        this->ObjectROI = QRect(-3, -3, 0, 0);
        this->update(previousROI.adjusted(-2, -2, 2, 2));

        // start recording the right ROI
        this->ObjectROI = QRect(event->pos(), event->pos());
    }
}



void AnnotateArea::mouseMoveEvent(QMouseEvent *event)
{
    // the mouse has moved, we record it and paint the content straight away
    if ((event->buttons() & Qt::LeftButton) && this->scribbling)
        this->drawLineTo(event->pos());
}



void AnnotateArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && this->scribbling)
    {
        // left mouse button released : end annotating, draw the last bits
        this->drawLineTo(event->pos());
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
            this->ObjectROI = QtCvUtils::cvRect2iToQRect( this->annotations->getRecord().getAnnotationById(this->selectedObjectId).BoundingBox );
        }
        else
        {
            this->annotations->removePixelsFromAnnotations(currentAnnotationMat, cv::Point2i(this->ObjectROI.left(), this->ObjectROI.top()));
            this->selectedObjectId = -1;
        }



        emit selectedObject(this->selectedObjectId);


        // update the corresponding image content
        this->update(this->ObjectROI.adjusted(-2, -2, 2, 2));
    }

    else if (event->button() == Qt::RightButton)
    {
        // no annotation, but an object selection

        // find the right annotation at the place where we clicked
        this->selectedObjectId = this->annotations->getObjectIdAtPosition(event->pos().x(), event->pos().y());

        // emit the information
        emit selectedObject(this->selectedObjectId);

        // ok i'm being lazy there - update the whole image...
        this->update();
    }
}


void AnnotateArea::paintEvent(QPaintEvent *event)
{
    // draw the content of the widget - we draw directly on the widget itself
    QPainter painter(this);

    // corresponds to the area covered by the widget
    QRect dirtyRect = event->rect();

    // first draw the BG image, we use only the visible part
    painter.setOpacity(1.);
    painter.drawImage(dirtyRect, this->BackgroundImage, dirtyRect);

    // now draw the annotations... remove some opacity so that the background is always visible
    painter.setOpacity(0.6);
    painter.drawImage(dirtyRect, this->PaintingImage, dirtyRect);

    // display the selected object
    if (!this->scribbling && this->selectedObjectId != -1)
    {
        // grab the object ROI within the record
        this->ObjectROI = QtCvUtils::cvRect2iToQRect(this->annotations->getRecord().getAnnotationById(this->selectedObjectId).BoundingBox);

        // first, the bounding box
        painter.setOpacity(0.8);
        painter.setPen(QPen(Qt::red, 2, Qt::DashDotDotLine));
        painter.drawRect(this->ObjectROI);
    }

    // that's all... at least before we add some additional layers
    painter.end();

    // signal that something has changed
    emit updateSignal();
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
    QPainter painter(&(this->PaintingImage));
    if (this->rubberMode)
    {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);    // this mode erases what was before - this is exactly what we are looking for
        painter.setPen(QPen(Qt::transparent, this->myPenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
    else
    {
        painter.setPen(QPen(this->myPenColor, this->myPenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }

    QPainter objectPainter(&(this->ObjectImage));
    objectPainter.setPen(QPen(Qt::black, this->myPenWidth, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));

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
    this->modified = true;

    this->ObjectROI |= QRect(endPoint, endPoint);

    //update();

    // updating only the right region
    int rad = (this->myPenWidth / 2) + 2;
    this->update(QRect(this->lastPoint, endPoint).normalized()
                                           .adjusted(-rad, -rad, +rad, +rad));
    this->lastPoint = endPoint;
}



void AnnotateArea::updatePaintImage(const QRect& ROI)
{
    QRect localROI = ROI;

    // update the paint image according to the annotations available
    if (ROI == QRect(-3, -3, 0, 0))
    {
        // no ROI was specified, fill the whole image. Intialize it properly beforehand
        this->PaintingImage = QImage(this->BackgroundImage.size(), QImage::Format_ARGB32);
        this->PaintingImage.fill(qRgba(255, 255, 255, 0));

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

    // now run through the ROI of the image and fill the pixels
    for (int i=localROI.top(); i<localROI.bottom(); i++)
    {
        for (int j=localROI.left(); j<localROI.right(); j++)
        {
            this->PaintingImage.setPixelColor(j,i, classesColorList[this->annotations->getCurrentAnnotationsClasses().at<int16_t>(i,j)]);
        }
    }

    // this->ObjectImage = QImage(this->BackgroundImage.size(), QImage::Format_Mono);
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

