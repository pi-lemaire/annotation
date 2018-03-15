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
    this->buttonGroupAnnotations =       new QPushButton(tr("&Group", "Annotations Browser"));
    this->buttonSeparateAnnotations =    new QPushButton(tr("S&eparate", "Annotation Browser"));
    this->buttonDeleteAnnotations =      new QPushButton(tr("&Delete", "Annotation Browser"));
    this->buttonSwitchAnnotationsClass = new QPushButton(tr("S&witch Class", "Annotation Browser"));
    this->buttonLockAnnotations =        new QPushButton(tr("&Lock", "Annotation Browser"));
    this->buttonUnlockAnnotations =      new QPushButton(tr("&Unlock", "Annotation Browser"));

    // then the layout
    this->browserLayout = new QGridLayout;
    this->browserLayout->addWidget(this->browser, 0, 0, 1, 2);
    this->browserLayout->addWidget(this->buttonDeleteAnnotations, 1, 0, 1, 1);
    this->browserLayout->addWidget(this->buttonSwitchAnnotationsClass, 1, 1, 1, 1);
    this->browserLayout->addWidget(this->buttonGroupAnnotations, 2, 0, 1, 1);
    this->browserLayout->addWidget(this->buttonSeparateAnnotations, 2, 1, 1, 1);
    this->browserLayout->addWidget(this->buttonLockAnnotations, 3, 0, 1, 1);
    this->browserLayout->addWidget(this->buttonUnlockAnnotations, 3, 1, 1, 1);

    // set the layout as the main thing
    this->setLayout(this->browserLayout);

    // connect the buttons
    QObject::connect(this->buttonGroupAnnotations,       SIGNAL(clicked()), this, SLOT(GroupAnnotationsClicked()));
    QObject::connect(this->buttonSeparateAnnotations,    SIGNAL(clicked()), this, SLOT(SeparateAnnotationsClicked()));
    QObject::connect(this->buttonDeleteAnnotations,      SIGNAL(clicked()), this, SLOT(DeleteAnnotationsClicked()));
    QObject::connect(this->buttonSwitchAnnotationsClass, SIGNAL(clicked()), this, SLOT(SwitchAnnotationsClassClicked()));
    QObject::connect(this->buttonLockAnnotations,        SIGNAL(clicked()), this, SLOT(LockAnnotationsClicked()));
    QObject::connect(this->buttonUnlockAnnotations,      SIGNAL(clicked()), this, SLOT(UnlockAnnotationsClicked()));

    // connect the browser
    QObject::connect(this->browser, SIGNAL(anchorClicked(QUrl)), this, SLOT(BrowserLinkClicked(QUrl)));

    this->updateBrowser(-1);
    this->currentClassSelected = 1; // default class selected
}


AnnotationsBrowser::~AnnotationsBrowser()
{

}


void AnnotationsBrowser::setClassSelected(int whichClass)
{
    this->currentClassSelected = whichClass;
    this->setButtonsActivation();
}



void AnnotationsBrowser::checkSelected()
{
    // checks the currently selected annotation
    if (this->currentAnnotSelected != -1)
    {
        // verify whether it's already in the lines checked record
        bool notYetChecked = true;

        for (size_t k=0; k<this->linesChecked.size(); k++)
        {
            if (this->linesChecked[k].recordId == this->currentAnnotSelected)
            {
                notYetChecked = false;
                break;
            }
        }

        if (notYetChecked)
        {
            // store the new checked object
            annotSelection newSel;
            newSel.recordId = this->currentAnnotSelected;

            // storing additional information will allow us to adapt when some annotation operations
            // affect the various IDs recorded
            newSel.frameId = this->annots->getRecord().getAnnotationById(this->currentAnnotSelected).FrameNumber;
            newSel.classId = this->annots->getRecord().getAnnotationById(this->currentAnnotSelected).ClassId;
            newSel.objectId = this->annots->getRecord().getAnnotationById(this->currentAnnotSelected).ObjectId;
            newSel.locked = this->annots->getRecord().getAnnotationById(this->currentAnnotSelected).locked;

            this->linesChecked.push_back(newSel);

            this->updateBrowser(this->currentAnnotSelected);
        }
    }
}



void AnnotationsBrowser::uncheckSelected()
{
    // unchecks the currently selected annotation
    if (this->currentAnnotSelected != -1)
    {
        // search for the corresponding line
        bool changesPerformed = false;

        for (size_t k=0; k<this->linesChecked.size(); k++)
        {
            if (this->linesChecked[k].recordId == this->currentAnnotSelected)
            {
                // found it, delete the line
                this->linesChecked.erase(this->linesChecked.begin()+k);
                changesPerformed = true;
                break;
            }
        }

        if (changesPerformed)
        {
            this->updateBrowser(this->currentAnnotSelected);
        }
    }
}




void AnnotationsBrowser::uncheckAll()
{
    this->linesChecked.clear();
    this->updateBrowser(this->currentAnnotSelected);
}




void AnnotationsBrowser::updateBrowser(int selected)
{
    this->currentAnnotSelected = selected;


    // verify that the checked lines are still coherent with the content of the record
    // starting by the end in case we need to erase elements dynamically
    for (int k=(int)this->linesChecked.size()-1; k>=0; k--)
    {
        // the record ID might have changed, but not the objects, class and frames
        int testRecId = this->annots->getRecord().searchAnnotation(linesChecked[k].frameId, linesChecked[k].classId, linesChecked[k].objectId);

        if (testRecId==-1)
            // no object found, we uncheck it
            this->linesChecked.erase(this->linesChecked.begin()+k);
        else
            // found something. However, the record ID might be different - we force the update, it doesn't cost anything more
            this->linesChecked[k].recordId = testRecId;
    }




    // the following section tries to determine the best browser localization

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

    QString htmlCode;
    htmlCode += "<table width=100%>";

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
            QString checkedCharacter = "[_]";
            QString lockedCharacter = (currObj.locked ? "-L-" : "-u-");

            for (size_t k=0; k<this->linesChecked.size(); k++)
            {
                if (this->linesChecked[k].recordId==currAnnotId)
                {
                    checkedCharacter = "[X]";
                    break;
                }
            }

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
            htmlCode += "<td align=left>- " + boldTagOpen + selectionLink + QString::fromStdString(this->annots->getConfig().getProperty(currObj.ClassId).className) + "</a>" + boldTagClosed + " -</td>";
            htmlCode += "<td align=left>" + boldTagOpen + selectionLink + tr("Object #", "Annotation Browser") + QString::number(currObj.ObjectId) + "</a>" + boldTagClosed + "</td>";
            htmlCode += "<td align=middle>" + boldTagOpen + lockedCharacter + boldTagClosed + "</td>";
            htmlCode += "<td align=center>" + boldTagOpen + "<a href='chk_" + QString::number(currAnnotId) + "'>" + checkedCharacter + "</a>" + boldTagClosed + "</td>";
            htmlCode += "</tr>";
        }
    }

    htmlCode += "</table>";

    this->browser->setHtml(htmlCode);

    this->browser->setSource(QUrl("#source"));

    this->setButtonsActivation();
}




void AnnotationsBrowser::GroupAnnotationsClicked()
{
    this->annots->mergeAnnotations(this->getCheckedRecordIds());

    this->linesChecked.clear();

    emit changesCausedByTheBrowser();

    emit annotationSelected(-1);
}

void AnnotationsBrowser::SeparateAnnotationsClicked()
{
    this->annots->separateAnnotations(this->getCheckedRecordIds());

    this->linesChecked.clear();

    // emit changesCausedByTheBrowser();
    // this time, don't emit changesCaused - it won't affect anything visually

    emit annotationSelected(-1);
}



void AnnotationsBrowser::DeleteAnnotationsClicked()
{
    this->annots->deleteAnnotations(this->getCheckedRecordIds());

    this->linesChecked.clear();

    emit changesCausedByTheBrowser();

    emit annotationSelected(-1);
}


void AnnotationsBrowser::SwitchAnnotationsClassClicked()
{
    this->annots->switchAnnotationsToClass(this->getCheckedRecordIds(), this->currentClassSelected);

    this->linesChecked.clear();

    emit changesCausedByTheBrowser();

    emit annotationSelected(-1);
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
        bool unchecked = false;

        // check wether it's a selection or a de-selection
        for (size_t k=0; k<this->linesChecked.size(); k++)
        {
            if (linkId == this->linesChecked[k].recordId)
            {
                this->linesChecked.erase(this->linesChecked.begin()+k);

                unchecked = true;
                break;
            }
        }

        if (!unchecked)
        {
            // store the new checked object
            annotSelection newSel;
            newSel.recordId = linkId;

            // storing additional information will allow us to adapt when some annotation operations
            // affect the various IDs recorded
            newSel.frameId  = this->annots->getRecord().getAnnotationById(linkId).FrameNumber;
            newSel.classId  = this->annots->getRecord().getAnnotationById(linkId).ClassId;
            newSel.objectId = this->annots->getRecord().getAnnotationById(linkId).ObjectId;
            newSel.locked   = this->annots->getRecord().getAnnotationById(linkId).locked;

            this->linesChecked.push_back(newSel);
        }

        this->updateBrowser(this->currentAnnotSelected);
    }

}


void AnnotationsBrowser::LockAnnotationsClicked()
{
    for (size_t k=0; k<this->linesChecked.size(); k++)
    {
        this->annots->setObjectLock(this->linesChecked[k].recordId, true);
        this->linesChecked[k].locked = true;
    }

    this->updateBrowser(this->currentAnnotSelected);
}

void AnnotationsBrowser::UnlockAnnotationsClicked()
{
    for (size_t k=0; k<this->linesChecked.size(); k++)
    {
        this->annots->setObjectLock(this->linesChecked[k].recordId, false);
        this->linesChecked[k].locked = false;
    }

    this->updateBrowser(this->currentAnnotSelected);
}


void AnnotationsBrowser::setButtonsActivation()
{
    // by default
    this->buttonGroupAnnotations->setEnabled(false);
    this->buttonSeparateAnnotations->setEnabled(false);
    this->buttonSwitchAnnotationsClass->setEnabled(false);
    this->buttonLockAnnotations->setEnabled(false);
    this->buttonUnlockAnnotations->setEnabled(false);


    // no deletion available if no line is checked
    this->buttonDeleteAnnotations->setEnabled(this->linesChecked.size()>0);


    const char LUSTrueNumberGoal = 1+2+4;
    char LUSTrueNumber = 0;

    // check if some checked object can change class
    for (size_t k=0; k<this->linesChecked.size(); k++)
    {
        if (linesChecked[k].classId != this->currentClassSelected)
        {
            this->buttonSwitchAnnotationsClass->setEnabled(true);
            LUSTrueNumber |= 1;
        }
        if (linesChecked[k].locked)
        {
            this->buttonUnlockAnnotations->setEnabled(true);
            LUSTrueNumber |= 2;
        }
        else
        {
            this->buttonLockAnnotations->setEnabled(true);
            LUSTrueNumber |= 4;
        }

        if (LUSTrueNumber == LUSTrueNumberGoal)
            break;
    }


    // check if the group or separate functionnalities can be used
    if (this->linesChecked.size()>1)
    {
        // group is available only if at least 2 objects of the same class are active
        // separate is available only if an unique object was mentionned at least 2 times
        std::vector<annotSelection> checkedCopies = this->linesChecked;

        // annotsBrowserUtilities::sortSelections(checkedCopies);
        sort(checkedCopies.begin(), checkedCopies.end(),
             [](const annotSelection& a, const annotSelection& b) -> bool{
            return ((a.classId<b.classId) || (a.classId==b.classId && a.objectId<b.objectId));}
        );

        annotSelection prevAnnot = checkedCopies[0];

        // trueNumber is some kind of bitwise object to prevent us from spending t
        char trueNumber = 0;
        const char trueNumberGoal = 1+2;

        /*
        if (prevAnnot.classId != this->currentClassSelected)
        {
            this->buttonSwitchAnnotationsClass->setEnabled(true);
            trueNumber = 4;
        }
        */

        for (size_t k=1; k<checkedCopies.size(); k++)
        {
            /*
            if (checkedCopies[k].classId != this->currentClassSelected)
            {
                // some checked object is from a different class than the one selected
                this->buttonSwitchAnnotationsClass->setEnabled(true);
                trueNumber |= 4;
            }
            */

            if ( (checkedCopies[k].classId == prevAnnot.classId)
                 && (this->annots->accessConfig().getProperty(prevAnnot.classId).classType != _ACT_Uniform) )
            {
                // multiple objects class, and same class - the only situation when group or separate can have some meaning
                if (checkedCopies[k].objectId == prevAnnot.objectId)
                {
                    // at least 2 occurrences of the same object
                    this->buttonSeparateAnnotations->setEnabled(true);
                    trueNumber |= 1;
                }
                else if (checkedCopies[k].objectId != prevAnnot.objectId)
                {
                    // at least 2 different objects of the same class
                    this->buttonGroupAnnotations->setEnabled(true);
                    trueNumber |= 2;
                }


                if (trueNumber == trueNumberGoal)
                    // both buttons are already set to true, no need to go further
                    break;
            }

            prevAnnot = checkedCopies[k];
        }

    }

}




std::vector<int> AnnotationsBrowser::getCheckedRecordIds() const
{
    std::vector<int> listIds;
    for (size_t k=0; k<this->linesChecked.size(); k++)
        listIds.push_back(this->linesChecked[k].recordId);
    return listIds;
}






