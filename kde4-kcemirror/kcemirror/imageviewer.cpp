/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 * Copyright (c) 2009 Mark Ellis <mark@mpellis.org.uk>                     *
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
#include <KFileDialog>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <KMessageBox>
#include <QPaintEvent>

#include <KDebug>
/*
#include <qpainter.h>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
*/

ImageViewer::ImageViewer(QWidget *parent, Qt::WFlags f)
        : QWidget(parent, f)
{
    this->setFocusPolicy(Qt::StrongFocus);

    QPalette palette;
    palette.setColor(this->backgroundRole(), QColor(0, 0, 0));
    this->setPalette(palette);
    this->setBackgroundRole(QPalette::NoRole);

    
    this->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
                                    QSizePolicy::Fixed
                                    )
                        );
    this->sizePolicy().setHorizontalStretch(0);
    this->sizePolicy().setVerticalStretch(0);
    
}


ImageViewer::~ImageViewer()
{
}


void ImageViewer::setPdaSize(int x, int y)
{
    this->setFixedSize(x, y);
}


void ImageViewer::drawImage()
{
    pm = QPixmap::fromImage(image, Qt::AutoColor);
    update();
}


void ImageViewer::loadImage(uchar *data, size_t size)
{
    if (!image.loadFromData(data, size))
        kDebug(2120) << "failed to load image data" << endl;

    setPdaSize(image.width(), image.height());
}


void ImageViewer::printImage()
{
    QPrinter printer;

    printer.setFullPage(true);

    QPrintDialog printDialog(&printer, this);

    if (printDialog.exec()) {
        QPainter painter;
        painter.begin( &printer );
        painter.drawPixmap(100, 100, pm);
        painter.end();
    }
}


void ImageViewer::saveImage()
{
    QString fileName = KFileDialog::getSaveFileName(KUrl(),
            "*.png *.bmp *.xbm *.xpm *.pnm *.jpeg *.mng *.pbm *.pgm *.ppm",
            this, "Save Screenshot");

    QString suffix = fileName.section('.', -1);


    if (!fileName.isEmpty()) {
        if (!suffix.isEmpty()) {
            if (!pm.save(fileName, suffix.toUpper().toAscii().data(), 100)) {
                KMessageBox::error(this, "Wrong image format suffix. Please specify a valid one.",
                        "Error Saving");
            }
        } else {
            KMessageBox::error(this, "Wrong image format suffix. Please specify a valid one.",
                    "Error Saving");
        }
    }
}


void ImageViewer::paintEvent(QPaintEvent *e)
{
    this->setMinimumSize(pm.width(), pm.height());
    if (pm.size() != QSize(0, 0)) {
        QPainter painter(this);
        painter.setClipRect(e->rect());
        painter.drawPixmap(0, 0, pm);
    }
}


void ImageViewer::mousePressEvent(QMouseEvent *e)
{
    e->accept();

    emit mousePressed(e->button(), e->x(), e->y());

    currentButton = e->button();
}


void ImageViewer::mouseReleaseEvent(QMouseEvent *e)
{
    e->accept();

    emit mouseReleased(e->button(), e->x(), e->y());

    currentButton = Qt::NoButton;
}


void ImageViewer::mouseMoveEvent(QMouseEvent *e)
{
    e->accept();

    emit mouseMoved(currentButton, e->x(), e->y());
}


void ImageViewer::wheelEvent(QWheelEvent *e)
{
    e->accept();

    emit wheelRolled(e->delta());
}


void ImageViewer::keyPressEvent(QKeyEvent *e)
{
    e->accept();
    kDebug(2120) << "Key Press: ASCII = " << e->text() <<
                     ", State = " << e->modifiers() <<
                     ", Key = " << e->key() << endl;

    emit keyPressed(e->ascii(), e->key());
}


void ImageViewer::keyReleaseEvent(QKeyEvent *e)
{
    e->accept();
    kDebug(2120) << "Key Release: ASCII = " << e->text() <<
                     ", State = " << e->modifiers() <<
                     ", Key = " << e->key() << endl;

    emit keyReleased(e->ascii(), e->key());
}


#include "imageviewer.moc"
