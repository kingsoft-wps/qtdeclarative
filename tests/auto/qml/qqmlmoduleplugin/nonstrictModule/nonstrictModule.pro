TEMPLATE = lib
CONFIG += plugin
SOURCES = plugin.cpp
QT = core qml
DESTDIR = ../imports/com/nokia/NonstrictModule

QT += core-private gui-private qml-private

IMPORT_FILES = \
        qmldir

include (../../../shared/imports.pri)
