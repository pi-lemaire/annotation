#ifndef DIALOGCLASSSELECTION_H
#define DIALOGCLASSSELECTION_H

#include <QDialog>
#include <QListWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDialogButtonBox>

#include "AnnotationsSet.h"
#include "QtCvUtils.h"

// class ImageViewer;

class DialogClassSelection : public QDialog
{
    Q_OBJECT

public:
    DialogClassSelection(AnnotationsSet* annotsSet, QWidget *parent=0, Qt::WindowFlags f = 0);
    ~DialogClassSelection();


public slots:
    void OnClassSelect(QListWidgetItem *);
    void OnClassCheck(QListWidgetItem *);


signals:
    void classSelected(int classId);
    // void classLocked(int classId);


private:
    AnnotationsSet* annots;
    QVBoxLayout* topLayout;
    // QDialogButtonBox* buttonBox;
    QListWidget *pListClasses;

};

#endif
