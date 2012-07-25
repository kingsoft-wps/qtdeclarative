/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <qdir.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QDebug>

#include "../../shared/testhttpserver.h"
#include "../../shared/util.h"

#define SERVER_ADDR "http://127.0.0.1:14456"
#define SERVER_PORT 14456

// Note: this test does not use module directives in the qmldir files, because
// it would result in repeated attempts to insert types into the same namespace.
// This occurs because type registration is process-global, while the test
// cases should really be run in proper per-process isolation.

class tst_qqmlmoduleplugin : public QQmlDataTest
{
    Q_OBJECT
public:

private slots:
    virtual void initTestCase();
    void importsPlugin();
    void importsPlugin2();
    void importsPlugin21();
    void importsMixedQmlCppPlugin();
    void incorrectPluginCase();
    void importPluginWithQmlFile();
    void remoteImportWithQuotedUrl();
    void remoteImportWithUnquotedUri();
    void versionNotInstalled();
    void versionNotInstalled_data();
    void implicitQmldir();
    void implicitQmldir_data();
    void importsNested();
    void importsNested_data();
    void importLocalModule();
    void importLocalModule_data();
    void importStrictModule();
    void importStrictModule_data();

private:
    QString m_importsDirectory;
    QString m_dataImportsDirectory;
};

void tst_qqmlmoduleplugin::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_importsDirectory = QFINDTESTDATA(QStringLiteral("imports"));
    QVERIFY2(QFileInfo(m_importsDirectory).isDir(),
             qPrintable(QString::fromLatin1("Imports directory '%1' does not exist.").arg(m_importsDirectory)));
    m_dataImportsDirectory = directory() + QStringLiteral("/imports");
    QVERIFY2(QFileInfo(m_dataImportsDirectory).isDir(),
             qPrintable(QString::fromLatin1("Imports directory '%1' does not exist.").arg(m_dataImportsDirectory)));
}

#define VERIFY_ERRORS(errorfile) \
    if (!errorfile) { \
        if (qgetenv("DEBUG") != "" && !component.errors().isEmpty()) \
            qWarning() << "Unexpected Errors:" << component.errors(); \
        QVERIFY(!component.isError()); \
        QVERIFY(component.errors().isEmpty()); \
    } else { \
        QString verify_errors_file_name = testFile(errorfile); \
        QFile file(verify_errors_file_name); \
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text)); \
        QByteArray data = file.readAll(); \
        file.close(); \
        QList<QByteArray> expected = data.split('\n'); \
        expected.removeAll(QByteArray("")); \
        QList<QQmlError> errors = component.errors(); \
        QList<QByteArray> actual; \
        for (int ii = 0; ii < errors.count(); ++ii) { \
            const QQmlError &error = errors.at(ii); \
            QByteArray errorStr = QByteArray::number(error.line()) + ":" +  \
                                  QByteArray::number(error.column()) + ":" + \
                                  error.description().toUtf8(); \
            actual << errorStr; \
        } \
        if (qgetenv("DEBUG") != "" && expected != actual) { \
            qWarning() << "Expected:" << expected << "Actual:" << actual; \
        } \
        if (qgetenv("QDECLARATIVELANGUAGE_UPDATEERRORS") != "" && expected != actual) {\
            QFile file(testFile(errorfile)); \
            QVERIFY(file.open(QIODevice::WriteOnly)); \
            for (int ii = 0; ii < actual.count(); ++ii) { \
                file.write(actual.at(ii)); file.write("\n"); \
            } \
            file.close(); \
        } else { \
            QCOMPARE(expected, actual); \
        } \
    }

void tst_qqmlmoduleplugin::importsPlugin()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);
    QTest::ignoreMessage(QtWarningMsg, "plugin created");
    QTest::ignoreMessage(QtWarningMsg, "import worked");
    QTest::ignoreMessage(QtWarningMsg, "Module 'com.nokia.AutoTestQmlPluginType' does not contain a module directive - it cannot be protected from external registrations.");
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("works.qml")));
    foreach (QQmlError err, component.errors())
    	qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qqmlmoduleplugin::importsPlugin2()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);
    QTest::ignoreMessage(QtWarningMsg, "plugin2 created");
    QTest::ignoreMessage(QtWarningMsg, "import2 worked");
    QTest::ignoreMessage(QtWarningMsg, "Module 'com.nokia.AutoTestQmlPluginType' does not contain a module directive - it cannot be protected from external registrations.");
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("works2.qml")));
    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qqmlmoduleplugin::importsPlugin21()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);
    QTest::ignoreMessage(QtWarningMsg, "plugin2.1 created");
    QTest::ignoreMessage(QtWarningMsg, "import2.1 worked");
    QTest::ignoreMessage(QtWarningMsg, "Module 'com.nokia.AutoTestQmlPluginType' does not contain a module directive - it cannot be protected from external registrations.");
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("works21.qml")));
    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qqmlmoduleplugin::incorrectPluginCase()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    QQmlComponent component(&engine, testFileUrl(QStringLiteral("incorrectCase.qml")));

    QList<QQmlError> errors = component.errors();
    QCOMPARE(errors.count(), 1);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
#if defined(Q_OS_MAC)
    QString libname = "libPluGin.dylib";
#elif defined(Q_OS_WIN32)
    QString libname = "PluGin.dll";
#endif
    QString expectedError = QLatin1String("plugin cannot be loaded for module \"com.nokia.WrongCase\": File name case mismatch for \"") + QDir(m_importsDirectory).filePath("com/nokia/WrongCase/" + libname) + QLatin1String("\"");
#else
    QString expectedError = QLatin1String("module \"com.nokia.WrongCase\" plugin \"PluGin\" not found");
#endif

    QCOMPARE(errors.at(0).description(), expectedError);
}

void tst_qqmlmoduleplugin::importPluginWithQmlFile()
{
    QString path = m_importsDirectory;

    // QTBUG-16885: adding an import path with a lower-case "c:" causes assert failure
    // (this only happens if the plugin includes pure QML files)
    #ifdef Q_OS_WIN
        QVERIFY(path.at(0).isUpper() && path.at(1) == QLatin1Char(':'));
        path = path.at(0).toLower() + path.mid(1);
    #endif

    QQmlEngine engine;
    engine.addImportPath(path);

    QTest::ignoreMessage(QtWarningMsg, "Module 'com.nokia.AutoTestPluginWithQmlFile' does not contain a module directive - it cannot be protected from external registrations.");

    QQmlComponent component(&engine, testFileUrl(QStringLiteral("pluginWithQmlFile.qml")));
    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qqmlmoduleplugin::remoteImportWithQuotedUrl()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(m_dataImportsDirectory);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import \"" SERVER_ADDR "/com/nokia/PureQmlModule\" \nComponentA { width: 300; ComponentB{} }", QUrl());

    QTRY_COMPARE(component.status(), QQmlComponent::Ready);
    QObject *object = component.create();
    QCOMPARE(object->property("width").toInt(), 300);
    QVERIFY(object != 0);
    delete object;

    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
}

void tst_qqmlmoduleplugin::remoteImportWithUnquotedUri()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(m_dataImportsDirectory);

    QQmlEngine engine;
    engine.addImportPath(m_dataImportsDirectory);
    QQmlComponent component(&engine);
    component.setData("import com.nokia.PureQmlModule 1.0 \nComponentA { width: 300; ComponentB{} }", QUrl());


    QTRY_COMPARE(component.status(), QQmlComponent::Ready);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("width").toInt(), 300);
    delete object;

    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
}

// QTBUG-17324

void tst_qqmlmoduleplugin::importsMixedQmlCppPlugin()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    QTest::ignoreMessage(QtWarningMsg, "Module 'com.nokia.AutoTestQmlMixedPluginType' does not contain a module directive - it cannot be protected from external registrations.");

    {
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("importsMixedQmlCppPlugin.qml")));

    QObject *o = component.create();
    QVERIFY2(o != 0, QQmlDataTest::msgComponentError(component, &engine));
    QCOMPARE(o->property("test").toBool(), true);
    delete o;
    }

    {
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("importsMixedQmlCppPlugin.2.qml")));

    QObject *o = component.create();
    QVERIFY2(o != 0, QQmlDataTest::msgComponentError(component, &engine));
    QCOMPARE(o->property("test").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    delete o;
    }


}

void tst_qqmlmoduleplugin::versionNotInstalled_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("versionNotInstalled") << "versionNotInstalled.qml" << "versionNotInstalled.errors.txt";
    QTest::newRow("versionNotInstalled") << "versionNotInstalled.2.qml" << "versionNotInstalled.2.errors.txt";
}

void tst_qqmlmoduleplugin::versionNotInstalled()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    static int count = 0;
    if (++count == 1)
        QTest::ignoreMessage(QtWarningMsg, "Module 'com.nokia.AutoTestQmlVersionPluginType' does not contain a module directive - it cannot be protected from external registrations.");

    QQmlComponent component(&engine, testFileUrl(file));
    VERIFY_ERRORS(errorFile.toLatin1().constData());
}


// test that errors are reporting correctly for plugin loading and qmldir parsing
void tst_qqmlmoduleplugin::implicitQmldir_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    // parsing qmldir succeeds, but plugin specified in the qmldir file doesn't exist
    QTest::newRow("implicitQmldir") << "implicit1" << "temptest.qml" << "implicitQmldir.errors.txt";

    // parsing qmldir fails due to syntax errors, etc.
    QTest::newRow("implicitQmldir2") << "implicit2" << "temptest2.qml" << "implicitQmldir.2.errors.txt";
}
void tst_qqmlmoduleplugin::implicitQmldir()
{
    QFETCH(QString, directory);
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QString importPath = testFile(directory);
    QString fileName = directory + QDir::separator() + file;
    QString errorFileName = directory + QDir::separator() + errorFile;
    QUrl testUrl = testFileUrl(fileName);

    QQmlEngine engine;
    engine.addImportPath(importPath);

    QQmlComponent component(&engine, testUrl);
    QList<QQmlError> errors = component.errors();
    VERIFY_ERRORS(errorFileName.toLatin1().constData());
    QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    QObject *obj = component.create();
    QVERIFY(!obj);
    delete obj;
}

void tst_qqmlmoduleplugin::importsNested_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    // Note: no other test case should import the plugin used for this test, or the
    // wrong order test will pass spuriously
    QTest::newRow("wrongOrder") << "importsNested.1.qml" << "importsNested.1.errors.txt";
    QTest::newRow("missingImport") << "importsNested.3.qml" << "importsNested.3.errors.txt";
    QTest::newRow("invalidVersion") << "importsNested.4.qml" << "importsNested.4.errors.txt";
    QTest::newRow("correctOrder") << "importsNested.2.qml" << QString();
}
void tst_qqmlmoduleplugin::importsNested()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    // Note: because imports are cached between test case data rows (and the plugins remain loaded),
    // these tests should really be run in new instances of the app...

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    if (!errorFile.isEmpty()) {
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    }

    static int count = 0;
    if (++count == 1)
        QTest::ignoreMessage(QtWarningMsg, "Module 'com.nokia.AutoTestQmlNestedPluginType' does not contain a module directive - it cannot be protected from external registrations.");

    QQmlComponent component(&engine, testFile(file));
    QObject *obj = component.create();

    if (errorFile.isEmpty()) {
        if (qgetenv("DEBUG") != "" && !component.errors().isEmpty())
            qWarning() << "Unexpected Errors:" << component.errors();
        QVERIFY(obj);
        delete obj;
    } else {
        QList<QQmlError> errors = component.errors();
        VERIFY_ERRORS(errorFile.toLatin1().constData());
        QVERIFY(!obj);
    }
}

void tst_qqmlmoduleplugin::importLocalModule()
{
    QFETCH(QString, qml);
    QFETCH(int, majorVersion);
    QFETCH(int, minorVersion);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), testFileUrl("empty.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != 0);
    QCOMPARE(object->property("majorVersion").value<int>(), majorVersion);
    QCOMPARE(object->property("minorVersion").value<int>(), minorVersion);
}

void tst_qqmlmoduleplugin::importLocalModule_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<int>("majorVersion");
    QTest::addColumn<int>("minorVersion");

    QTest::newRow("default version")
        << "import \"localModule\"\n"
           "TestComponent {}"
        << 2 << 0;

    QTest::newRow("specific version")
        << "import \"localModule\" 1.1\n"
           "TestComponent {}"
        << 1 << 1;

    QTest::newRow("lesser version")
        << "import \"localModule\" 1.0\n"
           "TestComponent {}"
        << 1 << 0;

    // Note: this does not match the behaviour of installed modules, which fail for this case:
    QTest::newRow("nonexistent version")
        << "import \"localModule\" 1.3\n"
           "TestComponent {}"
        << 1 << 1;

    QTest::newRow("high version")
        << "import \"localModule\" 2.0\n"
           "TestComponent {}"
        << 2 << 0;
}

void tst_qqmlmoduleplugin::importStrictModule()
{
    QFETCH(QString, qml);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    if (!warning.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    QUrl url(testFileUrl("empty.qml"));

    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), url);

    if (error.isEmpty()) {
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != 0);
    } else {
        QVERIFY(!component.isReady());
        QCOMPARE(component.errors().count(), 1);
        QCOMPARE(component.errors().first().toString(), url.toString() + error);
    }
}

void tst_qqmlmoduleplugin::importStrictModule_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("error");

    QTest::newRow("success")
        << "import com.nokia.StrictModule 1.0\n"
           "MyPluginType {}"
        << QString()
        << QString();

    QTest::newRow("wrong target")
        << "import com.nokia.InvalidStrictModule 1.0\n"
           "MyPluginType {}"
        << QString()
        << ":1:1: plugin cannot be loaded for module \"com.nokia.InvalidStrictModule\": Cannot install element 'MyPluginType' into unregistered namespace 'com.nokia.SomeOtherModule'";

    QTest::newRow("non-strict clash")
        << "import com.nokia.NonstrictModule 1.0\n"
           "MyPluginType {}"
        << "Module 'com.nokia.NonstrictModule' does not contain a module directive - it cannot be protected from external registrations."
        << ":1:1: plugin cannot be loaded for module \"com.nokia.NonstrictModule\": Cannot install element 'MyPluginType' into protected namespace 'com.nokia.StrictModule'";

    QTest::newRow("non-strict preemption")
        << "import com.nokia.PreemptiveModule 1.0\n"
           "import com.nokia.PreemptedStrictModule 1.0\n"
           "MyPluginType {}"
        << "Module 'com.nokia.PreemptiveModule' does not contain a module directive - it cannot be protected from external registrations."
        << ":2:1: plugin cannot be loaded for module \"com.nokia.PreemptedStrictModule\": Namespace 'com.nokia.PreemptedStrictModule' has already been used for type registration";

    QTest::newRow("invalid namespace")
        << "import com.nokia.InvalidNamespaceModule 1.0\n"
           "MyPluginType {}"
        << QString()
        << ":1:1: plugin cannot be loaded for module \"com.nokia.InvalidNamespaceModule\": Module namespace 'com.nokia.AwesomeModule' does not match import URI 'com.nokia.InvalidNamespaceModule'";
}

QTEST_MAIN(tst_qqmlmoduleplugin)

#include "tst_qqmlmoduleplugin.moc"
