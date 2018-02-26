#include "AnnotationsBrowser.h"



AnnotationsBrowser::AnnotationsBrowser(AnnotationsSet* annotsSet, QWidget* parent, Qt::WindowFlags flags): QWidget( parent, flags )
{
    // storing the pointer to the annotations system - we will only use the link to the config
    this->annots = annotsSet;

    // creating the main widget
    this->browser = new QTextBrowser;
    this->browser->setMaximumSize(QSize(300, 500));
    this->browser->setOpenLinks(false);

    // the annotations manipulation buttons
    this->buttonGroupAnnotations = new QPushButton(tr("&Group", "Annotations Browser"));
    this->buttonSeparateAnnotations = new QPushButton(tr("S&eparate", "Annotation Browser"));
    this->buttonDeleteAnnotations = new QPushButton(tr("&Delete", "Annotation Browser"));

    // then the layout
    this->browserLayout = new QGridLayout;
    this->browserLayout->addWidget(this->browser, 0, 0, 1, 3);
    this->browserLayout->addWidget(this->buttonGroupAnnotations, 1, 0, 1, 1);
    this->browserLayout->addWidget(this->buttonSeparateAnnotations, 1, 1, 1, 1);
    this->browserLayout->addWidget(this->buttonDeleteAnnotations, 1, 2, 1, 1);

    // set the layout as the main thing
    this->setLayout(this->browserLayout);

    // connect the buttons
    QObject::connect(this->buttonGroupAnnotations, SIGNAL(clicked()), this, SLOT(GroupAnnotationsClicked()));
    QObject::connect(this->buttonSeparateAnnotations, SIGNAL(clicked()), this, SLOT(SeparateAnnotationsClicked()));
    QObject::connect(this->buttonDeleteAnnotations, SIGNAL(clicked()), this, SLOT(DeleteAnnotationsClicked()));

    // connect the browser
    QObject::connect(this->browser, SIGNAL(anchorClicked(QUrl)), this, SLOT(BrowserLinkClicked(QUrl)));

    this->updateBrowser(-1);
}


AnnotationsBrowser::~AnnotationsBrowser()
{

}




void AnnotationsBrowser::updateBrowser(int selected)
{
    this->currentAnnotSelected = selected;

    QString htmlCode;
    htmlCode += "<table width=100%>";


    // by default, starting from the current frame...
    int startingFrame = this->annots->getCurrentFramePosition();

    // trying to find where to find the best source position
    // the indices are ordered as the following : x=frame number, y=object id within the framesIndex vector
    cv::Point2i bestSourcePosition(0,0);

    bool theBestSourceIsTheLastAnnotFromPrevFrame = true;


    if (selected != -1)
    {
        // some annotation was selected with the help of the browser, or through the annotation area

        // correcting the starting frame id, if needed (it should never be needed, actually)
        startingFrame = this->annots->getRecord().getAnnotationById(selected).FrameNumber;

        // we store locally the frame record content pointer for more convenience
        const std::vector<int>& frameRecordContent = this->annots->getRecord().getFrameContentIds(startingFrame);

        // the selected object is not the first from the frame, we can start the source within this frame
        if (frameRecordContent.size()>0 && frameRecordContent[0] != selected)
        {
            // don't forget to update the information
            theBestSourceIsTheLastAnnotFromPrevFrame = false;
            bestSourcePosition = cv::Point2i(startingFrame, 0);

            // find the best position
            for (int k=1; k<(int)frameRecordContent.size(); k++)
            {
                if (frameRecordContent[k]==selected)
                    break;
                bestSourcePosition.y = k;
            }
        }
    }


    if (theBestSourceIsTheLastAnnotFromPrevFrame)
    {
        // the source localization will be based on the frame id
        for (int k=startingFrame-1; k>=0; k--)
        {
            // find the last annotation from the previous frame
            if (this->annots->getRecord().getFrameContentIds(k).size()>0)
            {
                bestSourcePosition.x = k;
                bestSourcePosition.y = (int)this->annots->getRecord().getFrameContentIds(k).size() - 1;
                break;
            }
        }
    }


    // alright, we can start doing the actual stuff of feeding the browser area

    // frames loop
    for (int iFrame=0; iFrame<this->annots->getRecord().getRecordedFramesNumber(); iFrame++)
    {
        // objects loop
        for (int jAnnotId=0; jAnnotId<(int)this->annots->getRecord().getFrameContentIds(iFrame).size(); jAnnotId++)
        {
            int currAnnotId = this->annots->getRecord().getFrameContentIds(iFrame)[jAnnotId];

            AnnotationObject currObj = this->annots->getRecord().getAnnotationById(currAnnotId);

            QString boldTagOpen = "";
            QString boldTagClosed = "";
            QString sourceTag = "";

            if (cv::Point2i(iFrame, jAnnotId) == bestSourcePosition)
                sourceTag = "<a name='source'></a>";

            if (currAnnotId == selected)
            {
                boldTagOpen = "<b>";
                boldTagClosed = "</b>";
            }

            QString selectionLink = "<a href='sel_" + QString::number(currAnnotId) + "'>";

            htmlCode += "<tr>";
            htmlCode += "<td align=left>" + sourceTag + boldTagOpen + selectionLink + tr("Frame", "Annotation Browser") + " " + QString::number(currObj.FrameNumber) + "</a>" + boldTagClosed + "</td>";
            htmlCode += "<td align=left>- " + boldTagOpen + selectionLink + QString::fromStdString(this->annots->getConfig().getProperty(currObj.ClassId).ClassName) + "</a>" + boldTagClosed + " -</td>";
            htmlCode += "<td align=left>" + boldTagOpen + selectionLink + tr("Object #", "Annotation Browser") + QString::number(currObj.ObjectId) + "</a>" + boldTagClosed + "</td>";
            // htmlCode += "<td align=right><a href='chk_" + QString::number(frameNum) + "_" + QString::number(woodblockId) + "'><img src='" + checkBoxImg + "' /></a></td>";
            htmlCode += "</tr>";
        }
    }

    htmlCode += "</table>";

    this->browser->setHtml(htmlCode);

    this->browser->setSource(QUrl("#source"));
}




void AnnotationsBrowser::GroupAnnotationsClicked()
{

}

void AnnotationsBrowser::SeparateAnnotationsClicked()
{

}

void AnnotationsBrowser::DeleteAnnotationsClicked()
{

}


void AnnotationsBrowser::BrowserLinkClicked(const QUrl& url)
{

    QString urlVal = url.url();

    // get the pointed object ID
    int linkId = urlVal.mid( urlVal.lastIndexOf("_") + 1 ).toInt();

    if (urlVal[0] == 's')
    {
        // annotation selection
        this->currentAnnotSelected = linkId;

        emit annotationSelected(linkId);
    }

    if (urlVal[0] == 'c')
    {
        /*
        // woodblock checked for diverse operations...
        if (this->checkedAnnotation(linkFrameId, linkWBId))
            this->uncheckAnAnnotation(linkFrameId, linkWBId);
        else
            this->checkAnAnnotation(linkFrameId, linkWBId);

        this->updateBlocksSelection();
        this->updateManipulateButtons();
        */
    }

}











