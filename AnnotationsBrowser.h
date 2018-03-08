#ifndef ANNOTATIONSBROWSER_H
#define ANNOTATIONSBROWSER_H



#include <QTextBrowser>
#include <QPushButton>
#include <QGridLayout>
#include <QUrl>


#include "AnnotationsSet.h"


struct annotSelection
{
    int recordId, frameId, classId, objectId;
};


/*
namespace annotsBrowserUtilities
{
    inline void sortSelections(std::vector<annotSelection> &v)
    {
        // sort indexes based on comparing values in v
        sort( v.begin(), v.end(),
             [&v](size_t i1, size_t i2) {return ((v[i1].classId<v[i2].classId) || (v[i1].classId==v[i2].classId && v[i1].objectId<v[i2].objectId));} );
    }
}
*/




class AnnotationsBrowser : public QWidget
{
    Q_OBJECT

public:
    AnnotationsBrowser(AnnotationsSet* annotsSet, QWidget *parent=0, Qt::WindowFlags f=0);
    ~AnnotationsBrowser();


public slots:
    void updateBrowser(int);    // the int corresponds to the selected annotation // -1 means none selected


private slots:
    void GroupAnnotationsClicked();
    void SeparateAnnotationsClicked();
    void DeleteAnnotationsClicked();
    void BrowserLinkClicked(const QUrl&);


signals:
    void annotationSelected(int);
    void changesCausedByTheBrowser();


private:
    void setButtonsActivation();

    QPushButton *buttonGroupAnnotations, *buttonSeparateAnnotations, *buttonDeleteAnnotations;
    QGridLayout *browserLayout;
    QTextBrowser *browser;

    AnnotationsSet *annots;

    int currentAnnotSelected;

    std::vector<annotSelection> linesChecked;
};







#endif // ANNOTATIONSBROWSER_H
