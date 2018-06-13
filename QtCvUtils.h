#ifndef QTCVUTILS_H
#define QTCVUTILS_H



#include <opencv2/opencv.hpp>
#include <QImage>
#include <QColor>
#include <QDebug>
#include <QDateTime>
#include <QDir>




namespace QtCvUtils
{


// useful conversion stuff first


/**
   Functions to convert between OpenCV's cv::Mat and Qt's QImage and QPixmap.

   Andy Maloney
   https://asmaloney.com/2013/11/code/converting-between-cvmat-and-qimage-or-qpixmap
**/


   // NOTE: This does not cover all cases - it should be easy to add new ones as required.
   inline QImage cvMatToQImage( const cv::Mat &inMat )
   {
      switch ( inMat.type() )
      {
         // 8-bit, 4 channel
         case CV_8UC4:
         {
            QImage image( inMat.data,
                          inMat.cols, inMat.rows,
                          static_cast<int>(inMat.step),
                          QImage::Format_ARGB32 );

            return image;
         }

         // 8-bit, 3 channel
         case CV_8UC3:
         {
            QImage image( inMat.data,
                          inMat.cols, inMat.rows,
                          static_cast<int>(inMat.step),
                          QImage::Format_RGB888 );

            return image.rgbSwapped();
         }

         // 8-bit, 1 channel
         case CV_8UC1:
         {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)

            QImage image( inMat.data,
                          inMat.cols, inMat.rows,
                          static_cast<int>(inMat.step),
                          QImage::Format_Grayscale8 );

#else
            static QVector<QRgb>  sColorTable;

            // only create our color table the first time
            if ( sColorTable.isEmpty() )
            {
               sColorTable.resize( 256 );

               for ( int i = 0; i < 256; ++i )
               {
                  sColorTable[i] = qRgb( i, i, i );
               }
            }

            QImage image( inMat.data,
                          inMat.cols, inMat.rows,
                          static_cast<int>(inMat.step),
                          QImage::Format_Indexed8 );

            image.setColorTable( sColorTable );
#endif

            return image;
         }

         default:
            qWarning() << "ASM::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type();
            break;
      }

      return QImage();
   }



   inline QColor cvVec3bToQColor(const cv::Vec3b& c, int alpha=-1)
   {
       if (alpha == -1) // no alpha channel required
           return qRgb(c[0], c[1], c[2]);
       else
           return qRgba(c[0], c[1], c[2], alpha);
   }


   inline QRect cvRect2iToQRect(const cv::Rect2i& r)
   {
        return QRect(QPoint(r.tl().x, r.tl().y), QSize(r.size().width, r.size().height));
   }


   inline bool imwrite(const std::string& fileName, const cv::Mat& img)
   {
       // this function generates the standard image file
       // main difference with imwrite is that it can generate the path on the fly, using Qt
       const QString filePath = QString::fromStdString(fileName);
       QDir().mkpath(QFileInfo(filePath).absolutePath());

       return cv::imwrite(fileName, img);
   }


   inline void generatePath(const std::string& fileName)
   {
       // this function generates the standard image file
       // main difference with imwrite is that it can generate the path on the fly, using Qt
       const QString filePath = QString::fromStdString(fileName);
       QDir().mkpath(QFileInfo(filePath).absolutePath());
   }

   inline bool fileExists(const std::string& fileName)
   {
       // found at https://stackoverflow.com/questions/10273816/how-to-check-whether-file-exists-in-qt-in-c
       QFileInfo check_file(QString::fromStdString(fileName));

       // check if file exists and if yes: Is it really a file and no directory?
       if (check_file.exists() && check_file.isFile()) {
           return true;
       } else {
           return false;
       }
   }

   inline std::string getDateTimeStr()
   {
       return QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
   }


   template<typename T>
   inline T getMax(T v1, T v2) { return (v1>v2) ? v1 : v2; }

   template<typename T>
   inline T getMin(T v1, T v2) { return (v1<v2) ? v1 : v2; }


}







#endif // QTCVUTILS_H
