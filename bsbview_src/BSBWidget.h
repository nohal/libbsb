#ifndef BSBWidget_INCLUDED
#define BSBWidget_INCLUDED
/*
*  BSBWidget.h	- declaration of BSBWidget for qchart - a marine BSB chart viewer
*
*  Copyright (C) 2006  Michal Krombholz <michkrom@users.sourceforge.net>
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
*  $Id: BSBWidget.h,v 1.1 2007/02/18 07:56:56 mikrom Exp $
*
*/

#include <bsb.h>
#include <QWidget>
#include <QImage>
#include <QPaintEvent>

//
// BSBWidget - draws BSB image
//
class BSBWidget : public QWidget
{
    Q_OBJECT

    public:
    BSBWidget( QWidget *parent=0 );
    ~BSBWidget();

    void setChart(BSBImage* bsb);
    void setZoom(int zoom);
    int  getZoom() { return m_zoom; }
    QSize getZoomedSize();
    
    void screenToChart(int xs,int ys,int* xc, int* yc);
    void chartToScreen(int xc,int yc,int* xs, int* ys);
    void screenToLLStr(int x, int y, char* s);
                
    void setDisplayRefs(bool on) { display_refs = on; update(); }
    bool getDisplayRefs() { return display_refs; }
    void setDisplayBorder(bool on) { display_border = on; update(); }
    bool getDisplayBorder() { return display_border; }
    

    enum
    {
        MAXZOOM =  10,
        MINZOOM = -10
    };

protected:
    enum
    {
        MAXCOLORS = 256
    };

    virtual void paintEvent( QPaintEvent * );

    QImage* makeTileQuick(int xc, int yc, int zoom, const int TILESIZEX = 200,const int TILESIZEY = 200);
    QImage* makeTileSmooth(int xc, int yc, int zoom, const int TILESIZEX = 200,const int TILESIZEY = 200);

private:
    BSBImage*  m_bsb;
	int        m_zoom;
	bool       display_border;
	bool       display_refs;
};

#endif // #ifndef BSBWidget_INCLUDED
