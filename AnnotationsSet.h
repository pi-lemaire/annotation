#ifndef ANNOTATIONSSET_H
#define ANNOTATIONSSET_H




#include <opencv2/opencv.hpp>
#include <vector>
#include "QtCvUtils.h"





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
}













enum AnnotationClassType { _ACT_Uniform, _ACT_MultipleObjects};


class AnnotationsProperties
{
public:
    AnnotationsProperties() {}
    AnnotationsProperties(const AnnotationsProperties& ap) : className(ap.className), classType(ap.classType),
                                                             displayRGBColor(ap.displayRGBColor), minIdBGRRecRange(ap.minIdBGRRecRange), maxIdBGRRecRange(ap.maxIdBGRRecRange) {}
    AnnotationsProperties& operator=(const AnnotationsProperties& ap)
    {
        this->className = ap.className;
        this->classType = ap.classType;
        this->displayRGBColor = ap.displayRGBColor;
        this->minIdBGRRecRange = ap.minIdBGRRecRange;
        this->maxIdBGRRecRange = ap.maxIdBGRRecRange;
        return (*this);
    }

    void write(cv::FileStorage& fs) const
    {
        fs << "{:";
        fs << "Name" << this->className;
        fs << "Type" << (int)this->classType;
        fs << "DispColRGB" << this->displayRGBColor;
        fs << "IdRecRangeMinBGR" << this->minIdBGRRecRange;
        fs << "IdRecRangeMaxBGR" << this->maxIdBGRRecRange;
        fs << "}";
    }

    void read(const cv::FileNode& node)
    {
        node["Name"] >> this->className;
        int typeInt;
        node["Type"] >> typeInt;
        this->classType = (typeInt==_ACT_Uniform? _ACT_Uniform : _ACT_MultipleObjects);
        node["DispColRGB"] >> this->displayRGBColor;
        node["IdRecRangeMinBGR"] >> this->minIdBGRRecRange;
        node["IdRecRangeMaxBGR"] >> this->maxIdBGRRecRange;
    }

    //int ClassId;
    std::string className;
    AnnotationClassType classType;
    cv::Vec3b displayRGBColor;
    cv::Vec3b minIdBGRRecRange, maxIdBGRRecRange; // available range for encoding both the Id and the class. In BGR!!!
                                            // when in uniform class type, max and min shall be identical. Either way, only the min value will be used
                                            // when in MultipleObjects mode, setting min=Vec3b(0,0,128) and max=(255,255,128) allows
                                            // the Id to be encoded on B and G channels, which provides 16 bits of latitude.
};


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





/*
 * Set of annotations properties. Please note that the class numbering starts from 1 and not from 0
 * This allows us to store 0 as a "non-class" in the pixel images
 */


const std::string _AnnotationsConfig_FileNamingToken_OrigImgPath = "%OrImPa%";
const std::string _AnnotationsConfig_FileNamingToken_OrigImgFileName = "%OrImFiNa%";
const std::string _AnnotationsConfig_FileNamingToken_FrameNumber = "%FrNu%";
const int _AnnotationsConfig_FileNamingToken_FrameNumberZeroFillLength = 6;


class AnnotationsConfig
{
public:
    AnnotationsConfig() {}
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

class AnnotationObject
{
public:
    AnnotationObject() : ClassId(0), ObjectId(0), FrameNumber(0) {}
    AnnotationObject(const AnnotationObject& ao) : ClassId(ao.ClassId), ObjectId(ao.ObjectId), FrameNumber(ao.FrameNumber), BoundingBox(ao.BoundingBox) {}
    AnnotationObject& operator=(const AnnotationObject& ao) { this->ClassId=ao.ClassId; this->ObjectId=ao.ObjectId; this->FrameNumber=ao.FrameNumber; this->BoundingBox=ao.BoundingBox; return *(this); }

    void write(cv::FileStorage& fs) const
    {
        fs << "{:" << "C" << this->ClassId << "O" << this->ObjectId << "F" << this->FrameNumber << "B" << this->BoundingBox << "}";
    }

    void read(const cv::FileNode& node)
    {
        this->ClassId = (int)node["C"];
        this->ObjectId = (int)node["O"];
        this->FrameNumber = (int)node["F"];
        node["B"] >> this->BoundingBox;
    }

    int ClassId;
    int ObjectId;
    int FrameNumber;
    cv::Rect2i BoundingBox;
};

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



// this is the class that handles the indexing of all of the annotation objects

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


    void clear();   // the ultimate killer - simply clear all of the vectors


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


class AnnotationsSet
{
public:
    AnnotationsSet();
    ~AnnotationsSet();


    // standard accessors stuff.. those images are returned for index = currentImgIndex
    const cv::Mat& getCurrentOriginalImg() const;
    const cv::Mat& getCurrentAnnotationsClasses() const;
    const cv::Mat& getCurrentAnnotationsIds() const;

    // access any image within the buffer
    const cv::Mat& getOriginalImg(int id) const;
    const cv::Mat& getAnnotationsClasses(int id) const;
    const cv::Mat& getAnnotationsIds(int id) const;

    // get the object Id (within the record) to which the pixel (x,y) belongs in the current image
    int getObjectIdAtPosition(int x, int y) const;


    int getCurrentFramePosition() const { return this->currentImgIndex; }




    bool loadOriginalImage(const std::string& imgFileName);
    bool loadOriginalVideo(const std::string& videoFileName);
    bool loadAnnotations(const std::string& annotationsFileName);


    bool loadNextFrame();
    bool loadFrame(int fId);

    void closeFile(bool pleaseSave=false);


    bool isVideoOpen() const;
    bool isImageOpen() const;


    bool canReadNextFrame() const;
    bool canReadPrevFrame() const;





    bool saveCurrentAnnotationImage(const std::string& forcedFileName="") const;

    bool saveCurrentState(const std::string& forceFileName="") const;



    int addAnnotation(const cv::Mat& mask, const cv::Point2i& topLeftCorner, int whichClass, const cv::Point2i& annotStartingPoint=cv::Point2i(-1,-1));
            // add an annotation.
            // mask is in CV_8UC1 format, whatever is not zero belongs to the object.
            // annotStartingPoint is there to know whether we add this to a previous annotation (in which case we merge both), or whether we start a new one
            // the return index is the index of the ID, which is set automatically by this class
            // note that the merging thing only occurs when the starting point corresponds to an annotation of the same type

    void removePixelsFromAnnotations(const cv::Mat& mask, const cv::Point2i& topLeftCorner);
            // remove any class from an annotation at the pixel locations marked with a non-zero mask value.


    const AnnotationsConfig& getConfig() const { return this->config; }
    const AnnotationsRecord& getRecord() const { return this->annotsRecord; }
    // AnnotationsConfig& accessConfig() { return this->config; }  // non-const accessor version, should be used only very occasionnally..

    void addClassProperty(const AnnotationsProperties& ap);



private:
    void setDefaultConfig();


    void handleAnnotationsModifications(const std::vector<cv::Point2i>& affectedObjectsList, const std::vector<cv::Rect>& affectedBoundingBoxes);
                // when performing a new annotation or removing pixels, we are susceptible of modifying previous annotations or even deleting them
                // this method is designed to handle such cases




    cv::Mat& accessCurrentAnnotationsClasses();
    cv::Mat& accessCurrentAnnotationsIds();


    // this set of vectors stores the original images as well as their corresponding annotations.
    // those are buffers, since we are likely to annotate a video and it is convenient to recall the previous frames easily
    std::vector<cv::Mat> originalImagesBuffer;  	// original image, in CV8U_C1 or CV8UC3 (BGR) format
    std::vector<cv::Mat> annotationsClassesBuffer;	// corresponding class for every pixel, in CV_16SC1 format
    std::vector<cv::Mat> annotationsIdsBuffer;		// corresponding object Id for every pixel, in CV_32SC1 format

    // in order to know where we are within the buffer
    int currentImgIndex;
    int maxImgReached;
    bool reachedTheEndOfVideo;

    // those buffers will be assigned dynamically, in case we need to change their length
    int bufferLength;

    // store the configuration locally. I'm not sure about what we should do if it is modified during the execution
    AnnotationsConfig config;

    // store the record - kind of a way of indexing annotations into the system
    AnnotationsRecord annotsRecord;

    // this vector stores the first known available per class
    //std::vector<int> firstKnownAvailableObjectId;

    // video accessor object
    cv::VideoCapture vidCap;


    // store the image/video loaded
    std::string imageFileName;
    std::string imageFilePath;
};




#endif // ANNOTATIONSSET_H
