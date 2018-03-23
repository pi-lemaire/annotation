#ifndef OPTFLOWTRACKING_H
#define OPTFLOWTRACKING_H




#include "AnnotationsSet.h"
#include "ParamsHandler.h"

#include "opencv2/ximgproc/slic.hpp"


class OptFlowTracking : public ParamsHandler
{
public:
    OptFlowTracking(AnnotationsSet*);
    ~OptFlowTracking();

    void trackAnnotations();

    int getInterpolateLength() const { return this->interpolateLength; }


protected:
    virtual void initParamsHandler();

private:
    void setDefaultConfig();

    void findAffineTransformParams(const std::vector<cv::Point2f>& input, const std::vector<cv::Point2f>& output, cv::Mat& params) const;
        // reimplementation of the equivalent of OpenCV - we have found the opencv function to be somewhat flawed

    AnnotationsSet* originAnnots;

    float scaleDownFactor;
    double pyrScale;
    int levels;
    int winsize;
    int iterations;
    int polyN;
    double polySigma;
    bool gaussianWindow;

    int interpolateLength;

    // cv::Mat lastResult;

    /*
    int lastKnownFrameNumber;
    std::string lastKnownFileName;
    */
    // cv::Ptr<cv::ximgproc::SuperpixelSLIC> SPSegPtr;
};







#endif // OPTFLOWTRACKING_H
