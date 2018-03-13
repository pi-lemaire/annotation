#ifndef SUPERPIXELSANNOTATE_H
#define SUPERPIXELSANNOTATE_H


#include "AnnotationsSet.h"
#include "ParamsHandler.h"

#include "opencv2/ximgproc/slic.hpp"


class SuperPixelsAnnotate : public ParamsHandler
{
public:
    SuperPixelsAnnotate(AnnotationsSet*);
    ~SuperPixelsAnnotate();

    void buildSPMap();
    void expandAnnotation(int annotId);
    const cv::Mat& getContoursMask();

    void clearMap();


protected:
    virtual void initParamsHandler();

private:
    void setDefaultConfig();

    AnnotationsSet* originAnnots;
    cv::Mat labelContoursMask, labelsMap;

    int iterationsNumber;
    int regionSize;
    float ruler;

    int gaussianBlurKernel;
    int enforceConnectivityElemSize;

    int lastKnownFrameNumber;
    std::string lastKnownFileName;


    cv::Ptr<cv::ximgproc::SuperpixelSLIC> SPSegPtr;
};




#endif // SUPERPIXELSANNOTATE_H
