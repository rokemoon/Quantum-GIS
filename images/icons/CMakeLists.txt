
FILE (GLOB IMAGES *.png)
INSTALL (FILES ${IMAGES}
         DESTINATION ${QGIS_DATA_DIR}/images/icons)

IF(MSVC)
  INSTALL(FILES qgis.ico DESTINATION ${CMAKE_INSTALL_PREFIX}/icons)
ENDIF(MSVC)

# OS X app/doc icons
IF (APPLE)
  ADD_SUBDIRECTORY(mac)
ENDIF (APPLE)
