CONFIG += testcase
TARGET = tst_qqmldirparser
QT += qml testlib v8-private
macx:CONFIG -= app_bundle

SOURCES += tst_qqmldirparser.cpp

include (../../shared/util.pri)

CONFIG += parallel_test
