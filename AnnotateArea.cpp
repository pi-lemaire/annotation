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
    //this->modified = false;
    this->scribbling = false;
    this->rubberMode = false;

    this->scaleFactor = 1.0;

    this->myPenShape = AA_CS_square;

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

    this->updatePaintImage();

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

    this->updatePaintImage();

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
    QPainter cursorPainter(&(this->CursorBitmap));
    cursorPainter.setPen(Qt::black);
    // 32, 32 is supposed to be the middle of the area?
    if (this->myPenShape == AA_CS_round)
        cursorPainter.drawEllipse(QPointF(32,32), this->myPenWidth/2., this->myPenWidth/2.);
    else
        cursorPainter.drawRect(32-this->myPenWidth/2., 32-this->myPenWidth/2., this->myPenWidth, this->myPenWidth);
    cursorPainter.drawPoint(32,32);
    cursorPainter.end();

    // use this as a cursor
    this->setCursor(QCursor(this->CursorBitmap, this->CursorBitmap, 32, 32));
}


void AnnotateArea::setScale(float newScale)
{
    this->scaleFactor = newScale;
    this->setFixedSize(this->BackgroundImage.size() * newScale);

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
    this->ObjectImage = QImage(this->BackgroundImage.size(), QImage::Format_Mono);


    // set the widget to the right size - the QScrollArea should do the rest
    this->setFixedSize(this->BackgroundImage.size() * this->scaleFactor);

    this->selectedObjectId = -1;
    // this->selectedClass = 0;

    // prepare for the annotation stuff, and display the result
    //this->modified = false;
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


bool AnnotateArea::displayFrame(int id)
{
    if (this->annotations->loadFrame(id))
    {
        this->BackgroundImage = QtCvUtils::cvMatToQImage(this->annotations->getCurrentOriginalImg());

        this->updatePaintImage();

        this->selectAnnotation(-1);

        // nothing should have changed, so we don't need to update PaintingImage or ObjectImage

        update();

        return true;
    }

    return false;
}



void AnnotateArea::clearImage()
{
    this->PaintingImage.fill(qRgba(255, 255, 255, 0));
    //this->modified = true;
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
    this->update( this->adaptToScale(updateRoi.adjusted(-2,-2,2,2)) );

    emit selectedObject(annotId);
}





void AnnotateArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // the user starts drawing something with his left button.. store the position and record the status
        this->lastPoint = event->pos() / this->scaleFactor;
        this->firstAnnotPoint = this->lastPoint;
        this->scribbling = true;

        // filling the image ObjectImage. This image will record the pixels that were specifically annotated
        // we fill it with color1, that sets all the pixels to 1. Then we will paint it black, and finally extract the darkest pixels
        this->ObjectImage.fill(Qt::color1);


        // also, in order to go faster, we record ROI data, so that we don't have to run through the whole image to know where something was painted
        // .. but this implies some fiddling with ROIs

        // remove the displayed part corresponding to the previous ROI
        QRect previousROI = this->ObjectROI;
        this->ObjectROI = QRect(-3, -3, 0, 0);
        this->update( this->adaptToScale(previousROI.adjusted(-2, -2, 2, 2)) );

        // start recording the right ROI
        this->ObjectROI = QRect(this->firstAnnotPoint, this->firstAnnotPoint);
    }
}



void AnnotateArea::mouseMoveEvent(QMouseEvent *event)
{
    // the mouse has moved, we record it and paint the content straight away
    if ((event->buttons() & Qt::LeftButton) && this->scribbling)
        this->drawLineTo(event->pos() / this->scaleFactor);
}



void AnnotateArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && this->scribbling)
    {
        // left mouse button released : end annotating, draw the last bits
        this->drawLineTo(event->pos() / this->scaleFactor);
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
        this->update( this->adaptToScale(this->ObjectROI.adjusted(-2, -2, 2, 2)) );
    }

    else if (event->button() == Qt::RightButton)
    {
        // no annotation, but an object selection
        QPoint actualPos = event->pos() / this->scaleFactor;

        // find the right annotation at the place where we clicked
        this->selectedObjectId = this->annotations->getObjectIdAtPosition(actualPos.x(), actualPos.y());

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

    // dirtyRect corresponds to the area which we want to update in the widget
    // either it's external, thus it's related to the scaled version of the image (what's visible of the widget),
    // either it's internal, but we had to adapt to the external version, thus all the systematic calls to adaptToScale()
    QRect dirtyRect = event->rect();
    dirtyRect.setTop(floor(dirtyRect.top() / this->scaleFactor) * this->scaleFactor);
    dirtyRect.setLeft(floor(dirtyRect.left() / this->scaleFactor) * this->scaleFactor);
    dirtyRect.setRight(ceil(dirtyRect.right() / this->scaleFactor) * this->scaleFactor);
    dirtyRect.setBottom(ceil(dirtyRect.bottom() / this->scaleFactor) * this->scaleFactor);

    QRect origImgArea = QRect(dirtyRect.topLeft() / this->scaleFactor, dirtyRect.bottomRight() / this->scaleFactor);

    // qDebug() << "dirtyRect is " << dirtyRect;


    // first draw the BG image, use only the visible part
    painter.setOpacity(1.);
    // painter.drawImage(dirtyRect, this->BackgroundImage.scaled(this->BackgroundImage.size() * this->scaleFactor), dirtyRect);
    painter.drawImage(dirtyRect.topLeft(), this->BackgroundImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding));

    // now draw the annotations... remove some opacity so that the background is always visible
    painter.setOpacity(0.6);
    // painter.drawImage(dirtyRect, this->PaintingImage.scaled(this->BackgroundImage.size() * this->scaleFactor), dirtyRect);
    painter.drawImage(dirtyRect.topLeft(), this->PaintingImage.copy(origImgArea).scaled(dirtyRect.size(), Qt::KeepAspectRatioByExpanding));

    // display the selected object
    if (!this->scribbling && this->selectedObjectId != -1)
    {
        // grab the object ROI within the record
        this->ObjectROI = QtCvUtils::cvRect2iToQRect(this->annotations->getRecord().getAnnotationById(this->selectedObjectId).BoundingBox);

        QRect modifiedROI;
        modifiedROI.setTopLeft(this->ObjectROI.topLeft() * this->scaleFactor);
        modifiedROI.setBottomRight(this->ObjectROI.bottomRight() * this->scaleFactor);


        // first, the bounding box
        painter.setOpacity(0.8);
        painter.setPen(QPen(Qt::red, 2, Qt::DashDotDotLine));
        // painter.drawRect(this->ObjectROI);
        painter.drawRect(modifiedROI);
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
    Qt::PenCapStyle capStyle = (this->myPenShape==AA_CS_round) ? Qt::RoundCap : Qt::SquareCap;
    Qt::PenJoinStyle joinStyle = (this->myPenShape==AA_CS_round) ? Qt::RoundJoin : Qt::MiterJoin;

    QPainter painter(&(this->PaintingImage));
    if (this->rubberMode)
    {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);    // this mode erases what was before - this is exactly what we are looking for
        painter.setPen(QPen(Qt::transparent, this->myPenWidth / this->scaleFactor, Qt::SolidLine, capStyle, joinStyle));
    }
    else
    {
        painter.setPen(QPen(this->myPenColor, this->myPenWidth / this->scaleFactor, Qt::SolidLine, capStyle, joinStyle));
    }

    QPainter objectPainter(&(this->ObjectImage));
    objectPainter.setPen(QPen(Qt::black, this->myPenWidth / this->scaleFactor, Qt::SolidLine, capStyle, joinStyle));


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
    this->update( this->adaptToScale(QRect(this->lastPoint, endPoint).normalized()
                                           .adjusted(-rad, -rad, +rad, +rad)) );
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



// just a small function to translate from original size QRect objects to scaled (adapted to the zoom) QRect objects
QRect AnnotateArea::adaptToScale(const QRect& rect) const
{
    return QRect(rect.topLeft()*this->scaleFactor, rect.bottomRight()*this->scaleFactor);
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

