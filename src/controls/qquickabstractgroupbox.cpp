/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickabstractgroupbox_p.h"
#include "qquickabstractcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype GroupBox
    \inherits Container
    \instantiates QQuickAbstractGroupBox
    \inqmlmodule QtQuick.Controls
    \ingroup containers
    \brief A group box control.

    TODO
*/

class QQuickAbstractGroupBoxPrivate : public QQuickAbstractContainerPrivate
{
public:
    QQuickAbstractGroupBoxPrivate() : label(Q_NULLPTR), frame(Q_NULLPTR) { }

    QString title;
    QQuickItem *label;
    QQuickItem *frame;
};

QQuickAbstractGroupBox::QQuickAbstractGroupBox(QQuickItem *parent) :
    QQuickAbstractContainer(*(new QQuickAbstractGroupBoxPrivate), parent)
{
}

/*!
    \qmlproperty string QtQuickControls2::GroupBox::title

    TODO
*/
QString QQuickAbstractGroupBox::title() const
{
    Q_D(const QQuickAbstractGroupBox);
    return d->title;
}

void QQuickAbstractGroupBox::setTitle(const QString &title)
{
    Q_D(QQuickAbstractGroupBox);
    if (d->title != title) {
        d->title = title;
        emit titleChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::GroupBox::label

    TODO
*/
QQuickItem *QQuickAbstractGroupBox::label() const
{
    Q_D(const QQuickAbstractGroupBox);
    return d->label;
}

void QQuickAbstractGroupBox::setLabel(QQuickItem *label)
{
    Q_D(QQuickAbstractGroupBox);
    if (d->label != label) {
        delete d->label;
        d->label = label;
        if (label && !label->parentItem())
            label->setParentItem(this);
        emit labelChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::GroupBox::frame

    TODO
*/
QQuickItem *QQuickAbstractGroupBox::frame() const
{
    Q_D(const QQuickAbstractGroupBox);
    return d->frame;
}

void QQuickAbstractGroupBox::setFrame(QQuickItem *frame)
{
    Q_D(QQuickAbstractGroupBox);
    if (d->frame != frame) {
        delete d->frame;
        d->frame = frame;
        if (frame && !frame->parentItem())
            frame->setParentItem(this);
        emit frameChanged();
    }
}

QT_END_NAMESPACE
