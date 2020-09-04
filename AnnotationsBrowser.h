#ifndef ANNOTATIONSBROWSER_H
#define ANNOTATIONSBROWSER_H



#include <QTextBrowser>
#include <QPushButton>
#include <QGridLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QIntValidator>
#include <QUrl>


#include "AnnotationsSet.h"


struct annotSelection
{
    int recordId, frameId, classId, objectId;
    bool locked;
};





class AnnotationsBrowser : public QWidget
{
    Q_OBJECT

public:
    // AnnotationsBrowser(AnnotationsSet* annotsSet, QWidget *parent=0, Qt::WindowFlags f=NULL);
    AnnotationsBrowser(AnnotationsSet* annotsSet, QWidget *parent=0);
    ~AnnotationsBrowser();


public slots:
    void updateBrowser(int);    // the int corresponds to the selected annotation // -1 means none selected
    void setClassSelected(int); // this is useful for this object to know which class is selected, for the "switch" functionnality
    void checkSelected();       // allows to check an object already selected (for instance using the right click), for browser operations such as delete, merge, etc
    void uncheckSelected();     // remove the selected annotation from the checked list
    void uncheckAll();          // clear the checked list
    void GroupAnnotationsClicked();
    void SeparateAnnotationsClicked();
    void DeleteAnnotationsClicked();
    void SwitchAnnotationsClassClicked();
    void LockAnnotationsClicked();
    void UnlockAnnotationsClicked();

private slots:
    void BrowserLinkClicked(const QUrl&);
    void filtersModified();


signals:
    void annotationSelected(int);
    void changesCausedByTheBrowser(QRect);


private:
    void setButtonsActivation();

    std::vector<int> getCheckedRecordIds() const;

    QPushButton *buttonGroupAnnotations, *buttonSeparateAnnotations, *buttonDeleteAnnotations, *buttonSwitchAnnotationsClass, *buttonLockAnnotations, *buttonUnlockAnnotations;
    QGridLayout *browserLayout;
    QTextBrowser *browser;

    // filters section
    QCheckBox *filterFramesNeighborhoodCheckBox, *filterClassCheckBox, *filterObjectAreaCheckBox, *filterObjectCheckBox;
    QGroupBox *filtersGroupBox;
    QGridLayout *filtersLayout;
    QLineEdit *filterFramesNeighborhoodLineEdit, *filterObjectAreaLineEdit;

    AnnotationsSet *annots;

    int currentAnnotSelected;

    int currentClassSelected;

    std::vector<annotSelection> linesChecked;
};







#endif // ANNOTATIONSBROWSER_H
