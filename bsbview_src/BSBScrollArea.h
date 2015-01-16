#ifndef BSBScrollArea_INCLUDED
#define BSBScrollArea_INCLUDED
/*
*  BSBMainWindow.cpp - implementation of BSBView for qchart - a marine BSB chart viewer
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
*  $Id: BSBScrollArea.h,v 1.1 2007/02/18 07:56:56 mikrom Exp $
        *
*/

#include <QScrollArea>
#include <QMenu>
#include <QTime>
#include "BSBWidget.h"
#include "bsb.h"

class BSBScrollArea : public QScrollArea {
    Q_OBJECT
public:
    BSBScrollArea(QWidget* parent = 0);
    
    bool setChartFile(const char* filename);
    const char* chartName() const { return  bsb ? bsb->name : ""; }
    
    void setZoomAutoScroll(int zoom, int xcenter = -1, int ycenter = -1 );
    void scrollBy( int dx, int dy );
    void scrollTo( int x, int y );
    void scrollTo( QPoint p ) { scrollTo( p.x(), p.y() ); }
    void scrollCenterTo( int x, int y ) { scrollTo( QPoint(x,y)+centerViewportOffset() ); }
    QPoint centerViewportOffset() { return QPoint( viewport()->width()/2, viewport()->height()/2 ); }
    QPoint viewportOffset();
signals:
    void chartChanged();
protected:
    virtual void keyPressEvent( QKeyEvent * );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseReleaseEvent( QMouseEvent * );
    virtual void mouseDoubleClickEvent( QMouseEvent * );
    virtual void mouseMoveEvent( QMouseEvent * );
    virtual void contextMenuEvent( QContextMenuEvent * );
    virtual void wheelEvent( QWheelEvent * );
    virtual QMenu* createPopupMenu();
    
private slots:
    void openChart();
    void zoomIn();
    void zoomOut();
    void zoomNorm();
    void toggleDisplayRefs();
    void toggleDisplayBorder();
    void centerChart();
    void chartInfo();
    void contextMenu(QPoint pos);

private:
    BSBWidget* bsbw;
    BSBImage*   bsb;
    char* bsbFileName;

    bool       mseDown;              // TRUE if mouse button down
    QPoint     msedwnpos;
    QTime      msedwntme;
    
    QString    lastOpenDir;
};

#endif // BSBScrollArea_INCLUDED
