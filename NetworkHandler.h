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

#include "QtCvUtils.h"
#include <fstream>



const std::string _NetworkHandler_YAMLKey_NetworkConfigurationNode = "NetworkConfiguration";

const std::string _NetworkHandler_YAMLKey_HTTP_url = "HTTP_Url";

const std::string _NetworkHandler_YAMLKey_FTP_url = "FTP_Url";
const std::string _NetworkHandler_YAMLKey_FTP_login = "FTP_Login";
const std::string _NetworkHandler_YAMLKey_FTP_password = "FTP_Password";

const std::string _NetworkHandler_YAMLKey_SyncDataFilename = "SyncData_Filename";

const std::string _NetworkHandler_YAMLKey_ImgRelativePath = "ImagesRelPath";
const std::string _NetworkHandler_YAMLKey_AnnRelativePath = "AnnotsRelPath";
const std::string _NetworkHandler_YAMLKey_images_FileNames_list = "ImageFileNames";
const std::string _NetworkHandler_YAMLKey_AnnotsPostfixYaml = "PostfixYaml";
const std::string _NetworkHandler_YAMLKey_AnnotsPostfixCsv = "PostfixCsv";

const std::string _NetworkHandler_YAMLKey_AnnoterName = "AnnoterName";



enum _NetworkHandler_SyncStatus { _NHSS_Unknown, _NHSS_LocalOnly, _NHSS_DistantOnly, _NHSS_Synchronized };


struct _NetworkHandler_SyncEntry { std::string ImageFileName, FilenamePostfix, Author, DateTime; _NetworkHandler_SyncStatus Status; };


class NetworkHandler : public QObject {

    Q_OBJECT

    public:
        explicit NetworkHandler(QObject *parent = 0);
        virtual ~NetworkHandler();

        bool DownloadFileByUrl(const QString& path, const QString& differentLocalPath="");
        QByteArray downloadedData() const;

        bool UploadFile(const QString& path, const QString& differentDistantPath="");

        void notifyNewAnnotationFile(const QString& correspondingImageFileName);

        bool synchronize(bool firstCheck=false, bool hardSync=false);
        bool loadNetworkConfiguration(const QString& configFilePath);


    public slots:
        void hardSync();
        void softSync();

    signals:
        void downloaded();

    private slots:
        void fileDownloaded(QNetworkReply* pReply);

    private:
        bool loadSyncDataFile();
        bool recordSyncDataFile();

        QNetworkAccessManager m_WebCtrl;
        QByteArray m_DownloadedData;
        QString fileNameToSaveTo;

        QString distantDLUrl, localPath, syncDataFilename,
                distantULUrl, ULUserName, ULPassword,
                imgRelativePath, annotsRelativePath, annotsPostfixYaml, annotsPostfixCsv,
                annotaterName;

        std::vector< std::vector<_NetworkHandler_SyncEntry> > SyncEntries;  // same length as the imagesfilelist, every 2nd dim vector is a list of available annotations. Only the most recent one is important.

        std::vector<QString> ImagesFileList;
        std::vector<QString> KnownNewAnnotations;

        bool NetworkConfLoaded;
        bool allImagesPresent;
};



#endif // NETWORKHANDLER_H
