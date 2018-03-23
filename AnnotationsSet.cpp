#include "AnnotationsSet.h"


using namespace std;
using namespace cv;












// FileStorage stuff, required for the iterators, declared here in order to avoid multiple declarations and warnings in the .h file
// -- they shall be used only in the .cpp file anyway

static void write(cv::FileStorage& fs, const std::string&, const AnnotationsProperties& x)
{
    x.write(fs);
}

static void read(const cv::FileNode& node, AnnotationsProperties& x, const AnnotationsProperties& default_value = AnnotationsProperties())
{
    if (node.empty())
        x = default_value;
    else
        x.read(node);
}


static void write(cv::FileStorage& fs, const std::string&, const AnnotationObject& x)
{
    x.write(fs);
}

static void read(const cv::FileNode& node, AnnotationObject& x, const AnnotationObject& default_value = AnnotationObject())
{
    if (node.empty())
        x = default_value;
    else
        x.read(node);
}

















AnnotationsConfig::AnnotationsConfig(const AnnotationsConfig& ac)
{
    this->propsSet = ac.getPropsSet();
    this->imageFileNamingRule = ac.getImageFileNamingRule();
    this->summaryFileNamingRule = ac.getSummaryFileNamingRule();
}


void AnnotationsConfig::setDefaultConfig()
{
    // filling some basic configuration stuff
    AnnotationsProperties prop;
    prop.className = "Road"; prop.classType = _ACT_Uniform; prop.displayRGBColor=Vec3b(0, 85, 85); prop.minIdBGRRecRange=Vec3b(127, 127, 0);
    this->addProperty(prop);
    prop.className = "Car"; prop.classType = _ACT_MultipleObjects; prop.displayRGBColor=Vec3b(255, 0, 0); prop.minIdBGRRecRange=Vec3b(0, 0, 127); prop.maxIdBGRRecRange=Vec3b(255, 255, 255);
    this->addProperty(prop);
    prop.className = "Truck"; prop.classType = _ACT_MultipleObjects; prop.displayRGBColor=Vec3b(170, 0, 0); prop.minIdBGRRecRange=Vec3b(0, 0, 120); prop.maxIdBGRRecRange=Vec3b(255, 255, 120);
    this->addProperty(prop);
    prop.className = "Utility"; prop.classType = _ACT_MultipleObjects; prop.displayRGBColor=Vec3b(85, 0, 0); prop.minIdBGRRecRange=Vec3b(0, 0, 0); prop.maxIdBGRRecRange=Vec3b(255, 255, 0);
    this->addProperty(prop);
    prop.className = "BB Only"; prop.classType = _ACT_BoundingBoxOnly; prop.displayRGBColor=Vec3b(85, 0, 255); prop.minIdBGRRecRange=Vec3b(0, 0, 0); prop.maxIdBGRRecRange=Vec3b(255, 255, 0);
    this->addProperty(prop);

    this->imageFileNamingRule = _AnnotationsConfig_FileNamingToken_OrigImgPath + _AnnotationsConfig_FileNamingToken_OrigImgFileName + "_annotations/" + _AnnotationsConfig_FileNamingToken_FrameNumber + ".png";
    this->summaryFileNamingRule = _AnnotationsConfig_FileNamingToken_OrigImgPath + _AnnotationsConfig_FileNamingToken_OrigImgFileName + "_annotations/summary.yaml";
}




string AnnotationsConfig::getAnnotatedImageFileName(const std::string& origImgPath, const std::string& origImgFileName, int frameNumber) const
{
    string ret = this->imageFileNamingRule;
    AnnotationUtilities::strReplace(ret, _AnnotationsConfig_FileNamingToken_OrigImgPath, origImgPath);
    AnnotationUtilities::strReplace(ret, _AnnotationsConfig_FileNamingToken_OrigImgFileName, origImgFileName);

    // do the zero filling thing for the frame number string
    string frameNbStr = to_string(frameNumber);
    while (frameNbStr.length()<_AnnotationsConfig_FileNamingToken_FrameNumberZeroFillLength)
        frameNbStr = "0" + frameNbStr;
    AnnotationUtilities::strReplace(ret, _AnnotationsConfig_FileNamingToken_FrameNumber, frameNbStr);

    return ret;
}


string AnnotationsConfig::getSummaryFileName(const std::string& origImgPath, const std::string& origImgFileName) const
{
    // there should be no frame number in the summary file name..
    string ret = this->summaryFileNamingRule;
    AnnotationUtilities::strReplace(ret, _AnnotationsConfig_FileNamingToken_OrigImgPath, origImgPath);
    AnnotationUtilities::strReplace(ret, _AnnotationsConfig_FileNamingToken_OrigImgFileName, origImgFileName);
    return ret;
}


void AnnotationsConfig::writeContentToYaml(cv::FileStorage& fs) const
{
    fs << _AnnotsConfig_YAMLKey_Node << "{";

    fs << _AnnotsConfig_YAMLKey_ImageFileNamingRule << this->imageFileNamingRule;
    fs << _AnnotsConfig_YAMLKey_SummaryFileNamingRule << this->summaryFileNamingRule;

    fs << _AnnotsConfig_YAMLKey_ClassesDefs_Node << "[";
    for (size_t k=0; k<this->propsSet.size(); k++)
    {
        fs << this->propsSet[k];
    }
    fs << "]";

    fs << "}";
}



void AnnotationsConfig::readContentFromYaml(const cv::FileNode& fnd)
{
    if (fnd[_AnnotsConfig_YAMLKey_Node].empty())
        return;


    FileNode currNode = fnd[_AnnotsConfig_YAMLKey_Node];

    currNode[_AnnotsConfig_YAMLKey_ImageFileNamingRule] >> this->imageFileNamingRule;
    currNode[_AnnotsConfig_YAMLKey_SummaryFileNamingRule] >> this->summaryFileNamingRule;

    // now reading the properties
    this->propsSet.clear();

    // grab the right filenode and start the iterator
    FileNode nodeIt = currNode[_AnnotsConfig_YAMLKey_ClassesDefs_Node];
    FileNodeIterator it = nodeIt.begin(), it_end = nodeIt.end();
    for (; it != it_end; ++it)
    {
        AnnotationsProperties props;
        (*it) >> props;
        this->propsSet.push_back(props);
    }
}













AnnotationsRecord::AnnotationsRecord()
{
    // nothing to be done there, all of the vectors will be created without any help
}

AnnotationsRecord::~AnnotationsRecord()
{
    // same as for the constructor
}

const std::vector<AnnotationObject>& AnnotationsRecord::getRecord() const
{
    // access the entire record
    return this->record;
}

const vector<int>& AnnotationsRecord::getAnnotationIds(int classId, int objectId) const
{
    // retrieve the index of an object

    // generate an empty vector, that will be returned when no data corresponds to the request
    static vector<int> emptyVec;

    // don't forget that the classId index starts from 1...
    if (classId<1 || classId>(int)this->objectsIndex.size())
        return emptyVec;

    if (objectId<0 || objectId>=(int)this->objectsIndex[classId-1].size())
        return emptyVec;

    return this->objectsIndex[classId-1][objectId];
}

const std::vector<int>& AnnotationsRecord::getFrameContentIds(int id) const
{
    // get all the IDs inside a frame

    // generate an empty vector, that will be returned when no data corresponds to the request
    static vector<int> emptyVec;

    if (id<0 || id>=(int)this->framesIndex.size())
        return emptyVec;

    return this->framesIndex[id];
}

const AnnotationObject& AnnotationsRecord::getAnnotationById(int id) const
{
    // retrieve a specific object by his ID

    // generate an empty object to be returned when no answer is possible
    static AnnotationObject emptyAnnot; // its classId shall be 0 (by default), which should allow us to know whether there's data or not

    if (id<0 || id>=(int)this->record.size())
        return emptyAnnot;

    return this->record[id];
}

int AnnotationsRecord::addNewAnnotation(const AnnotationObject& annot)
{
    // push a new annotation object, at the end of the main vector - returns the new annotation index... if it is not an update!!

    // verify that this object is possible
    if ((annot.ClassId<1) || (annot.ObjectId<0) || (annot.FrameNumber<0))
        return -1;

    // first, we must verify whether this is actually an update, or a brand new annotation
    int searchRes = this->searchAnnotation(annot.FrameNumber, annot.ClassId, annot.ObjectId);
    if (searchRes != -1)
    {
        // this addition is actually an update, we update the bounding box using the | operator
        this->updateBoundingBox(searchRes, (this->getAnnotationById(searchRes).BoundingBox | annot.BoundingBox) );

        // that's it, we're done
        return searchRes;
    }

    // otherwise... creation of a new annotation


    // storing the new record index
    int newInd = this->record.size();

    // recording the annotation
    this->record.push_back(annot);

    // filling the framesIndex matrix if needed with empty vectors
    while((int)this->framesIndex.size()<=annot.FrameNumber)
        this->framesIndex.push_back(vector<int>());

    // recording the index into the frames record
    this->framesIndex[annot.FrameNumber].push_back(newInd);

    // filling the class matrix with empty vectors if needed
    while((int)this->objectsIndex.size()<annot.ClassId)
        this->objectsIndex.push_back(vector< vector<int> >());

    // now the corresponding objects index...
    while((int)this->objectsIndex[annot.ClassId-1].size()<=annot.ObjectId)
        this->objectsIndex[annot.ClassId-1].push_back(vector<int>());

    // finally, storing the index into the objects record...
    this->objectsIndex[annot.ClassId-1][annot.ObjectId].push_back(newInd);

    return newInd;
}



int AnnotationsRecord::getFirstAvailableObjectId(int classId) const
{
    // returns the first available object Id given a class id

    // perform some tests first
    if (classId < 1)
        return -1;  // such value is not supposed to exist...

    // is there anything recorded for this class yet?
    if ((int)this->objectsIndex.size() < classId)
        return 0; // answer : no -> we set the object Id to 0

    // explore and find the first available object id...
    // basically the right one will be when we find an empty vector of objectsIndex[classId-1]
    // if there is none, the right answer is just the size of objectsIndex[classId-1]
    int rightObjectId = 0;
    while ( (rightObjectId < (int)this->objectsIndex[classId-1].size())
            && (this->objectsIndex[classId-1][rightObjectId].size()>0) )
        rightObjectId++;

    // that's all, when we're out of the loop we've reached the right spot
    return rightObjectId;
}


int AnnotationsRecord::searchAnnotation(int frameId, int classId, int objectId) const
{
    /*
     * this is an old version, probably sub-optimal when looking at big videos with a lot of frames
     * and some objects that are visible throughout the video
     * It is likely more optimal to search the frame index
    // find the right object through the objectsIndex vector
    if (classId>=1 && classId<=(int)this->objectsIndex.size())
    {
        if (objectId>=0 && objectId<(int)this->objectsIndex[classId-1].size())
        {
            for (size_t k=0; k<this->objectsIndex[classId-1][objectId].size(); k++)
            {
                if (this->getAnnotationById(this->objectsIndex[classId-1][objectId][k]).FrameNumber == frameId)
                    return this->objectsIndex[classId-1][objectId][k];
            }
        }
    }
    */

    // look at the frames record..
    if (frameId>=0 && frameId<(int)this->framesIndex.size())
    {
        for (size_t k=0; k<this->framesIndex[frameId].size(); k++)
        {
            int candidateInd = this->framesIndex[frameId][k];

            if (this->getAnnotationById(candidateInd).ClassId==classId && this->getAnnotationById(candidateInd).ObjectId==objectId)
                return candidateInd;
        }
    }

    return -1;
}


void AnnotationsRecord::updateBoundingBox(int annotationIndex, const cv::Rect2i newBB)
{
    // edit a bounding box given the object ID in the record vector

    // this is the most straightforward thing that we can hope for
    if (annotationIndex<0 || annotationIndex>=(int)this->record.size())
        return;

    this->record[annotationIndex].BoundingBox = newBB;
}



void AnnotationsRecord::removeAnnotation(int annotationIndex)
{
    // remove an annotation given its id in the record vector

    // perform the usual checks
    if (annotationIndex<0 || annotationIndex>=(int)this->record.size())
        return;

    // we do have something that we can remove
    AnnotationObject oldAnnot = this->record[annotationIndex];

    // remove the data
    this->record.erase(this->record.begin() + annotationIndex);

    // remove the references...
    // ... into the frames index first
    for (size_t k=0; k<this->framesIndex[oldAnnot.FrameNumber].size(); k++)
    {
        if (this->framesIndex[oldAnnot.FrameNumber][k] == annotationIndex)
        {
            this->framesIndex[oldAnnot.FrameNumber].erase(this->framesIndex[oldAnnot.FrameNumber].begin()+k);
            break;
        }
    }

    // ... then into the objects index
    for (size_t k=0; k<this->objectsIndex[oldAnnot.ClassId-1][oldAnnot.ObjectId].size(); k++)
    {
        if (this->objectsIndex[oldAnnot.ClassId-1][oldAnnot.ObjectId][k] == annotationIndex)
        {
            this->objectsIndex[oldAnnot.ClassId-1][oldAnnot.ObjectId].erase(this->objectsIndex[oldAnnot.ClassId-1][oldAnnot.ObjectId].begin()+k);
            break;
        }
    }

    // finally correct the reference IDs

    // we run through the record object starting from the point where we removed the annot,
    // then decrement all of the references pointing to the previous annotations positions
    for (int i=annotationIndex; i<(int)this->record.size(); i++)
    {
        AnnotationObject currObject = this->record[i];

        for (size_t k=0; k<this->framesIndex[currObject.FrameNumber].size(); k++)
        {
            if (this->framesIndex[currObject.FrameNumber][k] == i+1)
            {
                this->framesIndex[currObject.FrameNumber][k] = i;
                break;
            }
        }

        // ... then into the objects index
        for (size_t k=0; k<this->objectsIndex[currObject.ClassId-1][currObject.ObjectId].size(); k++)
        {
            if (this->objectsIndex[currObject.ClassId-1][currObject.ObjectId][k] == i+1)
            {
                this->objectsIndex[currObject.ClassId-1][currObject.ObjectId][k] = i;
                break;
            }
        }
    }

    // that's it, we're done :)
}



void AnnotationsRecord::clearFrame(int frameId)
{
    // remove all of the objects included in a given frame

    // copy-store the indexes since the remaining of this method will impact the original vector
    vector<int> objectsToRemove = this->getFrameContentIds(frameId);

    // sort the indexes, so that we can start from the end
    // this way, we ensure that the remaining indices that will be left won't be affected by any other removal
    sort(objectsToRemove.begin(), objectsToRemove.end());

    // do the job
    for (int k=(int)objectsToRemove.size()-1; k>=0; k--)
        this->removeAnnotation(objectsToRemove[k]);

    // that's it, we're done
}




void AnnotationsRecord::mergeIntraFrameAnnotationsTo(const std::vector<int>& annotsList, int newClassId, int newObjectId)
{
    // set newClassId and newObjectId to the first element in annotsList
    // update its bounding box so that it includes all of the bounding box of the annotsList objects
    // this function doesn't delete

    if (annotsList.size()<1)
        return;

    if (newClassId<0)   // this case doesn't make any sense, discard it as well
        return;

    int firstAnnotId = annotsList[0];

    if (firstAnnotId<0 || firstAnnotId>=(int)this->record.size())
        return;


    // storing currAnnot as a reference - this way we shall(?) access directly to the object and modify it straight away
    AnnotationObject& currAnnot = this->record[firstAnnotId];

    // if the new class and/or object id are different, we need tu update the indexing accordingly
    if ((newClassId != currAnnot.ClassId) || (newObjectId != currAnnot.ObjectId))
    {
        // we also need to modify the indexation
        // for now, we suppose that it's ok and that there's no incoherence in the indexation
        for (size_t k=0; k<this->objectsIndex[currAnnot.ClassId-1][currAnnot.ObjectId].size(); k++)
        {
            // find the reference position and remove it
            if (this->objectsIndex[currAnnot.ClassId-1][currAnnot.ObjectId][k] == firstAnnotId)
            {
                this->objectsIndex[currAnnot.ClassId-1][currAnnot.ObjectId].erase(this->objectsIndex[currAnnot.ClassId-1][currAnnot.ObjectId].begin()+k);
                break;
            }
        }

        // store the new reference index at the right place
        // filling the class matrix with empty vectors if needed
        while((int)this->objectsIndex.size()<newClassId)
            this->objectsIndex.push_back(vector< vector<int> >());

        // now the corresponding objects index...
        while((int)this->objectsIndex[newClassId-1].size()<=newObjectId)
            this->objectsIndex[newClassId-1].push_back(vector<int>());

        // finally, storing the index into the objects record...
        this->objectsIndex[newClassId-1][newObjectId].push_back(firstAnnotId);

        // updating the reference
        currAnnot.ClassId = newClassId;
        currAnnot.ObjectId = newObjectId;
    }

    // running through the list to update the bounding box
    for (size_t k=1; k<annotsList.size(); k++)
    {
        // in theory, such check is unnecessary?
        if (annotsList[k]<0 || annotsList[k]>=(int)this->record.size())
            continue;

        if (this->record[annotsList[k]].FrameNumber != currAnnot.FrameNumber)
            // the operation that we want to perform is relevant only if we're working on the same frame
            continue;

        // we will just use the bounding boxes - the objects are to be erased later
        currAnnot.BoundingBox |= this->record[annotsList[k]].BoundingBox;
    }
}


void AnnotationsRecord::deleteAnnotationsGroup(const std::vector<int>& deleteList)
{
    vector<int> deleteListCopy = deleteList;

    sort(deleteListCopy.begin(), deleteListCopy.end());

    for (int k=(int)deleteListCopy.size()-1; k>=0; k--)
    {
        this->removeAnnotation(deleteListCopy[k]);
    }
}




vector<int> AnnotationsRecord::separateAnnotations(const std::vector<int>& separateList)
{
    // first step : find objects with similar class and object id - those are the objects that we will need to separate
    vector<Point2i> listObjects;
    vector<int> separateListCopy;

    vector<int> idsReturn;

    for (int k=0; k<(int)separateList.size(); k++)
    {
        // some safety check
        if (separateList[k]<0 || separateList[k]>=(int)this->record.size())
            continue;

        listObjects.push_back( Point2i(this->getAnnotationById(separateList[k]).ClassId, this->getAnnotationById(separateList[k]).ObjectId) );
        separateListCopy.push_back(separateList[k]);
    }

    // sort the objects
    vector<size_t> orderedObjects = AnnotationUtilities::sortP2iIndexes(listObjects);

    Point2i currObjIds(0,0); // setting an impossible set of object ids as a starting point

    for (size_t k=0; k<orderedObjects.size(); k++)
    {
        if (listObjects[orderedObjects[k]] != currObjIds)
        {
            // new object found - we just store the current object Ids - it is the other case that'll be interesting
            currObjIds = listObjects[orderedObjects[k]];
        }
        else
        {
            // same object as before - we need to provide it a new object id given its class
            int newObjId = this->getFirstAvailableObjectId(currObjIds.x);

            int oldObjId = listObjects[orderedObjects[k]].y;
            int recordId = separateListCopy[orderedObjects[k]];
            // fortunately this id won't have to change - it also means that we won't have to touch the frames record

            // record the new object id
            this->record[recordId].ObjectId = newObjId;

            // removing the old object reference
            // for once, we don't need to check that there's already a corresponding vector entry
            for (size_t l=0; l<this->objectsIndex[currObjIds.x-1][oldObjId].size(); l++)
            {
                if (this->objectsIndex[currObjIds.x-1][oldObjId][l] == recordId)
                {
                    this->objectsIndex[currObjIds.x-1][oldObjId].erase(this->objectsIndex[currObjIds.x-1][oldObjId].begin() + l);
                    break;
                }
            }

            // now adding the new one
            while ((int)this->objectsIndex[currObjIds.x-1].size()<=newObjId)
                this->objectsIndex[currObjIds.x-1].push_back(vector<int>());

            this->objectsIndex[currObjIds.x-1][newObjId].push_back(recordId);

            // finally we remember that this record entry has been modified
            idsReturn.push_back(recordId);
        }
    }

    return idsReturn;
}







void AnnotationsRecord::clear()
{
    // simply clean all the vectors
    this->record.clear();
    this->objectsIndex.clear();
    this->framesIndex.clear();
}







void AnnotationsRecord::writeContentToYaml(cv::FileStorage& fs) const
{
    fs << _AnnotsRecord_YAMLKey_Node << "[";
    for (size_t k=0; k<this->record.size(); k++)
    {
        fs << this->record[k];
    }
    fs << "]";
}



void AnnotationsRecord::readContentFromYaml(const cv::FileNode& fnd)
{
    // verify that there's a corresponding node into the file
    if (fnd[_AnnotsRecord_YAMLKey_Node].empty())
        return;

    // remove all data
    this->clear();

    // grab the right filenode
    FileNode nodeIt = fnd[_AnnotsRecord_YAMLKey_Node];

    // start the iterator
    FileNodeIterator it = nodeIt.begin(), it_end = nodeIt.end();

    // read the data
    int idx = 0;
    for (; it != it_end; ++it, idx++)
    {
        AnnotationObject annot;
        (*it) >> annot;

        if (annot.ClassId==0)   // that should be impossible?
            continue;

        this->record.push_back(annot);

        // filling the framesIndex matrix if needed with empty vectors
        while((int)this->framesIndex.size()<=annot.FrameNumber)
            this->framesIndex.push_back(vector<int>());

        // recording the index into the frames record
        this->framesIndex[annot.FrameNumber].push_back(idx);

        // filling the class matrix with empty vectors if needed
        while((int)this->objectsIndex.size()<annot.ClassId)
            this->objectsIndex.push_back(vector< vector<int> >());

        // now the corresponding objects index...
        while((int)this->objectsIndex[annot.ClassId-1].size()<=annot.ObjectId)
            this->objectsIndex[annot.ClassId-1].push_back(vector<int>());

        // finally, storing the index into the objects record...
        this->objectsIndex[annot.ClassId-1][annot.ObjectId].push_back(idx);
    }
}


/*
void AnnotationsRecord::lockObject(int id)
{
    if (id<0 || id>=(int)this->record.size())
        return;

    this->record[id].locked = true;
}



void AnnotationsRecord::unlockObject(int id)
{
    if (id<0 || id>=(int)this->record.size())
        return;

    this->record[id].locked = false;
}
*/















AnnotationsSet::AnnotationsSet()
{
    this->setDefaultConfig();
}

AnnotationsSet::~AnnotationsSet()
{

}


void AnnotationsSet::setDefaultConfig()
{
    // setting up the usual stuff...
    this->currentImgIndex = 0;
    this->bufferLength = _AnnotationsSet_default_bufferLength;
    this->changesPerformedUponCurrentAnnot = false;

    // initialization of the buffer
    this->originalImagesBuffer = vector<Mat>(this->bufferLength, Mat());
    this->annotationsClassesBuffer = vector<Mat>(this->bufferLength, Mat());
    this->annotationsIdsBuffer = vector<Mat>(this->bufferLength, Mat());
    this->annotationsContoursBuffer = vector<Mat>(this->bufferLength, Mat());
}


/*
void AnnotationsSet::addClassProperty(const AnnotationsProperties& ap)
{
    // ad the class property - pretty simple
    this->config.addProperty(ap);
}
*/



const cv::Mat& AnnotationsSet::getCurrentOriginalImg() const
{
    return this->getOriginalImg(this->currentImgIndex);
}

const cv::Mat& AnnotationsSet::getCurrentAnnotationsClasses() const
{
    return this->getAnnotationsClasses(this->currentImgIndex);
}

const cv::Mat& AnnotationsSet::getCurrentAnnotationsIds() const
{
    return this->getAnnotationsIds(this->currentImgIndex);
}

const cv::Mat& AnnotationsSet::getCurrentContours() const
{
    return this->getContours(this->currentImgIndex);
}




const cv::Mat& AnnotationsSet::getOriginalImg(int id) const
{
    return this->originalImagesBuffer[id % this->bufferLength];
}

const cv::Mat& AnnotationsSet::getAnnotationsClasses(int id) const
{
    return this->annotationsClassesBuffer[id % this->bufferLength];
}

const cv::Mat& AnnotationsSet::getAnnotationsIds(int id) const
{
    return this->annotationsIdsBuffer[id % this->bufferLength];
}

const cv::Mat& AnnotationsSet::getContours(int id) const
{
    return this->annotationsContoursBuffer[id % this->bufferLength];
}





int AnnotationsSet::getObjectIdAtPosition(int x, int y) const
{
    if (AnnotationUtilities::isThisPointWithinImageBoundaries(cv::Point2i(x,y), this->getCurrentOriginalImg()))
    {
        int classId = this->getCurrentAnnotationsClasses().at<int16_t>(y,x);
        int objectId = this->getCurrentAnnotationsIds().at<int32_t>(y,x);

        return this->annotsRecord.searchAnnotation(this->currentImgIndex, classId, objectId);
    }

    return -1;
}



int AnnotationsSet::getClosestBBFromPosition(int x, int y, int searchingWindowRadius) const
{
    // find the closest Bounding Box from the Bounding Box Only objects

    const vector<int>& listObjects = this->annotsRecord.getFrameContentIds(this->currentImgIndex);
    // look for the current frame objects

    int minDist = searchingWindowRadius + 1;
    int foundId = -1;

    for (size_t k=0; k<listObjects.size(); k++)
    {
        if (this->config.getProperty(this->annotsRecord.getAnnotationById(listObjects[k]).ClassId).classType != _ACT_BoundingBoxOnly)
            // we care only about BB only classes in this situation
            continue;

        // find the distance between the bounding box and the object
        const Rect2i& currBB = this->annotsRecord.getAnnotationById(listObjects[k]).BoundingBox;

        // min distance to a vertical and a horizontal line
        int minDistH = (abs(currBB.tl().x-x) < abs(currBB.br().x-1-x)) ? abs(currBB.tl().x-x) : abs(currBB.br().x-1-x);
        int minDistV = (abs(currBB.tl().y-y) < abs(currBB.br().y-1-y)) ? abs(currBB.tl().y-y) : abs(currBB.br().y-1-y);

        // determining whether we're within the range of the object
        bool withinHRange = (x > (currBB.tl().x-searchingWindowRadius)) && (x < (currBB.br().x+searchingWindowRadius));
        bool withinVRange = (y > (currBB.tl().y-searchingWindowRadius)) && (y < (currBB.br().y+searchingWindowRadius));

        if (    ((minDistH>searchingWindowRadius) && !withinHRange)
             || ((minDistV>searchingWindowRadius) && !withinVRange) )
            // one of the distances is greater than the searching window radius
            // and we're well outside of the boundaries of the object - don't go further
            continue;

        // only keep the smallest amongst them
        int minDistHandV = (minDistH < minDistV) ? minDistH : minDistV;

        // found the closest possible object - update
        if (minDistHandV < minDist)
        {
            foundId = listObjects[k];
            minDist = minDistHandV;
        }
    }

    return foundId;
}











cv::Mat& AnnotationsSet::accessCurrentAnnotationsClasses()
{
    return this->annotationsClassesBuffer[this->currentImgIndex % this->bufferLength];
}

cv::Mat& AnnotationsSet::accessCurrentAnnotationsIds()
{
    return this->annotationsIdsBuffer[this->currentImgIndex % this->bufferLength];
}

cv::Mat& AnnotationsSet::accessCurrentContours()
{
    return this->annotationsContoursBuffer[this->currentImgIndex % this->bufferLength];
}












bool AnnotationsSet::loadOriginalImage(const std::string& imgFileName)
{
    // load the file
    Mat im = imread(imgFileName);

    // couldn't read the file?
    if (!im.data)
        return false;

    // alright, we can use it

    // store the filename and path
    std::string::size_type slashPos = imgFileName.find_last_of('/');
    this->imageFilePath = imgFileName.substr(0, slashPos+1);
    this->imageFileName = imgFileName.substr(slashPos+1);
    this->videoFileName = "";

    // first : init (back?) all the buffers
    this->currentImgIndex = 0;
    this->maxImgReached = 0;

    for (size_t k=0; k<this->originalImagesBuffer.size(); k++)
    {
        this->originalImagesBuffer[k].release();
        this->annotationsClassesBuffer[k].release();
        this->annotationsIdsBuffer[k].release();
        this->annotationsContoursBuffer[k].release();
    }

    // then fill the data correctly
    im.copyTo(this->originalImagesBuffer[0]);
    this->annotationsClassesBuffer[0] = Mat::zeros(im.size(), CV_16SC1);
    this->annotationsIdsBuffer[0] = Mat::zeros(im.size(), CV_32SC1);
    this->annotationsContoursBuffer[0] = Mat::zeros(im.size(), CV_8UC1);

    // try to load a previously recorded annotation, if it exists
    // if (this->loadCurrentAnnotationImage())
        // cout << "we were able to load a previous annotation" << endl;
    this->loadCurrentAnnotationImage();

    // we have loaded a new frame - specify that nothing's changed
    this->changesPerformedUponCurrentAnnot = false;

    return true;
}



bool AnnotationsSet::loadOriginalVideo(const std::string& videoFileName)
{
    this->vidCap.release();

    if (!this->vidCap.open(videoFileName))
        return false;

    Mat im;
    if (!this->vidCap.read(im) || (!im.data))
    {
        this->vidCap.release();
        return false;
    }

    this->reachedTheEndOfVideo = false;

    // alright, we were able to grab the first frame
    std::string::size_type slashPos = videoFileName.find_last_of('/');
    this->imageFilePath = videoFileName.substr(0, slashPos+1);
    this->videoFileName = videoFileName.substr(slashPos+1);
    this->imageFileName = "";

    // init (back?) all the buffers
    this->currentImgIndex = 0;
    this->maxImgReached = 0;

    for (size_t k=0; k<this->originalImagesBuffer.size(); k++)
    {
        this->originalImagesBuffer[k].release();
        this->annotationsClassesBuffer[k].release();
        this->annotationsIdsBuffer[k].release();
        this->annotationsContoursBuffer[k].release();
    }

    // then fill the data correctly
    im.copyTo(this->originalImagesBuffer[0]);
    this->annotationsClassesBuffer[0] = Mat::zeros(im.size(), CV_16SC1);
    this->annotationsIdsBuffer[0] = Mat::zeros(im.size(), CV_32SC1);
    this->annotationsContoursBuffer[0] = Mat::zeros(im.size(), CV_8UC1);

    // try to load a previously recorded annotation, if it exists
    //if (this->loadCurrentAnnotationImage())
    //    cout << "we were able to load a previous annotation" << endl;
    this->loadCurrentAnnotationImage();

    // we have loaded a new frame - specify that nothing's changed
    this->changesPerformedUponCurrentAnnot = false;

    return true;
}







bool AnnotationsSet::loadNextFrame()
{
    // save what we were doing until now
    this->saveCurrentState("", true);

    // verify that we're indeed reading a video
    if (!this->vidCap.isOpened())
        return false;

    // verify where we're at regarding the buffers
    if (this->currentImgIndex >= this->maxImgReached)
    {
        // the displayed image is the last one that was ever read

        // try to load the image
        Mat im;
        if (!this->vidCap.read(im))
        {
            this->reachedTheEndOfVideo = true;
            return false;
        }

        if (!im.data)
            return false;

        // alright, we could read the frame, now update the indexes and the buffers

        // init (back?) all the buffers
        this->currentImgIndex++;
        this->maxImgReached = this->currentImgIndex;

        int bufferIndex = this->currentImgIndex % this->bufferLength;

        im.copyTo(this->originalImagesBuffer[bufferIndex]);
        this->annotationsClassesBuffer[bufferIndex] = Mat::zeros(im.size(), CV_16SC1);
        this->annotationsIdsBuffer[bufferIndex] = Mat::zeros(im.size(), CV_32SC1);
        this->annotationsContoursBuffer[bufferIndex] = Mat::zeros(im.size(), CV_8UC1);

        // try to load a previously recorded annotation, if it exists
        //if (this->loadCurrentAnnotationImage())
            //cout << "we were able to load a previous annotation" << endl;
        this->loadCurrentAnnotationImage();

        // we have loaded a new frame - specify that nothing's changed
        this->changesPerformedUponCurrentAnnot = false;

        return true;
    }
    else if (this->maxImgReached-this->currentImgIndex < this->bufferLength)
    {
        // we're inside the buffer boundaries
        this->currentImgIndex++;

        // we have loaded a new frame - specify that nothing's changed
        this->changesPerformedUponCurrentAnnot = false;

        // it should be all??? quite surprised, actually....
        return true;
    }

    // the case when we're outside of the buffer boundaries is not supported yet
    return false;
}





bool AnnotationsSet::loadFrame(int fId)
{
    // save what we were doing until now
    this->saveCurrentState("", true);

    // verify that we're indeed reading a video
    if (!this->vidCap.isOpened())
        return false;

    if (fId<0)  // shouldn't happen, but we better stay safe
        return false;

    // is this a previous frame?
    if ((fId<=this->currentImgIndex) && (this->maxImgReached-fId < this->bufferLength))
    {
        // we can load the required frame
        this->currentImgIndex = fId;

        // we have loaded a new frame - specify that nothing's changed
        this->changesPerformedUponCurrentAnnot = false;

        return true;
    }
    else if (fId>this->currentImgIndex)
    {
        // trying to read forward the video
        // we only read frames and we don't compute anything if we're out of the buffer boundaries
        Mat imThrash;
        for (; this->currentImgIndex<fId-this->bufferLength; this->currentImgIndex++)
            if (!this->vidCap.read(imThrash))
                return false;

        while (!this->reachedTheEndOfVideo && fId>this->currentImgIndex)
        {
            if (!this->loadNextFrame())  // we load every frame normally because we need to update the buffer
                return false;
        }

        if (fId == this->currentImgIndex)
        {
            // we have loaded a new frame - specify that nothing's changed
            this->changesPerformedUponCurrentAnnot = false;

            return true;
        }
    }
    else
    {
        // last case : we're out of the buffer and it isn't a case where we're supposed to read forward - which means that we want to frames before the buffer

        // i've witnessed a lot of situations where the frames set counter isn't working properly...
        // so i'm doing it "the hardcore way" : loading the video back and reading all the frames until i reach the buffer again
        if (!this->loadOriginalVideo(this->imageFilePath + this->videoFileName))
            return false;

        // read the video until we rich the start of the buffer again
        Mat imThrash;
        for (; this->currentImgIndex<fId-this->bufferLength; this->currentImgIndex++)
            if (!this->vidCap.read(imThrash))
                return false;

        // finally read and fill the buffers correctly
        while (!this->reachedTheEndOfVideo && fId>this->currentImgIndex)
        {
            if (!this->loadNextFrame())
                return false;
        }

        // we have loaded a new frame - specify that nothing's changed
        this->changesPerformedUponCurrentAnnot = false;

        return true;
    }

    return false;
}




void AnnotationsSet::closeFile(bool pleaseSave)
{
    if (pleaseSave)
        this->saveCurrentState();

    if (this->isVideoOpen())
        this->vidCap.release();

    for (int i=0; i<this->bufferLength; i++)
    {
        this->originalImagesBuffer[i].release();
        this->annotationsClassesBuffer[i].release();
        this->annotationsIdsBuffer[i].release();
        this->annotationsContoursBuffer[i].release();
    }

    this->annotsRecord.clear();

    this->currentImgIndex = 0;
}






bool AnnotationsSet::isVideoOpen() const
{
    return this->vidCap.isOpened();
}

bool AnnotationsSet::isImageOpen() const
{
    // simply tell whether an image is open - an image and NOT a video
    return (!this->vidCap.isOpened() && this->getCurrentOriginalImg().data);
}

bool AnnotationsSet::canReadNextFrame() const
{
    // a video must have been opened and we don't want to be at the end of the video
    return ( this->isVideoOpen() && (this->currentImgIndex<this->maxImgReached || !this->reachedTheEndOfVideo) );
}

bool AnnotationsSet::canReadPrevFrame() const
{
    // a video has been opened, we don't want to be at the very beginning of the video and we don't want to be at the lowest end of the buffer
    return (this->isVideoOpen() && (this->currentImgIndex>0) && ((this->maxImgReached-this->currentImgIndex+1)<this->bufferLength));
}


string AnnotationsSet::getDefaultAnnotationsSaveFileName() const
{
    return this->config.getSummaryFileName(this->imageFilePath, (this->isVideoOpen() ? this->videoFileName : this->imageFileName) );
}

string AnnotationsSet::getDefaultCurrentImageSaveFileName() const
{
    return this->config.getAnnotatedImageFileName(this->imageFilePath, (this->isVideoOpen() ? this->videoFileName : this->imageFileName) , this->currentImgIndex);
}







bool AnnotationsSet::loadAnnotations(const std::string& annotationsFileName)
{
    // open the file
    FileStorage fsR(annotationsFileName, FileStorage::READ);

    if (!fsR.isOpened())
        return false;

    FileNode globalAnnotsFnd = fsR[_AnnotationsSet_YAMLKey_Node];

    if (globalAnnotsFnd.empty())
        return false;

    globalAnnotsFnd[_AnnotationsSet_YAMLKey_FilePath] >> this->imageFilePath;
    globalAnnotsFnd[_AnnotationsSet_YAMLKey_ImageFileName] >> this->imageFileName;
    globalAnnotsFnd[_AnnotationsSet_YAMLKey_VideoFileName] >> this->videoFileName;

    // load the configuration before anything else
    this->config.readContentFromYaml(globalAnnotsFnd);

    // then read the record
    this->annotsRecord.readContentFromYaml(globalAnnotsFnd);

    // finally load the video or the image, depending on the case
    if (this->imageFileName.length()>1)
        return this->loadOriginalImage(this->imageFilePath + this->imageFileName);
    else if (this->videoFileName.length()>1)
        return this->loadOriginalVideo(this->imageFilePath + this->videoFileName);

    return false;
}






bool AnnotationsSet::loadConfiguration(const std::string& configFileName)
{
    // open the file
    FileStorage fsR(configFileName, FileStorage::READ);

    if (!fsR.isOpened())
        return false;

    FileNode globalConfigFnd = fsR[_AnnotationsSet_YAMLKey_Node];

    if (globalConfigFnd.empty())
        return false;


    // load the configuration before anything else
    this->config.readContentFromYaml(globalConfigFnd);

    return true;
}








bool AnnotationsSet::saveCurrentState(const std::string& forceFileName, bool saveOnlyIfNecessary)
{
    if (saveOnlyIfNecessary && !this->changesPerformedUponCurrentAnnot)
        return true;

    // the place where we're going to save the current frame annotation file
    string savingFileName = forceFileName;
    if (savingFileName.length()<2)
    {
        if (this->isImageOpen())
            savingFileName = this->config.getSummaryFileName(this->imageFilePath, this->imageFileName);
        else if (this->isVideoOpen())
            savingFileName = this->config.getSummaryFileName(this->imageFilePath, this->videoFileName);
    }

    // verify that we can indeed store the file where we intend to store it
    QtCvUtils::generatePath(savingFileName);

    // open the file
    FileStorage fs(savingFileName, FileStorage::WRITE);

    // add a node, since loading is way more complicated when we don't add a base node
    fs << _AnnotationsSet_YAMLKey_Node << "{";

    fs << _AnnotationsSet_YAMLKey_FilePath << this->imageFilePath;
    fs << _AnnotationsSet_YAMLKey_ImageFileName << this->imageFileName;
    fs << _AnnotationsSet_YAMLKey_VideoFileName << this->videoFileName;

    fs << _AnnotationsSet_YAMLKey_RecordingTimeDate << QtCvUtils::getDateTimeStr();


    // save the configuration
    this->config.writeContentToYaml(fs);

    // do the actual record
    this->annotsRecord.writeContentToYaml(fs);

    // close the AnnotationsSet node
    fs << "}";
    fs.release();

    // now store the current image
    if (!this->saveCurrentAnnotationImage())
        return false;

    // specify that we've recorded the changes
    this->changesPerformedUponCurrentAnnot = false;

    return true;
}




bool AnnotationsSet::saveCurrentAnnotationImage(const std::string& forcedFileName) const
{
    // the place where we're going to save the current frame annotation file
    string savingFileName = forcedFileName;
    if (savingFileName.length()<2)
    {
        if (this->isImageOpen())
            savingFileName = this->config.getAnnotatedImageFileName(this->imageFilePath, this->imageFileName, this->currentImgIndex);
        else if (this->isVideoOpen())
            savingFileName = this->config.getAnnotatedImageFileName(this->imageFilePath, this->videoFileName, this->currentImgIndex);
    }

    // find out how to associate a class and a color in the finally recorded annotation image
    vector<Point2i> colorsIndexSrc;
    vector<Vec3b> colorsIndexDst;
    //unordered_map<Point2i, Vec3b> colorsIndex;

    // this is the list of objects present in the image
    vector<int> presentObjects = this->annotsRecord.getFrameContentIds(this->currentImgIndex);

    // compute the associations
    for (size_t k=0; k<presentObjects.size(); k++)
    {
        AnnotationObject currentAnnotCaracs = this->annotsRecord.getAnnotationById(presentObjects[k]);


        // don't bother with this part if the class is Bounding Boxes Only
        if (this->config.getProperty(currentAnnotCaracs.ClassId).classType == _ACT_BoundingBoxOnly)
            continue;


        // the way we store each possible occurrence uniquely is with the help of a Point2i
        Point2i objectRef(currentAnnotCaracs.ClassId, currentAnnotCaracs.ObjectId);

        Vec3b currentAnnotColor;

        // retrieve the annotation properties
        AnnotationsProperties currentAnnotProps = this->config.getProperty(currentAnnotCaracs.ClassId);

        // easiest case : it's an uniform class
        if (currentAnnotProps.classType == _ACT_Uniform)
            currentAnnotColor = currentAnnotProps.minIdBGRRecRange;
        else if (currentAnnotProps.classType == _ACT_MultipleObjects)
        {
            // that's the difficult case now...
            // for now we will make it simple
            Vec3b minVal = currentAnnotProps.minIdBGRRecRange;
            Vec3b maxVal = currentAnnotProps.maxIdBGRRecRange;

            Vec3b availableRange = maxVal - minVal; // every byte should be positive since we assume that max >= min..!

            /*
            vector< vector<Vec3b> > encodingMultipliers;
            vector<int> leftMostBit;
            for (size_t i=0; i<3; i++)
            {
                uchar range = availableRange[i];
                int currBitValue = 1;
                encodingMultipliers.push_back(vector<Vec3b>());
                leftMostBit.push_back(0);
                while (range>=currBitValue)
                {
                    leftMostBit[i]++;
                    Vec3b val(0,0,0);
                    val[i] = currBitValue;
                    encodingMultipliers[i].push_back(val);
                    currBitValue*=2;
                }
            }

            // now re-ordering the stuff
            // int currColorIndex = 0;
            vector<Vec3b> multipliers;
            vector<int> components;

            for (int desiredBitPosition=8; desiredBitPosition>0; desiredBitPosition--)
            {
                for (int idx=0; idx<3; idx++)
                {
                    if (leftMostBit[idx]==desiredBitPosition)
                    {
                        components.push_back(idx);
                        multipliers.push_back(encodingMultipliers[idx][leftMostBit[idx]-1]);
                        leftMostBit[idx]--;
                    }
                }
            }
            */


            /*
            std::cout << "multipliers content : " << std::endl;
            for (size_t i=0; i<multipliers.size(); i++)
            {
                std::cout << i << ". " << multipliers[i] << std::endl;
            }
            std::cout << "=========" << std::endl;
            */

            /*
            // now fill
            Vec3b bytesContentExperiment(0,0,0);
            int objIdRemaining = currentAnnotCaracs.ObjectId;
            int currMultipliersInd = 0;
            while ((objIdRemaining>0) && (currMultipliersInd<(int)multipliers.size()))
            {
                int currComp = components[currMultipliersInd];
                int bitMult = objIdRemaining % 2;
                int testVal = (int)bytesContentExperiment[currComp] + (bitMult * multipliers[currMultipliersInd][currComp]);
                if (testVal<=availableRange[currComp])
                {
                    // we can record this bit
                    bytesContentExperiment += bitMult * multipliers[currMultipliersInd];
                    objIdRemaining /= 2;

                }
                currMultipliersInd++;
            }

            std::cout << "-------" << std::endl;
            std::cout << "final color value for objectId " << currentAnnotCaracs.ObjectId << " of class " << currentAnnotCaracs.ClassId << " : " << bytesContentExperiment << std::endl;
            std::cout << "=======" << std::endl;
            */

            Vec3b bytesContent;
            int currObjIdRemaining = currentAnnotCaracs.ObjectId;
            for (size_t i=0; i<3; i++)
            {
                // +1 is there because the max value is to be included
                // anyway, x%1 == 0 and x/1 == x
                bytesContent[i] = (uchar) currObjIdRemaining % (availableRange[i]+1);
                currObjIdRemaining /= (availableRange[i]+1);
            }

            // fill the color
            for (size_t i=0; i<3; i++)
            {
                currentAnnotColor[i] = bytesContent[i] + minVal[i];
            }
        }

        // finally store the association
        colorsIndexSrc.push_back(objectRef);
        colorsIndexDst.push_back(currentAnnotColor);
        //colorsIndex[objectRef] = currentAnnotColor;
    }


    // generate a new image
    Mat imgToStore = Mat::zeros(this->getCurrentOriginalImg().size(), CV_8UC3);

    for (int i=0; i<imgToStore.rows; i++)
    {
        for (int j=0; j<imgToStore.cols; j++)
        {
            // is there anything to display?
            if (this->getCurrentAnnotationsClasses().at<int16_t>(i,j)>0)
            {
                // yes : find the right color
                Point2i pointVals(this->getCurrentAnnotationsClasses().at<int16_t>(i,j), this->getCurrentAnnotationsIds().at<int32_t>(i,j));

                // run through the index...
                for (size_t k=0; k<colorsIndexSrc.size(); k++)
                {
                    // this is sub-optimal, but we will eventually do something better using idunnowhat
                    if (pointVals == colorsIndexSrc[k])
                    {
                        imgToStore.at<Vec3b>(i,j) = colorsIndexDst[k];
                        break;
                    }
                }

                //imgToStore.at<Vec3b>(i,j) = colorsIndex[pointVals];
            }
        }
    }

    return QtCvUtils::imwrite(savingFileName, imgToStore);
}






bool AnnotationsSet::loadCurrentAnnotationImage()
{
    // loads an already annotated image. Starts with the idea that both the class and the objectId matrices were already filled with 0s

    // the place where we're going to save the current frame annotation file
    string loadingFileName;

    if (this->isImageOpen())
        loadingFileName = this->config.getAnnotatedImageFileName(this->imageFilePath, this->imageFileName, this->currentImgIndex);
    else if (this->isVideoOpen())
        loadingFileName = this->config.getAnnotatedImageFileName(this->imageFilePath, this->videoFileName, this->currentImgIndex);



    // try to load the image, the color format is mandatory
    Mat imLoad = imread(loadingFileName, CV_LOAD_IMAGE_COLOR);

    if (!imLoad.data)
        return false;

    // verify that the dimensions are compliant with our data format
    if (imLoad.size() != this->getCurrentAnnotationsClasses().size())
        return false;

    // record the minColorIndex and the maxColorIndex, it is faster
    vector<Vec3b> minColorIndex, maxColorIndex;
    vector<Vec3i> multipliersIndex;
    vector<bool> classUniform;

    for (int i=0; i<this->config.getPropsNumber(); i++)
    {
        classUniform.push_back(this->config.getProperty(i+1).classType==_ACT_Uniform);

        if (this->config.getProperty(i+1).classType == _ACT_BoundingBoxOnly)
        {
            // give an impossible color so that it is not taken into account
            minColorIndex.push_back(Vec3b(0,0,0));
            maxColorIndex.push_back(Vec3b(0,0,0));
            multipliersIndex.push_back(Vec3i(1,1,1));
            continue;
        }

        minColorIndex.push_back(this->config.getProperty(i+1).minIdBGRRecRange);

        // warning : there's an exception when the class is uniform
        if (classUniform[i])
            maxColorIndex.push_back(minColorIndex[i]);
        else
            maxColorIndex.push_back(this->config.getProperty(i+1).maxIdBGRRecRange);

        // already calculate the multipliers
        multipliersIndex.push_back( Vec3i(1, (maxColorIndex[i][0]-minColorIndex[i][0]+1), (maxColorIndex[i][0]-minColorIndex[i][0]+1)*(maxColorIndex[i][1]-minColorIndex[i][1]+1)) );
    }



    // we also want to feed the record data, in case it's not compliant with what's recorded

    // so we're recording what we're observing
    vector<Point2i> observedObjectsList;
    vector<Rect2i> observedObjectsBBs;


    // read the image content...
    for (int i=0; i<imLoad.rows; i++)
    {
        for (int j=0; j<imLoad.cols; j++)
        {
            Vec3b pxValue = imLoad.at<Vec3b>(i,j);

            if (pxValue != Vec3b(0,0,0))    // there is something there..
            {
                Point2i currPointIds(0,0);

                for (int k=0; k<(int)minColorIndex.size(); k++)
                {
                    if ( (pxValue[0]>=minColorIndex[k][0]) && (pxValue[1]>=minColorIndex[k][1]) && (pxValue[2]>=minColorIndex[k][2])
                         && (pxValue[0]<=maxColorIndex[k][0]) && (pxValue[1]<=maxColorIndex[k][1]) && (pxValue[2]<=maxColorIndex[k][2]) )
                    {
                        // we belong to this class
                        this->accessCurrentAnnotationsClasses().at<int16_t>(i,j) = k+1;

                        currPointIds.x = k+1;

                        // if the class uniform, we don't need to do anything else
                        // if not, however, we need to decode the pixels information
                        if (!classUniform[k])
                        {
                            // this is just a matter of multiplication, nothing fancy here
                            int objInd = (multipliersIndex[k][0] * (pxValue[0]-minColorIndex[k][0]))
                                       + (multipliersIndex[k][1] * (pxValue[1]-minColorIndex[k][1]))
                                       + (multipliersIndex[k][2] * (pxValue[2]-minColorIndex[k][2]));

                            this->accessCurrentAnnotationsIds().at<int32_t>(i,j) = objInd;
                            currPointIds.y = objInd;
                        }

                        // there's no need to go any further, we know already the right class and object id
                        break;
                    }
                }


                if (currPointIds == Point2i(0,0))
                    std::cout << "something fishy happened there" << std::endl;


                // storing the object and/or updating the bounding box
                size_t k = 0;
                for (k=0; k<observedObjectsList.size(); k++)
                {
                    if (observedObjectsList[k] == currPointIds)
                    {
                        // we've already recorded this point, we just update the bounding box
                        observedObjectsBBs[k] |= Rect2i(Point2i(j,i), Size2i(1,1));
                        break;
                    }
                }

                // if the object hasn't been observed yet...
                if (k==observedObjectsList.size())
                {
                    // we add the new object
                    observedObjectsList.push_back(currPointIds);
                    observedObjectsBBs.push_back( Rect2i(Point2i(j,i), Size2i(1,1)) );
                }

            }
        }
    }


    // update the contours image
    if (observedObjectsBBs.size()>0)
    {
        // updating it is only useful if there is at least one object!
        Rect2i contoursROI = observedObjectsBBs[0];
        for (size_t k=1; k<observedObjectsBBs.size(); k++)
            contoursROI |= observedObjectsBBs[k];

        this->computeFrameContours(this->currentImgIndex, contoursROI);
    }







    // now checking and/or updating the compliance with the annotations record

    // first, perform some sorting... it's better to view the data in a slightly better arrangement
    vector<size_t> orderedObsOvjList = AnnotationUtilities::sortP2iIndexes(observedObjectsList);

    // now verify whether it's in the record, and if not, add it
    for (size_t kOrd=0; kOrd<orderedObsOvjList.size(); kOrd++)
    {
        size_t k = orderedObsOvjList[kOrd];

        // search the object within the record
        int recordedInd = this->annotsRecord.searchAnnotation(this->currentImgIndex, observedObjectsList[k].x, observedObjectsList[k].y);

        if (recordedInd == -1)
        {
            // couldn't find it - create it, then
            AnnotationObject annObj;
            annObj.FrameNumber = this->currentImgIndex;
            annObj.ClassId = observedObjectsList[k].x;
            annObj.ObjectId = observedObjectsList[k].y;
            annObj.BoundingBox = observedObjectsBBs[k];
            this->annotsRecord.addNewAnnotation(annObj);
        }
        else
        {
            // update the bounding boxes - probably useless, but sin'ce we've gone this far anyway...
            this->annotsRecord.updateBoundingBox(recordedInd, observedObjectsBBs[k]);
        }
    }


    return true;
}









void AnnotationsSet::editAnnotationBoundingBox(int recordId, const Rect2i& newBB)
{
    this->annotsRecord.updateBoundingBox( recordId, newBB & Rect2i(Point2i(0,0), this->getCurrentOriginalImg().size()) );

    this->changesPerformedUponCurrentAnnot = true;
}




void AnnotationsSet::interpolateLastBoundingBoxes(int interpolateRecordLength)
{
    int startingFrame = this->currentImgIndex-interpolateRecordLength;  // this is the frame on which we start the interpolation
    if (startingFrame<0) startingFrame = 0; // some safety net

    int actualInterpLength = this->currentImgIndex - startingFrame;
    if (actualInterpLength<2)   // doesn't make any sense to continue, we won't have anything to interpolate
        return;

    //const vector<int>& startingFrameObjects = this->annotsRecord.getFrameContentIds(startingFrame);
    //const vector<int>& endingFrameObjects = this->annotsRecord.getFrameContentIds(this->currentImgIndex);

    // now going through the middle frames
    for (int i=startingFrame+1; i<this->currentImgIndex; i++)
    {
        float interpFactorEnd = (float)(i-startingFrame) / (float)actualInterpLength;
        float interpFactorStart = 1. - interpFactorEnd;

        // now looking at all the BB Only objects within the frame
        const vector<int>& currFrameObjs = this->annotsRecord.getFrameContentIds(i);

        for (size_t k=0; k<currFrameObjs.size(); k++)
        {
            // storing the current object properties
            const AnnotationObject& objCharacs = this->annotsRecord.getAnnotationById(currFrameObjs[k]);

            // we care only about the bounding box only objects
            if (this->config.getProperty(objCharacs.ClassId).classType != _ACT_BoundingBoxOnly)
                continue;

            // verify if this particular object has both a counterpart on the starting frame and the ending frame
            int startingObjId = this->annotsRecord.searchAnnotation(startingFrame, objCharacs.ClassId, objCharacs.ObjectId);
            int endingObjId = this->annotsRecord.searchAnnotation(this->currentImgIndex, objCharacs.ClassId, objCharacs.ObjectId);

            // if not, discard
            if (startingObjId==-1 || endingObjId==-1)
                continue;

            // storing the starting and ending BBs
            const Rect2i& startingBB = this->annotsRecord.getAnnotationById(startingObjId).BoundingBox;
            const Rect2i& endingBB = this->annotsRecord.getAnnotationById(endingObjId).BoundingBox;

            // perform the actual interpolation
            Rect2i newBB = Rect2i( Point2i( round( (float)startingBB.tl().x * interpFactorStart + (float)endingBB.tl().x * interpFactorEnd),
                                            round( (float)startingBB.tl().y * interpFactorStart + (float)endingBB.tl().y * interpFactorEnd) ),
                                   Point2i( round( (float)startingBB.br().x * interpFactorStart + (float)endingBB.br().x * interpFactorEnd),
                                            round( (float)startingBB.br().y * interpFactorStart + (float)endingBB.br().y * interpFactorEnd) ) );

            // store the modification
            this->annotsRecord.updateBoundingBox(currFrameObjs[k], newBB);
        }
    }
}




int AnnotationsSet::addAnnotation(const cv::Point2i& topLeftCorner, const cv::Point2i& bottomRightCorner, int whichClass, int forceObjectId)
{
    if (whichClass<1 || whichClass>this->config.getPropsNumber())
        // wrong class number
        return -1;

    if (this->config.getProperty(whichClass).classType != _ACT_BoundingBoxOnly)
        return -1;

    // if this was called, this is necessarily a new annotation
    int objectId = (forceObjectId<0) ? this->annotsRecord.getFirstAvailableObjectId(whichClass) : forceObjectId;

    // store everything correctly
    AnnotationObject newAnnot;
    newAnnot.BoundingBox = cv::Rect2i(topLeftCorner, bottomRightCorner);  // we set +1 to the BR corner because this corner is not inclusive
    newAnnot.BoundingBox &= cv::Rect2i(Point2i(0,0), this->getCurrentOriginalImg().size()); // ensuring that the new annotation is well inside the boundaries of the image

    if (newAnnot.BoundingBox.width==0 || newAnnot.BoundingBox.height==0)
        // it doesn't make sense to record an empty object - we skip it
        return -1;

    newAnnot.ClassId = whichClass;
    newAnnot.ObjectId = objectId;
    newAnnot.FrameNumber = this->currentImgIndex;



    this->changesPerformedUponCurrentAnnot = true;

    // ...and finally, we return the object Id...
    // in case this is only an update, the annotsRecord object should handle it properly
    return this->annotsRecord.addNewAnnotation(newAnnot);
}







int AnnotationsSet::addAnnotation(const cv::Mat& mask, const cv::Point2i& topLeftCorner, int whichClass, const cv::Point2i& annotStartingPoint, int forceObjectId)
{
    // add an annotation.
    // mask is in CV_8UC1 format, whatever is not zero belongs to the object.
    // annotStartingPoint is there to know whether we add this to a previous annotation (in which case we merge both), or whether we start a new one
    // the return index is the index of the ID, which is set automatically by this class

    if (whichClass<1 || whichClass>this->config.getPropsNumber())
        // wrong class number
        return -1;


    // std::cout << "mask content : " << mask << std::endl;


    // finding the right object id

    // first, declare an ID and use the default, which is always right when we have an uniform class
    int objectId = 0;
    bool usedDefaultObjectId = true;

    if (forceObjectId != -1)
    {
        objectId = forceObjectId;
    }
    else if (this->config.getProperty(whichClass).classType == _ACT_MultipleObjects)
    {
        // not an uniform class, we have to look at where the annotation started first :
        // is it already part of an object of the same class?

        // first the safety check
        if (AnnotationUtilities::isThisPointWithinImageBoundaries(annotStartingPoint, this->getCurrentOriginalImg()))
        {
            // is it the same class?
            if (this->getCurrentAnnotationsClasses().at<int16_t>(annotStartingPoint.y, annotStartingPoint.x) == whichClass)
            {
                // it is : we use the same id, this annotation is an increment to a previous one
                objectId = this->getCurrentAnnotationsIds().at<int32_t>(annotStartingPoint.y, annotStartingPoint.x);
                usedDefaultObjectId = false;
            }
        }

        if (usedDefaultObjectId)
        {
            // last possible case : we use the default, we're not modifying any previously recorded object and we're in a multiple objects situation
            objectId = this->annotsRecord.getFirstAvailableObjectId(whichClass);
        }
    }


    // finding the exact bounding box is yet another task we need to perform there.
    // What I have implemented using the Qt interface doesn't always return something at pixel precision,
    // so we take the time to compute this again with accuracy
    int leftMostCoord, rightMostCoord, topMostCoord, bottomMostCoord;

    // giving some "absurd values" to ensure that the min / max things will work well
    leftMostCoord = topLeftCorner.x + mask.cols;
    rightMostCoord = topLeftCorner.x;
    topMostCoord = topLeftCorner.y + mask.rows;
    bottomMostCoord = topLeftCorner.y;



    // also, find out if we modified some previous annotation by replacing some pixels values
    vector<Point2i> affectedObjectsList;
    vector<Rect2i> affectedObjectsBBs;

    Point2i currClassValues(whichClass, objectId);

    // now filling the matrices, and probing the exact boundary coordinates
    for (int i=0; i<mask.rows; i++)
    {
        for (int j=0; j<mask.cols; j++)
        {
            if (mask.at<uchar>(i,j)>0)
            {
                // this pixel was annotated

                // perform some safety check, it shouldn't be necessary but ?!
                if (!AnnotationUtilities::isThisPointWithinImageBoundaries(Point2i(j+topLeftCorner.x, i+topLeftCorner.y), this->getCurrentAnnotationsClasses()))
                    continue;


                // store the pixel into the list of affected objects in case it was a pixel with something on it
                if (this->accessCurrentAnnotationsClasses().at<int16_t>(i+topLeftCorner.y, j+topLeftCorner.x) != _AnnotationsSet_default_classNoneValue)
                {
                    // this Point2i object stores the class and the object id
                    Point2i currObjectIds = Point2i( this->accessCurrentAnnotationsClasses().at<int16_t>(i+topLeftCorner.y, j+topLeftCorner.x),
                                                     this->accessCurrentAnnotationsIds().at<int32_t>(i+topLeftCorner.y, j+topLeftCorner.x) );


                    if ( this->config.getProperty(currObjectIds.x).locked ||
                         this->annotsRecord.getAnnotationById(this->annotsRecord.searchAnnotation(this->currentImgIndex, currObjectIds.x, currObjectIds.y)).locked )
                        // the object or the class is locked - avoid
                        continue;


                    // is it different from the one we're annotating right now?
                    if (currObjectIds != currClassValues)
                    {
                        // check whether it's already in our list of modified objects
                        bool noPreviousExample = true;
                        for (size_t k=0; k<affectedObjectsList.size(); k++)
                        {
                            if (affectedObjectsList[k] == currObjectIds)
                            {
                                // yes - update the bounding box
                                noPreviousExample = false;
                                affectedObjectsBBs[k] |= Rect2i(Point2i(j+topLeftCorner.x, i+topLeftCorner.y), Size2i(1,1));
                                break;
                            }
                        }

                        // no - record that this object was affected by the delete pixels procedure and give it a one-pixel wide BB
                        if (noPreviousExample)
                        {
                            affectedObjectsList.push_back(currObjectIds);
                            affectedObjectsBBs.push_back( Rect2i(Point2i(j+topLeftCorner.x, i+topLeftCorner.y), Size2i(1,1)) );
                        }
                    }
                }


                // update the current object bounding box
                if (j+topLeftCorner.x < leftMostCoord)
                    leftMostCoord = j+topLeftCorner.x;
                if (j+topLeftCorner.x > rightMostCoord)
                    rightMostCoord = j+topLeftCorner.x;
                if (i+topLeftCorner.y < topMostCoord)
                    topMostCoord = i+topLeftCorner.y;
                if (i+topLeftCorner.y > bottomMostCoord)
                    bottomMostCoord = i+topLeftCorner.y;


                // finally filling the pixels with the right values
                this->accessCurrentAnnotationsClasses().at<int16_t>(i+topLeftCorner.y, j+topLeftCorner.x) = whichClass;
                this->accessCurrentAnnotationsIds().at<int32_t>(i+topLeftCorner.y, j+topLeftCorner.x) = objectId;
            }
        }
    }


    // store everything correctly
    AnnotationObject newAnnot;
    newAnnot.BoundingBox = cv::Rect2i(Point2i(leftMostCoord, topMostCoord), Point2i(rightMostCoord+1, bottomMostCoord+1));  // we set +1 to the BR corner because this corner is not inclusive
    newAnnot.ClassId = whichClass;
    newAnnot.ObjectId = objectId;
    newAnnot.FrameNumber = this->currentImgIndex;


    // we're going to compute the contours
    // since the new annot may have affected surrounding objects, we grow the area by 1 pixel in every direction
    Rect2i contoursBB = Rect2i(Point2i(leftMostCoord-1, topMostCoord-1), Point2i(rightMostCoord+2, bottomMostCoord+2));
    this->computeFrameContours(this->currentImgIndex, contoursBB);



    // handle the cases where we have modified previously recorded objects first
    // (since it may modify the main record indexes, we want to do it before assigning an index to the new object)
    this->handleAnnotationsModifications(affectedObjectsList, affectedObjectsBBs);


    // specify that we have changed something to the image
    this->changesPerformedUponCurrentAnnot = true;



    // ...and finally, we return the object Id...
    // in case this is only an update, the annotsRecord object should handle it properly
    return this->annotsRecord.addNewAnnotation(newAnnot);

}








void AnnotationsSet::removePixelsFromAnnotations(const cv::Mat& mask, const cv::Point2i& topLeftCorner)
{
    // remove any class from an annotation at the pixel locations marked with a non-zero mask value.

    // we also have to spend some time to ensure that the record stays coherent with the annotation content


    // first : keep a record of affected annotation objects. We will check later if their bounding boxes were affected
    // ... or if they were erased eventually

    vector<Point2i> affectedObjectsList;
    vector<Rect2i> affectedObjectsBBs;


    // first removing the pixels within the images... this is pretty straightforward...
    for (int i=0; i<mask.rows; i++)
    {
        for (int j=0; j<mask.cols; j++)
        {
            if (mask.at<uchar>(i,j)>0)
            {
                // this pixel was stored as "to erase"

                // perform some safety check, it shouldn't be necessary but ?!
                if (!AnnotationUtilities::isThisPointWithinImageBoundaries(Point2i(j+topLeftCorner.x, i+topLeftCorner.y), this->getCurrentAnnotationsClasses()))
                    continue;

                // store the pixel into the list of affected objects in case it was a pixel with something on it
                if (this->accessCurrentAnnotationsClasses().at<int16_t>(i+topLeftCorner.y, j+topLeftCorner.x) != _AnnotationsSet_default_classNoneValue)
                {
                    Point2i currObjectIds = Point2i( this->accessCurrentAnnotationsClasses().at<int16_t>(i+topLeftCorner.y, j+topLeftCorner.x),
                                                     this->accessCurrentAnnotationsIds().at<int32_t>(i+topLeftCorner.y, j+topLeftCorner.x) );

                    if ( this->config.getProperty(currObjectIds.x).locked ||
                         this->annotsRecord.getAnnotationById(this->annotsRecord.searchAnnotation(this->currentImgIndex, currObjectIds.x, currObjectIds.y)).locked )
                        // the object or the class is locked - avoid
                        continue;

                    bool noPreviousExample = true;
                    for (size_t k=0; k<affectedObjectsList.size(); k++)
                    {
                        if (affectedObjectsList[k] == currObjectIds)
                        {
                            noPreviousExample = false;
                            affectedObjectsBBs[k] |= Rect2i(Point2i(j+topLeftCorner.x, i+topLeftCorner.y), Size2i(1,1));
                            break;
                        }
                    }

                    // record that this object was affected by the delete pixels procedure
                    if (noPreviousExample)
                    {
                        affectedObjectsList.push_back(currObjectIds);
                        affectedObjectsBBs.push_back( Rect2i(Point2i(j+topLeftCorner.x, i+topLeftCorner.y), Size2i(1,1)) );
                    }
                }

                this->accessCurrentAnnotationsClasses().at<int16_t>(i+topLeftCorner.y, j+topLeftCorner.x) = _AnnotationsSet_default_classNoneValue;
                this->accessCurrentAnnotationsIds().at<int32_t>(i+topLeftCorner.y, j+topLeftCorner.x) = _AnnotationsSet_default_classNoneValue; // set this one to 0 as well, not very useful but anyway..
            }
        }
    }


    // specify that we have changed something to the image
    this->changesPerformedUponCurrentAnnot = true;

    // now we should spend some time updating the record - we need to find the new bounding boxes, as well as the cases when an object was completely erased
    this->handleAnnotationsModifications(affectedObjectsList, affectedObjectsBBs);


    // also compute again the contours thing
    // since the new annot may have affected surrounding objects, we grow the area by 1 pixel in every direction
    Rect2i contoursBB = Rect2i(topLeftCorner.x-1, topLeftCorner.y-1, mask.cols+2, mask.rows+2);
    this->computeFrameContours(this->currentImgIndex, contoursBB);
}








void AnnotationsSet::mergeAnnotations(const std::vector<int>& annotationsList)
{
    // determine which classes and which objects we are supposed to merge
    vector<Point2i> listObjects;
    // vector<int> listFrames;

    // use a copy to protect from wrong indices
    vector<int> annotationsListCopy;

    // first run through the annotations that we're supposed to merge
    for (size_t k=0; k<annotationsList.size(); k++)
    {
        if (annotationsList[k]<0 || annotationsList[k]>=(int)this->annotsRecord.getRecord().size())
            continue;

        annotationsListCopy.push_back(annotationsList[k]);
        listObjects.push_back( Point2i(this->annotsRecord.getAnnotationById(annotationsList[k]).ClassId, this->annotsRecord.getAnnotationById(annotationsList[k]).ObjectId) );
        // listFrames.push_back( this->annotsRecord.getAnnotationById(annotationsList[k]).FrameNumber );
    }

    if (annotationsListCopy.size()<2)
        // nothing to do here
        return;

    // get to know which objects are different and which are not
    vector<size_t> orderedInds = AnnotationUtilities::sortP2iIndexes(listObjects);

    Point2i prevObjectIds = Point2i(0,0);   // defining an impossible class Id so that we initiate the loop correctly

    // the aim here is to determine which are the new objects that we are going to create, and from which origins
    // since the indices have been sorted, we are guaranted that the new objects will have the lowest possible ID
    vector<Point2i> newObjectIdsList;
    int currNewObjInd = -1;
    vector<vector<int>> correspondingOrigIdsList;
    vector<vector<int>> correspondingObjIdsList;    // stores the object Ids for a given class


    for (size_t k=0; k<orderedInds.size(); k++)
    {
        Point2i currObjectIds = listObjects[orderedInds[k]];
        if (prevObjectIds.x != currObjectIds.x)
        {
            // we're in the case of a different class - we generate a new family of objects
            newObjectIdsList.push_back(currObjectIds);
            correspondingOrigIdsList.push_back(vector<int>());
            correspondingObjIdsList.push_back(vector<int>());
            currNewObjInd++;
        }

        // record the object into the list, at the correct place
        correspondingOrigIdsList[currNewObjInd].push_back( annotationsListCopy[orderedInds[k]] );
        correspondingObjIdsList[currNewObjInd].push_back(currObjectIds.y);
        prevObjectIds = currObjectIds;
    }



    // we store here a list of objects that we will need to remove at the very end of the procedure
    vector<int> objectsToRemove;



    // now merging each object as required
    for (size_t newObjK=0; newObjK<newObjectIdsList.size(); newObjK++)
    {
        if (correspondingOrigIdsList[newObjK].size()<2)
            // actually, there's no merge here at all
            continue;

        // these are the new Ids of the merged object
        int newObjectClassId = newObjectIdsList[newObjK].x;
        int newObjectObjId = newObjectIdsList[newObjK].y;


        // eliminating doublons
        vector<int> elagatedObjectsList;
        sort(correspondingObjIdsList[newObjK].begin(), correspondingObjIdsList[newObjK].end());
        if (correspondingObjIdsList[newObjK].size()>=1)
        {
            elagatedObjectsList.push_back(correspondingObjIdsList[newObjK][0]);
            size_t currListIdx = 0;
            for (size_t k=1; k<correspondingObjIdsList[newObjK].size(); k++)
            {
                if (correspondingObjIdsList[newObjK][k] != elagatedObjectsList[currListIdx])
                {
                    elagatedObjectsList.push_back(correspondingObjIdsList[newObjK][k]);
                    currListIdx++;
                }
            }
        }


        // now determining intra frame merges
        vector<Point2i> frameAndObjectId;
        for (size_t k=0; k<elagatedObjectsList.size(); k++)
        {
            for (size_t i=0; i<this->annotsRecord.getAnnotationIds(newObjectClassId, elagatedObjectsList[k]).size(); i++)
            {
                int currId = this->annotsRecord.getAnnotationIds(newObjectClassId, elagatedObjectsList[k])[i];
                frameAndObjectId.push_back(Point2i(this->annotsRecord.getAnnotationById(currId).FrameNumber, currId));
            }
        }

        // there should be no doublons in this vector, in theory. I need to confirm that however



        /*
        // determining intra-frame merges
        vector<Point2i> frameAndObjectId;
        for (size_t k=0; k<correspondingOrigIdsList[newObjK].size(); k++)
        {
            int currId = correspondingOrigIdsList[newObjK][k];
            frameAndObjectId.push_back(Point2i(this->annotsRecord.getAnnotationById(currId).FrameNumber, currId));
        }
        */



        // we sort the indexes so that the frame id appears first
        vector<size_t> orderedFramesAndIds = AnnotationUtilities::sortP2iIndexes(frameAndObjectId);

        int prevFrameId = -1;
        // when we see that we're caring about a new frame, we call a function that merges the Ids into
        // the intraFrameMergeIndices vector as an intra frame merge - we give the merged object the ID newObjectObjId of class newObjectClassId
        // we also keep a track of all the objects that are no longer useful into objectsToRemove - this will be called at the end
        vector<int> intraFrameMergeIndices;
        // vector<int> objectsToRemove;

        for (size_t k=0; k<orderedFramesAndIds.size(); k++)
        {
            if (frameAndObjectId[orderedFramesAndIds[k]].x != prevFrameId)
            {
                // we're having a new frame there, store what changes have been recorded until then
                this->mergeIntraFrameAnnotations(newObjectClassId, newObjectObjId, intraFrameMergeIndices);

                // this has been processed - just clear
                intraFrameMergeIndices.clear();

                // store the new frame id
                prevFrameId = frameAndObjectId[orderedFramesAndIds[k]].x;
            }
            else
                // if we're there, it's not a first object. It's supposed to be removed
                objectsToRemove.push_back(frameAndObjectId[orderedFramesAndIds[k]].y);

            // record the object into the list
            intraFrameMergeIndices.push_back(frameAndObjectId[orderedFramesAndIds[k]].y);
        }

        // don't forget that the last recorded frame might contain some stufffff
        this->mergeIntraFrameAnnotations(newObjectClassId, newObjectObjId, intraFrameMergeIndices);
    }

    // finally erase all of the unnecessary objects
    this->deleteAnnotations(objectsToRemove, true);

    // don't forget to state that changes were performed!
    this->changesPerformedUponCurrentAnnot = true;
}






void AnnotationsSet::deleteAnnotations(const std::vector<int>& annotationsList, bool onlyFromRecord)
{
    if (!onlyFromRecord)
    {
        // also remove the annotations from the images

        // first sort the list according to the frame id - this way we handle every frame separately
        vector<Point2i> frameAndIdVec;
        for (size_t k=0; k<annotationsList.size(); k++)
        {
            // const AnnotationObject& currObj = this->annotsRecord.getAnnotationById(annotationsList[k]);
            frameAndIdVec.push_back(Point2i(this->annotsRecord.getAnnotationById(annotationsList[k]).FrameNumber, annotationsList[k]));
        }

        vector<size_t> vecOrder = AnnotationUtilities::sortP2iIndexes(frameAndIdVec);

        // then treat each frame after the other

        // init with an impossible frame number
        int currFrame = -1;
        vector<int> currFrameAnnotList;

        for (size_t k=0; k<vecOrder.size(); k++)
        {
            if (frameAndIdVec[vecOrder[k]].x != currFrame)
            {
                // call the mergeIntraFrameAnnotations procedure.... this works only if the annotations record is still up to date
                // this is something that we need to ensure....?
                this->mergeIntraFrameAnnotations(0, 0, currFrameAnnotList);
                currFrameAnnotList.clear();
            }
            currFrame = frameAndIdVec[vecOrder[k]].x;
            currFrameAnnotList.push_back(frameAndIdVec[vecOrder[k]].y);
        }

        // do the final deletion
        this->mergeIntraFrameAnnotations(0, 0, currFrameAnnotList);
    }

    // finally delete them from the record
    this->annotsRecord.deleteAnnotationsGroup(annotationsList);

    // don't forget to state that changes were performed!
    this->changesPerformedUponCurrentAnnot = true;
}





void AnnotationsSet::clearCurrentFrame()
{
    // nullify everything
    this->accessCurrentAnnotationsClasses() *= 0;
    this->accessCurrentAnnotationsIds() *= 0;
    this->accessCurrentContours() *= 0;

    //store a copy of the list of annotations
    vector<int> deleteIds = this->annotsRecord.getFrameContentIds(this->currentImgIndex);
    this->annotsRecord.deleteAnnotationsGroup(deleteIds);

    // don't forget to state that changes were performed!
    this->changesPerformedUponCurrentAnnot = true;
}







void AnnotationsSet::separateAnnotations(const std::vector<int>& separateList)
{
    // we store the old object ids
    vector<int> separateListPrevObjIds;
    for (size_t k=0; k<separateList.size(); k++)
        separateListPrevObjIds.push_back(this->annotsRecord.getAnnotationById(separateList[k]).ObjectId);


    // for once, we start with the record modification
    vector<int> modifiedObjects = this->annotsRecord.separateAnnotations(separateList);

    // store the pair formed by prev and new object ids
    vector<Point2i> oldAndNewObjectIds;

    // now finding the frames where something happened
    vector<Point2i> modifiedFramesAndObjects;

    for (size_t k=0; k<modifiedObjects.size(); k++)
    {
        modifiedFramesAndObjects.push_back( Point2i(this->annotsRecord.getAnnotationById(modifiedObjects[k]).FrameNumber, modifiedObjects[k]) );

        // finding the old object correspondance, in the original separateList
        // this is in N2 complexity but I assume that the number of elements of separateList will never be so big as
        // this could be a problem...?!?
        for (size_t l=0; l<separateList.size(); l++)
        {
            if (separateList[l] == modifiedObjects[k])  // same ID, same object then
            {
                oldAndNewObjectIds.push_back( Point2i(separateListPrevObjIds[l], this->annotsRecord.getAnnotationById(modifiedObjects[k]).ObjectId) );
                break;
            }
        }
    }


    // now sorting frames and Ids so that we only handle each frame once
    vector<size_t> orderedByFramesIndexes = AnnotationUtilities::sortP2iIndexes(modifiedFramesAndObjects);

    int currFrame = -1;
    Mat classesMat, objIdsMat;

    string imgFileName = (this->isVideoOpen() ? this->videoFileName : this->imageFileName);
    string currannotsFileName;


    for (size_t k=0; k<orderedByFramesIndexes.size(); k++)
    {
        if (modifiedFramesAndObjects[orderedByFramesIndexes[k]].x != currFrame)
        {
            if (classesMat.data)
            {
                // store the modifications that have been performed before
                this->saveAnnotationsImageFile(currannotsFileName, classesMat, objIdsMat);

                // also updating the buffers
                if (currFrame>(this->maxImgReached-this->bufferLength) && currFrame<=this->maxImgReached)
                {
                    classesMat.copyTo(this->annotationsClassesBuffer[currFrame%this->bufferLength]);
                    objIdsMat.copyTo(this->annotationsIdsBuffer[currFrame%this->bufferLength]);
                }
            }

            // this is the frame we're working on
            currFrame = modifiedFramesAndObjects[orderedByFramesIndexes[k]].x;
            currannotsFileName = this->config.getAnnotatedImageFileName(this->imageFilePath, imgFileName, currFrame);

            // is it in the buffer?
            if (currFrame>(this->maxImgReached-this->bufferLength) && currFrame<=this->maxImgReached)
            {
                // yes - just copy the content of the buffers
                this->annotationsClassesBuffer[currFrame%this->bufferLength].copyTo(classesMat);
                this->annotationsIdsBuffer[currFrame%this->bufferLength].copyTo(objIdsMat);
            }
            else
                this->loadAnnotationsImageFile(currannotsFileName, classesMat, objIdsMat);
        }


        // now perform the actual modifications
        const AnnotationObject& currentAnnotObj = this->annotsRecord.getAnnotationById( modifiedFramesAndObjects[orderedByFramesIndexes[k]].y );
        int oldObjId = oldAndNewObjectIds[orderedByFramesIndexes[k]].x;
        int newObjId = oldAndNewObjectIds[orderedByFramesIndexes[k]].y;

        // run through the image and perform the modifications
        for (int i=currentAnnotObj.BoundingBox.tl().y; i<currentAnnotObj.BoundingBox.br().y; i++)
        {
            for (int j=currentAnnotObj.BoundingBox.tl().x; j<currentAnnotObj.BoundingBox.br().x; j++)
            {
                if ((classesMat.at<int16_t>(i,j)==currentAnnotObj.ClassId) && (objIdsMat.at<int32_t>(i,j)==oldObjId))
                    objIdsMat.at<int32_t>(i,j) = newObjId;
            }
        }
    }

    // don't forget to store the last result that was not store within the loop in case it's needed
    if (classesMat.data)
    {
        // store the modifications that have been performed before
        this->saveAnnotationsImageFile(currannotsFileName, classesMat, objIdsMat);

        // also updating the buffers
        if (currFrame>(this->maxImgReached-this->bufferLength) && currFrame<=this->maxImgReached)
        {
            classesMat.copyTo(this->annotationsClassesBuffer[currFrame%this->bufferLength]);
            objIdsMat.copyTo(this->annotationsIdsBuffer[currFrame%this->bufferLength]);
        }
    }

    // normally, every changes have been recorded already, however it seems more safe to state that changes can have been performed
    this->changesPerformedUponCurrentAnnot = true;

    // pfffouuuhhh... it's over i think
}





void AnnotationsSet::switchAnnotationsToClass(const std::vector<int>& switchList, int classId)
{
    // well, there are 2 rather different cases for this functionnality
    // 1. the class pointed with classId is uniform : this means that we must merge all of the objects
    // in a single frame to only one object, and perhaps even merge it with an already present one
    // 2. the class is made of multiple objects : this case is simpler, we create a new object everytime

    // stores the original data and make a copy of the IDs that we will send to the record
    // in return, the record will provide us back with a list of the new object IDs corresponding to the passed argument vector
    vector<Point2i> frameAndIdSwList;

    for (size_t k=0; k<switchList.size(); k++)
    {
        if (switchList[k]<0 || switchList[k]>=(int)this->annotsRecord.getRecord().size())
            continue;

        if (this->annotsRecord.getAnnotationById(switchList[k]).ClassId != classId)
        {
            frameAndIdSwList.push_back(Point2i(this->annotsRecord.getAnnotationById(switchList[k]).FrameNumber, switchList[k]));
        }
    }

    // sort according to the frames
    vector<size_t> orderedFrames = AnnotationUtilities::sortP2iIndexes(frameAndIdSwList);

    // do a list of frames that we want to process, and their associated indexes
    vector<int> listFrames;
    vector<vector<int>> correspondingIdsList;
    int currListFramesIndex = -1;

    // start with some impossible index
    int prevFrame = -1;

    for (size_t k=0; k<orderedFrames.size(); k++)
    {
        int currFrame = frameAndIdSwList[orderedFrames[k]].x;

        if (currFrame!=prevFrame)
        {
            listFrames.push_back(currFrame);
            correspondingIdsList.push_back(vector<int>());
            currListFramesIndex++;
        }

        correspondingIdsList[currListFramesIndex].push_back(frameAndIdSwList[orderedFrames[k]].y);

        prevFrame = currFrame;
    }



    // keep a track of indices that we will need to delete - this vector will be filled only if the class we will set
    // existing annotations to is uniform
    vector<int> deleteIndices;
    vector<AnnotationObject> oldObjectsCharacsList;  // oldObjectsCaracs stores the objects characteristics before we modify it into annotsRecord
    vector<int> newObjectIds;

    // now handling every frame separately
    for (size_t fr=0; fr<listFrames.size(); fr++)
    {
        int frameNumber = listFrames[fr];

        // load the images that we will need to modify
        // prepare the image to be modified - we only need to modify the object ids mat
        Mat imToModifyObjIds, imToModifyClasses;

        string imgFileName = this->config.getAnnotatedImageFileName(this->imageFilePath, (this->isVideoOpen() ? this->videoFileName : this->imageFileName), frameNumber);


        // is it in the buffer?
        if (frameNumber>(this->maxImgReached-this->bufferLength) && frameNumber<=this->maxImgReached)
        {
            this->annotationsClassesBuffer[frameNumber%this->bufferLength].copyTo(imToModifyClasses);
            this->annotationsIdsBuffer[frameNumber%this->bufferLength].copyTo(imToModifyObjIds);
        }
        else
        {
            // load the images if available
            this->loadAnnotationsImageFile(imgFileName, imToModifyClasses, imToModifyObjIds);
        }

        // oldObjectsCharacsList is a frame dependent vector
        oldObjectsCharacsList.clear();
        newObjectIds.clear();





        // in case it's a uniform class we're switching to
        Rect2i contoursBB;





        if (this->config.getProperty(classId).classType == _ACT_Uniform)
        {
            // case one : it's a uniform class ID - we merge it to one single object
            // - perhaps we even need to merge it to an already existing object

            int newObjectId = 0;    // this id is fixed anyway

            // looking for a previously recorded object?
            int alreadyExistingId = this->annotsRecord.searchAnnotation(frameNumber, classId, newObjectId);
            if (alreadyExistingId != -1)
                // found one ! add it at the beginning of the stack
                correspondingIdsList[fr].insert(correspondingIdsList[fr].begin(), alreadyExistingId);

            // don't forget to add merged items to the delete list - we will only keep the first one
            // store their old characs as well
            for (size_t ob=1; ob<correspondingIdsList[fr].size(); ob++)
            {
                deleteIndices.push_back(correspondingIdsList[fr][ob]);
                oldObjectsCharacsList.push_back(this->annotsRecord.getAnnotationById(correspondingIdsList[fr][ob]));
                newObjectIds.push_back(newObjectId);
            }

            // call the merging procedure from the record
            this->annotsRecord.mergeIntraFrameAnnotationsTo(correspondingIdsList[fr], classId, newObjectId);

            // update the contours BB, we're in the case when there is a possibility that the contours were modified
            contoursBB = this->annotsRecord.getAnnotationById(this->annotsRecord.searchAnnotation(frameNumber, classId, newObjectId)).BoundingBox;

            // the images modifications will happen later
        }
        else
        {
            // we will attribute a new record ID to every element
            for (size_t ob=0; ob<correspondingIdsList[fr].size(); ob++)
            {
                int newObjectId = this->annotsRecord.getFirstAvailableObjectId(classId);

                // store the changes for when we will actually modify the concerned images
                oldObjectsCharacsList.push_back(this->annotsRecord.getAnnotationById(correspondingIdsList[fr][ob]));
                newObjectIds.push_back(newObjectId);

                // create an one-sized vector so that we can call the merge procedure instead of creating a new one
                vector<int> modifVec;
                modifVec.push_back(correspondingIdsList[fr][ob]);
                this->annotsRecord.mergeIntraFrameAnnotationsTo(modifVec, classId, newObjectId);

                // that's as simple as that - now go to the image modification
            }
        }


        if ((!imToModifyClasses.data) || (!imToModifyObjIds.data))
            continue;   // no data available, just continue...


        /*
        if (oldObjectsCharacsList.size()>0)
            contoursBB = oldObjectsCharacsList[0].BoundingBox;
        */


        // alright, the record has been modified, now fix the image
        for (size_t k=0; k<oldObjectsCharacsList.size(); k++)
        {
            const AnnotationObject& oldAnnot=oldObjectsCharacsList[k];

            // contoursBB |= oldAnnot.BoundingBox;

            for (int i=oldAnnot.BoundingBox.tl().y; i<oldAnnot.BoundingBox.br().y; i++)
            {
                for (int j=oldAnnot.BoundingBox.tl().x; j<oldAnnot.BoundingBox.br().x; j++)
                {
                    if ( (imToModifyClasses.at<int16_t>(i,j)==oldAnnot.ClassId) && (imToModifyObjIds.at<int32_t>(i,j)==oldAnnot.ObjectId))
                    {
                        // found a pixel!
                        imToModifyClasses.at<int16_t>(i,j) = classId;
                        imToModifyObjIds.at<int32_t>(i,j) = newObjectIds[k];
                    }
                }
            }
        }


        // finally store the result
        this->saveAnnotationsImageFile(imgFileName, imToModifyClasses, imToModifyObjIds);

        // copy back the data to the buffer in case it was already buffered
        if (frameNumber>(this->maxImgReached-this->bufferLength) && frameNumber<=this->maxImgReached)
        {
            imToModifyClasses.copyTo(this->annotationsClassesBuffer[frameNumber%this->bufferLength]);
            imToModifyObjIds.copyTo(this->annotationsIdsBuffer[frameNumber%this->bufferLength]);

            // if we're in the buffer, then we might need to update the contours image
            if (this->config.getProperty(classId).classType == _ACT_Uniform)
            {
                // it is not useful to update contours unless we switch to a uniform class (which would mean that we merge
                contoursBB = Rect2i(contoursBB.tl().x-1, contoursBB.tl().y-1, contoursBB.size().width+2, contoursBB.size().height+2);
                this->computeFrameContours(frameNumber, contoursBB);
            }
        }

    }


    // don't forget to remove now useless items
    this->annotsRecord.deleteAnnotationsGroup(deleteIndices);

    // don't forget to state that changes were performed!
    this->changesPerformedUponCurrentAnnot = true;
}










void AnnotationsSet::mergeIntraFrameAnnotations(int newClassId, int newObjectId, const std::vector<int>& listObjects)
{
    // some safety check
    if (listObjects.size()<1)
        return;

    // retrieve the frame number
    int frameNumber = this->annotsRecord.getAnnotationById(listObjects[0]).FrameNumber;



    if (this->config.getProperty(newClassId).classType == _ACT_BoundingBoxOnly)
    {
        // if it's bounding boxes only, we won't ever need to modify some images

        // prepare the image to be modified - we only need to modify the object ids mat
        Mat imToModifyObjIds, imToModifyClasses;

        string imgFileName = this->config.getAnnotatedImageFileName(this->imageFilePath, (this->isVideoOpen() ? this->videoFileName : this->imageFileName), frameNumber);


        // storing the bounding boxes so that we know which region within the image was affected
        Rect2i contoursBB = this->annotsRecord.getAnnotationById(listObjects[0]).BoundingBox;



        // is it in the buffer?
        if (frameNumber>(this->maxImgReached-this->bufferLength) && frameNumber<=this->maxImgReached)
        {
            this->annotationsClassesBuffer[frameNumber%this->bufferLength].copyTo(imToModifyClasses);
            this->annotationsIdsBuffer[frameNumber%this->bufferLength].copyTo(imToModifyObjIds);
        }
        else
        {
            // load the images if available
            this->loadAnnotationsImageFile(imgFileName, imToModifyClasses, imToModifyObjIds);
        }



        // alright, proceed
        if (imToModifyClasses.data && imToModifyObjIds.data)
        {
            // we can proceed to modifying the images
            for (size_t k=0; k<listObjects.size(); k++)
            {
                // storing the references of the object that we wish to modify
                const AnnotationObject& annObj = this->annotsRecord.getAnnotationById(listObjects[k]);

                // updating the contours BB so that it contains every object within the concerned area
                contoursBB |= annObj.BoundingBox;

                // avoiding to do some unnecessary thing..
                if (annObj.ClassId==newClassId && annObj.ObjectId==newObjectId)
                    continue;

                // running through the image (only the bounding box, actually) to modify the pixels
                for (int i=annObj.BoundingBox.tl().y; i<annObj.BoundingBox.br().y; i++)
                {
                    for (int j=annObj.BoundingBox.tl().x; j<annObj.BoundingBox.br().x; j++)
                    {
                        if (imToModifyClasses.at<int16_t>(i,j)==annObj.ClassId && imToModifyObjIds.at<int32_t>(i,j)==annObj.ObjectId)
                        {
                            imToModifyClasses.at<int16_t>(i,j) = newClassId;
                            imToModifyObjIds.at<int32_t>(i,j) = newObjectId;
                        }
                    }
                }
            }

            // storing the "upgraded" version of this image
            this->saveAnnotationsImageFile(imgFileName, imToModifyClasses, imToModifyObjIds);

            // copy back the data to the buffer in case it was already buffered
            if (frameNumber>(this->maxImgReached-this->bufferLength) && frameNumber<=this->maxImgReached)
            {
                imToModifyClasses.copyTo(this->annotationsClassesBuffer[frameNumber%this->bufferLength]);
                imToModifyObjIds.copyTo(this->annotationsIdsBuffer[frameNumber%this->bufferLength]);

                // this is where the contoursBB thing appears
                contoursBB = Rect2i(contoursBB.tl().x-1, contoursBB.tl().y-1, contoursBB.size().width+2, contoursBB.size().height+2);
                this->computeFrameContours(frameNumber, contoursBB);
            }
        }
    }


    // finally, we simply call the merge procedure from the record
    // beware for one exception : sometimes we call this function within an object deletion
    // in this case, we don't want to merge anything
    if (newClassId != 0)
        this->annotsRecord.mergeIntraFrameAnnotationsTo(listObjects, newClassId, newObjectId);


    // don't forget to state that changes were performed!
    if (frameNumber == this->currentImgIndex)
        this->changesPerformedUponCurrentAnnot = true;
}






void AnnotationsSet::computeFrameContours(int frameId, const cv::Rect2i& ROI)
{
    // verifying that the frameId makes sense
    if (frameId==-1)
        frameId = this->currentImgIndex;
    else if ((frameId<=this->maxImgReached-this->bufferLength) || (frameId>this->maxImgReached))
        return;


    // simply fills a
    Rect2i currentROI = ROI;

    if (ROI == cv::Rect2i(-3,-3,0,0))
    {
        // using the whole image
        currentROI = Rect2i(Point2i(0, 0), this->getOriginalImg(frameId).size());
    }
    else
    {
        // ensuring that the argument passed is well within the boundaries of the image
        currentROI = ROI & Rect2i(Point2i(0, 0), this->getOriginalImg(frameId).size());
    }

    // checking that the image is already filled correctly
    if (!this->annotationsContoursBuffer[frameId % this->bufferLength].data)
    {
        this->annotationsContoursBuffer[frameId % this->bufferLength] = Mat::zeros(this->getOriginalImg(frameId).size(), CV_8UC1);
    }

    // storing useful data and pointers for more readability...
    const Mat& currClasses = this->annotationsClassesBuffer[frameId % this->bufferLength];
    const Mat& currObjIds = this->annotationsIdsBuffer[frameId % this->bufferLength];
    Mat& currContours = this->annotationsContoursBuffer[frameId % this->bufferLength];

    int lastRow = this->getOriginalImg(frameId).rows -1;
    int lastCol = this->getOriginalImg(frameId).cols -1;

    // now running through the selected area
    for (int i=currentROI.tl().y; i<currentROI.br().y; i++)
    {
        for (int j=currentROI.tl().x; j<currentROI.br().x; j++)
        {
            // default : not part of a contour
            currContours.at<uchar>(i,j) = 0;

            // is it some part of an object?
            if (currClasses.at<int16_t>(i,j) != 0)
            {
                // yes
                if (i==0 || i==lastRow || j==0 || j==lastCol)
                {
                    // we are necessarily at the boundaries of the image, it's a contour
                    currContours.at<uchar>(i,j) = 1;
                    continue;   // no need to go further...!
                }

                // ok, need to check the surrounding pixels
                // fortunately, we already have handled the case when we're at the borders of the image

                bool alreadySolved = false;

                int currClass = currClasses.at<int16_t>(i,j);
                int currObjId = currObjIds.at<int32_t>(i,j);

                // evaluating the surrounding pixels
                for (int u=i-1; u<=i+1; u++)
                {
                    for (int v=j-1; v<=j+1; v++)
                    {
                        if (u==i && v==j)
                            continue;

                        // one object different is enough
                        if (currClasses.at<int16_t>(u,v)!=currClass || currObjIds.at<int32_t>(u,v)!=currObjId)
                        {
                            currContours.at<uchar>(i,j) = 1;    // store the result
                            alreadySolved = true;   // end the loops
                            break;
                        }
                    }

                    if (alreadySolved)
                        break;
                }
            }
        }
    }

    // that's all folks :)
}






void AnnotationsSet::loadAnnotationsImageFile(const std::string& fileName, cv::Mat& classesMat, cv::Mat& objIdsMat) const
{
    // this method only loads the data - unlike loadCurrentAnnotation, it takes for granted that the file is well formatted
    // we don't compute the contours there, they're useless for this operation

    // remove all of the data first - the presence or absence of data will show the success of the method or not
    classesMat.release();
    objIdsMat.release();

    // try to load the image, the color format is mandatory
    Mat imLoad = imread(fileName, CV_LOAD_IMAGE_COLOR);

    // if it was impossible to load the file, simply quit
    if (!imLoad.data)
        return;

    // look at the config and record the minColorIndex and the maxColorIndex, it is faster
    vector<Vec3b> minColorIndex, maxColorIndex;
    vector<Vec3i> multipliersIndex;
    vector<bool> classUniform;

    // fill the vectors..
    for (int i=0; i<this->config.getPropsNumber(); i++)
    {
        classUniform.push_back(this->config.getProperty(i+1).classType==_ACT_Uniform);

        if (this->config.getProperty(i+1).classType == _ACT_BoundingBoxOnly)
        {
            // give an impossible color so that it is not taken into account
            minColorIndex.push_back(Vec3b(0,0,0));
            maxColorIndex.push_back(Vec3b(0,0,0));
            multipliersIndex.push_back(Vec3i(1,1,1));
            continue;
        }

        minColorIndex.push_back(this->config.getProperty(i+1).minIdBGRRecRange);

        // warning : there's an exception when the class is uniform
        if (classUniform[i])
            maxColorIndex.push_back(minColorIndex[i]);
        else
            maxColorIndex.push_back(this->config.getProperty(i+1).maxIdBGRRecRange);

        // already calculate the multipliers
        multipliersIndex.push_back( Vec3i(1, (maxColorIndex[i][0]-minColorIndex[i][0]+1), (maxColorIndex[i][0]-minColorIndex[i][0]+1)*(maxColorIndex[i][1]-minColorIndex[i][1]+1)) );
    }

    // initialize classesMat and objIdsMat
    classesMat = Mat::zeros(imLoad.size(), CV_16SC1);
    objIdsMat = Mat::zeros(imLoad.size(), CV_32SC1);


    // read the image content...
    for (int i=0; i<imLoad.rows; i++)
    {
        for (int j=0; j<imLoad.cols; j++)
        {
            Vec3b pxValue = imLoad.at<Vec3b>(i,j);

            if (pxValue != Vec3b(0,0,0))    // there is something there..
            {
                for (int k=0; k<(int)minColorIndex.size(); k++)
                {
                    if ( (pxValue[0]>=minColorIndex[k][0]) && (pxValue[1]>=minColorIndex[k][1]) && (pxValue[2]>=minColorIndex[k][2])
                         && (pxValue[0]<=maxColorIndex[k][0]) && (pxValue[1]<=maxColorIndex[k][1]) && (pxValue[2]<=maxColorIndex[k][2]) )
                    {
                        // we belong to this class
                        classesMat.at<int16_t>(i,j) = k+1;

                        // if the class uniform, we don't need to do anything else
                        // if not, however, we need to decode the pixels information
                        if (!classUniform[k])
                        {
                            // this is just a matter of multiplication, nothing fancy here
                            int objInd = (multipliersIndex[k][0] * (pxValue[0]-minColorIndex[k][0]))
                                       + (multipliersIndex[k][1] * (pxValue[1]-minColorIndex[k][1]))
                                       + (multipliersIndex[k][2] * (pxValue[2]-minColorIndex[k][2]));

                            objIdsMat.at<int32_t>(i,j) = objInd;
                        }

                        // there's no need to go any further, we know already the right class and object id
                        break;
                    }
                }
            }
        }
    }

    // that's all
}







void AnnotationsSet::saveAnnotationsImageFile(const std::string& fileName, const cv::Mat& classesMat, const cv::Mat& objIdsMat) const
{
    // this method only loads the data - unlike loadCurrentAnnotation, it takes for granted that the file is well formatted

    if (!classesMat.data || (classesMat.size() != objIdsMat.size()))
        return;

    // generate the image that we will want to store
    Mat generateIm = Mat::zeros(classesMat.size(), CV_8UC3);


    // look at the config and record the minColorIndex and the maxColorIndex, it is faster
    vector<Vec3b> minColorIndex, maxColorIndex;
    vector<Vec3i> dividersIndex;
    vector<bool> classUniform;

    // fill the vectors..
    for (int i=0; i<this->config.getPropsNumber(); i++)
    {
        classUniform.push_back(this->config.getProperty(i+1).classType==_ACT_Uniform);

        if (this->config.getProperty(i+1).classType == _ACT_BoundingBoxOnly)
        {
            // give an impossible color so that it is not taken into account
            minColorIndex.push_back(Vec3b(0,0,0));
            maxColorIndex.push_back(Vec3b(0,0,0));
            dividersIndex.push_back(Vec3i(1,1,1));
            continue;
        }

        minColorIndex.push_back(this->config.getProperty(i+1).minIdBGRRecRange);

        // warning : there's an exception when the class is uniform
        if (classUniform[i])
            maxColorIndex.push_back(minColorIndex[i]);
        else
            maxColorIndex.push_back(this->config.getProperty(i+1).maxIdBGRRecRange);

        // already calculate the dividers
        dividersIndex.push_back( Vec3i((maxColorIndex[i][0]-minColorIndex[i][0]+1), (maxColorIndex[i][1]-minColorIndex[i][1]+1), (maxColorIndex[i][2]-minColorIndex[i][2]+1)) );
    }


    // run through the image content and fill the pixel colors...
    for (int i=0; i<classesMat.rows; i++)
    {
        for (int j=0; j<classesMat.cols; j++)
        {
            if (classesMat.at<int16_t>(i,j) != 0)    // there is something there..
            {
                int currClass = classesMat.at<int16_t>(i,j);

                if (classUniform[currClass-1])
                    // simplest case - uniform : we use the min value
                    generateIm.at<Vec3b>(i,j) = minColorIndex[currClass-1];
                else
                {
                    // hardest case : we have to calculate the Vec3b value
                    // unlike the saveCurrentAnnot, we cannot calculate it "offline" because we cannot count on the record indexation
                    Vec3b bytesContent;
                    int currObjIdRemaining = objIdsMat.at<int32_t>(i,j);
                    for (size_t c=0; c<3; c++)
                    {
                        // +1 is there because the max value is to be included
                        // anyway, x%1 == 0 and x/1 == x
                        bytesContent[c] = (uchar) currObjIdRemaining % (dividersIndex[currClass-1][c]);
                        currObjIdRemaining /= (dividersIndex[currClass-1][c]);
                    }

                    // fill the color
                    for (size_t c=0; c<3; c++)
                    {
                        generateIm.at<Vec3b>(i,j)[c] = bytesContent[c] + minColorIndex[currClass-1][c];
                    }
                }
            }
        }
    }

    // finally store the image
    QtCvUtils::imwrite(fileName, generateIm);
}









void AnnotationsSet::handleAnnotationsModifications(const vector<Point2i>& affectedObjectsList, const vector<Rect2i>& affectedObjectsBBs)
{

    // if some objects need to be erased, we cannot do it on the fly. We need to wait until the whole procedure has been completed,
    // in order to protect that
    vector<int> eraseList;

    for (size_t k=0; k<affectedObjectsList.size(); k++)
    {
        // get the ID of the object that was affected
        int recordId = this->annotsRecord.searchAnnotation(this->currentImgIndex, affectedObjectsList[k].x, affectedObjectsList[k].y);

        // store some usefull things...
        int currClassId = affectedObjectsList[k].x;
        int currObjectId = affectedObjectsList[k].y;

        // some safety check. This should not happen however
        if (recordId == -1)
            continue;

        // store the original annotation BB, we will use it quite extensively..
        Rect2i annotOrigBB = this->annotsRecord.getAnnotationById(recordId).BoundingBox;

        // actually, I had tried something implying less pixels evaluation.
        // but since we cannot prevent the affected annotations from having some holes,
        // we're forced to evaluate the whole original annotation area and we cannot just evaluate the modified bounding box

        // the only case when we know for sure that we don't have to do any modification is
        // when the modified area is strictly inside the original annotation area
        Rect2i commonBB = annotOrigBB & affectedObjectsBBs[k];

        if ( (commonBB.tl().x > annotOrigBB.tl().x) &&
             (commonBB.tl().y > annotOrigBB.tl().y) &&
             (commonBB.br().x < annotOrigBB.br().x) &&
             (commonBB.br().y < annotOrigBB.br().y) )
            continue;

        // OK we've handled the easiest case, now we need to run through the whole original area
        // initiate some boundary variables outside of the area
        int TCoord=annotOrigBB.br().y, BCoord=annotOrigBB.tl().y-1, LCoord=annotOrigBB.br().x, RCoord=annotOrigBB.tl().x-1;

        // now updating all of this mess :)
        for (int i=annotOrigBB.tl().y; i<annotOrigBB.br().y; i++)
        {
            for (int j=annotOrigBB.tl().x; j<annotOrigBB.br().x; j++)
            {
                if ( (this->accessCurrentAnnotationsClasses().at<int16_t>(i,j) == currClassId)
                     && (this->accessCurrentAnnotationsIds().at<int32_t>(i,j) == currObjectId) )
                    // we found a pixel that corresponds to the object
                {
                    // update the bounding box
                    if (j<LCoord)
                        LCoord = j;
                    if (j>RCoord)
                        RCoord = j;
                    if (i<TCoord)
                        TCoord = i;
                    if (i>BCoord)
                        BCoord = i;
                }
            }
        }

        // if the coordinates haven't been touched, then it means that the object has been suppressed
        // this is easy to check with the condition about BCoord/TCoord or LCoord/RCoord - only one of them two is sufficient
        if (TCoord>BCoord)
            // this is a suppression case - store it for later
            eraseList.push_back(recordId);
        else
            // simply update the value with the new ones acquired
            this->annotsRecord.updateBoundingBox( recordId, Rect2i(Point2i(LCoord, TCoord), Point2i(RCoord+1,BCoord+1)) );

    }


    // finally erase the objects that have been completely removed
    if (eraseList.size()>0)
    {
        // as usual, start with the biggest index first
        sort(eraseList.begin(), eraseList.end());

        for (int k=(int)eraseList.size()-1; k>=0; k--)
            this->annotsRecord.removeAnnotation(eraseList[k]);
    }


}








