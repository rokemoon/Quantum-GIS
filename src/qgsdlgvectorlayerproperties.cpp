/***************************************************************************
                          qgsdlgvectorlayerproperties.cpp
                   Unified property dialog for vector layers
                             -------------------
    begin                : 2004-01-28
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /* $Id$ */
#include <qstring.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qtextstream.h>
#include <qtable.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qspinbox.h>
#include <qwidgetstack.h>
#include <qpushbutton.h>
#include <qwidget.h>

#include "qgis.h"
#include "qgsrect.h"
#include "qgsfield.h"
#include "qgsdlgvectorlayerproperties.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include "qgssinglesymrenderer.h"
#include "qgsgraduatedmarenderer.h"
#include "qgsgraduatedsymrenderer.h"
#include "qgscontinuouscolrenderer.h"
#include "qgsuniquevalrenderer.h"
#include "qgsuvalmarenderer.h"
#include "qgssimarenderer.h"
#include "qgssimadialog.h"
#include "qgslegenditem.h"
#include "qgssisydialog.h"
#include "qgsgramadialog.h"
#include "qgsgrasydialog.h"
#include "qgscontcoldialog.h"
#include "qgsuvaldialog.h"
#include "qgsuvalmadialog.h"
#include "qobjectlist.h"
#include "qgsgramadialog.h"
#include "qgslabelattributes.h"
#include "qgslabel.h"
#include "qgslabeldialog.h"
#include "qgsattributeactiondialog.h"


QgsDlgVectorLayerProperties::QgsDlgVectorLayerProperties(QgsVectorLayer * lyr, QWidget * parent, const char *name, bool modal):QgsDlgVectorLayerPropertiesBase(parent, name, modal), layer(lyr), rendererDirty(false), bufferDialog(layer->rendererDialog()),
bufferRenderer(layer->
               renderer())
{
  // populate the general information
  QString source = layer->source();
  source = source.left(source.find("password"));
  lblSource->setText(source);
  txtDisplayName->setText(layer->name());
  // display type and feature count
  lblGeometryType->setText(QGis::qgisVectorGeometryType[layer->vectorType()]);
  //QgsVectorDataProvider *dp = dynamic_cast<QgsVectorDataProvider *>(layer->getDataProvider());
  QgsVectorDataProvider *dp = (QgsVectorDataProvider *)(layer->getDataProvider());
  QString numFeatures;
  numFeatures = numFeatures.setNum(dp->featureCount());
  lblFeatureCount->setText(numFeatures);
  QgsRect *extent = dp->extent();
  QString ll;
  //QTextOStream (&ll) << extent->xMin() << ", " << extent->yMin();
  lblLowerLeft->setText(ll.sprintf("%16f, %16f", extent->xMin(), extent->yMin()));
  QString ur;
//  QTextOStream (&ur) << extent->xMax() << ", " << extent->yMax(); 
  lblUpperRight->setText(ur.sprintf("%16f, %16f", extent->xMax(), extent->yMax()));
  std::vector < QgsField > fields = dp->fields();
  // populate the table and the display field drop-down with the field
  // information

  for (int i = 0; i < fields.size(); i++)
  {
    QgsField fld = fields[i];
    QListViewItem *lvi = new QListViewItem(listViewFields, fld.name(),
					   fld.type(), QString("%1").arg(fld.length()),
					   QString("%1").arg(fld.precision()));
    displayFieldComboBox->insertItem( fld.name() );
  }
  displayFieldComboBox->setCurrentItem(0); // the blank one

  // set up the scale based layer visibility stuff....
  chkUseScaleDependentRendering->setChecked(lyr->scaleBasedVisibility());
  spinMinimumScale->setValue(lyr->minScale());
  spinMaximumScale->setValue(lyr->maxScale());

  // symbology initialization
  legendtypecombobox->insertItem(tr("Single Symbol"));
  legendtypecombobox->insertItem(tr("Graduated Symbol"));
  legendtypecombobox->insertItem(tr("Continuous Color"));
  legendtypecombobox->insertItem(tr("Unique Value"));
  if( layer->vectorType()==QGis::Point )
  {
      legendtypecombobox->insertItem(tr("Single Marker"));
      legendtypecombobox->insertItem(tr("Graduated Marker"));
      legendtypecombobox->insertItem(tr("Unique Value Marker"));
  }

  QVBoxLayout *layout = new QVBoxLayout( labelOptionsFrame );
  labelDialog = new QgsLabelDialog ( layer->label(),labelOptionsFrame);
  layout->addWidget( labelDialog );

  QVBoxLayout *actionLayout = new QVBoxLayout( actionOptionsFrame );
  actionDialog = new QgsAttributeActionDialog ( layer->actions(), actionOptionsFrame );
  actionLayout->addWidget( actionDialog );

  QObject::connect(legendtypecombobox, SIGNAL(activated(const QString &)), this, SLOT(alterLayerDialog(const QString &)));

  //insert the renderer dialog of the vector layer into the widget stack
  widgetStackRenderers->addWidget(bufferDialog);
  widgetStackRenderers->raiseWidget(bufferDialog);

}

QgsDlgVectorLayerProperties::~QgsDlgVectorLayerProperties()
{
  widgetStackRenderers->removeWidget(bufferDialog);
  if (rendererDirty)
    {
      delete bufferDialog;
      delete bufferRenderer;
    }
}

void QgsDlgVectorLayerProperties::alterLayerDialog(const QString & string)
{
  if (rendererDirty)
    {
      widgetStackRenderers->removeWidget(bufferDialog);
      delete bufferDialog;
      delete bufferRenderer;
    }
  //create a new Dialog
  if (string == tr("Single Symbol"))
    {
      bufferRenderer = new QgsSingleSymRenderer();
  } else if (string == tr("Graduated Symbol"))
    {
      bufferRenderer = new QgsGraduatedSymRenderer();
  } else if (string == tr("Continuous Color"))
    {
      bufferRenderer = new QgsContinuousColRenderer();
  } else if (string == tr("Single Marker"))
  {
    // On win32 we can't support svg without qt commercial
    // so we beg a bit (i know its tacky to repeat this for
    // each case, but i'm in a hurry to get the 0.5 release 
    // out - besides this will go away when the money roles in...)
#ifdef WIN32
    QMessageBox::warning(this, "No SVG Support", 
  "In order for QGIS to support SVG markers under Windows, we need to build QGIS\n" 
  " using the commercial version of Qt. As this project is developed by volunteers\n"
  " donating their time, we don't have the financial resources to purchase Qt\n"
  " commercial.  If you would like to help us, please visit the QGIS sourceforge\n"
  " home page to make a donation");
    // use the single symbol renderer
      bufferRenderer = new QgsSingleSymRenderer();
      legendtypecombobox->setCurrentText("Single Symbol");
#else
      bufferRenderer = new QgsSiMaRenderer();
#endif
  } else if (string == tr("Graduated Marker"))
  {
      bufferRenderer = new QgsGraduatedMaRenderer();
  } else if(string == tr("Unique Value"))
  {
      bufferRenderer = new QgsUniqueValRenderer();
  } else if(string == tr("Unique Value Marker"))
  {
#ifdef WIN32
    QMessageBox::warning(this, "No SVG Support", 
  "In order for QGIS to support SVG markers under Windows, we need to build QGIS\n"
  " using the commercial version of Qt. As this project is developed by volunteers\n"
  " donating their time, we don't have the financial resources to purchase Qt\n"
  " commercial.  If you would like to help us, please visit the QGIS sourceforge\n"
  " home page to make a donation");
    // use the single symbol renderer
      bufferRenderer = new QgsSingleSymRenderer();
      legendtypecombobox->setCurrentText("Single Symbol");
#else
      bufferRenderer = new QgsUValMaRenderer();
#endif
  }
  bufferRenderer->initializeSymbology(layer, this);

  widgetStackRenderers->addWidget(bufferDialog);
  widgetStackRenderers->raiseWidget(bufferDialog);
  rendererDirty = true;
}

void QgsDlgVectorLayerProperties::setRendererDirty(bool enabled)
{
  rendererDirty = enabled;
}


QDialog *QgsDlgVectorLayerProperties::getBufferDialog()
{
  return bufferDialog;
}

QgsRenderer *QgsDlgVectorLayerProperties::getBufferRenderer()
{
  return bufferRenderer;
}

void QgsDlgVectorLayerProperties::setLegendType(QString type)
{
  legendtypecombobox->setCurrentText(tr(type));
}

void QgsDlgVectorLayerProperties::setDisplayField(QString name)
{
  displayFieldComboBox->setCurrentText(name);
}

void QgsDlgVectorLayerProperties::reset( void )
{
    actionDialog->init();
    labelDialog->init();
    labelCheckBox->setChecked(layer->labelOn());
}
//
// methods reimplemented from qt designer base class
//

void QgsDlgVectorLayerProperties::pbnCancel_clicked()
{
 reject();
}
void QgsDlgVectorLayerProperties::btnHelp_clicked()
{

}
void QgsDlgVectorLayerProperties::pbnOK_clicked()
{
  //make sure changes are applied
  pbnApply_clicked(); 
  //
  if (rendererDirty)
    {
      widgetStackRenderers->removeWidget(bufferDialog);
      delete bufferDialog;
      delete bufferRenderer;
      bufferDialog = layer->rendererDialog();
      bufferRenderer = layer->renderer();
      widgetStackRenderers->addWidget(bufferDialog);
      widgetStackRenderers->raiseWidget(bufferDialog);
      rendererDirty = false;
      //restore the right name in the combobox
      if(bufferRenderer)
      {
	  legendtypecombobox->setCurrentText(tr(bufferRenderer->name()));
      }
    }
  reject();
}
void QgsDlgVectorLayerProperties::pbnApply_clicked()
{

  // set up the scale based layer visibility stuff....
  layer->setScaleBasedVisibility(chkUseScaleDependentRendering->isChecked());
  layer->setMinScale(spinMinimumScale->value());
  layer->setMaxScale(spinMaximumScale->value());

  // update the display field
  layer->setDisplayField(displayFieldComboBox->currentText());
  
  if (rendererDirty)
    {
      layer->setRenderer(bufferRenderer);
      layer->setRendererDialog(bufferDialog);
    }
  
  actionDialog->apply();

  labelDialog->apply();
  layer->setLabelOn(labelCheckBox->isChecked());
  layer->setLayerName(displayName());


  QgsSiSyDialog *sdialog = dynamic_cast < QgsSiSyDialog * >(layer->rendererDialog());
  QgsGraSyDialog *gdialog = dynamic_cast < QgsGraSyDialog * >(layer->rendererDialog());
  QgsContColDialog *cdialog = dynamic_cast < QgsContColDialog * >(layer->rendererDialog());
  QgsSiMaDialog* smdialog = dynamic_cast < QgsSiMaDialog * >(layer->rendererDialog());
  QgsGraMaDialog* gmdialog = dynamic_cast< QgsGraMaDialog * >(layer->rendererDialog());
  QgsUValDialog* udialog = dynamic_cast< QgsUValDialog * > (layer->rendererDialog());
  QgsUValMaDialog* umdialog = dynamic_cast< QgsUValMaDialog * > (layer->rendererDialog());

  if (sdialog)
    {
      sdialog->apply();
  } else if (gdialog)
    {
      gdialog->apply();
  } else if (cdialog)
    {
      cdialog->apply();
    }
  else if(smdialog)
  {
      smdialog->apply();
  }
  else if(gmdialog)
  {
      gmdialog->apply();
  }
  else if(udialog)
  {
      udialog->apply();
  }
  else if(umdialog)
  {
      umdialog->apply();
  }

  rendererDirty = false;
}
