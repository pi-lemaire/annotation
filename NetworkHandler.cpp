#include "NetworkHandler.h"




NetworkHandler::NetworkHandler( QObject *parent) : QObject(parent)
{
    this->NetworkConfLoaded = false;

    this->WebCtrl_DL = new QNetworkAccessManager(this);
    this->WebCtrl_UL = new QNetworkAccessManager(this);

    connect( WebCtrl_DL, SIGNAL (finished(QNetworkReply*)), this, SLOT (fileDownloaded(QNetworkReply*)) );
    connect( WebCtrl_UL, SIGNAL (finished(QNetworkReply*)), this, SLOT (fileUploaded(QNetworkReply*)) );

    connect(this, SIGNAL(queuesDetermined()), this, SLOT(syncProcessImagesQueue()));
    connect(this, SIGNAL(imagesQueueProcessed()), this, SLOT(syncProcessAnnotsDLQueue()));
    connect(this, SIGNAL(annotsDownloadQueueProcessed()), this, SLOT(syncProcessAnnotsULQueue()));
    connect(this, SIGNAL(annotsUploadQueueProcessed()), this, SLOT(syncProcessSave()));
}



NetworkHandler::~NetworkHandler() { }






void NetworkHandler::DownloadFile(const QString& distantFilename, const QString& localFilename)
{
    // qDebug() << "DownloadFile called : " << distantFilename << " - " << localFilename;

    if (localFilename=="")
        this->fileNameToSaveTo = this->localPath + distantFilename;
    else
        this->fileNameToSaveTo = this->localPath + localFilename;


    //m_WebCtrl_DL.get(request);

    WebCtrl_DL->get(QNetworkRequest( QUrl(distantDLUrl + distantFilename) ));

    qDebug() << "download called with url " << distantDLUrl + distantFilename << "is to be recorded to " << this->fileNameToSaveTo;
}




void NetworkHandler::fileDownloaded(QNetworkReply* pReply)
{
    //qDebug() << "=== entering fileDownloaded()";

    bool success = false;

    if (pReply->error()==QNetworkReply::NoError)
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

        success = true;
    }

    // actually, setting a small timer allows the state machine to work properly
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(_NetworkHandler_Sync_WaitTimer);
    loop.exec();

    if (success)
    {
        qDebug() << "    -- no error";
        emit downloaded();
    }
    else
    {
        qDebug() << "    -- error :(";
        emit downloadFailed();
    }
}




void NetworkHandler::UploadFile(const QString& path, const QString& distantFilename)
{
    qDebug() << "attempting to upload file " << path;

    QString distPath = distantFilename;
    if (distPath == "")
        distPath = path;

    // fileNameToSaveTo = dest;
    QUrl ulUrl = QUrl(distantULUrl + distPath);
    ulUrl.setPassword(ULPassword);
    ulUrl.setUserName(ULUserName);

    m_UploadData.setFileName(localPath + path);
    m_UploadData.open(QIODevice::ReadOnly);

    WebCtrl_UL->put(QNetworkRequest(ulUrl), &m_UploadData);
}





void NetworkHandler::fileUploaded(QNetworkReply* pReply)
{
    //qDebug() << "=== entering fileUploaded()";

    bool success = false;

    if (pReply->error()==QNetworkReply::NoError)
    {
        pReply->close();
        success = true;
    }

    m_UploadData.close();

    // actually, setting a small timer allows the state machine to work properly
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(_NetworkHandler_Sync_WaitTimer);
    loop.exec();

    if (success)
    {
        qDebug() << "    -- success";
        emit uploaded();
    }
    else
    {
        qDebug() << "    -- error :(";
        emit uploadFailed();
    }
}



void NetworkHandler::softSync()
{
    if (this->NetworkConfLoaded)
    {
        qDebug() << "----";
        qDebug() << "Starting the SOFT synchronization process...";

        this->synchronize();
    }
}



void NetworkHandler::hardSync()
{
    if (this->NetworkConfLoaded)
    {
        qDebug() << "----";
        qDebug() << "Starting the HARD synchronization process...";

        this->synchronize(false, true);
    }
}


bool NetworkHandler::networkSyncEquals(const _NetworkHandler_SyncEntry& nsa, const _NetworkHandler_SyncEntry& nsb) const
{
    return (nsa.Author==nsb.Author && nsa.DateTime==nsb.DateTime && nsa.ImageFileName==nsb.ImageFileName && nsa.FilenamePostfix==nsb.FilenamePostfix);
}



void NetworkHandler::synchronize(bool firstCheck, bool hardSync)
{
    // QString fCQStr =  (firstCheck) ? " first check mode ON" : "";
    // QString hSQStr =  (hardSync) ? " hard sync mode ON" : "";
    // qDebug() << "entering synchronize()" << fCQStr << hSQStr;


    firstCheckMode = firstCheck;
    hardSyncMode = hardSync;

    if (!this->NetworkConfLoaded)
    {
        qDebug() << "No network configuration was loaded";
        return;
    }

    // first thing we need to do is to download the sync data file
    connect( this, SIGNAL(downloaded()), this, SLOT(syncDetermineQueues()) );

    this->DownloadFile(this->syncDataFilename);

    return;

}


void NetworkHandler::syncDetermineQueues()
{

    // qDebug() << "entering syncDetermineQueues()";


    CurrentSyncStep = _NHSyncStep_DetermineQueues;


    // when we're there, we've just downloaded the new Sync csv file
    disconnect(this, SIGNAL(downloaded()), this, SLOT(syncDetermineQueues()));

    if (!this->loadSyncDataFile())
        // no need to go further, there's an issue here
        return;


    this->annotationsDownloadQueue.clear();
    this->annotationsDownloadPos = 0;
    this->annotationsUploadQueue.clear();
    this->annotationsUploadPos = 0;
    this->downloadSimpleList.clear();
    this->downloadListPos = 0;

    if (firstCheckMode)
    {
        QtCvUtils::generatePath( QString(localPath + this->imgRelativePath).toStdString().c_str() );
        QtCvUtils::generatePath( QString(localPath + this->annotsRelativePath).toStdString().c_str() );


        // at first, synchronize the images
        for (size_t k=0; k<this->ImagesFileList.size(); k++)
        {
            if (!QtCvUtils::fileExists( QString(this->localPath + this->imgRelativePath + this->ImagesFileList[k]).toStdString() ))
            {

                // if we lack some images, we have never been synchronized, so we'll need to check everything anyway
                //  distantCheck = true;

                downloadSimpleList.push_back(this->imgRelativePath + this->ImagesFileList[k]);

                // this->ImagesSyncList[k] = _NHSS_Synchronized;
            }
        }
    }


    newEntries = false;
    newLocalEntries = false;


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

            currEntry.ImageIndex = k;


            // now we want to determine if we have to download the distant file
            bool doWeDownload =    !QtCvUtils::fileExists( QString(this->localPath + localPathWithoutExtension + this->annotsPostfixYaml).toStdString() )
                                || !QtCvUtils::fileExists( QString(this->localPath + localPathWithoutExtension + this->annotsPostfixCsv ).toStdString() );

            // if we cannot find the corresponding file on the local system : we have to download the file

            // another case is HardSync. If we're in hard sync mode and the file exists, it should be recorded locally somewhere
            // in this case, if the entry is different, then: we have to download it

            if (hardSyncMode)
            {
                if (this->LocalSyncEntries[k].size()==0)
                    doWeDownload = true;
                else if (!this->networkSyncEquals(LocalSyncEntries[k][0],currEntry))
                    doWeDownload = true;
            }

            // if we download it, we also save it
            if (doWeDownload)
            {
                currEntry.csvDone = false;

                this->annotationsDownloadQueue.push_back(currEntry);

                newLocalEntries = true;
            }

            // one last case (mostly when it's a first synchronization : there's no entry because the
        }
        else if (hardSyncMode || firstCheckMode)
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
                newEntry.ImageIndex = k;
                newEntry.csvDone = false;

                this->annotationsUploadQueue.push_back(newEntry);
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


        for (size_t IId=0; IId<this->ImagesFileList.size(); IId++)
        {
            // inserting the new entry directly in place
            // actually, it might be possible to not start from 0 but the last tested index since everytime the file is recorded it should be ordered properly...
            if (this->ImagesFileList[IId] == QString::fromStdString(newEntry.ImageFileName))
            {
                newEntry.ImageIndex = IId;
            }
        }

        newEntry.csvDone = false;

        annotationsUploadQueue.push_back(newEntry);

    }

    /*
    qDebug() << "list of image files to download :";
    for (size_t i=0; i<this->downloadSimpleList.size(); i++)
        qDebug() << this->downloadSimpleList[i];

    qDebug() << "list of annotation files to download : ";
    for (size_t i=0; i<this->annotationsDownloadQueue.size(); i++)
        qDebug() << QString::fromStdString(this->annotationsDownloadQueue[i].ImageFileName + this->annotationsDownloadQueue[i].FilenamePostfix);

    qDebug() << "list of annotation files to upload : ";
    for (size_t i=0; i<this->annotationsUploadQueue.size(); i++)
        qDebug() << QString::fromStdString(this->annotationsUploadQueue[i].ImageFileName + this->annotationsUploadQueue[i].FilenamePostfix);
    */

    // there's no chance that we're doing any new first check, unless it's specified by loading a new configuration file
    firstCheckMode = false;


    // now going into "processing the queues" mode
    disconnect( this, SIGNAL(downloaded()), this, SLOT(syncDetermineQueues()) );


    // now go to the next step
    emit queuesDetermined();
}




void NetworkHandler::syncProcessImagesQueue()
{
    // qDebug() << "Entering syncProcessImagesQueue()";

    if (CurrentSyncStep!=_NHSyncStep_ImagesDL)
    {
        // handle signals and slots connections locally seems much more efficient to me
        connect( this, SIGNAL(downloaded()), this, SLOT(syncProcessImagesQueue()) );
        connect( this, SIGNAL(downloadFailed()), this, SLOT(syncProcessImagesQueue()) );
        CurrentSyncStep = _NHSyncStep_ImagesDL;
    }

    if (this->downloadListPos < static_cast<int>(this->downloadSimpleList.size()))
    {
        // qDebug() << ". Image downloading section()";

        QString fileToDL = this->downloadSimpleList[this->downloadListPos];

        // process to download the next file in queue
        this->downloadListPos++;
        this->DownloadFile(fileToDL);
    }
    else
    {
        disconnect( this, SIGNAL(downloaded()), this, SLOT(syncProcessImagesQueue()) );
        disconnect( this, SIGNAL(downloadFailed()), this, SLOT(syncProcessImagesQueue()) );

        emit imagesQueueProcessed();
    }
}



void NetworkHandler::syncProcessAnnotsDLQueue()
{
    // qDebug() << "Entering syncProcessAnnotsDLQueue()";

    if (CurrentSyncStep!=_NHSyncStep_AnnotsDL)
    {
        // qDebug() << ".setting the signals and stuff";

        // handle signals and slots connections locally seems much more efficient to me
        connect( this, SIGNAL(downloaded()), this, SLOT(recordAnnotationDownload()) );
        connect( this, SIGNAL(downloadFailed()), this, SLOT(syncProcessAnnotsDLQueue()) );
        connect( this, SIGNAL(annotationDownloadRecorded()), this, SLOT(syncProcessAnnotsDLQueue()) );
        CurrentSyncStep = _NHSyncStep_AnnotsDL;
    }

    if (this->annotationsDownloadPos < static_cast<int>(this->annotationsDownloadQueue.size()))
    {
        // qDebug() << ". Annotations downloading section";

        // be careful about signals
        currentlyDownloadedEntry = this->annotationsDownloadQueue[this->annotationsDownloadPos];
        QString localPathWithoutExtension = this->annotsRelativePath + QString::fromStdString(currentlyDownloadedEntry.ImageFileName);
        QString distantPathWithoutExtension = localPathWithoutExtension + QString::fromStdString(currentlyDownloadedEntry.FilenamePostfix);

        this->DownloadFile(distantPathWithoutExtension + this->annotsPostfixCsv, localPathWithoutExtension + this->annotsPostfixCsv);
        this->annotationsDownloadPos++;
    }
    else
    {
        disconnect( this, SIGNAL(downloaded()), this, SLOT(recordAnnotationDownload()) );
        disconnect( this, SIGNAL(downloadFailed()), this, SLOT(syncProcessAnnotsDLQueue()) );
        disconnect( this, SIGNAL(annotationDownloadRecorded()), this, SLOT(syncProcessAnnotsDLQueue()) );

        emit annotsDownloadQueueProcessed();
    }

}


void NetworkHandler::recordAnnotationDownload()
{
    // qDebug() << "recordAnnotationDownload() called";

    if (!currentlyDownloadedEntry.csvDone)
    {
        // qDebug() << " csv part";

        // if we're here, that's because we've just downloaded the csv file
        QString localPathWithoutExtension = this->annotsRelativePath + QString::fromStdString(currentlyDownloadedEntry.ImageFileName);
        QString distantPathWithoutExtension = localPathWithoutExtension + QString::fromStdString(currentlyDownloadedEntry.FilenamePostfix);

        currentlyDownloadedEntry.csvDone = true;
        this->DownloadFile(distantPathWithoutExtension + this->annotsPostfixYaml, localPathWithoutExtension + this->annotsPostfixYaml);
    }
    else
    {
        // qDebug() << " yaml part";

        // the download went alright, record it
        this->SyncEntries[currentlyDownloadedEntry.ImageIndex].push_back(currentlyDownloadedEntry);
        this->LocalSyncEntries[currentlyDownloadedEntry.ImageIndex].clear();
        this->LocalSyncEntries[currentlyDownloadedEntry.ImageIndex].push_back(currentlyDownloadedEntry);
        newLocalEntries = true;


        emit annotationDownloadRecorded();
    }
}




void NetworkHandler::syncProcessAnnotsULQueue()
{
    // qDebug() << "Entering syncProcessAnnotsULQueue()";

    if (CurrentSyncStep!=_NHSyncStep_AnnotsUL)
    {
        // qDebug() << ".setting the signals and stuff";

        // handle signals and slots connections locally seems much more efficient to me
        connect( this, SIGNAL(uploaded()), this, SLOT(recordAnnotationUpload()) );
        connect( this, SIGNAL(uploadFailed()), this, SLOT(syncProcessAnnotsULQueue()) );
        connect( this, SIGNAL(annotationUploadRecorded()), this, SLOT(syncProcessAnnotsULQueue()) );
        CurrentSyncStep = _NHSyncStep_AnnotsUL;
    }

    if (this->annotationsUploadPos < static_cast<int>(this->annotationsUploadQueue.size()))
    {
        // qDebug() << ". Annotations uploading section";

        currentlyUploadedEntry = this->annotationsUploadQueue[this->annotationsUploadPos];
        QString localPathWithoutExtension = this->annotsRelativePath + QString::fromStdString(currentlyUploadedEntry.ImageFileName);
        QString distantPathWithoutExtension = localPathWithoutExtension + QString::fromStdString(currentlyUploadedEntry.FilenamePostfix);

        this->UploadFile(localPathWithoutExtension + this->annotsPostfixCsv, distantPathWithoutExtension + this->annotsPostfixCsv);
        this->annotationsUploadPos++;
    }
    else
    {
        disconnect( this, SIGNAL(uploaded()), this, SLOT(recordAnnotationUpload()) );
        disconnect( this, SIGNAL(uploadFailed()), this, SLOT(syncProcessAnnotsULQueue()) );
        disconnect( this, SIGNAL(annotationUploadRecorded()), this, SLOT(syncProcessAnnotsULQueue()) );

        emit annotsUploadQueueProcessed();
    }
}


void NetworkHandler::recordAnnotationUpload()
{
    if (!currentlyUploadedEntry.csvDone)
    {
        // qDebug() << " csv part";

        // if we're here, that's because we've just downloaded the csv file
        QString localPathWithoutExtension = this->annotsRelativePath + QString::fromStdString(currentlyUploadedEntry.ImageFileName);
        QString distantPathWithoutExtension = localPathWithoutExtension + QString::fromStdString(currentlyUploadedEntry.FilenamePostfix);

        currentlyUploadedEntry.csvDone = true;
        this->UploadFile(localPathWithoutExtension + this->annotsPostfixYaml, distantPathWithoutExtension + this->annotsPostfixYaml);
    }
    else
    {
        // qDebug() << " yaml part";

        // the download went alright, record it
        this->SyncEntries[currentlyUploadedEntry.ImageIndex].push_back(currentlyUploadedEntry);
        this->LocalSyncEntries[currentlyUploadedEntry.ImageIndex].clear();
        this->LocalSyncEntries[currentlyUploadedEntry.ImageIndex].push_back(currentlyUploadedEntry);

        newLocalEntries = true;
        newEntries = true;

        emit annotationUploadRecorded();
    }
}



void NetworkHandler::syncProcessSave()
{
    // qDebug() << "Entering syncProcessSave()";

    this->CurrentSyncStep = _NHSyncStep_Recording;

    if (this->newLocalEntries || this->newEntries)
    {
        // record the
        if (!this->recordSyncDataFile(!this->newEntries))
            qDebug() << "having troubles saving the sync data file";
    }
    if (this->newEntries)
    {
        connect(this, SIGNAL(uploaded()), this, SLOT(syncProcessEnd()));
        this->UploadFile(this->syncDataFilename);
    }

    this->syncProcessEnd();
}




void NetworkHandler::syncProcessEnd()
{
    qDebug() << "Entering syncProcessEnd()";

    disconnect(this, SIGNAL(uploaded()), this, SLOT(syncProcessEnd()));

    this->KnownNewAnnotations.clear();

    QString fCQStr =  (firstCheckMode) ? " - first check mode ON" : "";
    QString hSQStr =  (hardSyncMode) ? "in HARD sync mode" : "in SOFT sync mode";
    qDebug() << "----";
    qDebug() << "Ended the synchronization process" << hSQStr << fCQStr;
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




void NetworkHandler::loadNetworkConfiguration(const QString& configFilePath)
{
    this->NetworkConfLoaded = false;

    this->ImagesFileList.clear();
    this->SyncEntries.clear();
    this->LocalSyncEntries.clear();


    // just a json / yaml loader, leave it as simple as possible
    this->localPath = QtCvUtils::getDirectory(configFilePath);

    cv::FileStorage fsR(configFilePath.toStdString().c_str(), cv::FileStorage::READ);

    if (!fsR.isOpened())
        return;

    if (fsR[_NetworkHandler_YAMLKey_NetworkConfigurationNode].empty())
        return;

    cv::FileNode NetworkConfigFnd = fsR[_NetworkHandler_YAMLKey_NetworkConfigurationNode];

    std::string StdStrDistantDLUrl,
                StdStrDistantULUrl, StdStrULUserName, StdStrULPassword,
                StdStrImgRelativePath, StdStrAnnotsRelativePath, StdStrAnnotsPostfixYaml, StdStrAnnotsPostfixCsv,
                StdStrAuthor, StdStrSyncDataFile, StdStrLocalSyncDataFile;

    // we will consider that every field was entered correctly in the json file...
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

    // dumb conversion stuff, should have coded it better
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

    this->synchronize(true);
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
                currEntry.ImageIndex = k;
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
                    currEntry.ImageIndex = k;
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
            qDebug() << "line " << k << "," << j << " : " << QString::fromStdString(this->SyncEntries[k][j].ImageFileName) << " - " <<
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





