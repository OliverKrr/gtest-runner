#-------------------------------------------------------------------------------
#	CMAKE SETTINGS
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.12...4.0.0)
set(QT_MINIMUM_VERSION 6.9.0)

#-------------------------------------------------------------------------------
#	PROJECT SETTINGS
#-------------------------------------------------------------------------------

project(gtest-runner C CXX)

# Use git tag as project name if clones from git
find_package(Git)
if(GIT_FOUND)
	execute_process(
	  COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  OUTPUT_VARIABLE BRANCH
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	# Replace / by - in branch
	string(REPLACE "/" "-" BRANCH ${BRANCH})
	execute_process(
	  COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  OUTPUT_VARIABLE VERSION
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()


#-------------------------------------------------------------------------------
#	CMake Modules
#-------------------------------------------------------------------------------

LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules/)

#-------------------------------------------------------------------------------
#	Qt Settings
#-------------------------------------------------------------------------------

# Required Qt settings
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(QT_USE_QTMAIN TRUE)

# Setup the cmake prefix path so Qt can be found. Use the QT_ROOT_DIR variable if defined
if(NOT DEFINED ENV{QT_ROOT_DIR})
	MESSAGE(STATUS "QT_ROOT_DIR environment variable not set. Be sure to set the QT_ROOT_DIR
	cmake cache variable to the Qt root install location")
endif()

set(QT_ROOT_DIR $ENV{QT_ROOT_DIR} CACHE STRING "Qt install path")
list(APPEND CMAKE_PREFIX_PATH ${QT_ROOT_DIR})

# Find the necessary Qt6 modules
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6Xml REQUIRED)
find_package(Qt6Test REQUIRED)

# Test for supported Qt version
find_program(QMAKE_EXECUTABLE NAMES qmake HINTS ${QT_ROOT_DIR} ENV QT_ROOT_DIR PATH_SUFFIXES bin)
execute_process(COMMAND ${QMAKE_EXECUTABLE} -query QT_VERSION OUTPUT_VARIABLE QT_VERSION)
if(QT_VERSION VERSION_LESS QT_MINIMUM_VERSION)
	MESSAGE(FATAL_ERROR "Minimum supported Qt version: ${QT_MINIMUM_VERSION}. Installed version: ${QT_VERSION}")
endif()

# find thread library 
find_package(Threads REQUIRED)

#-------------------------------------------------------------------------------
#	COMPILE FLAGS
#-------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

if(MSVC)
  set(GUI_TYPE WIN32)
  add_compile_options(/MP)
elseif(APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)
endif(MSVC)

#-------------------------------------------------------------------------------
#	CONFIGURATION
#-------------------------------------------------------------------------------

set(APPLICATION_NAME ${PROJECT_NAME})
set(APPLICATION_VERSION ${VERSION})
set(APPLICATION_ORGANIZATION "OliverKrr")
set(APPLICATION_ORGANIZATION_DOMAIN "https://github.com/OliverKrr/gtest-runner")

configure_file(src/appinfo.h.in appinfo.h @ONLY NEWLINE_STYLE UNIX)

#-------------------------------------------------------------------------------
#	SOURCE CODE
#-------------------------------------------------------------------------------

qt6_wrap_ui(UI_HDRS
src/qtfindreplacedialog/findreplacedialog.ui
src/qtfindreplacedialog/findreplaceform.ui
)

# .h files
list(APPEND HEADER_FILES
src/mainwindow.h
src/mainwindow_p.h
src/executableModelDelegate.h
src/executableSettingsDialog.h
src/domitem.h
src/flatDomeItem.h
src/GTestFailureModel.h
src/gtestModel.h
src/testsController.h
src/utilities.h
src/qexecutablemodel.h
src/qexecutabletreeview.h
src/GTestModelSortFilterProxy.h
src/QFilterEmptyColumnProxy.h
src/QStdOutSyntaxHighlighter.h
src/tree.h
src/QTreeModel.h
src/qtfindreplacedialog/finddialog.h
src/qtfindreplacedialog/findform.h
src/qtfindreplacedialog/findreplace_global.h
src/qtfindreplacedialog/findreplacedialog.h
src/qtfindreplacedialog/findreplaceform.h
src/qtfindreplacedialog/findreplace_global.h
src/killTestWrapper.h
)

# .cpp files
list(APPEND SOURCE_FILES
src/main.cpp
src/mainwindow.cpp
src/mainwindow_p.cpp
src/executableModelDelegate.cpp
src/executableSettingsDialog.cpp
src/domitem.cpp
src/flatDomeItem.cpp
src/GTestFailureModel.cpp
src/gtestModel.cpp
src/testsController.cpp
src/utilities.cpp
src/qexecutablemodel.cpp
src/qexecutabletreeview.cpp
src/GTestModelSortFilterProxy.cpp
src/QFilterEmptyColumnProxy.cpp
src/qtfindreplacedialog/finddialog.cpp
src/qtfindreplacedialog/findform.cpp
src/qtfindreplacedialog/findreplacedialog.cpp
src/qtfindreplacedialog/findreplaceform.cpp
)

LIST(APPEND SOURCE_FILES ${HEADER_FILES})

#-------------------------------------------------------------------------------
#	SOURCE GROUPS
#-------------------------------------------------------------------------------

source_group(generated REGULAR_EXPRESSION ".*_automoc.cpp")
source_group(resources REGULAR_EXPRESSION ".*[.][q]?rc")
source_group(source REGULAR_EXPRESSION ".*[.]cpp")
source_group(headers REGULAR_EXPRESSION ".*[.]h")
source_group(continuous-integration REGULAR_EXPRESSION ".*[.]yml")

#-------------------------------------------------------------------------------
#	RESOURCES
#-------------------------------------------------------------------------------

list(APPEND QT_RESOURCES
	resources/resources.qrc
)

qt6_add_resources(RESOURCES ${QT_RESOURCES})

list(APPEND RESOURCES
	resources/resources.rc
)

# On macos, add icns to application bundle
if(APPLE)
  LIST(APPEND SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/logo.icns)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/logo.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set(MACOSX_BUNDLE_ICON_FILE "logo.icns")
endif()

#-------------------------------------------------------------------------------
#	ADDITIONAL INCLUDE DIRECTORIES
#-------------------------------------------------------------------------------

list(APPEND INCLUDE_DIRS
${CMAKE_CURRENT_BINARY_DIR}
${CMAKE_CURRENT_SOURCE_DIR}/src/qtfindreplacedialog/
)

# Remove duplicate include directories
list(REMOVE_DUPLICATES INCLUDE_DIRS)

#-------------------------------------------------------------------------------
#	ADDITIONAL LIBRARIES
#-------------------------------------------------------------------------------

list(APPEND LIBRARIES
Qt6::Core
Qt6::Gui
Qt6::Widgets
Qt6::Xml
Qt6::Test
Threads::Threads
)

# Remove duplicate library dependencies. 
list(REMOVE_DUPLICATES LIBRARIES)

#-------------------------------------------------------------------------------
#	GENERATE EXECUTABLE
#-------------------------------------------------------------------------------

# Set additional include directories
include_directories(${INCLUDE_DIRS})

# Generate the executable
add_executable(${PROJECT_NAME} ${GUI_TYPE} ${SOURCE_FILES} ${UI_HDRS} ${RESOURCES})

# link libraries
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

# Use the automatically determined RPATH values
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

if (APPLE)
  INSTALL(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION bin)
else()
  INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
endif()
INSTALL(FILES ${CMAKE_SOURCE_DIR}/resources/desktop/gtest-runner.desktop DESTINATION share/applications)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/resources/desktop/gtest-runner.png DESTINATION share/icons)
INCLUDE(InstallRequiredSystemLibraries)

# Windows specific build steps
if(WIN32)
	# Run winddeployqt if it can be found
	find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS ${QT_ROOT_DIR} ENV QT_ROOT_DIR PATH_SUFFIXES bin)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:${PROJECT_NAME}>)
	# Gather up all the dependencies in the bin dir
	# Lack of a trailing slash means install this directory, trailing slash means install everything in this directory.
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Release/
			RUNTIME DESTINATION bin
			FILES_MATCHING
			PATTERN "_CPack*" EXCLUDE
			PATTERN "*.dll"
			)
endif()

if(WIN32)
	# NSIS specific configuration
	set(CPACK_GENERATOR "NSIS;ZIP")
	SET(CPACK_NSIS_URL_INFO_ABOUT ${APPLICATION_ORGANIZATION_DOMAIN})
	set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\$<TARGET_FILE:${PROJECT_NAME}>")
	SET(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/resources/icons\\\\logo.ico)
	SET(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
	SET(CPACK_NSIS_MUI_FINISHPAGE_RUN ${PROJECT_NAME}.exe)
	set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/resources/images\\\\logo.bmp)
	set(CPACK_PACKAGE_VERSION ${VERSION})
else(WIN32)
	# DEBIAN CPACK_GENERATOR
	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Oliver Karrenbauer")
	SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)	
	# Deb packages can't have the 'v' in the version name
	string(REGEX MATCH "[0-9][.][0-9][.][0-9]" VERSION_NUM "${APPLICATION_VERSION}")
	set(CPACK_PACKAGE_VERSION ${VERSION_NUM})
endif()

set(CPACK_PACKAGE_CONTACT "Oliver Karrenbauer <oliver.karrenbauer@gmail.com>")
set(CPACK_PACKAGE_NAME ${APPLICATION_NAME})
set(CPACK_PACKAGE_VENDOR ${APPLICATION_ORGANIZATION})
SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GTest-Runner - an automated test runner and user interface for google tests")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_PACKAGE_DIRECTORY ${PROJECT_BINARY_DIR}/Release)
set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME}" "gtest-runner")

INCLUDE(CPack)
