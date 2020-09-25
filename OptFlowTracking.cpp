#include "OptFlowTracking.h"

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


OptFlowTracking::OptFlowTracking(AnnotationsSet* AS)
{
    this->originAnnots = AS;

    this->setDefaultConfig();
}



void OptFlowTracking::setDefaultConfig()
{
    this->scaleDownFactor = 1.;
    this->pyrScale = 0.4;
    this->levels = 3;
    this->winsize = 15;
    this->iterations = 3;
    this->polyN = 5;
    this->polySigma = 1.2;
    this->interpolateLength = 5;
    this->gaussianWindow = true;

    this->initParamsHandler();
}


void OptFlowTracking::initParamsHandler()
{
    this->parametersSectionName = "Dense Optical Flow Trakcing Configuration";

    this->pushParam<float>("Scale Down Factor", &(this->scaleDownFactor), "Image scaling before applying the optical flow");
    this->pushParam<double>("Pyramid Scale", &(this->pyrScale), "Relative scale of every pyramid object");
    this->pushParam<int>("Pyramid levels Number", &(this->levels), "");
    this->pushParam<int>("Window Size", &(this->winsize), "Searching window size");
    this->pushParam<int>("Iterations", &(this->iterations), "Number of iterations");
    this->pushParam<int>("Polynomial Window", &(this->polyN), "Size of the pixel neighborhood to find polynomial expansion");
    this->pushParam<double>("Polynomial Sigma", &(this->polySigma), "Standard Deviation of the gaussian used in the polynomial expansion");
    this->pushParam<bool>("Gaussian Window", &(this->gaussianWindow), "Use a gaussian instead of a box for the search");
    this->pushParam<int>("Interpolation window", &(this->interpolateLength), "Number of successive frames to analyze then to interpolate (when in bounding boxes only mode)only on BB only objects)");
}



void OptFlowTracking::trackAnnotations()
{
    // at first, find the bounding box - we're not going to work on the whole image if it is not required

    // copy the original annotations references
    const vector<int> origAnnotsIds = this->originAnnots->getRecord().getFrameContentIds(this->originAnnots->getCurrentFramePosition()-1);

    if (origAnnotsIds.size()<1)
        return; // nothing to track at all

    Rect2i workingArea = this->originAnnots->getRecord().getAnnotationById(origAnnotsIds[0]).BoundingBox;

    for (size_t k=1; k<origAnnotsIds.size(); k++)
        workingArea |= this->originAnnots->getRecord().getAnnotationById(origAnnotsIds[k]).BoundingBox;

    // increasing the size of the working area by the search window
    int increaseSize = ceil(this->winsize/this->scaleDownFactor);
    workingArea.x -= increaseSize;
    workingArea.y -= increaseSize;
    workingArea.width += 2*increaseSize;
    workingArea.height += 2*increaseSize;

    // verify that we're not outside of the image boundaries
    workingArea &= Rect2i(Point2i(0,0), this->originAnnots->getCurrentOriginalImg().size());


    Mat prevImg, currImg;

    // allright, now we can process the images on which we're going to perform the dense optical flow algorithm
    cvtColor(Mat(this->originAnnots->getOriginalImg(this->originAnnots->getCurrentFramePosition()-1), workingArea), prevImg, COLOR_BGR2GRAY);
    cvtColor(Mat(this->originAnnots->getCurrentOriginalImg(), workingArea), currImg, COLOR_BGR2GRAY);

    resize(prevImg, prevImg, Size2i(prevImg.size().width*this->scaleDownFactor, prevImg.size().height*this->scaleDownFactor));
    resize(currImg, currImg, Size2i(prevImg.size().width*this->scaleDownFactor, prevImg.size().height*this->scaleDownFactor));

    // computing the flow matrix...
    Mat flowMat;
    calcOpticalFlowFarneback( prevImg, currImg, flowMat,
                              this->pyrScale, this->levels, this->winsize,
                              this->iterations, this->polyN, this->polySigma,
                             (this->gaussianWindow ? OPTFLOW_FARNEBACK_GAUSSIAN : 0) );

    // now resizing the flow matrix
    flowMat *= (1. / this->scaleDownFactor);
    resize(flowMat, flowMat, workingArea.size());


    // alright, now what we want to do is find how previously annotated pixels have moved.
    // two options are available there :
    // 1) use the flow to displace pixels within an annotation - more logical, but no guarantee to keep the coherence of objects
    // 2) calculate the optimal transformation applied to all the pixels within the annotation, and apply it to the original annotation
    //    this second solution will have the advantage of keeping the connexivity and coherence of objects, but won't be able to track
    //    morphological changes within objects (say, the legs of someone walking, for instance)

    for (size_t k=0; k<origAnnotsIds.size(); k++)
    {
        const AnnotationObject& prevAnnot = this->originAnnots->getRecord().getAnnotationById(origAnnotsIds[k]);

        if (prevAnnot.BoundingBox.size() == Size2i(0,0))
            continue;

        Rect2i newBB( Point2i(prevAnnot.BoundingBox.tl().x-increaseSize, prevAnnot.BoundingBox.tl().y-increaseSize),
                      Size2i(prevAnnot.BoundingBox.size().width + (2*increaseSize), prevAnnot.BoundingBox.size().height + (2*increaseSize)) );

        // verify that the new BB is compliant with the image size
        newBB &= Rect2i(Point2i(0,0), this->originAnnots->getCurrentOriginalImg().size());

        Mat newAnnotationMask = Mat::zeros(newBB.size(), CV_8UC1);




        bool BBClassOnly = this->originAnnots->getConfig().getProperty(prevAnnot.ClassId).classType==_ACT_BoundingBoxOnly;
        bool FCClassOnly = this->originAnnots->getConfig().getProperty(prevAnnot.ClassId).classType==_ACT_CentroidFrontOnly;

        bool FeaturePointsClassOnly = BBClassOnly || FCClassOnly;

        Rect2i BBTracked;



        /*
        int deltaX = prevAnnot.BoundingBox.tl().x - newBB.tl().x;   // should always be greater than 0
        int deltaY = prevAnnot.BoundingBox.tl().y - newBB.tl().y;
        */
        if (false)
        {
            // this is the version 1 (see above) where we move every pixel from the coordinates in flowMat
            // this creates a lot of holes, it is difficult to keep a coherent object at the output
            // in the end, we spend more time smoothing the object than what was gained during the annotation

            // now looking at the points in the previous annotation
            const Mat& prevClassMat = this->originAnnots->getAnnotationsClasses(prevAnnot.FrameNumber);
            const Mat& prevObjIdMat = this->originAnnots->getAnnotationsIds(prevAnnot.FrameNumber);

            for (int i=prevAnnot.BoundingBox.tl().y; i<prevAnnot.BoundingBox.br().y; i++)
            {
                for (int j=prevAnnot.BoundingBox.tl().x; j<prevAnnot.BoundingBox.br().x; j++)
                {
                    if ((prevClassMat.at<int16_t>(i,j)==prevAnnot.ClassId) && (prevObjIdMat.at<int32_t>(i,j)==prevAnnot.ObjectId))
                    {
                        // found an element of this annotation, define its position in the newAnnotationMask matrix
                        int newPosI = i + flowMat.at<Vec2f>(i-workingArea.tl().y, j-workingArea.tl().x)[1] - newBB.tl().y;
                        int newPosJ = j + flowMat.at<Vec2f>(i-workingArea.tl().y, j-workingArea.tl().x)[0] - newBB.tl().x;

                        if (newPosI>=0 && newPosI<newAnnotationMask.rows && newPosJ>=0 && newPosJ<newAnnotationMask.cols)
                            // it's inside the mask, add the pixel
                            newAnnotationMask.at<uchar>(newPosI, newPosJ) = 1;
                    }
                }
            }

            // perform some filtering on the mask to avoid most holes
            medianBlur(newAnnotationMask, newAnnotationMask, 3);
        }
        else
        {
            // version 2 of the algorithm


            // what we will actually do will depend greatly on whether we're in the case of a BB only class or not
            // bool BBClassOnly = (this->originAnnots->getConfig().getProperty(prevAnnot.ClassId).classType == _ACT_BoundingBoxOnly);


            if (!FeaturePointsClassOnly)
            {
                // case pixel level : find the displacement of every pixel, find the affine transform indicated by the optical flow vector
                vector<Point2f> originPoints, destPoints;

                Mat origAnnotMask;
                newAnnotationMask.copyTo(origAnnotMask);

                // now looking at the points in the previous annotation
                const Mat& prevClassMat = this->originAnnots->getAnnotationsClasses(prevAnnot.FrameNumber);
                const Mat& prevObjIdMat = this->originAnnots->getAnnotationsIds(prevAnnot.FrameNumber);

                for (int i=prevAnnot.BoundingBox.tl().y; i<prevAnnot.BoundingBox.br().y; i++)
                {
                    for (int j=prevAnnot.BoundingBox.tl().x; j<prevAnnot.BoundingBox.br().x; j++)
                    {
                        if ((prevClassMat.at<int16_t>(i,j)==prevAnnot.ClassId) && (prevObjIdMat.at<int32_t>(i,j)==prevAnnot.ObjectId))
                        {
                            // found an element of this annotation, store its position in both the newAnnotationMask matrix and the
                            origAnnotMask.at<uchar>(i-newBB.tl().y, j-newBB.tl().x) = 255;

                            originPoints.push_back( Point2f( j-newBB.tl().x,
                                                             i-newBB.tl().y ) );    // position in the "newAnnotationMask" matrix

                            destPoints.push_back( Point2f( j - newBB.tl().x + flowMat.at<Vec2f>(i-workingArea.tl().y, j-workingArea.tl().x)[0],
                                                           i - newBB.tl().y + flowMat.at<Vec2f>(i-workingArea.tl().y, j-workingArea.tl().x)[1] ) );
                                                                                    // theoretical position in the tracked "newAnnotationMask" matrix
                        }
                    }
                }


                // calculate the affine transform
                Mat transform;
                this->findAffineTransformParams(originPoints, destPoints, transform);

                // apply it to the object mask
                warpAffine(origAnnotMask, newAnnotationMask, transform, origAnnotMask.size());

                // threshold it so that we stay in pixel level domain
                threshold(newAnnotationMask, newAnnotationMask, 127, 255, THRESH_BINARY);
            }
            else
            {
                // we will only translate the bounding box according to the median of the observed flow vector
                // (before we find something more accurate, like perhaps weighting the sum with a gaussian to take the center of the area into account?)

                std::vector<float> displacementX, displacementY;


                if (BBClassOnly)
                {
                    /*
                    const Mat& prevClassMat = this->originAnnots->getAnnotationsClasses(prevAnnot.FrameNumber);
                    const Mat& prevObjIdMat = this->originAnnots->getAnnotationsIds(prevAnnot.FrameNumber);
                    */

                    for (int i=prevAnnot.BoundingBox.tl().y; i<prevAnnot.BoundingBox.br().y; i++)
                    {
                        for (int j=prevAnnot.BoundingBox.tl().x; j<prevAnnot.BoundingBox.br().x; j++)
                        {
                            displacementX.push_back(flowMat.at<Vec2f>(i-workingArea.tl().y, j-workingArea.tl().x)[0]);
                            displacementY.push_back(flowMat.at<Vec2f>(i-workingArea.tl().y, j-workingArea.tl().x)[1]);
                        }
                    }

                    sort(displacementX.begin(), displacementX.end());
                    sort(displacementY.begin(), displacementY.end());
                    int actualDisplacementX = round(displacementX[(int)displacementX.size()/2]);
                    int actualDisplacementY = round(displacementY[(int)displacementY.size()/2]);

                    BBTracked = prevAnnot.BoundingBox;
                    BBTracked.x += actualDisplacementX;
                    BBTracked.y += actualDisplacementY;
                }
                else if (FCClassOnly)
                {
                    Point2i newCentroid=prevAnnot.Centroid, newFront=prevAnnot.Front;
                    newCentroid.x += round(flowMat.at<Vec2f>(prevAnnot.Centroid.y-workingArea.tl().y, prevAnnot.Centroid.x-workingArea.tl().x)[0]);
                    newCentroid.y += round(flowMat.at<Vec2f>(prevAnnot.Centroid.y-workingArea.tl().y, prevAnnot.Centroid.x-workingArea.tl().x)[1]);
                    newFront.x += round(flowMat.at<Vec2f>(prevAnnot.Front.y-workingArea.tl().y, prevAnnot.Front.x-workingArea.tl().x)[0]);
                    newFront.y += round(flowMat.at<Vec2f>(prevAnnot.Front.y-workingArea.tl().y, prevAnnot.Front.x-workingArea.tl().x)[1]);

                    this->originAnnots->addAnnotation(newCentroid, newFront, prevAnnot.ClassId, prevAnnot.ObjectId);
                }



            }

        }

        // finally add the new annotation
        if (BBClassOnly)
            this->originAnnots->addAnnotation(BBTracked.tl(), BBTracked.br(), prevAnnot.ClassId, prevAnnot.ObjectId);
        else if (!FCClassOnly)
            this->originAnnots->addAnnotation(newAnnotationMask, newBB.tl(), prevAnnot.ClassId, Point2i(-1,-1), prevAnnot.ObjectId);
    }
}






void OptFlowTracking::findAffineTransformParams(const std::vector<cv::Point2f>& input, const std::vector<cv::Point2f>& output, cv::Mat& params) const
{
    params = cv::Mat::eye(2, 3, CV_64FC1);

    if (input.size() != output.size())
        return;


    if (input.size()<6) // we lack solutions - we will just return the mean Tx-Ty value
    {
        float elemsNb = (float)input.size();
        for (size_t k=0; k<input.size(); k++)
        {
            params.at<double>(0,2) += (output[k].x - input[k].x) / elemsNb;
            params.at<double>(1,2) += (output[k].y - input[k].y) / elemsNb;
        }
        return;
    }



    float subSampling = 1.;
    int finalSolutionsUsed = input.size();

    if (finalSolutionsUsed>500)
    {
        // subsample the data, we don' need so many samples
        subSampling = (float) finalSolutionsUsed / 500.;
        finalSolutionsUsed = 500;
    }

    // create the left hand and right hands of the system
    cv::Mat leftHandMat = cv::Mat::zeros(finalSolutionsUsed*2, 6, CV_64FC1);
    cv::Mat rightHandMat = cv::Mat::zeros(finalSolutionsUsed*2, 1, CV_64FC1);

    // filling the corresponding values
    for (int k=0; k<finalSolutionsUsed; k++)
    {
        long int i = round(subSampling * (float)k);
        if (i>=(long int)input.size())
            break;  // some safety check

        leftHandMat.at<double>(k*2  , 0) = input[i].x;
        leftHandMat.at<double>(k*2  , 1) = input[i].y;
        leftHandMat.at<double>(k*2  , 2) = 1.;
        leftHandMat.at<double>(k*2  , 3) = 0.;
        leftHandMat.at<double>(k*2  , 4) = 0.;
        leftHandMat.at<double>(k*2  , 5) = 0.;
        leftHandMat.at<double>(k*2+1, 0) = 0.;
        leftHandMat.at<double>(k*2+1, 1) = 0.;
        leftHandMat.at<double>(k*2+1, 2) = 0.;
        leftHandMat.at<double>(k*2+1, 3) = input[i].x;
        leftHandMat.at<double>(k*2+1, 4) = input[i].y;
        leftHandMat.at<double>(k*2+1, 5) = 1.;

        rightHandMat.at<double>(k*2  , 0) = output[i].x;
        rightHandMat.at<double>(k*2+1, 0) = output[i].y;
    }

    cv::Mat solution = cv::Mat::zeros(6,1,CV_64FC1);
    solution.at<double>(0,0) = 1.0;
    solution.at<double>(1,1) = 1.0;

    if (!cv::solve(leftHandMat, rightHandMat, solution, cv::DECOMP_SVD))
        return;

    params.at<double>(0,0) = solution.at<double>(0,0);  // a
    params.at<double>(0,1) = solution.at<double>(1,0);  // b
    params.at<double>(0,2) = solution.at<double>(2,0);  // c
    params.at<double>(1,0) = solution.at<double>(3,0);  // d
    params.at<double>(1,1) = solution.at<double>(4,0);  // e
    params.at<double>(1,2) = solution.at<double>(5,0);  // f
}



