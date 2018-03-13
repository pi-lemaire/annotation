#ifndef PARAMSQEDITORLINE_H
#define PARAMSQEDITORLINE_H




#include <QWidget>
#include "ParamsHandler.h"

#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QRegExpValidator>






class ParamsQEditorLine : public QWidget
{
    Q_OBJECT

public:
    ParamsQEditorLine(ParamInterface* pI);
    virtual ~ParamsQEditorLine();

public slots:
    void valueEdited();
    void updateContent();
    // void validatorChanged(int);

signals:
    void valueUpdated();

private:
    void fillContent();
    void fillEditLine();

    QLabel *varNameLabel, *varDescLabel;
    QGridLayout *editLayout;

    QLineEdit *paramLineEdit;
    QValidator *lineEditValidator;

    ParamInterface* pICopy;
};







#endif // PARAMSQEDITORLINE_H
