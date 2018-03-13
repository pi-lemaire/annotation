#include "SuperPixelsAnnotate.h"


/*
class SuperPixelsAnnotate : public ParamsHandler
{
public:
    SuperPixelsAnnotate::SuperPixelsAnnotate(AnnotationsSet*);

    void buildSPMap();
    void expandAnnotation(int annotId);


protected:
    virtual void initParamsHandler();

private:
    AnnotationsSet* originAnnots;
    cv::Mat labelContoursMask;

    int iterationsNumber;
    int regionSize;
    float ruler;


    cv::Ptr<cv::ximgproc::SuperpixelSLIC> SPSegPtr;
};
*/


using namespace cv;
using namespace std;


SuperPixelsAnnotate::SuperPixelsAnnotate(AnnotationsSet* AS)
{
    this->originAnnots = AS;

    this->setDefaultConfig();
}



void SuperPixelsAnnotate::setDefaultConfig()
{
    this->iterationsNumber = 20;
    this->regionSize = 15;
    this->ruler = 10.;
    this->gaussianBlurKernel = 3;
    this->enforceConnectivityElemSize = 25;

    this->initParamsHandler();
}


void SuperPixelsAnnotate::initParamsHandler()
{
    this->parametersSectionName = "Superpixels Configuration";

    this->pushParam<int>("Iterations Number", &(this->iterationsNumber), "");
    this->pushParam<int>("Regions Size", &(this->regionSize), "");
    this->pushParam<float>("Ruler", &(this->ruler), "");
    this->pushParam<int>("Gaussian Blur Kernel", &(this->gaussianBlurKernel), "Used to smooth images before computing the super pixels map (use 1 to disable)");
    this->pushParam<int>("Enforce Connectivity", &(this->enforceConnectivityElemSize), "in %, used to enforce the relative connectivity elements sizes");
}



void SuperPixelsAnnotate::buildSPMap()
{
    this->labelContoursMask.release();
    this->labelsMap.release();

    if (this->originAnnots->getCurrentOriginalImg().data)
    {
        this->lastKnownFileName = this->originAnnots->getOpenedFileName();
        this->lastKnownFrameNumber = this->originAnnots->getCurrentFramePosition();

        Mat origImg;

        if (this->gaussianBlurKernel>1)
            GaussianBlur(this->originAnnots->getCurrentOriginalImg(), origImg, Size(this->gaussianBlurKernel, this->gaussianBlurKernel), 0);
        else
            origImg = this->originAnnots->getCurrentOriginalImg();


        this->SPSegPtr = ximgproc::createSuperpixelSLIC( origImg,
                                                         ximgproc::SLIC::SLICO,
                                                         this->regionSize,
                                                         this->ruler );

        this->SPSegPtr->iterate(this->iterationsNumber);

        if (this->enforceConnectivityElemSize>0)
            this->SPSegPtr->enforceLabelConnectivity(this->enforceConnectivityElemSize);

        this->SPSegPtr->getLabelContourMask(this->labelContoursMask);
        this->SPSegPtr->getLabels(this->labelsMap);
    }
}


void SuperPixelsAnnotate::clearMap()
{
    this->labelContoursMask.release();
    this->labelsMap.release();
    this->SPSegPtr.release();
}


const cv::Mat& SuperPixelsAnnotate::getContoursMask()
{
    // put some protection : prevents the app from providing a map that was initialized on another image

    if ( (this->lastKnownFileName != this->originAnnots->getOpenedFileName()) ||
         (this->lastKnownFrameNumber != this->originAnnots->getCurrentFramePosition()) )
        this->clearMap();

    return this->labelContoursMask;
}


void SuperPixelsAnnotate::expandAnnotation(int annotId)
{
    // look at the annotation and everytime we find a pixel inside a superpixel,
    // add all the other pixels of this superpixel if they weren't in the annotation already

    // first : verify that the data within the SP map is compliant with what we can find in annotId
    if (!this->getContoursMask().data)
        return;

    // then retrieve the current object characteristics
    const AnnotationObject& currAnnot = this->originAnnots->getRecord().getAnnotationById(annotId);

    if (currAnnot.FrameNumber != this->lastKnownFrameNumber)
        return;

    // store the classes and ids images references for more convenience
    const Mat& currImClasses = this->originAnnots->getCurrentAnnotationsClasses();
    const Mat& currImObjIds  = this->originAnnots->getCurrentAnnotationsIds();

    // keep a reference of a point within the current annotation
    Point2i startingPoint;

    // run through the image and find the labels
    vector<int> listLabels;
    for (int i=currAnnot.BoundingBox.tl().y; i<currAnnot.BoundingBox.br().y; i++)
    {
        for (int j=currAnnot.BoundingBox.tl().x; j<currAnnot.BoundingBox.br().x; j++)
        {
            if ((currImClasses.at<int16_t>(i,j)==currAnnot.ClassId) && (currImObjIds.at<int32_t>(i,j)==currAnnot.ObjectId))
            {
                // we're in the annotation that we want to grow
                int currSPLabel = this->labelsMap.at<int32_t>(i,j);

                startingPoint = Point2i(j,i);

                // find its position within the listLabels vector
                bool notFound = true;
                for (size_t k=0; k<listLabels.size(); k++)
                {
                    if (currSPLabel==listLabels[k])
                    {
                        notFound = false;
                        break;  // the label is already recorded, no need to add it
                    }
                    else if (currSPLabel<listLabels[k])
                    {
                        // insert it
                        listLabels.insert(listLabels.begin()+k, currSPLabel);
                        notFound = false;
                        break;
                    }
                }

                if (notFound)
                    // currSPLabel is greater than any other label recorded, store it at the end
                    listLabels.push_back(currSPLabel);
            }
        }
    }


    /*
    for (size_t k=0; k<listLabels.size(); k++)
        std::cout << "label in the list : id " << listLabels[k] << std::endl;
        */


    // now find the bounding box of the new object
    bool topReached=false, leftReached=false, rightReached=false, bottomReached=false;
    Rect2i newBB = currAnnot.BoundingBox;


    // trying to grow the BB as much as possible
    while (!(topReached && leftReached && rightReached && bottomReached))
    {
        // prevent from being fooled by weird-shaped SPs
        topReached=true; leftReached=true; rightReached=true; bottomReached=true;

        if (newBB.tl().x>1)
        {
            // evaluating the left side
            int v = newBB.tl().x - 1;
            for (int u=newBB.tl().y; u<newBB.br().y; u++)
            {
                int currLbl = this->labelsMap.at<int32_t>(u,v);
                for (size_t k=0; k<listLabels.size(); k++)
                {
                    if (currLbl==listLabels[k])
                    {
                        // found an object - we expand the BB
                        leftReached = false;
                        newBB.x--;
                        newBB.width++;  // don't forget to increase the width!!!
                        break;
                    }
                }
                if (!leftReached)
                    break;
            }
        }

        if (newBB.tl().y>1)
        {
            // evaluating the top side
            int u = newBB.tl().y - 1;
            for (int v=newBB.tl().x; v<newBB.br().x; v++)
            {
                int currLbl = this->labelsMap.at<int32_t>(u,v);
                for (size_t k=0; k<listLabels.size(); k++)
                {
                    if (currLbl==listLabels[k])
                    {
                        // found an object - we expand the BB
                        topReached = false;
                        newBB.y--;
                        newBB.height++;  // don't forget to increase the width!!!
                        break;
                    }
                }
                if (!topReached)
                    break;
            }
        }

        if (newBB.br().x < this->labelsMap.cols)
        {
            // evaluating the right side
            int v = newBB.br().x;
            for (int u=newBB.tl().y; u<newBB.br().y; u++)
            {
                int currLbl = this->labelsMap.at<int32_t>(u,v);
                for (size_t k=0; k<listLabels.size(); k++)
                {
                    if (currLbl==listLabels[k])
                    {
                        // found an object - we expand the BB
                        rightReached = false;
                        newBB.width++;
                        break;
                    }
                }
                if (!rightReached)
                    // no need to go further
                    break;
            }
        }

        if (newBB.tl().y < this->labelsMap.rows)
        {
            // evaluating the bottom side
            int u = newBB.br().y;
            for (int v=newBB.tl().x; v<newBB.br().x; v++)
            {
                const int currLbl = this->labelsMap.at<int32_t>(u,v);
                for (size_t k=0; k<listLabels.size(); k++)
                {
                    if (currLbl==listLabels[k])
                    {
                        // found an object - we expand the BB
                        bottomReached = false;
                        newBB.height++;
                        break;
                    }
                }
                if (!bottomReached)
                    // no need to go further
                    break;
            }
        }
    }

    // std::cout << "previous BB : " << currAnnot.BoundingBox << " ; new BB : " << newBB << std::endl;

    // now building the mask with the new pixels
    Mat newAnnotMask = Mat::zeros(newBB.size(), CV_8UC1);

    int minusI = newBB.tl().y;
    int minusJ = newBB.tl().x;
    for (int i=newBB.tl().y; i<newBB.br().y; i++)
    {
        for (int j=newBB.tl().x; j<newBB.br().x; j++)
        {
            int currLbl = this->labelsMap.at<int32_t>(i,j);
            for (size_t k=0; k<listLabels.size(); k++)
            {
                if (currLbl==listLabels[k])
                {
                    // we're inside the new annotation
                    newAnnotMask.at<uchar>(i-minusI, j-minusJ) = 255;
                    break;
                }
                /*
                else if (currLbl>listLabels[k])
                    // the list is ordered, it doesn't make any sense to go further
                    break;
                    */
            }
        }
    }

    // that's all, we can add this back to the previously recorded annotation

    this->originAnnots->addAnnotation(newAnnotMask, newBB.tl(), currAnnot.ClassId, startingPoint);
}


