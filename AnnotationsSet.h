#ifndef ANNOTATIONSSET_H
#define ANNOTATIONSSET_H




#include <opencv2/opencv.hpp>
#include <vector>
#include "QtCvUtils.h"

#include <algorithm>
#include <numeric>



/*
 * This is the set of classes designed to handle pixel level annotations
 * Those annotations have several characteristics : class and ID
 * A pixel can only have One class and One ID. Ids correspond to objects within a given class. An object without a class cannot have any ID
 *
 * Annotations are stored into simple images. The class is represented by the color of objects
 * The id of objects is encoded into the RGB value bits that are not used by the class colors.
 */



namespace AnnotationUtilities
{
    inline bool isThisPointWithinImageBoundaries(const cv::Point2i& p, const cv::Mat& im)
    {
        return (p.x>=0 && p.x<im.cols && p.y>=0 && p.y<im.rows);
    }

    // original function found at https://stackoverflow.com/questions/1494399/how-do-i-search-find-and-replace-in-a-standard-string
    inline void strReplace(std::string& str, const std::string& token, const std::string& replace)
    {
        std::string::size_type pos = 0u;
        while ((pos = str.find(token, pos)) != std::string::npos)
        {
            str.replace(pos, token.length(), replace);
            pos += replace.length();
        }
    }

    //inline bool comparePoints(const cv::Point2i& p1, const cv::Point2i& p2) { return ((p1.x<p2.x) || (p1.x==p2.x && p1.y<p2.y)); }

    inline std::vector<size_t> sortP2iIndexes(const std::vector<cv::Point2i> &v)
    {
        // initialize original index locations
        std::vector<size_t> idx(v.size());
        std::iota(idx.begin(), idx.end(), 0);

        // sort indexes based on comparing values in v
        sort( idx.begin(), idx.end(),
             [&v](size_t i1, size_t i2) {return ((v[i1].x<v[i2].x) || (v[i1].x==v[i2].x && v[i1].y<v[i2].y));} );

        return idx;
    }

}













enum AnnotationClassType { _ACT_Uniform, _ACT_MultipleObjects };



const std::string _AnnotProp_YAMLKey_ClassName  = "Name";
const std::string _AnnotProp_YAMLKey_ClassType  = "Type";
const std::string _AnnotProp_YAMLKey_DispRGBCol = "DispColRGB";
const std::string _AnnotProp_YAMLKey_RecBGRMin  = "IdRecRangeMinBGR";
const std::string _AnnotProp_YAMLKey_RecBGRMax  = "IdRecRangeMaxBGR";



class AnnotationsProperties
{
public:
    AnnotationsProperties() { this->locked = false; }
    AnnotationsProperties(const AnnotationsProperties& ap) : className(ap.className), classType(ap.classType), locked(ap.locked),
                                                             displayRGBColor(ap.displayRGBColor), minIdBGRRecRange(ap.minIdBGRRecRange), maxIdBGRRecRange(ap.maxIdBGRRecRange) {}
    AnnotationsProperties& operator=(const AnnotationsProperties& ap)
    {
        this->className = ap.className;
        this->classType = ap.classType;
        this->locked = ap.locked;
        this->displayRGBColor = ap.displayRGBColor;
        this->minIdBGRRecRange = ap.minIdBGRRecRange;
        this->maxIdBGRRecRange = ap.maxIdBGRRecRange;
        return (*this);
    }

    void write(cv::FileStorage& fs) const
    {
        fs << "{:";
        fs << _AnnotProp_YAMLKey_ClassName  << this->className;
        fs << _AnnotProp_YAMLKey_ClassType  << (int)this->classType;
        fs << _AnnotProp_YAMLKey_DispRGBCol << this->displayRGBColor;
        fs << _AnnotProp_YAMLKey_RecBGRMin  << this->minIdBGRRecRange;
        fs << _AnnotProp_YAMLKey_RecBGRMax  << this->maxIdBGRRecRange;
        fs << "}";
    }

    void read(const cv::FileNode& node)
    {
        node[_AnnotProp_YAMLKey_ClassName]  >> this->className;
        int typeInt;
        node[_AnnotProp_YAMLKey_ClassType]  >> typeInt;
        this->classType = static_cast<AnnotationClassType>(typeInt);
        node[_AnnotProp_YAMLKey_DispRGBCol] >> this->displayRGBColor;
        node[_AnnotProp_YAMLKey_RecBGRMin]  >> this->minIdBGRRecRange;
        node[_AnnotProp_YAMLKey_RecBGRMax]  >> this->maxIdBGRRecRange;
    }

    //int ClassId;
    std::string className;
    AnnotationClassType classType;
    bool locked;
    cv::Vec3b displayRGBColor;
    cv::Vec3b minIdBGRRecRange, maxIdBGRRecRange; // available range for encoding both the Id and the class. In BGR!!!
                                            // when in uniform class type, max and min shall be identical. Either way, only the min value will be used
                                            // when in MultipleObjects mode, setting min=Vec3b(0,0,128) and max=(255,255,128) allows
                                            // the Id to be encoded on B and G channels, which provides 16 bits of latitude.
};




// the following declarations are necessary to the FileStorage stuff, but they are declared in the .cpp source file
// static void write(cv::FileStorage& fs, const std::string&, const AnnotationsProperties& x);
// static void read(const cv::FileNode& node, AnnotationsProperties& x, const AnnotationsProperties& default_value = AnnotationsProperties())







/*
 * Set of annotations properties. Please note that the class numbering starts from 1 and not from 0
 * This allows us to store 0 as a "non-class" in the pixel images
 */


const std::string _AnnotationsConfig_FileNamingToken_OrigImgPath = "%OrImPa%";
const std::string _AnnotationsConfig_FileNamingToken_OrigImgFileName = "%OrImFiNa%";
const std::string _AnnotationsConfig_FileNamingToken_FrameNumber = "%FrNu%";
const int _AnnotationsConfig_FileNamingToken_FrameNumberZeroFillLength = 6;


const std::string _AnnotsConfig_YAMLKey_Node  = "Configuration";
const std::string _AnnotsConfig_YAMLKey_ClassesDefs_Node  = "ClassesDefinitions";
const std::string _AnnotsConfig_YAMLKey_ImageFileNamingRule  = "ImageFilesNamingRule";
const std::string _AnnotsConfig_YAMLKey_SummaryFileNamingRule  = "SummaryFileNamingRule";


class AnnotationsConfig
{
public:
    AnnotationsConfig() { this->setDefaultConfig(); }
    AnnotationsConfig(const AnnotationsConfig& ac); //{ this->propsSet = ac.getPropsSet(); this->fileNamingRule = ac.getFileNamingRule(); }
    ~AnnotationsConfig() {}

    // classic accessors stuff
    int getPropsNumber() const { return (int)this->propsSet.size(); }
    const std::vector<AnnotationsProperties>& getPropsSet() const { return this->propsSet; }
    const AnnotationsProperties& getProperty(int id) const { return this->propsSet[id-1]; } // id has to be within range..

    void addProperty(const AnnotationsProperties& ap) { this->propsSet.push_back(ap); }
    void removeProperty(int id) { if (id<1 || id>=(int)this->propsSet.size()) return; this->propsSet.erase(this->propsSet.begin()+id-1); }
    void clearProperties() { this->propsSet.clear(); }

    void setImageFileNamingRule(const std::string& rule) { this->imageFileNamingRule = rule; }
    const std::string& getImageFileNamingRule() const { return this->imageFileNamingRule; }
    void setSummaryFileNamingRule(const std::string& rule) { this->summaryFileNamingRule = rule; }
    const std::string& getSummaryFileNamingRule() const { return this->summaryFileNamingRule; }

    std::string getAnnotatedImageFileName(const std::string& origImgPath, const std::string& origImgFileName, int frameNumber) const;
    std::string getSummaryFileName(const std::string& origImgPath, const std::string& origiImgFileName) const;


    // default config stuff
    void setDefaultConfig();

    void setClassLock(int id, bool lock) { if ((id<1)||(id>(int)this->propsSet.size())) return; this->propsSet[id-1].locked=lock; }


    void writeContentToYaml(cv::FileStorage& fs) const;
    void readContentFromYaml(const cv::FileNode& fnd);




private:
    std::vector<AnnotationsProperties> propsSet;

    std::string imageFileNamingRule;
    std::string summaryFileNamingRule;
};





// this is a way to record objects occurrences and provides a faster access to their content, using the bounding box
// however, managing such data is complex, since it introduces redundancy... it forces us to maintain consistency throughout the database,
// which can be sometimes rather complex...



const std::string _AnnotObj_YAMLKey_Class = "Cl";
const std::string _AnnotObj_YAMLKey_ObjId = "Ob";
const std::string _AnnotObj_YAMLKey_Frame = "Fr";
const std::string _AnnotObj_YAMLKey_BBox  = "BB";



class AnnotationObject
{
public:
    AnnotationObject() : ClassId(0), ObjectId(0), FrameNumber(0), locked(false) {}
    AnnotationObject(const AnnotationObject& ao) : ClassId(ao.ClassId), ObjectId(ao.ObjectId), FrameNumber(ao.FrameNumber), BoundingBox(ao.BoundingBox), locked(ao.locked) {}
    AnnotationObject& operator=(const AnnotationObject& ao) { this->ClassId=ao.ClassId; this->ObjectId=ao.ObjectId; this->FrameNumber=ao.FrameNumber; this->BoundingBox=ao.BoundingBox; this->locked=ao.locked; return *(this); }

    void write(cv::FileStorage& fs) const
    {
        fs << "{:" << _AnnotObj_YAMLKey_Class << this->ClassId
                   << _AnnotObj_YAMLKey_ObjId << this->ObjectId
                   << _AnnotObj_YAMLKey_Frame << this->FrameNumber
                   << _AnnotObj_YAMLKey_BBox  << this->BoundingBox << "}";
    }

    void read(const cv::FileNode& node)
    {
        node[_AnnotObj_YAMLKey_Class] >> this->ClassId;
        node[_AnnotObj_YAMLKey_ObjId] >> this->ObjectId;
        node[_AnnotObj_YAMLKey_Frame] >> this->FrameNumber;
        node[_AnnotObj_YAMLKey_BBox]  >> this->BoundingBox;
    }

    int ClassId;
    int ObjectId;
    int FrameNumber;
    cv::Rect2i BoundingBox;
    bool locked;
};


// same as above regarding the FileStorage declarations stuff
// static void write(cv::FileStorage& fs, const std::string&, const AnnotationObject& x)
// static void read(const cv::FileNode& node, AnnotationObject& x, const AnnotationObject& default_value = AnnotationObject())










// this is the class that handles the indexing of all of the annotation objects


const std::string _AnnotsRecord_YAMLKey_Node  = "AnnotationsRecord";



class AnnotationsRecord
{
public:
    AnnotationsRecord();
    ~AnnotationsRecord();

    const std::vector<AnnotationObject>& getRecord() const;     // access the entire record
    const std::vector<int>& getAnnotationIds(int classId, int objectId) const;    // retrieve the indices of objects corresponding to a given class and a given object ID.
                                                                                  // several results are possible given they are each located on a separate frame
    const std::vector<int>& getFrameContentIds(int id) const;   // get all the IDs inside a frame
    int getRecordedFramesNumber() const { return this->framesIndex.size(); }
    const AnnotationObject& getAnnotationById(int id) const;    // retrieve a specific object by his ID

    int getFirstAvailableObjectId(int classId) const;       // when we want to create a new annotation, we need to know a new object id, given a class.
                                                            // this function allows us to find such an object ID
    int searchAnnotation(int frameId, int classId, int objectId) const;     // search  an annotation in the record vector. returns -1 if it wasn't found


    int addNewAnnotation(const AnnotationObject&);                          // push a new annotation object, at the end of the main vector (return its index)
    void updateBoundingBox(int annotationIndex, const cv::Rect2i newBB);    // edit a bounding box given the object ID in the record vector
    void removeAnnotation(int annotationIndex);                             // remove an annotation given its id in the record vector
    void clearFrame(int frameId);                                           // remove all of the objects included in a given frame


    void mergeIntraFrameAnnotationsTo(const std::vector<int>& annotsList, int newClassId, int newObjectId);
                                                                                    // merges all the annotations of a single frame into one and gives them the newClassId and ObjectId
                                                                                    // data    /!\ doesn't delete the now (supposedly) useless merged data
    void deleteAnnotationsGroup(const std::vector<int>& deleteList);                // delete the annotations which record Ids are included in the deleteList
    std::vector<int> separateAnnotations(const std::vector<int>& separateList);     // the concept here is to dissociate annotations that are the same object (class and object id)
                                                                                    // but on different frames - returns the ids of objects which objectId has changed


    void clear();   // the ultimate killer - simply clear all of the vectors


    void setObjectLock(int id, bool lock) { if (id<0 || id>=(int)this->record.size()) return; this->record[id].locked = lock; }


    void writeContentToYaml(cv::FileStorage& fs) const;
    void readContentFromYaml(const cv::FileNode& fnd);



private:
    std::vector<AnnotationObject> record;           // stores all the objects
    std::vector< std::vector<int> > framesIndex;    // first index corresponds to the frame number, second index corresponds to the entry index into the record vector
    std::vector< std::vector< std::vector<int> > > objectsIndex;    // first index = ClassId-1, second index = ObjectId, last index to the position in record
};









// finally, the class that stores the pixel-level information
// stores all of the data into images, loads original images or videos (to be annotated), handles the communication with the GUI as well

const int _AnnotationsSet_default_bufferLength = 16;
const int _AnnotationsSet_default_classNoneValue = 0;





const std::string _AnnotationsSet_YAMLKey_Node  = "AnnotationsSet";
const std::string _AnnotationsSet_YAMLKey_FilePath  = "FilePath";
const std::string _AnnotationsSet_YAMLKey_ImageFileName  = "ImageFileName";
const std::string _AnnotationsSet_YAMLKey_VideoFileName  = "VideoFileName";
const std::string _AnnotationsSet_YAMLKey_RecordingTimeDate  = "RecordingTimeDate";






class AnnotationsSet
{
public:
    AnnotationsSet();
    ~AnnotationsSet();


    // standard accessors stuff.. those images are returned for index = currentImgIndex
    const cv::Mat& getCurrentOriginalImg() const;
    const cv::Mat& getCurrentAnnotationsClasses() const;
    const cv::Mat& getCurrentAnnotationsIds() const;
    const cv::Mat& getCurrentContours() const;



    // access any image within the buffer
    const cv::Mat& getOriginalImg(int id) const;
    const cv::Mat& getAnnotationsClasses(int id) const;
    const cv::Mat& getAnnotationsIds(int id) const;
    const cv::Mat& getContours(int id) const;




    // get the object Id (within the record) to which the pixel (x,y) belongs in the current image
    int getObjectIdAtPosition(int x, int y) const;

    const std::vector<int>& getObjectsListOnCurrentFrame() const { return this->annotsRecord.getFrameContentIds(this->currentImgIndex); }

    int getCurrentFramePosition() const { return this->currentImgIndex; }




    bool loadOriginalImage(const std::string& imgFileName);
    bool loadOriginalVideo(const std::string& videoFileName);
    bool loadAnnotations(const std::string& annotationsFileName);

    bool loadConfiguration(const std::string& configFileName);



    bool loadNextFrame();
    bool loadFrame(int fId);



    bool isVideoOpen() const;
    bool isImageOpen() const;


    bool canReadNextFrame() const;
    bool canReadPrevFrame() const;


    bool loadCurrentAnnotationImage();
    bool saveCurrentAnnotationImage(const std::string& forcedFileName="") const;


    void closeFile(bool pleaseSave=false);

    // save the yaml file and the image file. forceFilename concerns the yaml file.
    // The boolean prevents the software from saving systematically even if no changes were done
    bool saveCurrentState(const std::string& forceFileName="", bool saveOnlyIfNecessary=false);


    bool thereWereChangesPerformedUponCurrentAnnot() const { return this->changesPerformedUponCurrentAnnot; }



    void setObjectLock(int objectId, bool lock) { this->annotsRecord.setObjectLock(objectId, lock); }
    void setClassLock(int classId, bool lock) { this->config.setClassLock(classId, lock); }




    std::string getDefaultAnnotationsSaveFileName() const;
    std::string getDefaultCurrentImageSaveFileName() const;



    const std::string& getOpenedFileName() const { return (this->isVideoOpen() ? this->videoFileName : this->imageFileName); }




    int addAnnotation(const cv::Mat& mask, const cv::Point2i& topLeftCorner, int whichClass, const cv::Point2i& annotStartingPoint=cv::Point2i(-1,-1), int forceObjectId=-1);
            // add an annotation.
            // mask is in CV_8UC1 format, whatever is not zero belongs to the object.
            // annotStartingPoint is there to know whether we add this to a previous annotation (in which case we merge both), or whether we start a new one
            // the return index is the index of the ID, which is set automatically by this class
            // note that the merging thing only occurs when the starting point corresponds to an annotation of the same type

    void removePixelsFromAnnotations(const cv::Mat& mask, const cv::Point2i& topLeftCorner);
            // remove any class from an annotation at the pixel locations marked with a non-zero mask value.


    void mergeAnnotations(const std::vector<int>& annotationsList);
            // merge objects, both inter and intra frame given their IDs. The merge is only internal to a class - when the vector contains objects from various classes,
            // we perform the computation only class by class

    void deleteAnnotations(const std::vector<int>& annotationsList, bool onlyFromRecord=false);
            // delete annotations given a list. Doesn't affect the pixels data if onlyFromRecord is set to true (useful when called from a merge procedure)

    void clearCurrentFrame();

    void separateAnnotations(const std::vector<int>& separateList);
            // separate annotations : gives objects with the same object ID on different frames a separate object ID
            // one of the separated objects keeps the same object id as before : it's the one that appears first in the list
            // (maybe the one that has been checked first in the browser?)

    void switchAnnotationsToClass(const std::vector<int>& switchList, int classId);
            // changes the classes of the selected objects (when they don't already belong to this class)
            // Creates new objects in the selected class when the class is not uniform



    const AnnotationsConfig& getConfig() const { return this->config; }
    AnnotationsConfig& accessConfig() { return this->config; }
    const AnnotationsRecord& getRecord() const { return this->annotsRecord; }
    // AnnotationsConfig& accessConfig() { return this->config; }  // non-const accessor version, should be used only very occasionnally..


    // void addClassProperty(const AnnotationsProperties& ap);



private:
    void setDefaultConfig();


    void handleAnnotationsModifications(const std::vector<cv::Point2i>& affectedObjectsList, const std::vector<cv::Rect>& affectedBoundingBoxes);
                // when performing a new annotation or removing pixels, we are susceptible of modifying previous annotations or even deleting them
                // this method is designed to handle such cases



    void loadAnnotationsImageFile(const std::string& fileName, cv::Mat& classesMat, cv::Mat& objIdsMat) const;
    void saveAnnotationsImageFile(const std::string& fileName, const cv::Mat& classesMat, const cv::Mat& objIdsMat) const;



    // used within the class to make easier the modification of both matrices
    cv::Mat& accessCurrentAnnotationsClasses();
    cv::Mat& accessCurrentAnnotationsIds();
    cv::Mat& accessCurrentContours();



    void mergeIntraFrameAnnotations(int newClassId, int newObjectId, const std::vector<int>& listObjects);

    void computeFrameContours(int frameId=-1, const cv::Rect2i& ROI=cv::Rect2i(-3,-3,0,0));



    bool isAnnotLockedOnCurrentFrame(int classId, int objId) const;





    // this set of vectors stores the original images as well as their corresponding annotations.
    // those are buffers, since we are likely to annotate a video and it is convenient to recall the previous frames easily
    std::vector<cv::Mat> originalImagesBuffer;  	// original image, in CV8U_C1 or CV8UC3 (BGR) format
    std::vector<cv::Mat> annotationsClassesBuffer;	// corresponding class for every pixel, in CV_16SC1 format
    std::vector<cv::Mat> annotationsIdsBuffer;		// corresponding object Id for every pixel, in CV_32SC1 format
    std::vector<cv::Mat> annotationsContoursBuffer; // stores the contours of objects, in CV_8UC1 format - 0 = no contour, anything above = contour

    // in order to know where we are within the buffer
    int currentImgIndex;
    int maxImgReached;
    bool reachedTheEndOfVideo;

    // records whether some changes were done on the current annotation
    bool changesPerformedUponCurrentAnnot;

    // those buffers will be assigned dynamically, in case we need to change their length
    int bufferLength;

    // store the configuration locally. It is not supposed to be modified once the annotation is launched
    AnnotationsConfig config;

    // store the record - kind of a way of indexing annotations into the system
    AnnotationsRecord annotsRecord;

    // this vector stores the first known available per class
    //std::vector<int> firstKnownAvailableObjectId;

    // video accessor object
    cv::VideoCapture vidCap;


    // store the image/video/annotation loaded
    std::string imageFileName;
    std::string videoFileName;
    std::string imageFilePath;
};




#endif // ANNOTATIONSSET_H
