#include "NetworkHandler.h"

NetworkHandler::NetworkHandler( QObject *parent) : QObject(parent)
{
    // connect( &m_WebCtrl, SIGNAL (finished(QNetworkReply*)), this, SLOT (fileDownloaded(QNetworkReply*)) );
    this->NetworkConfLoaded = false;
}

NetworkHandler::~NetworkHandler() { }

bool NetworkHandler::DownloadFileByUrl(const QString& path, const QString& differentLocalPath)
{
    QString localFilePath = differentLocalPath;
    if (localFilePath=="")
        localFilePath = path;

    // fileNameToSaveTo = dest;
    QNetworkRequest request(QUrl(distantDLUrl + path));
    QNetworkReply *reply = m_WebCtrl.get(request);

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error()==QNetworkReply::NoError)
    {
        m_DownloadedData = reply->readAll();
        //emit a signal
        reply->deleteLater();

        QtCvUtils::generatePath( QString(localPath+localFilePath).toStdString().c_str() );
        // Save the data to a file.
        QSaveFile file(localPath + localFilePath);
        file.open(QIODevice::WriteOnly);
        file.write(m_DownloadedData);
        // Calling commit() is mandatory, otherwise nothing will be written.
        file.commit();

        reply->close();

        return true;
    }

    return false;
    // emit downloaded();
}



bool NetworkHandler::UploadFile(const QString& path, const QString& distantFilename)
{
    QString distPath = distantFilename;
    if (distPath == "")
        distPath = path;

    // fileNameToSaveTo = dest;
    QUrl ulUrl = QUrl(distantULUrl + distPath);
    ulUrl.setPassword(ULPassword);
    ulUrl.setUserName(ULUserName);

    QFile file(localPath + path);
    file.open(QIODevice::ReadOnly);
    QNetworkAccessManager *nam = new QNetworkAccessManager;
    QNetworkRequest requp(ulUrl);
    QNetworkReply *reply = nam->put(requp,&file);

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    loop.exec();

    file.close();

    if (reply->error()==QNetworkReply::NoError)
    {
        reply->close();
        return true;
    }

    reply->close();
    return false;
    // emit downloaded();
}





void NetworkHandler::fileDownloaded(QNetworkReply* pReply)
{
    m_DownloadedData = pReply->readAll();
    //emit a signal
    pReply->deleteLater();


    // Save the data to a file.
    QSaveFile file(fileNameToSaveTo);
    file.open(QIODevice::WriteOnly);
    file.write(m_DownloadedData);
    // Calling commit() is mandatory, otherwise nothing will be written.
    file.commit();

    emit downloaded();
}

QByteArray NetworkHandler::downloadedData() const {
    return m_DownloadedData;
}



void NetworkHandler::softSync()
{
    if (this->NetworkConfLoaded)
        this->synchronize();
}



void NetworkHandler::hardSync()
{
    if (this->NetworkConfLoaded)
        this->synchronize(false, true);
}



bool NetworkHandler::synchronize(bool firstCheck, bool hardSync)
{
    bool distantCheck = firstCheck;

    if (!this->NetworkConfLoaded)
        return false;

    // load the sync file, in case someone else is working on it at roughly the same moment..
    //if (!this->DownloadFileByUrl(this->syncDataFilename) || !this->loadSyncDataFile())
    //    qDebug() << "there was an issue loading the synchronization file";


    if (firstCheck)
    {
        // at first, synchronize the images
        for (size_t k=0; k<this->ImagesFileList.size(); k++)
        {
            if (!QtCvUtils::fileExists( QString(this->localPath + this->imgRelativePath + this->ImagesFileList[k]).toStdString() ))
            {

                // if we lack some images, we have never been synchronized, so we'll need to check everything anyway
                distantCheck = true;

                // need to download the file
                if (!this->DownloadFileByUrl(this->imgRelativePath + this->ImagesFileList[k]))
                {
                    qDebug() << "we had an issue downloading the image file " + this->ImagesFileList[k];
                }

                // this->ImagesSyncList[k] = _NHSS_Synchronized;
            }
        }
    }


    // std::vector<_NetworkHandler_SyncEntry> additionalEntries;
    bool newEntries = false;

    if (!this->DownloadFileByUrl(this->syncDataFilename) || !this->loadSyncDataFile())
        qDebug() << "there was an issue loading the synchronization file";


    /*
    // in any case, synchronize the SyncData file
    if (!this->DownloadFileByUrl(this->syncDataFilename))
         qDebug() << "we had an issue downloading the synchronization file " + this->syncDataFilename;
    else
    {
        // read it then
        if (!this->loadSyncDataFile())
            qDebug() << "we had an issue loading the synch file locally";
        else
        {
        */
    // now compare it with what we have locally
    for (size_t k=0; k<this->SyncEntries.size(); k++)
    {
        bool skip = false;

        // check if this file is in the list of known new stuff, in which case : don't touch that
        for (size_t i=0; i<this->KnownNewAnnotations.size(); i++)
            if (this->KnownNewAnnotations[i]==this->ImagesFileList[k])
            {
                skip = true;
                break;
            }

        if (skip)
            continue;

        if (this->SyncEntries[k].size()>0)
        {
            // there's at least one entry
            _NetworkHandler_SyncEntry currEntry = this->SyncEntries[k][this->SyncEntries[k].size()-1];
            QString localPathWithoutExtension = this->annotsRelativePath + QString::fromStdString(currEntry.ImageFileName);
            QString distantPathWithoutExtension = localPathWithoutExtension + QString::fromStdString(currEntry.FilenamePostfix);

            // now one option is hard-sync : we sync anyway
            // if we cannot find the corresponding file on the local system : we do too
            if (    hardSync
                 || !QtCvUtils::fileExists( QString(this->localPath + localPathWithoutExtension + this->annotsPostfixYaml).toStdString() )
                 || !QtCvUtils::fileExists( QString(this->localPath + localPathWithoutExtension + this->annotsPostfixCsv ).toStdString() ) )
            {
                if (!this->DownloadFileByUrl(distantPathWithoutExtension + this->annotsPostfixYaml, localPathWithoutExtension + this->annotsPostfixYaml))
                    qDebug() << "troubles downloading " << distantPathWithoutExtension << this->annotsPostfixYaml;
                if (!this->DownloadFileByUrl(distantPathWithoutExtension + this->annotsPostfixCsv, localPathWithoutExtension + this->annotsPostfixCsv))
                    qDebug() << "troubles downloading " << distantPathWithoutExtension << this->annotsPostfixCsv;
            }
            // now check if the file exists locally
        }
        else if (hardSync || firstCheck)
        {
            // no entry detected, but we check if there's an annotation file.
            // if so, we need to synchronize it
            if (    QtCvUtils::fileExists( QString(this->localPath + this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixYaml).toStdString() )
                 && QtCvUtils::fileExists( QString(this->localPath + this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv ).toStdString() ) )
            {
                _NetworkHandler_SyncEntry newEntry;
                newEntry.ImageFileName = this->ImagesFileList[k].toStdString();
                newEntry.FilenamePostfix = "";  // no need for a postfix since it's supposed to be a first entry
                newEntry.Author = this->annotaterName.toStdString();
                newEntry.DateTime = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss").toStdString();

                if (    this->UploadFile(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixYaml)
                     && this->UploadFile(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv) )
                {
                    newEntries = true;
                    this->SyncEntries[k].push_back(newEntry);
                }
            }
        }
    }



    // alright, now handling the known additional stuff
    for (size_t k=0; k<this->KnownNewAnnotations.size(); k++)
    {
        size_t imageId = 0;
        for (imageId=0; imageId<this->ImagesFileList.size(); imageId++)
        {
            if (this->KnownNewAnnotations[k] == this->ImagesFileList[imageId])
                break;
        }

        QString newPostfix = (this->SyncEntries[imageId].size() > 0) ? "_" + QString::number((int)this->SyncEntries[imageId].size()) : "";

        _NetworkHandler_SyncEntry newEntry;
        newEntry.ImageFileName = this->ImagesFileList[imageId].toStdString();
        newEntry.FilenamePostfix = newPostfix.toStdString();  // no need for a postfix since it's supposed to be a first entry
        newEntry.Author = this->annotaterName.toStdString();
        newEntry.DateTime = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss").toStdString();

        if (    this->UploadFile(this->annotsRelativePath + this->ImagesFileList[imageId] + this->annotsPostfixYaml, this->annotsRelativePath + this->ImagesFileList[imageId] + newPostfix + this->annotsPostfixYaml)
             && this->UploadFile(this->annotsRelativePath + this->ImagesFileList[imageId] + this->annotsPostfixCsv , this->annotsRelativePath + this->ImagesFileList[imageId] + newPostfix + this->annotsPostfixCsv) )
        {
            newEntries = true;
            this->SyncEntries[imageId].push_back(newEntry);
        }
        else
            qDebug() << "trouble uploading the " << this->KnownNewAnnotations[k] << " new annotation file";
    }


    // now, simply record the new entries
    if (newEntries)
    {
        if (!this->recordSyncDataFile() || !this->UploadFile(this->syncDataFilename))
            qDebug() << "issue saving and uploading the sync data file";
        else
            qDebug() << "saved the sync data file alright";
    }

    this->KnownNewAnnotations.clear();




        // then, synchronize the


            /*

            if (    !QtCvUtils::fileExists( QString(this->localPath + this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixYaml).toStdString() )
                 || !QtCvUtils::fileExists( QString(this->localPath + this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv ).toStdString() ) )
            {
                // the file does not exist locally
                // does the file exist on the distant repo???
                // this may be quite painful, with a lot of connections, so we avoid to do it when it's not necessary
                if ( distantCheck
                     && this->DownloadFileByUrl(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv) )
                {
                    this->AnnotsSyncList[k] = _NHSS_DistantOnly;

                    if (!this->DownloadFileByUrl(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixYaml, false))
                        // files exist
                        qDebug() << "we had an issue downloading the file " + this->ImagesFileList[k] + this->annotsPostfixYaml;

                    if (!this->DownloadFileByUrl(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv, false))
                        // files exist
                        qDebug() << "we had an issue downloading the file " + this->ImagesFileList[k] + this->annotsPostfixCsv;

                    // we downloaded the thing, we're up to date
                    this->AnnotsSyncList[k] = _NHSS_Synchronized;
                }
            }

            else
            {
                // the file exists locally
                // we have just loaded the configuration file, so we assume that the data is the same between the server
                // just suppose that the file is the same between the distant repo and the local one
                if ( this->DownloadFileByUrl(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv) )
                    this->AnnotsSyncList[k] = _NHSS_Synchronized;
                else
                {
                    if (    this->UploadFile(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixYaml)
                         && this->UploadFile(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv) )
                        this->AnnotsSyncList[k] = _NHSS_Synchronized;
                    else
                    {
                        qDebug() << "issue uploading the file";
                        this->AnnotsSyncList[k] = _NHSS_LocalOnly;
                    }
                }
            }

            */

            /*
        }
    }
    else
    {
        // just trust the vectors
        for (size_t k=0; k<this->ImagesFileList.size(); k++)
        {
            // the annotation file already exists on the local system, does it exist online?
            if (this->AnnotsSyncList[k] == _NHSS_LocalOnly)
            {
                // perform a check (in case the file was uploaded before the synchronization happened)
                if (    this->DownloadFileByUrl(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixYaml)
                     && this->DownloadFileByUrl(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv) )
                {
                    qDebug() << "there was a conflict around file " + this->ImagesFileList[k] + this->annotsPostfixYaml + " -- trying to upload it under a different number";

                    int index = 0;
                    while ( this->DownloadFileByUrl(this->annotsRelativePath + this->ImagesFileList[k] + "_" + index + this->annotsPostfixCsv) )
                        index++;

                    if (    this->UploadFile(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixYaml, this->annotsRelativePath + this->ImagesFileList[k] + "_" + index + this->annotsPostfixYaml)
                         && this->UploadFile(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv,  this->annotsRelativePath + this->ImagesFileList[k] + "_" + index + this->annotsPostfixCsv) )
                        this->AnnotsSyncList[k] = _NHSS_Synchronized;
                }
                else
                {
                    // just upload without asking any question
                    if (    this->UploadFile(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixYaml)
                         && this->UploadFile(this->annotsRelativePath + this->ImagesFileList[k] + this->annotsPostfixCsv) )
                        this->AnnotsSyncList[k] = _NHSS_Synchronized;
                    else
                    {
                        qDebug() << "issue uploading the file";
                        this->AnnotsSyncList[k] = _NHSS_LocalOnly;
                    }
                }
            }
            */

    return true;
}




void NetworkHandler::notifyNewAnnotationFile(const QString& correspondingImageFileName)
{
    qDebug() << "notify called : " << correspondingImageFileName;

    if (!this->NetworkConfLoaded)
        return;

    for (size_t k=0; k<this->KnownNewAnnotations.size(); k++)
    {
        if (correspondingImageFileName == this->KnownNewAnnotations[k])
        {
            // this was already notified
            return;
        }
    }

    this->KnownNewAnnotations.push_back(correspondingImageFileName);
}




bool NetworkHandler::loadNetworkConfiguration(const QString& configFilePath)
{
    this->localPath = QtCvUtils::getDirectory(configFilePath);

    cv::FileStorage fsR(configFilePath.toStdString().c_str(), cv::FileStorage::READ);

    if (!fsR.isOpened())
        return false;

    if (fsR[_NetworkHandler_YAMLKey_NetworkConfigurationNode].empty())
        return false;

    cv::FileNode NetworkConfigFnd = fsR[_NetworkHandler_YAMLKey_NetworkConfigurationNode];

    std::string StdStrDistantDLUrl,
                StdStrDistantULUrl, StdStrULUserName, StdStrULPassword,
                StdStrImgRelativePath, StdStrAnnotsRelativePath, StdStrAnnotsPostfixYaml, StdStrAnnotsPostfixCsv,
                StdStrAuthor, StdStrSyncDataFile;

    NetworkConfigFnd[_NetworkHandler_YAMLKey_HTTP_url] >> StdStrDistantDLUrl;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_FTP_url] >> StdStrDistantULUrl;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_FTP_login] >> StdStrULUserName;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_FTP_password] >> StdStrULPassword;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_ImgRelativePath] >> StdStrImgRelativePath;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_AnnRelativePath] >> StdStrAnnotsRelativePath;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_AnnotsPostfixYaml] >> StdStrAnnotsPostfixYaml;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_AnnotsPostfixCsv] >> StdStrAnnotsPostfixCsv;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_AnnoterName] >> StdStrAuthor;
    NetworkConfigFnd[_NetworkHandler_YAMLKey_SyncDataFilename] >> StdStrSyncDataFile;


    this->distantDLUrl = QString::fromStdString(StdStrDistantDLUrl);
    this->distantULUrl = QString::fromStdString(StdStrDistantULUrl);
    this->ULUserName = QString::fromStdString(StdStrULUserName);
    this->ULPassword = QString::fromStdString(StdStrULPassword);
    this->imgRelativePath = QString::fromStdString(StdStrImgRelativePath);
    this->annotsRelativePath = QString::fromStdString(StdStrAnnotsRelativePath);
    this->annotsPostfixYaml = QString::fromStdString(StdStrAnnotsPostfixYaml);
    this->annotsPostfixCsv = QString::fromStdString(StdStrAnnotsPostfixCsv);
    this->annotaterName = QString::fromStdString(StdStrAuthor);
    this->syncDataFilename = QString::fromStdString(StdStrSyncDataFile);

    this->ImagesFileList.clear();
    this->SyncEntries.clear();

    // grab the right filenode and start the iterator
    cv::FileNode nodeIt = NetworkConfigFnd[_NetworkHandler_YAMLKey_images_FileNames_list];
    cv::FileNodeIterator it = nodeIt.begin(), it_end = nodeIt.end();
    for (; it != it_end; ++it)
    {
        std::string _fileName;
        (*it) >> _fileName;
        this->ImagesFileList.push_back(QString::fromStdString(_fileName));
        this->SyncEntries.push_back( std::vector<_NetworkHandler_SyncEntry>() );
    }

    this->NetworkConfLoaded = true;

    /*
    qDebug() << "displaying list of image files";
    for (size_t k=0; k<this->ImagesFileList.size(); k++)
    {
        qDebug() << QString::fromStdString(this->ImagesFileList[k]);
    }
    */

    // return this->synchronize(true);


    // there should be no known new annotations.
    // however, since we're performing a first check, we will check whether there's new content over the internet.
    // we want it to happen in THIS file and not in the loadSyncDataFile() in case someone else is working at the same time...
    this->KnownNewAnnotations.clear();

    return this->synchronize(true);
}







bool NetworkHandler::loadSyncDataFile()
{
    std::ifstream fsIn;
    fsIn.open( QString(this->localPath + this->syncDataFilename).toStdString().c_str(), std::ifstream::in );
    if (!fsIn.is_open())
        return false;

    this->SyncEntries.clear();
    for (size_t k=0; k<this->ImagesFileList.size(); k++)
        this->SyncEntries.push_back(std::vector<_NetworkHandler_SyncEntry>());


    while (!fsIn.eof())
    {
        std::string line;

        //ifs.getline(newLine, 2048);
        if (!std::getline(fsIn, line))
            break;

        std::vector<std::string> elems;
        int prevPos = 0;
        for (int i=0; i<(int)line.length(); i++)
        {
            if (line[i]==',')
            {
                elems.push_back(line.substr(prevPos, (i-prevPos)));
                prevPos = i+1;
                //i++;
            }
        }
        // don't forget to add the last elements
        elems.push_back(line.substr(prevPos));

        _NetworkHandler_SyncEntry currEntry;
        currEntry.ImageFileName = elems[0];
        currEntry.FilenamePostfix = elems[1];
        currEntry.Author = elems[2];
        currEntry.DateTime = elems[3];
        currEntry.Status = _NHSS_Unknown;

        for (size_t k=0; k<this->ImagesFileList.size(); k++)
        {
            // inserting the new entry directly in place
            // actually, it might be possible to not start from 0 but the last tested index since everytime the file is recorded it should be ordered properly...
            if (this->ImagesFileList[k] == QString::fromStdString(currEntry.ImageFileName))
            {
                this->SyncEntries[k].push_back(currEntry);
                break;
            }
        }

    }


    for (size_t k=0; k<this->SyncEntries.size(); k++)
    {
        for (size_t j=0; j<this->SyncEntries[k].size(); j++)
        {
            qDebug() << "line " << k << " : " << QString::fromStdString(this->SyncEntries[k][j].ImageFileName) << " - " <<
                                                 QString::fromStdString(this->SyncEntries[k][j].FilenamePostfix) << " - " <<
                                                 QString::fromStdString(this->SyncEntries[k][j].Author) << " - " <<
                                                 QString::fromStdString(this->SyncEntries[k][j].DateTime);
        }
    }

    return true;
}

bool NetworkHandler::recordSyncDataFile()
{
    std::ofstream fsOut;
    fsOut.open( QString(this->localPath + this->syncDataFilename).toStdString().c_str(), std::ofstream::out );
    if (!fsOut.is_open())
        return false;

    for (size_t i=0; i<this->SyncEntries.size(); i++)
        for (size_t j=0; j<this->SyncEntries[i].size(); j++)
            fsOut << this->SyncEntries[i][j].ImageFileName << "," << this->SyncEntries[i][j].FilenamePostfix << "," << this->SyncEntries[i][j].Author << "," << this->SyncEntries[i][j].DateTime << std::endl;

    fsOut.close();

    return true;
}





