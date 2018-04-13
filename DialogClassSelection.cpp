#include "DialogClassSelection.h"


#include <QDebug>



DialogClassSelection::DialogClassSelection(AnnotationsSet* annotsSet, QWidget* parent, Qt::WindowFlags flags): QDialog( parent, flags )
{
    // storing the pointer to the annotations system - we will only use the link to the config
    this->annots = annotsSet;

    // generating the main layout
    this->topLayout = new QVBoxLayout;

    // and here is the list of available classes
    pListClasses = new QListWidget();

    for (int iClass=1; iClass<=this->annots->getConfig().getPropsNumber(); iClass++)
    {
        // adding a small icon for more convenience
        QPixmap classColor(30, 20);
        classColor.fill( QtCvUtils::cvVec3bToQColor(this->annots->getConfig().getProperty(iClass).displayRGBColor) );
        pListClasses->addItem( QString::fromStdString(this->annots->getConfig().getProperty(iClass).className) );
        pListClasses->item(iClass-1)->setIcon( QIcon(classColor) );
        pListClasses->item(iClass-1)->setFlags(pListClasses->item(iClass-1)->flags() | Qt::ItemIsUserCheckable);
        pListClasses->item(iClass-1)->setCheckState( (this->annots->getConfig().getProperty(iClass).locked ? Qt::Checked : Qt::Unchecked) );
    }

    // ensure that we can only select one class at a time
    pListClasses->setSelectionMode(QAbstractItemView::SingleSelection);
    pListClasses->setCurrentRow(0);


    // set the proper layout
    this->topLayout->addWidget(pListClasses);
    this->setLayout(this->topLayout);

    // already select the first item
    connect(pListClasses, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(OnClassSelect(QListWidgetItem*)));
    connect(pListClasses, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(OnClassCheck(QListWidgetItem*)));
}


DialogClassSelection::~DialogClassSelection()
{
    // this has to be filled some day, oh maybe someday, maybe someday day day..!
}




void DialogClassSelection::OnClassSelect(QListWidgetItem *)
{
    // just send the signal that
    // don't forget the +1 since the numbering starts with 1
    emit classSelected( pListClasses->currentRow()+1 );
}


void DialogClassSelection::OnClassCheck(QListWidgetItem *)
{
    for (int i=0; i<this->annots->getConfig().getPropsNumber(); i++)
    {
        this->annots->setClassLock( i+1, (pListClasses->item(i)->checkState()==Qt::Checked) );
    }
}
