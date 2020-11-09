#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSaveFile>
#include <QEventLoop>
#include <QDateTime>
#include <QTimer>

#include "QtCvUtils.h"
#include <fstream>



const std::string _NetworkHandler_YAMLKey_NetworkConfigurationNode = "NetworkConfiguration";

const std::string _NetworkHandler_YAMLKey_HTTP_url = "HTTP_Url";

const std::string _NetworkHandler_YAMLKey_FTP_url = "FTP_Url";
const std::string _NetworkHandler_YAMLKey_FTP_login = "FTP_Login";
const std::string _NetworkHandler_YAMLKey_FTP_password = "FTP_Password";

const std::string _NetworkHandler_YAMLKey_SyncDataFilename = "SyncData_Filename";
const std::string _NetworkHandler_YAMLKey_LocalSyncDataFilename = "LocalSyncData_Filename";

const std::string _NetworkHandler_YAMLKey_ImgRelativePath = "ImagesRelPath";
const std::string _NetworkHandler_YAMLKey_AnnRelativePath = "AnnotsRelPath";
const std::string _NetworkHandler_YAMLKey_images_FileNames_list = "ImageFileNames";
const std::string _NetworkHandler_YAMLKey_AnnotsPostfixYaml = "PostfixYaml";
const std::string _NetworkHandler_YAMLKey_AnnotsPostfixCsv = "PostfixCsv";

const std::string _NetworkHandler_YAMLKey_AnnoterName = "AnnoterName";

const int _NetworkHandler_Sync_WaitTimer = 150;

enum _NetworkHandler_SyncStatus { _NHSS_Unknown, _NHSS_LocalOnly, _NHSS_DistantOnly, _NHSS_Synchronized };


struct _NetworkHandler_SyncEntry { std::string ImageFileName, FilenamePostfix, Author, DateTime; _NetworkHandler_SyncStatus Status; int ImageIndex; bool csvDone; };

enum _NetworkHandler_SyncStep { _NHSyncStep_DetermineQueues, _NHSyncStep_ImagesDL, _NHSyncStep_AnnotsDL, _NHSyncStep_AnnotsUL, _NHSyncStep_Recording };


/*
class FileDownloader : public QObject {

Q_OBJECT
public:
 explicit FileDownloader(QUrl imageUrl, QObject *parent = 0);
 virtual ~FileDownloader();
 QByteArray downloadedData() const;

signals:
 void downloaded();

private slots:
 void fileDownloaded(QNetworkReply* pReply);

private:
 QNetworkAccessManager m_WebCtrl;
 QByteArray m_DownloadedData;

};
*/



class NetworkHandler : public QObject {

    Q_OBJECT

    public:
        explicit NetworkHandler(QObject *parent = 0);
        virtual ~NetworkHandler();

        void DownloadFile(const QString& distantFilename, const QString& localFilename="");
        void UploadFile(const QString& path, const QString& differentDistantPath="");
        // QByteArray downloadedData() const;

        void notifyNewAnnotationFile(const QString& correspondingImageFileName);

        void synchronize(bool firstCheck=false, bool hardSync=false);

        void loadNetworkConfiguration(const QString& configFilePath);
        bool networkConfigLoaded() const { return this->NetworkConfLoaded; }


    public slots:
        void hardSync();
        void softSync();

    signals:
        void downloaded();
        void downloadFailed();
        void uploaded();
        void uploadFailed();
        void annotationDownloadRecorded();
        void annotationUploadRecorded();

        void queuesDetermined();
        void imagesQueueProcessed();
        void annotsDownloadQueueProcessed();
        void annotsUploadQueueProcessed();


    private slots:
        void fileDownloaded(QNetworkReply* pReply);
        void fileUploaded(QNetworkReply* pReply);

        void syncDetermineQueues();
        //void syncProcessQueues();
        void syncProcessImagesQueue();
        void syncProcessAnnotsDLQueue();
        void syncProcessAnnotsULQueue();
        void syncProcessSave();
        void syncProcessEnd();

        void recordAnnotationDownload();
        void recordAnnotationUpload();


    private:
        bool loadSyncDataFile();

        bool recordSyncDataFile(bool onlyLocal=false);

        bool networkSyncEquals(const _NetworkHandler_SyncEntry&, const _NetworkHandler_SyncEntry&) const;

        // QNetworkAccessManager m_WebCtrl;
        //QNetworkAccessManager m_WebCtrl_DL, m_WebCtrl_UL;
        QNetworkAccessManager *WebCtrl_DL, *WebCtrl_UL;;
        QByteArray m_DownloadedData;
        QFile m_UploadData;
        QString fileNameToSaveTo;



        QString distantDLUrl, localPath, syncDataFilename, localSyncDataFilename,
                distantULUrl, ULUserName, ULPassword,
                imgRelativePath, annotsRelativePath, annotsPostfixYaml, annotsPostfixCsv,
                annotaterName;

        std::vector< std::vector<_NetworkHandler_SyncEntry> > SyncEntries;  // same length as the imagesfilelist, every 2nd dim vector is a list of available annotations. Only the most recent one is important.
        std::vector< std::vector<_NetworkHandler_SyncEntry> > LocalSyncEntries;  // same length as the imagesfilelist, every 2nd dim vector is a list of available annotations. Only the most recent one is important.

        std::vector<QString> ImagesFileList;
        std::vector<QString> KnownNewAnnotations;

        bool NetworkConfLoaded;
        bool allImagesPresent;



        std::vector<_NetworkHandler_SyncEntry> annotationsUploadQueue, annotationsDownloadQueue;
        int annotationsUploadPos, annotationsDownloadPos;

        _NetworkHandler_SyncEntry currentlyDownloadedEntry, currentlyUploadedEntry;

        std::vector<QString> downloadSimpleList;
        int downloadListPos;

        // bool csvForNow;

        bool hardSyncMode, firstCheckMode, newEntries, newLocalEntries;

        _NetworkHandler_SyncStep CurrentSyncStep;

};



#endif // NETWORKHANDLER_H
