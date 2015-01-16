#ifndef BSBWidget_INCLUDED
#define BSBWidget_INCLUDED
/*
*  BSBWidget.h	- declaration of BSBWidget for qchart - a marine BSB chart viewer
*
*  Copyright (C) 2006  Michal Krombholz <mikrom@users.sourceforge.net>
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
*  $Id: BSBWidget.h,v 1.1 2007/02/06 16:05:25 mikrom Exp $
*
*/

#include <bsb.h>

#include <qwidget.h>
#include <qcursor.h>
#include <qpoint.h>
#include <qdatetime.h>

//
// BSBWidget - draws BSB image
//
class BSBWidget : public QWidget
{
//    Q_OBJECT

    public:
    BSBWidget( QWidget *parent=0, const char *name=0 );
    ~BSBWidget();

    void setChartFile(const char* filename);
    void selectChartFile();
    void testGeoTransform(bool verbose);

protected:
    enum
    {
        MAXCOLORS = 256,
        MAXZOOM = 256
    };

    virtual void paintEvent( QPaintEvent * );
    virtual void mousePressEvent( QMouseEvent *);
    virtual void mouseReleaseEvent( QMouseEvent *);
    virtual void mouseDoubleClickEvent( QMouseEvent *);
    virtual void mouseMoveEvent( QMouseEvent *);
    virtual void doContextMenu( QPoint p );
    virtual void keyPressEvent( QKeyEvent * );

    enum Action
    {
        A_ZOOMIN,
        A_ZOOMOUT,
        A_ZOOMMAX,
        A_ZOOMMIN,
        A_CENTER ,
        A_FULLSCREEN,
        A_CHARTINFO,
        A_DISPBORDER,
        A_DISPREFS,
        A_OPEN,
        A_EXIT,
        A_ABOUT
    };

    void doAction(enum Action action,int par1 = 0, int par2 = 0);
    void zoomToFitChart();

    uint8_t*    getRow(int row);

    void screenToChart(int x, int y, int* xc, int* yc);
    void chartToScreen(int xc, int yc, int* x, int* y);
    void screenToLLStr(int x, int y, char* s);

private:
    QCursor    cursor;
    bool       mseDown;              // TRUE if mouse button down
    QPoint     msedwnpos;
    QTime      msedwntme;

    const char* bsbFileName;
    bool        bsbLoaded;
    uint8_t*    bsbBuf;
    BSBImage    bsb;
    uint        colors[MAXCOLORS]; // color array

    int        xc_ofs;
    int        yc_ofs;
    int        zoom;
    int        x_dwn;
    int        y_dwn;
    bool display_border;
    bool display_refs;
};

#endif // #ifndef BSBWidget_INCLUDED
