/*
*  BSBScrollArea.cpp - implementation of BSBView for qchart - a marine BSB chart viewer
*
*  Copyright (C) 2006-2007  Michal Krombholz <mikrom@users.sourceforge.net>
*
*  This software is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  This software is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*  $Id: BSBScrollArea.cpp,v 1.1 2007/02/18 07:50:31 mikrom Exp $
        *
*/

#include <QScrollArea>
#include <QMenu>
#include <QFileDialog>
#include <QAction>
#include <QScrollBar>
#include <QMessageBox>
#include <QPainter>

#include "BSBWidget.h"
#include "BSBScrollArea.h"
        
BSBScrollArea::BSBScrollArea(QWidget* parent)
    : QScrollArea(parent), 
    bsbw(0), bsb(0), bsbFileName(0),
    mseDown(false),
    lastOpenDir(QDir::homePath()+"/BSB_ROOT")
{
//    horizontalScrollBar()->setSingleStep(1);
//    verticalScrollBar()->setSingleStep(1);
}

/**
 * Scrolls the chart relative to current position (by dx/dy offset) 
 * @param dx
 * @param dy
 */
void BSBScrollArea::scrollBy(int dx, int dy)
{
    verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
}

/**
 * Scrolls the chart to the absolute x/y position
 * @param x
 * @param y
 */
void BSBScrollArea::scrollTo(int x, int y)
{
    verticalScrollBar()->setValue(y);
    horizontalScrollBar()->setValue(x);
}

/**
 * Returns current content offset
 * @return QPoint with content offset
 */
QPoint BSBScrollArea::viewportOffset()
{
    return QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );
}

/**
 * Sets the chart
 * @param filename full path to the char KAP file
 * @return true if successfull
 */
bool BSBScrollArea::setChartFile(const char* filename)
{
    bool success = true;
    BSBImage* b = new BSBImage();
    if ( bsb_open_header((char*)filename, b) )
    {
        delete bsb;
        bsb = b;
        printf("Opened: %s\n",filename);
        bsbw = new BSBWidget(this);
        bsbw->setChart(bsb);
        bsbw->setZoom(-2);
        setWidget(bsbw);
        setWidgetResizable( true );
        centerChart();
        free(bsbFileName);
        bsbFileName = strdup(filename);
        emit chartChanged();
        
    }
    else
    {
        delete b;
        printf("Failed to open %s\n",filename);
        success = false;
    }
    return success;
}

/**
 * Sets zoom for the view
 * if xcenter/ycenter are specified, they become center of the chart otherwise the center is maintained
 */
void BSBScrollArea::setZoomAutoScroll(int zoom, int xcenter, int ycenter)
{
    int xc,yc;
    if( xcenter < 0 )
    {
        xcenter = viewport()->width()/2;
    }
    if( ycenter < 0 )
    {
        ycenter = viewport()->height()/2;
    }
    bsbw->screenToChart(xcenter+horizontalScrollBar()->value(),
                        ycenter+verticalScrollBar()->value(),
                        &xc, &yc);
    bsbw->setZoom( zoom );
    int xs,ys;
    bsbw->chartToScreen(xc,yc,&xs,&ys);
    scrollTo(xs-viewport()->width()/2,ys-viewport()->height()/2);
}

/**
 * Handles key events for the widget.
 */
void BSBScrollArea::keyPressEvent( QKeyEvent *e )
{
    int dx = 0;
    int dy = 0;
    switch ( e->key() )
    {
        case Qt::Key_Up: dy = -1; break;
        case Qt::Key_Down: dy = 1; break;
        case Qt::Key_Left: dx = -1; break;
        case Qt::Key_Right: dx = 1; break;
    }
    if( dx || dy )
    {
        int shift = (e->modifiers() & Qt::ControlModifier) ? 3 : 1;
        scrollBy( dx*viewport()->height()*shift/4, dy+viewport()->width()*shift/4 );
        return;
    }
    switch ( e->key() )
    {
        case Qt::Key_Plus:
        case Qt::Key_Z: 
            zoomIn(); 
            break;
        case Qt::Key_Minus:
        case Qt::Key_X: 
            zoomOut(); 
            break;
        case Qt::Key_C:
            centerChart(); 
            break;
        case Qt::Key_N:
            zoomNorm(); 
            break;
        case Qt::Key_R:
            toggleDisplayRefs();
            break;
        case Qt::Key_B:
            toggleDisplayBorder();
            break;
        case Qt::Key_I:
            chartInfo();
            break;
        case Qt::Key_O:
            openChart();
            break;
        case Qt::Key_F:
            /*todo full screen*/; break;
        default: return;
    }
}


/**
 * Creats context popup menu
 */
QMenu* BSBScrollArea::createPopupMenu ()
{
    QMenu* pm = new QMenu(this);
    pm->addAction(tr("Zoom In"),this,SLOT(zoomIn()),tr("Z"));
    pm->addAction(tr("Zoom Out"),this,SLOT(zoomOut()),tr("X"));
    pm->addAction(tr("Zoom Normal (1:1)"),this,SLOT(zoomNorm()),tr("N"));
    pm->addAction(tr("Center"),this,SLOT(centerChart()),tr("C"));
    pm->addSeparator();
    pm->addAction(tr("Display Chart Refs"),this,SLOT(toggleDisplayRefs()),tr("R"));
    pm->addAction(tr("Display Chart Border"),this,SLOT(toggleDisplayBorder()),tr("B"));
    pm->addSeparator();
    pm->addAction(tr("Open chart"),this,SLOT(openChart()),tr("O"));
    pm->addAction(tr("Chart Info"),this,SLOT(chartInfo()),tr("I"));
    return pm;
}

/**
 * Handles context menu event
 */
void BSBScrollArea::contextMenuEvent ( QContextMenuEvent * e )
{
// just do nothing so default contex menu handling is supressed
}

/**
 * Handles mouse press events for the widget.
 */
void BSBScrollArea::mousePressEvent( QMouseEvent *e )
{
    msedwntme.start();
    msedwnpos = e->pos();
    switch ( e->button() )
    {
        case Qt::LeftButton : {
            //cursor.setShape( Qt::SizeAllCursor );
            //setCursor( cursor );
            mseDown = true;
            break;
        }
        case Qt::RightButton : {
            break;
        }
        default:
            break;
    }
}


/**
 * Handles mouse release events for the connect widget.
 * The code is smart on right click:
 * - if it was a short click and mouse did not move much then it's a popup menu
 * - if the time was longer and mouse move then it is a gesture
 */
void BSBScrollArea::mouseReleaseEvent( QMouseEvent *e )
{
    //cursor.setShape( Qt::CrossCursor );
    //setCursor( cursor );
    QPoint mseuppos = e->pos();
    switch ( e->button() )
    {
        case Qt::LeftButton :
            if ( mseDown )
            {
                mseDown = false;
                int dx = - (mseuppos.x()-msedwnpos.x());
                int dy = - (mseuppos.y()-msedwnpos.y());
                scrollBy( dx, dy );
            }
            break;
        case Qt::RightButton : {
            QPoint d = msedwnpos - mseuppos;
            int adx = d.x() > 0 ? d.x() : -d.x();
            int ady = d.y() > 0 ? d.y() : -d.y();
            if ( msedwntme.elapsed() < 300 /* ms */ && adx < 10 && ady < 10 )
            {
                contextMenu( QPoint(e->x(),e->y()) );
            }
            else /* its a gesture - up (zoom in) or down (zoom out) */
            {
                QPoint p = msedwnpos/*+QPoint(d.x()/2, d.y()/2)*/+viewportOffset();
                int xc,yc;
                bsbw->screenToChart(p.x(), p.y(), &xc, &yc);
                int newzoom = bsbw->getZoom();
                newzoom = (d.y()>0) ? newzoom+1 : newzoom-1;
                bsbw->setZoom( newzoom );
                int x,y;
                bsbw->chartToScreen(xc, yc, &x, &y);
                scrollTo(QPoint(x, y)-centerViewportOffset());
            }
            break;
        }
        default:
            break;
    }
}

/**
 * Handles mouse move events for the widget.
 */
void BSBScrollArea::mouseMoveEvent( QMouseEvent *e )
{
    if ( mseDown )
    {
        QPoint d = msedwnpos-e->pos();
        scrollBy(d.x(), d.y());
        msedwnpos = e->pos();
    }
}

/**
 * Handles mouse double click press events for the connect widget.
 * Here execute ZOOM IN action at the click point.
 */
void BSBScrollArea::mouseDoubleClickEvent ( QMouseEvent * e )
{
    // compute new chart coord in e->xy
    QPoint ofs = viewportOffset() + e->pos();
    int xc, yc;
    bsbw->screenToChart( ofs.x(), ofs.y(), &xc, &yc );
    bsbw->setZoom( bsbw->getZoom()+1 );
    // compute back and scroll to new pos
    int x,y;
    bsbw->chartToScreen( xc, yc, &x, &y );
    x -= e->x();
    y -= e->y();
    scrollTo(x, y);
}


/**
 * Handles mouse double click press events for the connect widget.
 * Here execute ZOOM IN action at the click point.
 */
void BSBScrollArea::wheelEvent( QWheelEvent * e )
{
    // compute new chart coord in e->xy
    QPoint ofs = viewportOffset() + e->pos();
    int xc, yc;
    bsbw->screenToChart( ofs.x(), ofs.y(), &xc, &yc );
    bsbw->setZoom( bsbw->getZoom() + (e->delta() > 0 ? 1 : -1) );
    // compute back and scroll to new pos
    int x,y;
    bsbw->chartToScreen( xc, yc, &x, &y );
    x -= e->x();
    y -= e->y();
    scrollTo(x, y);
}

/**
 * Open dialog and let the user select the chart file
 */
void BSBScrollArea::openChart()
{
    QFileDialog fd(this,"Open BSB(KAP) chart file",lastOpenDir,"BSB Charts (*.KAP)");
    if( fd.exec() == QDialog::Accepted )
    {
        QStringList qfn = fd.selectedFiles();
        setChartFile( qfn.first().toAscii( ) );
        lastOpenDir=fd.directory().absolutePath();
    }
}

/**
 * Slot to zoom in
 */
void BSBScrollArea::zoomIn()
{
    setZoomAutoScroll( bsbw->getZoom()+1 );
}

/**
 * Slot to zoom out
 */
void BSBScrollArea::zoomOut()
{
    setZoomAutoScroll( bsbw->getZoom()-1 );
}

/**
 * Slot to set normal zoom
 */
void BSBScrollArea::zoomNorm()
{
    setZoomAutoScroll( 0 );
}

/**
 * Slot to set normal zoom
 */
void BSBScrollArea::toggleDisplayRefs()
{
    bsbw->setDisplayRefs( !bsbw->getDisplayRefs() );
}

/**
 * Slot to center the chart
 */
void BSBScrollArea::toggleDisplayBorder()
{
    bsbw->setDisplayBorder( !bsbw->getDisplayBorder() );
}

/**
 * Slot to center the chart
 */
void BSBScrollArea::centerChart()
{
    QSize zs = bsbw->getZoomedSize();
    int vx = viewport()->width()/2;
    int vy = viewport()->height()/2;
    scrollTo(zs.width()/2-vx,zs.height()/2-vy);
}

/**
 * Slot to show chart infor
 */
void BSBScrollArea::chartInfo()
{
    char txt[1000];
    sprintf(txt,
            "%s<br>"
            "%s<br>"
            "version   : %f<br>"
            "%s/%s<br>"
            "scale     : %.0f at %.3f lat<br>"
            "res x/y   : %.3f/%.3f<br>"
            "# refs    : %d<br>"
            "# borders : %d<br>"
            "# geocoef : %d/%d/%d/%d  %d/%d/%d/%d<br>"
            "cph       : %f<br>"
            ,
            bsbFileName,
            bsb->name, bsb->version,
            bsb->projection, bsb->datum,
            bsb->scale, bsb->projectionparam,
            bsb->xresolution, bsb->yresolution,
            bsb->num_refs, bsb->num_plys,
            bsb->num_wpxs, bsb->num_wpys, bsb->num_pwxs, bsb->num_pwys,
            bsb->wpx_level, bsb->wpy_level, bsb->pwx_level, bsb->pwy_level, bsb->cph );
    QMessageBox::information(this, "About This Chart", txt, QMessageBox::Ok);
}


/**
 * Slot to popup contex menu
 */
void BSBScrollArea::contextMenu(QPoint pos)
{
    char s[100];
    QPoint pv = pos + viewportOffset();
    bsbw->screenToLLStr( pv.x(), pv.y(), s );
    QMenu* pm = createPopupMenu();
    pm->addSeparator();
    pm->addAction(s);
    if( pm )
    {
        pm->exec(mapToGlobal(pos));
    }
}
