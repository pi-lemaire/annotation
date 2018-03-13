


//#include <QtWidgets>

#include "ParamsQEditorLine.h"





ParamsQEditorLine::ParamsQEditorLine(ParamInterface* pI)
{
    // copy the adress of the parameter interface
    //will allow us to store locally the address where we retrieve the type and all useful informations
    this->pICopy = pI;

    this->lineEditValidator = NULL;

    // init the graphics... I did this in a separate function so that we can handle better
    this->fillContent();
}


ParamsQEditorLine::~ParamsQEditorLine()
{
    QLayoutItem *item;
    while ((item = this->editLayout->takeAt(0)))
        delete item;

    delete this->editLayout;

    if (this->lineEditValidator != NULL)
        delete this->lineEditValidator;
}

void ParamsQEditorLine::fillContent()
{
    this->varNameLabel = new QLabel( QString::fromStdString(this->pICopy->getName()) );
    this->varDescLabel = new QLabel( QString::fromStdString(this->pICopy->getDescription()) );

    this->paramLineEdit = new QLineEdit;
    QObject::connect(this->paramLineEdit, SIGNAL(editingFinished()), this, SLOT(valueEdited()));
    // this->paramLineEdit->setPlaceholderText(QString());

    this->editLayout = new QGridLayout;
    this->editLayout->addWidget(this->varNameLabel, 0, 0);
    this->editLayout->addWidget(this->paramLineEdit, 0, 1);
    this->editLayout->addWidget(this->varDescLabel, 1, 0, 1, 2);

    this->setLayout(this->editLayout);

    this->fillEditLine();
}


void ParamsQEditorLine::valueEdited()
{
    if (this->pICopy->getType() == "int")
    {
        bool ok;
        int tmpVal = this->lineEditValidator->locale().toInt(this->paramLineEdit->text(), &ok);
        if (ok)
        {
            if (tmpVal != this->pICopy->getValue<int>())
                // modify the value only if necessary
                this->pICopy->setValue<int>( tmpVal );
        }
    }
    else if (this->pICopy->getType() == "float")
    {
        bool ok;
        float tmpVal = this->lineEditValidator->locale().toFloat(this->paramLineEdit->text(), &ok);
        if (ok)
        {
            if (tmpVal != this->pICopy->getValue<float>())
                // modify the value only if necessary
                this->pICopy->setValue<float>(tmpVal);
        }
    }
    else if (this->pICopy->getType() == "double")
    {
        bool ok;
        double tmpVal = this->lineEditValidator->locale().toDouble(this->paramLineEdit->text(), &ok);
        if (ok)
        {
            if (tmpVal != this->pICopy->getValue<double>())
                // modify the value only if necessary
                this->pICopy->setValue<double>(tmpVal);
        }
    }
    else if (this->pICopy->getType() == "std::string")
    {
        this->pICopy->setValue<std::string>( this->paramLineEdit->text().toStdString() );
    }

    emit valueUpdated();
}

void ParamsQEditorLine::updateContent()
{
    this->fillEditLine();
}

void ParamsQEditorLine::fillEditLine()
{
    if (this->pICopy->getType() == "int")
    {
        // understand the Validator thing : see the float section
        if (this->lineEditValidator == NULL)    // hasn't been created yet
        {
            this->lineEditValidator = new QIntValidator(this->paramLineEdit);
            this->lineEditValidator->setLocale(QLocale::C);
            this->paramLineEdit->setValidator(this->lineEditValidator);
        }

        this->paramLineEdit->setText(QString::number(this->pICopy->getValue<int>()));
    }
    else if (this->pICopy->getType() == "float")
    {
        // this validator thing is quite annoying
        if (this->lineEditValidator == NULL)    // hasn't been created yet
        {
            // put everything in C format. Quite strangely, the number procedure produces C-Format strings
            // but the validator ON without specifying the C format locale makes such strings impossible to edit
            // this is especially an issue with float or double format numbers
            this->lineEditValidator = new QDoubleValidator(this->paramLineEdit);
            this->lineEditValidator->setLocale(QLocale::C);
            this->paramLineEdit->setValidator(this->lineEditValidator);
        }

        this->paramLineEdit->setText(QString::number(this->pICopy->getValue<float>()));
    }
    else if (this->pICopy->getType() == "double")
    {
        // see the validator comments above...
        if (this->lineEditValidator == NULL)
        {
            this->lineEditValidator = new QDoubleValidator(this->paramLineEdit);
            this->lineEditValidator->setLocale(QLocale::C);
            this->paramLineEdit->setValidator(this->lineEditValidator);
        }

        this->paramLineEdit->setText(QString::number(this->pICopy->getValue<double>()));
    }
    else if (this->pICopy->getType() == "std::string")
    {
        this->paramLineEdit->setText(QString::fromStdString(this->pICopy->getValue<std::string>()));
    }
}

