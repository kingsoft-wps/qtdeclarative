QT += qml qml-private quick-private core-private
qtHaveModule(widgets): QT += widgets

CONFIG += no_import_scan

QTPLUGIN.platforms = qminimal

# We cannot use libQmlCompiler as that is built for the host
# and qmlplugindump needs to be built for the target.

INCLUDEPATH += $$PWD/../../src/qmlcompiler

SOURCES += \
    ../../src/qmlcompiler/qmlstreamwriter.cpp \
    main.cpp \
    qmltypereader.cpp

HEADERS += \
    ../../src/qmlcompiler/qmlstreamwriter_p.h \
    qmltypereader.h

macx {
    # Prevent qmlplugindump from popping up in the dock when launched.
    # We embed the Info.plist file, so the application doesn't need to
    # be a bundle.
    QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$shell_quote($$PWD/Info.plist)
    CONFIG -= app_bundle
}

QMAKE_TARGET_DESCRIPTION = QML Plugin Metadata Dumper

load(qt_tool)
