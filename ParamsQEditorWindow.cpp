

#include "ParamsQEditorWindow.h"



ParamsQEditorWindow::ParamsQEditorWindow(ParamsHandler* pH)
{
    this->sectionGroup = new QGroupBox(QString::fromStdString(pH->getParametersSectionName()));
    this->groupLayout = new QVBoxLayout;


    for (int i=0; i<(int)pH->getParamsNumber(); i++)
    {
        this->editorLines.emplace_back(new ParamsQEditorLine(pH->accessLine(i)));
        this->groupLayout->addWidget(this->editorLines[i]);

        QObject::connect(this->editorLines[i], SIGNAL(valueUpdated()), this, SLOT(someValueChanged()));
    }


    this->sectionGroup->setLayout(this->groupLayout);


    this->globalLayout = new QVBoxLayout;
    this->globalLayout->addWidget(this->sectionGroup);

    this->scrollContent = new QWidget;
    this->scrollContent->setLayout(this->globalLayout);

    this->setWidget(this->scrollContent);

    this->setWindowTitle(tr("Configuration"));

}



ParamsQEditorWindow::~ParamsQEditorWindow()
{
    QLayoutItem *item;
    while ((item = this->groupLayout->takeAt(0)))
        delete item;

    delete this->groupLayout;

    while ((item = this->globalLayout->takeAt(0)))
        delete item;

    delete this->globalLayout;

    delete this->scrollContent;
}





void ParamsQEditorWindow::someValueChanged()
{
    for (size_t i=0; i<this->editorLines.size(); i++)
    {
        this->editorLines[i]->updateContent();
    }
}



