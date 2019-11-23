/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "chasewidget.h"

#include <QtCore/QPoint>

#include <QtWidgets/QApplication>
#include <QtGui/QHideEvent>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QShowEvent>

#define USE_NEW_STYLE

#ifdef USE_NEW_STYLE
    #define TIMER_DELAY 20
#else
    #define TIMER_DELAY 100
#endif

ChaseWidget::ChaseWidget(QWidget *parent, QPixmap pixmap, bool pixmapEnabled)
    : QWidget(parent)
    , m_segment(0)
    , m_delay(TIMER_DELAY)
    , m_step(40)
    , m_timerId(-1)
    , m_animated(false)
    , m_pixmap(pixmap)
    , m_pixmapEnabled(pixmapEnabled)
    , m_active(true)
{
}

void ChaseWidget::setAnimated(bool value)
{
    if (m_animated == value)
        return;

    m_animated = value;
    if (m_timerId != -1) {
        killTimer(m_timerId);
        m_timerId = -1;
    }
    if (m_animated) {
        m_segment = 0;
        m_astep = 0;
        m_timerId = startTimer(m_delay);
    }
    update();
}

void ChaseWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

#ifndef USE_NEW_STYLE
    QPainter p(this);
    if (m_pixmapEnabled && !m_pixmap.isNull()) {
        p.drawPixmap(0, 0, m_pixmap);
        return;
    }

    const int extent = qMin(width() - 8, height() - 8);
    const int displ = extent / 4;
    const int ext = extent / 4 - 1;

    p.setRenderHint(QPainter::Antialiasing, true);

    if (m_animated)
        p.setPen(Qt::gray);
    else
        p.setPen(QPen(palette().dark().color()));

    p.translate(width() / 2, height() / 2); // center

    for (int segment = 0; segment < segmentCount(); ++segment) {
        p.rotate(QApplication::isRightToLeft() ? m_step : -m_step);
        if (m_animated)
            p.setBrush(colorForSegment(segment));
        else
            p.setBrush(palette().background());
        p.drawEllipse(QRect(displ, -ext / 2, ext, ext));
    }

#else

    if ( ! m_active) return;

    if ( ! m_animated)  return;


    const int size = height();
    const int iconSize = size * 0.75;
    const int offset = (size - iconSize) / 2;
    //const int ext = extent / 4 - 1;

    QRect rect(0,offset,iconSize,iconSize);
    QPainter p(this);

    QSize m_scaledSize;// = rect.size();

    paint(&p, rect, m_scaledSize);

#endif
}

void ChaseWidget::paint(QPainter *painter, const QRect &rect, const QSize &m_scaledSize)
{
    QColor m_color = QColor(0, 0, 0, 200);

    const int maxSize = qMin(rect.width(), rect.height());

    QSize iconSize;
    if (m_scaledSize.isValid())
    {
        iconSize = m_scaledSize;
    }
    else
    {
        iconSize = QSize(maxSize * 0.8, maxSize * 0.8);
    }

    const int offset_x = (maxSize - iconSize.width()) / 2;
    const int offset_y = (maxSize - iconSize.height()) / 2;

    const int x = rect.x() + offset_x;
    const int y = rect.y() + offset_y;

    const qreal circleOffset = (iconSize.width() / 8.0);
    const QRectF targetRectangle((x + circleOffset), (y + circleOffset), (iconSize.width() - (circleOffset * 2)), (iconSize.height() - (circleOffset * 2)));
    QConicalGradient gradient(targetRectangle.center(), m_astep);
    gradient.setColorAt(0, m_color);
    gradient.setColorAt(1, Qt::transparent);

    painter->save();
    painter->setRenderHint(QPainter::HighQualityAntialiasing);
    painter->setPen(QPen(gradient, (circleOffset * 1.5)));
    painter->drawArc(targetRectangle, 0, 5760); //16 * 360
    painter->restore();
}

void ChaseWidget::setActive(bool active)
{
    m_active = active;
}

QSize ChaseWidget::sizeHint() const
{
    return QSize(20, 30);
}

void ChaseWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timerId) {
        ++m_segment;

        m_astep -= 6;

        if (m_astep == -360)
        {
            m_astep = 0;
        }

        update();

        emit updated();
    }

    QWidget::timerEvent(event);
}

QColor ChaseWidget::colorForSegment(int seg) const
{
    int index = ((seg + m_segment) % segmentCount());
    int comp = qMax(0, 255 - (index * (255 / segmentCount())));
    return QColor(comp, comp, comp, 255);
}

int ChaseWidget::segmentCount() const
{
    return 360 / m_step;
}

void ChaseWidget::setPixmapEnabled(bool enable)
{
    m_pixmapEnabled = enable;
}
