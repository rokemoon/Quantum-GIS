#!/bin/sh
# Copy Py supporting libraries to qgis bundle
# and make search paths for them relative to bundle

# Edit INSTALLPREFIX to match the value of cmake INSTALL_PREFIX
INSTALLPREFIX=$PWD

BUNDLE=qgis0.9.0.app/Contents/MacOS
SITEPKG=/Library/Python/2.3/site-packages

# Copy supporting libraries to application bundle
cd $BUNDLE/share/qgis/python
if test ! -f sip.so; then
	cp $SITEPKG/sip.so sip.so
	cp $SITEPKG/sipconfig.py sipconfig.py
fi
if test ! -d PyQt4; then
	cp -r $SITEPKG/PyQt4 .
	for LIBPYQT4 in QtCore QtGui QtNetwork QtSql QtSvg QtXml QtAssistant QtOpenGL QtTest
	do
		cp $SITEPKG/PyQt4/$LIBPYQT4.so PyQt4/$LIBPYQT4.so
		# Update path to supporting libraries
		install_name_tool -change $LIBPYQT4.framework/Versions/4/$LIBPYQT4 \
			@executable_path/lib/$LIBPYQT4.framework/Versions/4/$LIBPYQT4 \
			PyQt4/$LIBPYQT4.so
		install_name_tool -change QtCore.framework/Versions/4/QtCore \
			@executable_path/lib/QtCore.framework/Versions/4/QtCore \
			PyQt4/$LIBPYQT4.so
		install_name_tool -change QtGui.framework/Versions/4/QtGui \
			@executable_path/lib/QtGui.framework/Versions/4/QtGui \
			PyQt4/$LIBPYQT4.so
	done
	install_name_tool -change QtXml.framework/Versions/4/QtXml \
		@executable_path/lib/QtXml.framework/Versions/4/QtXml \
		PyQt4/QtSvg.so
	install_name_tool -change QtNetwork.framework/Versions/4/QtNetwork \
		@executable_path/lib/QtNetwork.framework/Versions/4/QtNetwork \
		PyQt4/QtAssistant.so
fi

# Update path to supporting libraries
for LIBQGIS in core gui
do
	install_name_tool -change $INSTALLPREFIX/src/$LIBQGIS/libqgis_$LIBQGIS.dylib \
		@executable_path/lib/libqgis_$LIBQGIS.dylib \
		qgis/$LIBQGIS.so
	install_name_tool -change $INSTALLPREFIX/src/core/libqgis_core.dylib \
		@executable_path/lib/libqgis_core.dylib \
		qgis/$LIBQGIS.so
	for FRAMEWORK in QtCore QtGui QtNetwork QtSql QtSvg QtXml Qt3Support
	do
		install_name_tool -change $FRAMEWORK.framework/Versions/4/$FRAMEWORK \
			@executable_path/lib/$FRAMEWORK.framework/Versions/4/$FRAMEWORK \
			qgis/$LIBQGIS.so
	done
done
cd ../../../../../../
