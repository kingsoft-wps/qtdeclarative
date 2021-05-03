/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QDebug>

#include <QtCore/qscopedpointer.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>

#include <private/qqmlengine_p.h>
#include <private/qqmltypedata_p.h>

#include "../../shared/util.h"

#include <array>
#include <memory>

class tst_qmlcompiler_manual : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void cppBinding();
    void signalHandlers();
    void signalHandlers_qmlcachegen();
    void jsFunctions();
    void changingBindings();
    void propertyAlias();
    void propertyChangeHandler();
    void propertyReturningFunction();

private:
    void signalHandlers_impl(const QUrl &url);
};

// test workaround: hardcode runtime function indices. because they could be
// rather unexpected and passing wrong ones leads to UB and flakiness.
//
// NB: if you update the QML files that are used by the QQmlEngine runtime
// function execution, make sure that the hardcoded values are in sync with
// those changes! An example of when things could go wrong: adding new, removing
// old or changing the order of e.g. bindings on properties, signal handlers, JS
// functions
namespace FunctionIndices {
static constexpr int HELLO_WORLD_GREETING_BINDING = 0;

static constexpr int SIGNAL_HANDLERS_ON_SIGNAL1 = 1;
static constexpr int SIGNAL_HANDLERS_ON_SIGNAL2 = 3;
static constexpr int SIGNAL_HANDLERS_QML_EMIT_SIGNAL1 = 4;
static constexpr int SIGNAL_HANDLERS_QML_EMIT_SIGNAL2 = 5;
static constexpr int SIGNAL_HANDLERS_QML_EMIT_SIGNAL2_WITH_ARGS = 6;

static constexpr int JS_FUNCTIONS_FUNC1 = 0;
static constexpr int JS_FUNCTIONS_FUNC2 = 1;
static constexpr int JS_FUNCTIONS_FUNC3 = 2;

static constexpr int CHANGING_BINDINGS_P2_BINDING = 0;
static constexpr int CHANGING_BINDINGS_RESET_TO_CONSTANT = 1;
static constexpr int CHANGING_BINDINGS_RESET_TO_NEW_BINDING = 2;

static constexpr int PROPERTY_ALIAS_ORIGIN_BINDING = 0;
static constexpr int PROPERTY_ALIAS_RESET_ALIAS_TO_CONSTANT = 1;
static constexpr int PROPERTY_ALIAS_RESET_ORIGIN_TO_CONSTANT = 2;
static constexpr int PROPERTY_ALIAS_RESET_ALIAS_TO_NEW_BINDING = 3;
static constexpr int PROPERTY_ALIAS_RESET_ORIGIN_TO_NEW_BINDING = 5;
static constexpr int PROPERTY_ALIAS_GET_ALIAS_VALUE = 7;

static constexpr int PROPERTY_CHANGE_HANDLER_P_BINDING = 0;
static constexpr int PROPERTY_CHANGE_HANDLER_ON_P_CHANGED = 1;

static constexpr int PROPERTY_RETURNING_FUNCTION_F_BINDING = 0;
};

// test utility function for type erasure. the "real" code would be
// auto-generated by the compiler
template<typename... IOArgs, size_t Size = sizeof...(IOArgs) + 1>
static void typeEraseArguments(std::array<void *, Size> &a, std::array<QMetaType, Size> &t,
                               std::nullptr_t, IOArgs &&... args)
{
    a = { /* return value */ nullptr, /* rest */
          const_cast<void *>(
                  reinterpret_cast<const void *>(std::addressof(std::forward<IOArgs>(args))))... };
    t = { /* return type */ QMetaType::fromType<void>(),
          /* types */ QMetaType::fromType<std::decay_t<IOArgs>>()... };
}

template<typename... IOArgs, size_t Size = sizeof...(IOArgs)>
static void typeEraseArguments(std::array<void *, Size> &a, std::array<QMetaType, Size> &t,
                               IOArgs &&... args)
{
    a = { /* all values, including return value */ const_cast<void *>(
            reinterpret_cast<const void *>(std::addressof(std::forward<IOArgs>(args))))... };
    t = { /* types */ QMetaType::fromType<std::decay_t<IOArgs>>()... };
}

// utility class that sets up QQmlContext for passed QObject. can be used as a
// base class to ensure that qmlEngine(object) is valid during initializer list
// evaluation
struct ContextRegistrator
{
    ContextRegistrator(QQmlEngine *engine, QObject *This)
    {
        Q_ASSERT(engine && This);
        // if This object has a parent, it's not considered to be a root object,
        // so it must instead have a dedicated context, but what to do when we
        // reparent the root item to e.g. QQuickWindow::contentItem()?
        Q_ASSERT(!This->parent() || engine->contextForObject(This->parent())
                 || engine->rootContext());
        QQmlContext *context = engine->rootContext();
        if (This->parent()) {
            QQmlContext *parentContext = engine->contextForObject(This->parent());
            if (parentContext)
                context = new QQmlContext(parentContext, This);
        }
        Q_ASSERT(context);
        // NB: not only sets the context, but also seeds engine into This, so
        // that qmlEngine(This) works
        engine->setContextForObject(This, context);
        Q_ASSERT(qmlEngine(This));
    }
};

class HelloWorld : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HelloWorld);
    Q_PROPERTY(QString hello READ getHello WRITE setHello BINDABLE bindableHello)
    Q_PROPERTY(QString greeting READ getGreeting WRITE setGreeting BINDABLE bindableGreeting)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    HelloWorld(QQmlEngine *e, QObject *parent = nullptr)
        : QObject(parent), ContextRegistrator(e, this)
    {
        hello = QStringLiteral("Hello, World");
        QPropertyBinding<QString> HelloWorldCpp_greeting_binding(
                [&]() {
                    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                    const auto index = FunctionIndices::HELLO_WORLD_GREETING_BINDING;
                    constexpr int argc = 0;
                    QString ret {};
                    std::array<void *, argc + 1> a {};
                    std::array<QMetaType, argc + 1> t {};
                    typeEraseArguments(a, t, ret);
                    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                    return ret;
                },
                QT_PROPERTY_DEFAULT_BINDING_LOCATION);
        bindableGreeting().setBinding(HelloWorldCpp_greeting_binding);
    }

    QString getHello() { return hello.value(); }
    QString getGreeting() { return greeting.value(); }

    void setHello(const QString &hello_) { hello.setValue(hello_); }
    void setGreeting(const QString &greeting_) { greeting.setValue(greeting_); }

    QBindable<QString> bindableHello() { return QBindable<QString>(&hello); }
    QBindable<QString> bindableGreeting() { return QBindable<QString>(&greeting); }

    Q_OBJECT_BINDABLE_PROPERTY(HelloWorld, QString, hello);
    Q_OBJECT_BINDABLE_PROPERTY(HelloWorld, QString, greeting);
};
QUrl HelloWorld::url = QUrl(); // workaround

void tst_qmlcompiler_manual::cppBinding()
{
    QQmlEngine e;
    HelloWorld::url = testFileUrl("HelloWorld.qml");
    HelloWorld created(&e);

    QCOMPARE(created.property("hello").toString(), QStringLiteral("Hello, World"));
    QCOMPARE(created.getGreeting(), QStringLiteral("Hello, World!"));
    QCOMPARE(created.property("greeting").toString(), QStringLiteral("Hello, World!"));

    created.setProperty("hello", QStringLiteral("Hello, Qml"));

    QCOMPARE(created.property("hello").toString(), QStringLiteral("Hello, Qml"));
    QCOMPARE(created.property("greeting").toString(), QStringLiteral("Hello, Qml!"));
    QCOMPARE(created.getGreeting(), QStringLiteral("Hello, Qml!"));
}

class ANON_signalHandlers : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int signal1P READ getSignal1P WRITE setSignal1P BINDABLE bindableSignal1P)
    Q_PROPERTY(QString signal2P1 READ getSignal2P1 WRITE setSignal2P1 BINDABLE bindableSignal2P1)
    Q_PROPERTY(int signal2P2 READ getSignal2P2 WRITE setSignal2P2 BINDABLE bindableSignal2P2)
    Q_PROPERTY(QString signal2P3 READ getSignal2P3 WRITE setSignal2P3 BINDABLE bindableSignal2P3)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_signalHandlers(QQmlEngine *e, QObject *parent = nullptr)
        : QObject(parent), ContextRegistrator(e, this)
    {
        signal1P = 0;
        signal2P1 = QStringLiteral("");
        signal2P2 = 0;
        signal2P3 = QStringLiteral("");

        QObject::connect(this, &ANON_signalHandlers::signal1, this,
                         &ANON_signalHandlers::onSignal1);
        QObject::connect(this, &ANON_signalHandlers::signal2, this,
                         &ANON_signalHandlers::onSignal2);
    }

    int getSignal1P() { return signal1P.value(); }
    QString getSignal2P1() { return signal2P1.value(); }
    int getSignal2P2() { return signal2P2.value(); }
    QString getSignal2P3() { return signal2P3.value(); }

    void setSignal1P(const int &signal1P_) { signal1P.setValue(signal1P_); }
    void setSignal2P1(const QString &signal2P1_) { signal2P1.setValue(signal2P1_); }
    void setSignal2P2(const int &signal2P2_) { signal2P2.setValue(signal2P2_); }
    void setSignal2P3(const QString &signal2P3_) { signal2P3.setValue(signal2P3_); }

    QBindable<int> bindableSignal1P() { return QBindable<int>(&signal1P); }
    QBindable<QString> bindableSignal2P1() { return QBindable<QString>(&signal2P1); }
    QBindable<int> bindableSignal2P2() { return QBindable<int>(&signal2P2); }
    QBindable<QString> bindableSignal2P3() { return QBindable<QString>(&signal2P3); }

    Q_OBJECT_BINDABLE_PROPERTY(ANON_signalHandlers, int, signal1P);
    Q_OBJECT_BINDABLE_PROPERTY(ANON_signalHandlers, QString, signal2P1);
    Q_OBJECT_BINDABLE_PROPERTY(ANON_signalHandlers, int, signal2P2);
    Q_OBJECT_BINDABLE_PROPERTY(ANON_signalHandlers, QString, signal2P3);

signals:
    void signal1();
    void signal2(QString x, int y);

public slots:
    void onSignal1()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::SIGNAL_HANDLERS_ON_SIGNAL1;
        e->executeRuntimeFunction(url, index, this);
    }

    void onSignal2(QString x, int y)
    {
        constexpr int argc = 2;
        std::array<void *, argc+1> a {};
        std::array<QMetaType, argc + 1> t {};
        typeEraseArguments(a, t, nullptr, x, y);

        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const qsizetype index = FunctionIndices::SIGNAL_HANDLERS_ON_SIGNAL2;
        e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
    }

public:
    void qmlEmitSignal1()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::SIGNAL_HANDLERS_QML_EMIT_SIGNAL1;
        e->executeRuntimeFunction(url, index, this);
    }

    void qmlEmitSignal2()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::SIGNAL_HANDLERS_QML_EMIT_SIGNAL2;
        e->executeRuntimeFunction(url, index, this);
    }

    void qmlEmitSignal2WithArgs(QString x, int y)
    {
        constexpr int argc = 2;
        std::array<void *, argc+1> a {};
        std::array<QMetaType, argc + 1> t {};
        typeEraseArguments(a, t, nullptr, x, y);

        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::SIGNAL_HANDLERS_QML_EMIT_SIGNAL2_WITH_ARGS;
        e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
    }
};
QUrl ANON_signalHandlers::url = QUrl(); // workaround

void tst_qmlcompiler_manual::signalHandlers_impl(const QUrl &url)
{
    QQmlEngine e;
    ANON_signalHandlers::url = url;
    ANON_signalHandlers created(&e);

    // signal emission works from C++
    emit created.signal1();
    emit created.signal2(QStringLiteral("42"), 42);

    QCOMPARE(created.property("signal1P").toInt(), 1);
    QCOMPARE(created.property("signal2P1").toString(), QStringLiteral("42"));
    QCOMPARE(created.property("signal2P2").toInt(), 42);
    QCOMPARE(created.property("signal2P3").toString(), QStringLiteral("4242"));

    // signal emission works through meta object system
    QMetaObject::invokeMethod(&created, "signal1");
    QMetaObject::invokeMethod(&created, "signal2", Q_ARG(QString, QStringLiteral("foo")),
                              Q_ARG(int, 23));

    QCOMPARE(created.property("signal1P").toInt(), 2);
    QCOMPARE(created.property("signal2P1").toString(), QStringLiteral("foo"));
    QCOMPARE(created.property("signal2P2").toInt(), 23);
    QCOMPARE(created.property("signal2P3").toString(), QStringLiteral("foo23"));

    // signal emission works through QML/JS
    created.qmlEmitSignal1();
    created.qmlEmitSignal2();

    QCOMPARE(created.property("signal1P").toInt(), 3);
    QCOMPARE(created.property("signal2P1").toString(), QStringLiteral("xyz"));
    QCOMPARE(created.property("signal2P2").toInt(), 123);
    QCOMPARE(created.property("signal2P3").toString(), QStringLiteral("xyz123"));

    created.qmlEmitSignal2WithArgs(QStringLiteral("abc"), 0);
    QCOMPARE(created.property("signal2P1").toString(), QStringLiteral("abc"));
    QCOMPARE(created.property("signal2P2").toInt(), 0);
    QCOMPARE(created.property("signal2P3").toString(), QStringLiteral("abc0"));
}

void tst_qmlcompiler_manual::signalHandlers()
{
    // use QQmlTypeCompiler's compilation unit
    signalHandlers_impl(testFileUrl("signalHandlers.qml"));
}

void tst_qmlcompiler_manual::signalHandlers_qmlcachegen()
{
    // use qmlcachegen's compilation unit
    signalHandlers_impl(QUrl("qrc:/data/signalHandlers.qml"));
}

class ANON_javaScriptFunctions : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int func1P READ getFunc1P WRITE setFunc1P)
    Q_PROPERTY(QString func2P READ getFunc2P WRITE setFunc2P)
    Q_PROPERTY(bool func3P READ getFunc3P WRITE setFunc3P)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_javaScriptFunctions(QQmlEngine *e, QObject *parent = nullptr)
        : QObject(parent), ContextRegistrator(e, this)
    {
        func1P = 0;
        func2P = QStringLiteral("");
        func3P = false;
    }

    int getFunc1P() { return func1P.value(); }
    QString getFunc2P() { return func2P.value(); }
    bool getFunc3P() { return func3P.value(); }

    void setFunc1P(const int &func1P_) { func1P.setValue(func1P_); }
    void setFunc2P(const QString &func2P_) { func2P.setValue(func2P_); }
    void setFunc3P(const bool &func3P_) { func3P.setValue(func3P_); }

    // try if just QProperty works
    QProperty<int> func1P;
    QProperty<QString> func2P;
    QProperty<bool> func3P;

    void func1()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::JS_FUNCTIONS_FUNC1;
        e->executeRuntimeFunction(url, index, this);
    }

    void func2(QString x)
    {
        constexpr int argc = 1;
        std::array<void *, argc+1> a {};
        std::array<QMetaType, argc + 1> t {};
        typeEraseArguments(a, t, nullptr, x);

        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::JS_FUNCTIONS_FUNC2;
        e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
    }

    bool func3()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::JS_FUNCTIONS_FUNC3;
        constexpr int argc = 0;
        bool ret {};
        std::array<void *, argc + 1> a {};
        std::array<QMetaType, argc + 1> t {};
        typeEraseArguments(a, t, ret);
        e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
        return ret;
    }
};
QUrl ANON_javaScriptFunctions::url = QUrl(); // workaround

void tst_qmlcompiler_manual::jsFunctions()
{
    QQmlEngine e;
    ANON_javaScriptFunctions::url = testFileUrl("javaScriptFunctions.qml");
    ANON_javaScriptFunctions created(&e);

    created.func1();
    created.func2(QStringLiteral("abc"));

    QCOMPARE(created.property("func1P").toInt(), 1);
    QCOMPARE(created.property("func2P").toString(), QStringLiteral("abc"));
    QCOMPARE(created.func3(), false);

    created.setProperty("func3P", true);
    QCOMPARE(created.func3(), true);
}

class ANON_changingBindings : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int p1 READ getP1 WRITE setP1 BINDABLE bindableP1)
    Q_PROPERTY(int p2 READ getP2 WRITE setP2 BINDABLE bindableP2)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;
    // test util to monitor binding execution
    int initialBindingCallCount = 0;
    // test util: allows to set C++ binding multiple times
    void resetToInitialBinding()
    {
        QPropertyBinding<int> ANON_changingBindings_p2_binding(
                [&]() {
                    initialBindingCallCount++;

                    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                    const auto index = FunctionIndices::CHANGING_BINDINGS_P2_BINDING;
                    constexpr int argc = 0;
                    int ret {};
                    std::array<void *, argc + 1> a {};
                    std::array<QMetaType, argc + 1> t {};
                    typeEraseArguments(a, t, ret);
                    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                    return ret;
                },
                QT_PROPERTY_DEFAULT_BINDING_LOCATION);
        bindableP2().setBinding(ANON_changingBindings_p2_binding);
    }

    ANON_changingBindings(QQmlEngine *e, QObject *parent = nullptr)
        : QObject(parent), ContextRegistrator(e, this)
    {
        p1 = 1;
        resetToInitialBinding();
    }

    int getP1() { return p1.value(); }
    int getP2() { return p2.value(); }

    void setP1(int p1_) { p1.setValue(p1_); }
    void setP2(int p2_) { p2.setValue(p2_); }

    QBindable<int> bindableP1() { return QBindable<int>(&p1); }
    QBindable<int> bindableP2() { return QBindable<int>(&p2); }

    Q_OBJECT_BINDABLE_PROPERTY(ANON_changingBindings, int, p1);
    Q_OBJECT_BINDABLE_PROPERTY(ANON_changingBindings, int, p2);

    void resetToConstant()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::CHANGING_BINDINGS_RESET_TO_CONSTANT;
        e->executeRuntimeFunction(url, index, this);
    }

    void resetToNewBinding()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::CHANGING_BINDINGS_RESET_TO_NEW_BINDING;
        e->executeRuntimeFunction(url, index, this);
    }
};
QUrl ANON_changingBindings::url = QUrl(); // workaround

void tst_qmlcompiler_manual::changingBindings()
{
    QQmlEngine e;
    ANON_changingBindings::url = testFileUrl("changingBindings.qml");
    ANON_changingBindings created(&e);

    // test initial binding
    QCOMPARE(created.initialBindingCallCount, 1); // eager evaluation
    QCOMPARE(created.property("p2").toInt(), 2); // p1 + 1
    QCOMPARE(created.initialBindingCallCount, 1);

    // test JS constant value
    created.resetToConstant();
    QCOMPARE(created.property("p2").toInt(), 42); // p2 = 42
    QCOMPARE(created.initialBindingCallCount, 1);

    // test Qt.binding()
    created.resetToNewBinding();
    created.setProperty("p1", 100);
    QCOMPARE(created.property("p2").toInt(), 200); // p1 * 2
    QCOMPARE(created.initialBindingCallCount, 1);

    // test setting initial (C++) binding
    created.setProperty("p1", 11);
    created.resetToInitialBinding();
    QCOMPARE(created.initialBindingCallCount, 2); // eager evaluation
    QCOMPARE(created.property("p2").toInt(), 12); // p1 + 1 (again)
    QCOMPARE(created.initialBindingCallCount, 2);

    // test resetting value through C++
    created.setP2(0);
    created.setP1(-10);

    QCOMPARE(created.property("p2").toInt(), 0);
    QCOMPARE(created.initialBindingCallCount, 2);

    created.setProperty("p2", 1);
    QCOMPARE(created.property("p2").toInt(), 1);
    QCOMPARE(created.initialBindingCallCount, 2);

    // test binding can be set again even after reset from C++
    created.resetToNewBinding();
    QCOMPARE(created.property("p2").toInt(), -20);
    QCOMPARE(created.initialBindingCallCount, 2);
}

class ANON_propertyAlias : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int dummy READ getDummy WRITE setDummy NOTIFY dummyChanged)
    Q_PROPERTY(int origin READ getOrigin WRITE setOrigin BINDABLE bindableOrigin)
    Q_PROPERTY(int aliasToOrigin READ getAliasToOrigin WRITE setAliasToOrigin BINDABLE
                       bindableAliasToOrigin)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;
    // test util: allows to set C++ binding multiple times
    void resetToInitialBinding()
    {
        QPropertyBinding<int> ANON_propertyAlias_origin_binding(
                [&]() {
                    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                    const auto index = FunctionIndices::PROPERTY_ALIAS_ORIGIN_BINDING;
                    constexpr int argc = 0;
                    int ret {};
                    std::array<void *, argc + 1> a {};
                    std::array<QMetaType, argc + 1> t {};
                    typeEraseArguments(a, t, ret);
                    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                    return ret;
                },
                QT_PROPERTY_DEFAULT_BINDING_LOCATION);
        bindableOrigin().setBinding(ANON_propertyAlias_origin_binding);
    }

    ANON_propertyAlias(QQmlEngine *e, QObject *parent = nullptr)
        : QObject(parent), ContextRegistrator(e, this)
    {
        dummy = 12;
        resetToInitialBinding();
    }

    int getDummy() { return dummy.value(); }
    int getOrigin() { return origin.value(); }
    int getAliasToOrigin() { return getOrigin(); }

    void setDummy(int dummy_)
    {
        dummy.setValue(dummy_);
        // emit is essential for Qt.binding() to work correctly
        emit dummyChanged();
    }
    void setOrigin(int origin_) { origin.setValue(origin_); }
    void setAliasToOrigin(int aliasToOrigin_) { setOrigin(aliasToOrigin_); }

    QBindable<int> bindableOrigin() { return QBindable<int>(&origin); }
    QBindable<int> bindableAliasToOrigin() { return bindableOrigin(); }

    QProperty<int> dummy;
    QProperty<int> origin;

    void resetAliasToConstant()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::PROPERTY_ALIAS_RESET_ALIAS_TO_CONSTANT;
        e->executeRuntimeFunction(url, index, this);
    }
    void resetOriginToConstant()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::PROPERTY_ALIAS_RESET_ORIGIN_TO_CONSTANT;
        e->executeRuntimeFunction(url, index, this);
    }
    void resetAliasToNewBinding()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::PROPERTY_ALIAS_RESET_ALIAS_TO_NEW_BINDING;
        e->executeRuntimeFunction(url, index, this);
    }
    void resetOriginToNewBinding()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::PROPERTY_ALIAS_RESET_ORIGIN_TO_NEW_BINDING;
        e->executeRuntimeFunction(url, index, this);
    }

    int getAliasValue()
    {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
        const auto index = FunctionIndices::PROPERTY_ALIAS_GET_ALIAS_VALUE;
        constexpr int argc = 0;
        int ret {};
        std::array<void *, argc + 1> a {};
        std::array<QMetaType, argc + 1> t {};
        typeEraseArguments(a, t, ret);
        e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
        return ret;
    }

signals:
    void dummyChanged();
};
QUrl ANON_propertyAlias::url = QUrl(); // workaround

void tst_qmlcompiler_manual::propertyAlias()
{
    QQmlEngine e;
    ANON_propertyAlias::url = testFileUrl("propertyAlias.qml");
    ANON_propertyAlias created(&e);

    // test initial binding
    QCOMPARE(created.property("origin").toInt(), 6); // dummy / 2
    QCOMPARE(created.property("aliasToOrigin").toInt(), 6);

    QCOMPARE(created.getAliasValue(), 6);
    QCOMPARE(created.getAliasToOrigin(), 6);
    created.setDummy(10);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 5);
    QCOMPARE(created.getAliasValue(), 5);
    QCOMPARE(created.getAliasToOrigin(), 5);

    // test the C++ setter
    created.setOrigin(7);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 7);
    QCOMPARE(created.getAliasValue(), 7);
    QCOMPARE(created.getAliasToOrigin(), 7);

    // test meta-object setter
    created.setProperty("origin", 1);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 1);
    QCOMPARE(created.getAliasValue(), 1);
    QCOMPARE(created.getAliasToOrigin(), 1);

    // test QML/JS setter
    created.resetOriginToConstant();
    QCOMPARE(created.property("aliasToOrigin").toInt(), 189);
    QCOMPARE(created.getAliasValue(), 189);
    QCOMPARE(created.getAliasToOrigin(), 189);

    // test QML/JS alias setter
    created.resetAliasToConstant();
    QCOMPARE(created.property("origin").toInt(), 42);
    QCOMPARE(created.getOrigin(), 42);
    // check the alias just to make sure it also works
    QCOMPARE(created.property("aliasToOrigin").toInt(), 42);
    QCOMPARE(created.getAliasValue(), 42);
    QCOMPARE(created.getAliasToOrigin(), 42);

    // test QML/JS binding reset
    created.resetOriginToNewBinding(); // dummy
    created.setDummy(99);
    QCOMPARE(created.property("aliasToOrigin").toInt(), 99);
    QCOMPARE(created.getAliasValue(), 99);
    QCOMPARE(created.getAliasToOrigin(), 99);

    // test QML/JS binding reset through alias
    created.resetAliasToNewBinding(); // dummy * 3
    created.setDummy(-8);
    QCOMPARE(created.property("origin").toInt(), -24);
    QCOMPARE(created.getOrigin(), -24);
    QCOMPARE(created.property("aliasToOrigin").toInt(), -24);
    QCOMPARE(created.getAliasValue(), -24);
    QCOMPARE(created.getAliasToOrigin(), -24);
}

class ANON_propertyChangeHandler : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int dummy READ getDummy WRITE setDummy)
    Q_PROPERTY(int p READ getP WRITE setP BINDABLE bindableP)
    Q_PROPERTY(int watcher READ getWatcher WRITE setWatcher)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_propertyChangeHandler(QQmlEngine *e, QObject *parent = nullptr)
        : QObject(parent), ContextRegistrator(e, this)
    {
        dummy = 42;
        QPropertyBinding<int> ANON_propertyChangeHandler_p_binding(
                [&]() {
                    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                    const auto index = FunctionIndices::PROPERTY_CHANGE_HANDLER_P_BINDING;
                    constexpr int argc = 0;
                    int ret {};
                    std::array<void *, argc + 1> a {};
                    std::array<QMetaType, argc + 1> t {};
                    typeEraseArguments(a, t, ret);
                    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                    return ret;
                },
                QT_PROPERTY_DEFAULT_BINDING_LOCATION);
        bindableP().setBinding(ANON_propertyChangeHandler_p_binding);
        watcher = 0;

        // NB: make sure property change handler appears after setBinding().
        // this prevents preliminary binding evaluation (which would fail as
        // this object doesn't yet know about qmlEngine(this))
        pChangeHandler.reset(new QPropertyChangeHandler<ANON_propertyChangeHandler_p_changeHandler>(
                bindableP().onValueChanged(ANON_propertyChangeHandler_p_changeHandler(this))));
    }

    int getDummy() { return dummy.value(); }
    int getP() { return p.value(); }
    int getWatcher() { return watcher.value(); }

    void setDummy(int dummy_) { dummy.setValue(dummy_); }
    void setP(int p_) { p.setValue(p_); }
    void setWatcher(int watcher_) { watcher.setValue(watcher_); }

    QBindable<int> bindableP() { return QBindable<int>(&p); }

    QProperty<int> dummy;
    QProperty<int> p;
    QProperty<int> watcher;

    // property change handler:
    struct ANON_propertyChangeHandler_p_changeHandler
    {
        ANON_propertyChangeHandler *This = nullptr;
        ANON_propertyChangeHandler_p_changeHandler(ANON_propertyChangeHandler *obj) : This(obj) { }

        void operator()()
        {
            QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(This));
            const auto index = FunctionIndices::PROPERTY_CHANGE_HANDLER_ON_P_CHANGED;
            e->executeRuntimeFunction(This->url, index, This);
        }
    };
    // the handler object has to be alive as long as the object
    std::unique_ptr<QPropertyChangeHandler<ANON_propertyChangeHandler_p_changeHandler>>
            pChangeHandler;
};
QUrl ANON_propertyChangeHandler::url = QUrl(); // workaround

void tst_qmlcompiler_manual::propertyChangeHandler()
{
    QQmlEngine e;
    ANON_propertyChangeHandler::url = testFileUrl("propertyChangeHandler.qml");
    ANON_propertyChangeHandler created(&e);

    // test that fetching "dirty" property value doesn't trigger property change
    // handler
    QCOMPARE(created.getWatcher(), 0);
    QCOMPARE(created.getP(), 42); // due to binding
    QCOMPARE(created.getWatcher(), 0);
    QCOMPARE(created.property("watcher").toInt(), 0);

    // test that binding triggers property change handler
    created.setDummy(20);
    QCOMPARE(created.getWatcher(), 20);
    QCOMPARE(created.property("watcher").toInt(), 20);

    // test that property setting (through C++) triggers property change handler
    created.setWatcher(-100);
    created.setProperty("p", 18);
    QCOMPARE(created.getWatcher(), 18);

    // test that property setting triggers property change handler
    created.setWatcher(-47);
    created.setP(96);
    QCOMPARE(created.property("watcher").toInt(), 96);
}

class ANON_propertyReturningFunction : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int counter READ getCounter WRITE setCounter)
    Q_PROPERTY(QVariant f READ getF WRITE setF BINDABLE bindableF)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_propertyReturningFunction(QQmlEngine *e, QObject *parent = nullptr)
        : QObject(parent), ContextRegistrator(e, this)
    {
        QPropertyBinding<QVariant> ANON_propertyReturningFunction_f_binding(
                [&]() {
                    QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(this));
                    const auto index = FunctionIndices::PROPERTY_RETURNING_FUNCTION_F_BINDING;
                    constexpr int argc = 0;
                    QVariant ret {};
                    std::array<void *, argc + 1> a {};
                    std::array<QMetaType, argc + 1> t {};
                    typeEraseArguments(a, t, ret);
                    e->executeRuntimeFunction(url, index, this, argc, a.data(), t.data());
                    return ret;
                },
                QT_PROPERTY_DEFAULT_BINDING_LOCATION);
        bindableF().setBinding(ANON_propertyReturningFunction_f_binding);
    }

    int getCounter() { return counter.value(); }
    QVariant getF() { return f.value(); }

    void setCounter(int counter_) { counter.setValue(counter_); }
    void setF(QVariant f_) { f.setValue(f_); }

    QBindable<QVariant> bindableF() { return QBindable<QVariant>(&f); }

    QProperty<int> counter;
    QProperty<QVariant> f;
};
QUrl ANON_propertyReturningFunction::url = QUrl(); // workaround

void tst_qmlcompiler_manual::propertyReturningFunction()
{
    QQmlEngine e;
    ANON_propertyReturningFunction::url = testFileUrl("propertyReturningFunction.qml");
    ANON_propertyReturningFunction created(&e);

    QCOMPARE(created.getCounter(), 0);
    QVariant function = created.getF();
    Q_UNUSED(function); // ignored as it can't be used currently
    QCOMPARE(created.getCounter(), 0);

    created.property("f");
    QCOMPARE(created.getCounter(), 0);
}

QTEST_MAIN(tst_qmlcompiler_manual)

#include "tst_qmlcompiler_manual.moc"
