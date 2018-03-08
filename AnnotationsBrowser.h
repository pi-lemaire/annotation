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
    QPushButton *buttonGroupAnnotations, *buttonSeparateAnnotations, *buttonDeleteAnnotations;
    QGridLayout *browserLayout;
    QTextBrowser *browser;

    AnnotationsSet *annots;

    int currentAnnotSelected;

    std::vector<annotSelection> linesChecked;
};







#endif // ANNOTATIONSBROWSER_H
