/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/

#include "imageviewer.h"


ImageViewer::ImageViewer(QWidget *parent, const char * name, WFlags f)
        : QWidget(parent, name, f), painter(this)
{
    this->setFocusPolicy(QWidget::StrongFocus);
    this->setBackgroundColor(QColor(0, 0, 0));
    this->setMinimumSize(240, 320);
}


ImageViewer::~ImageViewer()
{
}


void ImageViewer::drawImage(uchar *data, size_t size)
{
    image.loadFromData(data, size);
    pm.convertFromImage(image, 0);
    setBackgroundMode(Qt::NoBackground);
    update();
}


void ImageViewer::paintEvent(QPaintEvent *e)
{
    this->setMinimumSize(pm.width(), pm.height());
    if (pm.size() != QSize(0, 0)) {
        painter.setClipRect(e->rect());
        painter.drawPixmap(0, 0, pm);
    }
}


void ImageViewer::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    kdDebug(2120) << "Mouse Press: x = " << e->x() << ", y = " << e->y() << endl;

    emit mousePressed(e->button(), e->x(), e->y());

    currentButton = e->button();

    if (e->button() == Qt::LeftButton) {
        kdDebug(2120) << "    Left Button" << endl;
    } else if (e->button() == Qt::RightButton) {
        kdDebug(2120) << "    Right Button" << endl;
    } else if (e->button() == Qt::MidButton) {
        kdDebug(2120) << "    Mid Button" << endl;
    } else if (e->button() == Qt::NoButton) {
        kdDebug(2120) << "    No Button" << endl;
    }
}


void ImageViewer::mouseReleaseEvent(QMouseEvent *e)
{
    e->accept();
    kdDebug(2120) << "Mouse Release: x = " << e->x() << ", y = " << e->y() << endl;

    emit mouseReleased(e->button(), e->x(), e->y());

    currentButton = Qt::NoButton;

    if (e->button() == Qt::LeftButton) {
        kdDebug(2120) << "    Left Button" << endl;
    } else if (e->button() == Qt::RightButton) {
        kdDebug(2120) << "    Right Button" << endl;
    } else if (e->button() == Qt::MidButton) {
        kdDebug(2120) << "    Mid Button" << endl;
    } else if (e->button() == Qt::NoButton) {
        kdDebug(2120) << "    No Button" << endl;
    }
}


void ImageViewer::mouseMoveEvent(QMouseEvent *e)
{
    e->accept();
    kdDebug(2120) << "Mouse Move: x = " << e->x() << ", y = " << e->y() << endl;

    emit mouseMoved(currentButton, e->x(), e->y());

    if (e->button() == Qt::LeftButton) {
        kdDebug(2120) << "    Left Button" << endl;
    } else if (e->button() == Qt::RightButton) {
        kdDebug(2120) << "    Right Button" << endl;
    } else if (e->button() == Qt::MidButton) {
        kdDebug(2120) << "    Mid Button" << endl;
    } else if (e->button() == Qt::NoButton) {
        kdDebug(2120) << "    No Button" << endl;
    }
}


void ImageViewer::wheelEvent(QWheelEvent *e)
{
    e->accept();
    kdDebug(2120) << "WheelEvent" << endl;

    emit wheelRolled(e->delta());
}


void ImageViewer::keyPressEvent(QKeyEvent *e)
{
    e->accept();
    kdDebug(2120) << "Key Press: ASCII = " << e->ascii() <<
                     ", State = " << e->state() <<
                     ", Key = " << e->key() << endl;

    emit keyPressed(e->ascii(), e->key());
}


void ImageViewer::keyReleaseEvent(QKeyEvent *e)
{
    e->accept();
    kdDebug(2120) << "Key Release: ASCII = " << e->ascii() <<
                     ", State = " << e->state() <<
                     ", Key = " << e->key() << endl;

    emit keyReleased(e->ascii(), e->key());
}


void ImageViewer::enterEvent(QEvent */*e*/)
{
    kdDebug(2120) << "Enter Event" << endl;
}


void ImageViewer::leaveEvent(QEvent */*e*/)
{
    kdDebug(2120) << "Leave Event" << endl;
}


#include "imageviewer.moc"
