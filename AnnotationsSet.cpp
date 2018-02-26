#include "AnnotationsSet.h"


using namespace std;
using namespace cv;













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

    // initialization of the buffer
    this->originalImagesBuffer = vector<Mat>(this->bufferLength, Mat());
    this->annotationsClassesBuffer = vector<Mat>(this->bufferLength, Mat());
    this->annotationsIdsBuffer = vector<Mat>(this->bufferLength, Mat());

    // filling some basic configuration stuff
    AnnotationsProperties prop;
    prop.ClassName = "Road"; prop.classType = _ACT_Uniform; prop.displayRGBColor=Vec3b(0, 85, 85); prop.minIdRepRange=Vec3b(127, 127, 0);
    this->addClassProperty(prop);
    prop.ClassName = "Car"; prop.classType = _ACT_MultipleObjects; prop.displayRGBColor=Vec3b(255, 0, 0); prop.minIdRepRange=Vec3b(255, 0, 0); prop.maxIdRepRange=Vec3b(255, 255, 255);
    this->addClassProperty(prop);
    prop.ClassName = "Truck"; prop.classType = _ACT_MultipleObjects; prop.displayRGBColor=Vec3b(170, 0, 0); prop.minIdRepRange=Vec3b(170, 0, 0); prop.maxIdRepRange=Vec3b(170, 255, 255);
    this->addClassProperty(prop);
    prop.ClassName = "Bus"; prop.classType = _ACT_MultipleObjects; prop.displayRGBColor=Vec3b(85, 0, 0); prop.minIdRepRange=Vec3b(85, 0, 0); prop.maxIdRepRange=Vec3b(85, 255, 255);
    this->addClassProperty(prop);
}

void AnnotationsSet::addClassProperty(const AnnotationsProperties& ap)
{
    // ad the class property - pretty simple
    this->config.addProperty(ap);

    // don't forget to get the firstKnownAvailableObjectId vector right
    //this->firstKnownAvailableObjectId.push_back(0);
}



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




cv::Mat& AnnotationsSet::accessCurrentAnnotationsClasses()
{
    return this->annotationsClassesBuffer[this->currentImgIndex % this->bufferLength];
}

cv::Mat& AnnotationsSet::accessCurrentAnnotationsIds()
{
    return this->annotationsIdsBuffer[this->currentImgIndex % this->bufferLength];
}





bool AnnotationsSet::loadOriginalImage(const std::string& imgFileName)
{
    // load the file
    Mat im = imread(imgFileName);

    // couldn't read the file?
    if (!im.data)
        return false;

    // alright, we can use it

    // store the filename
    this->imageFileName = imgFileName;

    // first : init (back?) all the buffers
    this->currentImgIndex = 0;

    for (size_t k=0; k<this->originalImagesBuffer.size(); k++)
    {
        this->originalImagesBuffer[k].release();
        this->annotationsClassesBuffer[k].release();
        this->annotationsIdsBuffer[k].release();
    }

    // then fill the data correctly
    im.copyTo(this->originalImagesBuffer[0]);
    this->annotationsClassesBuffer[0] = Mat::zeros(im.size(), CV_16SC1);
    this->annotationsIdsBuffer[0] = Mat::zeros(im.size(), CV_32SC1);

    return true;
}




int AnnotationsSet::addAnnotation(const cv::Mat& mask, const cv::Point2i& topLeftCorner, int whichClass, const cv::Point2i& annotStartingPoint)
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

    if (this->config.getProperty(whichClass).classType == _ACT_MultipleObjects)
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



    // handle the cases where we have modified previously recorded objects first
    // (since it may modify the main record indexes, we want to do it before assigning an index to the new object)
    this->handleAnnotationsModifications(affectedObjectsList, affectedObjectsBBs);



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


    // now we should spend some time updating the record - we need to find the new bounding boxes, as well as the cases when an object was completely erased
    this->handleAnnotationsModifications(affectedObjectsList, affectedObjectsBBs);
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


        /*
        // commonBB is the area of the object that was modified. We know that the object cannot have been modified outside of this area
        Rect2i commonBB = annotOrigBB & affectedObjectsBBs[k];

        // those variables will store the new object coords
        int TCoord, LCoord, RCoord, BCoord;


        // evaluating the LCoord
        if (commonBB.tl().x > annotOrigBB.tl().x)
            // the modification cannot have affected the TL.x value
            LCoord = annotOrigBB.tl().x;
        else
            // the modification can have affected the TL.x value, we need to check everything - we save this information for later
            LCoord = commonBB.br().x-1;

        // evaluating the TCoord
        if (commonBB.tl().y > annotOrigBB.tl().y)
            // the modification cannot have affected the TL.y value
            TCoord = annotOrigBB.tl().y;
        else
            // the modification can have affected the TL.y value, we need to check the whole area - we save this information for later
            TCoord = commonBB.br().y-1;

        // evaluating the RCoord
        if (commonBB.br().x < annotOrigBB.br().x)
            // the modification cannot have affected the BR.x value
            RCoord = annotOrigBB.br().x-1;
        else
            // the modification can have affected the BR.x value, we need to check everything - we save this information for later
            RCoord = commonBB.tl().x;

        // evaluating the BCoord
        if (commonBB.br().y < annotOrigBB.br().y)
            // the modification cannot have affected the BR.y value
            BCoord = annotOrigBB.br().y-1;
        else
            // the modification can have affected the BR.y value, we need to check the whole area - we save this information for later
            BCoord = commonBB.tl().y;
            */

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








