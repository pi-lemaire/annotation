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
    }

    // ensure that we can only select one class at a time
    pListClasses->setSelectionMode(QAbstractItemView::SingleSelection);
    pListClasses->setCurrentRow(0);


    // set the proper layout
    this->topLayout->addWidget(pListClasses);
    this->setLayout(this->topLayout);

    // already select the first item
    connect(pListClasses, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(OnClassSelect(QListWidgetItem*)));
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
