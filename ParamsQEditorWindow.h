#ifndef PARAMSQEDITORWINDOW_H
#define PARAMSQEDITORWINDOW_H




#include "ParamsQEditorLine.h"


#include <QWidget>
#include <QScrollArea>
#include <QGroupBox>
#include <QVBoxLayout>





class ParamsQEditorWindow : public QScrollArea
{
    Q_OBJECT

public:
    ParamsQEditorWindow(ParamsHandler* pH);
    ~ParamsQEditorWindow();

public slots:
    void someValueChanged();

private:
    QGroupBox *sectionGroup;
    QVBoxLayout *groupLayout, *globalLayout;
    QWidget *scrollContent;
    std::vector<ParamsQEditorLine*> editorLines;
};



#endif // PARAMSQEDITORWINDOW_H
