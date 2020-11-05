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

    reply->close();

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
    {
        qDebug() << "----";
        qDebug() << "Starting the SOFT synchronization process...";
        if (this->synchronize())
        {
            qDebug() << "... The SOFT synchronization process ended well.";
            qDebug() << "----";
        }
    }
}



void NetworkHandler::hardSync()
{
    if (this->NetworkConfLoaded)
    {
        qDebug() << "----";
        qDebug() << "Starting the HARD synchronization process...";
        if (this->synchronize(false, true))
        {
            qDebug() << "... The HARD synchronization process ended well.";
            qDebug() << "----";
        }
    }
}


bool NetworkHandler::networkSyncEquals(const _NetworkHandler_SyncEntry& nsa, const _NetworkHandler_SyncEntry& nsb) const
{
    return (nsa.Author==nsb.Author && nsa.DateTime==nsb.DateTime && nsa.ImageFileName==nsb.ImageFileName && nsa.FilenamePostfix==nsb.FilenamePostfix);
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
    bool newLocalEntries = false;

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


            // now we want to determine if we have to download the distant file
            bool doWeDownload =    !QtCvUtils::fileExists( QString(this->localPath + localPathWithoutExtension + this->annotsPostfixYaml).toStdString() )
                                || !QtCvUtils::fileExists( QString(this->localPath + localPathWithoutExtension + this->annotsPostfixCsv ).toStdString() );

            // if we cannot find the corresponding file on the local system : we have to download the file

            // another case is HardSync. If we're in hard sync mode and the file exists, it should be recorded locally somewhere
            // in this case, if the entry is different, then: we have to download it

            if (hardSync)
            {
                if (this->LocalSyncEntries[k].size()==0)
                    doWeDownload = true;
                else if (!this->networkSyncEquals(LocalSyncEntries[k][0],currEntry))
                    doWeDownload = true;
            }

            // if we download it, we also save it
            if (doWeDownload)
            {
                if (!this->DownloadFileByUrl(distantPathWithoutExtension + this->annotsPostfixYaml, localPathWithoutExtension + this->annotsPostfixYaml))
                    qDebug() << "troubles downloading " << distantPathWithoutExtension << this->annotsPostfixYaml;
                if (!this->DownloadFileByUrl(distantPathWithoutExtension + this->annotsPostfixCsv, localPathWithoutExtension + this->annotsPostfixCsv))
                    qDebug() << "troubles downloading " << distantPathWithoutExtension << this->annotsPostfixCsv;

                // qDebug() << "downloaded file " << localPathWithoutExtension;

                this->LocalSyncEntries[k].clear();
                this->LocalSyncEntries[k].push_back(currEntry);

                newLocalEntries = true;
            }

            // one last case (mostly when it's a first synchronization : there's no entry because the
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
                    this->LocalSyncEntries[k].clear();
                    this->LocalSyncEntries[k].push_back(newEntry);
                    newLocalEntries = true;
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
            this->LocalSyncEntries[k].clear();
            this->LocalSyncEntries[k].push_back(newEntry);
            newLocalEntries = true;
        }
        else
            qDebug() << "trouble uploading the " << this->KnownNewAnnotations[k] << " new annotation file";
    }


    // now, simply record the new entries
    if (newEntries)
    {
        if (!this->recordSyncDataFile() || !this->UploadFile(this->syncDataFilename))
            qDebug() << "issue saving and uploading the sync data file";
        //else
        //    qDebug() << "saved the sync data file alright";
    }
    else if (newLocalEntries)
    {
        if (!this->recordSyncDataFile(true))
            qDebug() << "having troubles saving the local only sync data file";
    }

    this->KnownNewAnnotations.clear();


    return true;
}




void NetworkHandler::notifyNewAnnotationFile(const QString& correspondingImageFileName)
{
    // qDebug() << "notify called : " << correspondingImageFileName;

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
                StdStrAuthor, StdStrSyncDataFile, StdStrLocalSyncDataFile;

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
    NetworkConfigFnd[_NetworkHandler_YAMLKey_LocalSyncDataFilename] >> StdStrLocalSyncDataFile;


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
    this->localSyncDataFilename = QString::fromStdString(StdStrLocalSyncDataFile);

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
        this->LocalSyncEntries.push_back( std::vector<_NetworkHandler_SyncEntry>() );
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

    qDebug() << "----";
    qDebug() << "Network Configuration file loaded";
    qDebug() << "----";

    return this->synchronize(true);
}







bool NetworkHandler::loadSyncDataFile()
{
    std::ifstream fsIn;
    fsIn.open( QString(this->localPath + this->syncDataFilename).toStdString().c_str(), std::ifstream::in );
    if (!fsIn.is_open())
        return false;

    std::ifstream fsInLocal;
    fsInLocal.open( QString(this->localPath + this->localSyncDataFilename).toStdString().c_str(), std::ifstream::in );
    //if (!fsInLocal.is_open())
    //    return false;
    // if the local file doesn't exist : well it's no big deal, it will be created very soon. Just clear the list and don't care about it

    this->SyncEntries.clear();
    this->LocalSyncEntries.clear();
    for (size_t k=0; k<this->ImagesFileList.size(); k++)
    {
        this->SyncEntries.push_back(std::vector<_NetworkHandler_SyncEntry>());
        this->LocalSyncEntries.push_back(std::vector<_NetworkHandler_SyncEntry>());
    }


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

    fsIn.close();



    if (fsInLocal.is_open())
    {
        while (!fsInLocal.eof())
        {
            std::string line;

            //ifs.getline(newLine, 2048);
            if (!std::getline(fsInLocal, line))
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
                    this->LocalSyncEntries[k].push_back(currEntry);
                    break;
                }
            }
            // there should be only one entry / image at most in this local version
        }

        fsInLocal.close();
    }

    /*
    qDebug() << "distant entries :";
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

    qDebug() << "local entries :";
    for (size_t k=0; k<this->LocalSyncEntries.size(); k++)
    {
        for (size_t j=0; j<this->LocalSyncEntries[k].size(); j++)
        {
            qDebug() << "line " << k << " : " << QString::fromStdString(this->LocalSyncEntries[k][j].ImageFileName) << " - " <<
                                                 QString::fromStdString(this->LocalSyncEntries[k][j].FilenamePostfix) << " - " <<
                                                 QString::fromStdString(this->LocalSyncEntries[k][j].Author) << " - " <<
                                                 QString::fromStdString(this->LocalSyncEntries[k][j].DateTime);
        }
    }
    */

    return true;
}

bool NetworkHandler::recordSyncDataFile(bool localOnly)
{
    if (!localOnly)
    {
        std::ofstream fsOut;
        fsOut.open( QString(this->localPath + this->syncDataFilename).toStdString().c_str(), std::ofstream::out );
        if (!fsOut.is_open())
            return false;

        for (size_t i=0; i<this->SyncEntries.size(); i++)
            for (size_t j=0; j<this->SyncEntries[i].size(); j++)
                fsOut << this->SyncEntries[i][j].ImageFileName << "," << this->SyncEntries[i][j].FilenamePostfix << "," << this->SyncEntries[i][j].Author << "," << this->SyncEntries[i][j].DateTime << std::endl;

        fsOut.close();
    }


    std::ofstream fsOutLocal;
    fsOutLocal.open( QString(this->localPath + this->localSyncDataFilename).toStdString().c_str(), std::ofstream::out );
    if (!fsOutLocal.is_open())
        return false;

    for (size_t i=0; i<this->LocalSyncEntries.size(); i++)
        for (size_t j=0; j<this->LocalSyncEntries[i].size(); j++)
            fsOutLocal << this->LocalSyncEntries[i][j].ImageFileName << "," << this->LocalSyncEntries[i][j].FilenamePostfix << "," << this->LocalSyncEntries[i][j].Author << "," << this->LocalSyncEntries[i][j].DateTime << std::endl;

    fsOutLocal.close();

    return true;
}





