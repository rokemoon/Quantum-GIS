/***************************************************************************
 *   Copyright (C) 2003 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "plugingui.h"

//qt includes
#include <qtimer.h>
#include <qregexp.h>
#include <qlineedit.h>
//standard includes
#include <iostream>
#include <assert.h>

PluginGui::PluginGui() : PluginGuiBase()
{
  
}

PluginGui::PluginGui( QWidget* parent , const char* name , bool modal , WFlags fl  )
: PluginGuiBase( parent, name, modal, fl )
{
   
}  
PluginGui::~PluginGui()
{
}

void PluginGui::pbnOK_clicked()
{
  //
  // If you have a produced a raster layer using your plugin, you can ask qgis to 
  // add it to the view using:
  // emit drawRasterLayer(QString("layername"));
  // or for a vector layer
  // emit drawVectorLayer(QString("pathname"),QString("layername"),QString("provider name (either ogr or postgres"));
  //
  
  
  //Test the url snarfer
  mQUrl = QUrl("http://www.marlinnut.com/latlon.shtml");
  check(); 
  
  
} 

void PluginGui::pbnCancel_clicked()
{
 close(1);
}


//
// The following methods are adapted from some sample code I found here
//


void PluginGui::check()
{
  
  connect(&mQhttp, SIGNAL(stateChanged(int)), 
          this, SLOT(slotStateChanged(int)));
  connect(&mQhttp, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
          this, SLOT(slotResponseHeaderReceived(const QHttpResponseHeader &)));
  connect(&mQhttp, SIGNAL(requestFinished(int, bool)),
          this, SLOT(slotRequestFinished(int, bool)));

  std::cerr << "-------------------------------------------" << std::endl;
  std::cerr << "Web Snarfer running " <<  std::endl;
  std::cerr << "Setting host to: " << mQUrl.host() << std::endl;
  std::cerr << "Path in url is:  " << mQUrl.path() << std::endl;
  //std::cerr << "Query in url is:  " << mQUrl.query() << std::endl;
  mSetHostIdInt = mQhttp.setHost(mQUrl.host());
  std::cerr << "ID sethost: " << mSetHostIdInt << std::endl;

  if(!mQUrl.query().isNull() || mQUrl.hasRef()) 
    mRequestQString = mQUrl.toString();
  else mRequestQString = mQUrl.path();

  mHeaderIdInt = mQhttp.head(mRequestQString);
  std::cerr << "ID head: " << mHeaderIdInt << std::endl;
  
  //QTimer::singleShot( mTimeOutInt * 1000, this, SLOT(slotTimeOut()) );
}

void PluginGui::slotRequestFinished(int id, bool error)
{
  std::cerr << "slotRequestFinished: " << id << std::endl;
  if(error)
    {
      std::cerr << "Error: " << mQhttp.errorString() << std::endl;
      finish();
    }
  
  else if(id == mHeaderIdInt)
    {
      requestHeadFinished(id);
    }

  else if(id == mGetIdInt) 
    {
      requestGetFinished(id);
    }
}

void PluginGui::slotResponseHeaderReceived(const QHttpResponseHeader& resp)
{
  std::cerr << "slotResponseHeaderReceived " << std::endl;
  std::cerr << resp.toString() << std::endl;
  mQHttpResponseHeader = resp;
}

void PluginGui::slotTimeOut()
{
  std::cerr << "slotTimeout " << std::endl;
  if(mQhttp.state() == QHttp::Connecting) 
    {
      assert(mQhttp.currentId() == mHeaderIdInt || 
             mQhttp.currentId() == mGetIdInt);
      
      std::cerr << "Error: Time out" << std::endl;
      
      finish();
    }
}

void PluginGui::slotStateChanged(int state) 
{
  std::cerr << "slotStateChanged " << std::endl;
  std::cerr << "Current id: " << mQhttp.currentId() << std::endl;
  std::cerr << "State: " << state << std::endl;

  switch (state)
  {
      case QHttp::Unconnected: std::cout <<"Unconnected" << std::endl; break;
      case QHttp::HostLookup:  std::cout <<"Host Lookup" << std::endl; break;
      case QHttp::Connecting:  std::cout <<"Connecting"  << std::endl; break;
      case QHttp::Sending:     std::cout <<"Sending"     << std::endl; break;
      case QHttp::Reading:     std::cout <<"Reading"     << std::endl; break;
      case QHttp::Connected:   std::cout <<"Connected"   << std::endl; break;
      case QHttp::Closing:     std::cout <<"Closing"     << std::endl; break;
      default :  std::cout <<"Illegal state"             << std::endl; break;
  }

}

void PluginGui::requestHeadFinished(int id)
{
  std::cerr << "requestHeadFinished: " << id << std::endl;

  assert(id = mHeaderIdInt);
  assert(!mQhttp.hasPendingRequests());

  // 405 -> it might not support HEAD request's...
  if( (mQHttpResponseHeader.statusCode() == 200 || mQHttpResponseHeader.statusCode() == 405) && 
      (mQHttpResponseHeader.contentType() == "text/html" || 
       mQHttpResponseHeader.contentType() == "text/plain") )
    {
      mGetIdInt = mQhttp.get(mRequestQString);
      std::cerr << "ID GET: " << mGetIdInt << std::endl;
    }
  else if(mQHttpResponseHeader.statusCode() == 301 || 
          mQHttpResponseHeader.statusCode() == 302)
    {
      // ...
    }
  else
    {
      finish();
    }
}

//
// This is run once the web page has been retrieved.
//

void PluginGui::requestGetFinished(int id)
{
  std::cerr << "requestGetFinished: " << id << std::endl;

  assert(id == mGetIdInt);

  QString myPageQString(mQhttp.readAll());
  std::cerr << "Getting document contents : " << std::endl;
  //std::cerr << myPageQString << std::endl;
  assert(!myPageQString.isEmpty());

  //now we parse the file looking for lat long occurrences
  QRegExp myQRegExp( leRecordRegex->text() ); // match using regex in our dialog
  int myPosInt = 0;    // where we are in the string
  int myCountInt = 0;  // how many matches we find
  while ( myPosInt >= 0 ) 
  {
    myPosInt = myQRegExp.search( myPageQString, myPosInt );
    if ( myPosInt >= 0 ) 
    {
      std::cerr << "************************************* " << std::endl;
      myPosInt += myQRegExp.matchedLength(); //skip the length of the matched string
      myCountInt++;    // count the number of matches
      std::cerr << myPageQString.mid(myPosInt,myQRegExp.matchedLength()) << std::endl;
    }
  }
  std::cerr << "************************************* " << std::endl;
  //std::cerr << myPageQString << std::endl;
  std::cout << myCountInt << " records found" << std::endl;
  //std::cerr << myPageQString << std::endl;
  std::cerr << "************************************* " << std::endl;
  finish();
}

void PluginGui::finish()
{
  std::cerr << "finish: " <<  std::endl;
/*
  Don't go back to slotRequestFinished
*/
  disconnect(&mQhttp, SIGNAL(requestFinished(int, bool)), 
             this, SLOT(slotRequestFinished(int, bool)));

  mQhttp.closeConnection();
  //close the dialog
  done(1);
}


