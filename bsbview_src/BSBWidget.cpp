/*
*  BSBWidget.cpp - implementation of BSBWidget for qchart - a marine BSB chart viewer
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
*  $Id: BSBWidget.cpp,v 1.1 2007/02/18 07:50:31 mikrom Exp $
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <bsb.h>
#include <qpainter.h>
#include <qpolygon.h>

#include "BSBWidget.h"

/**
 * Constructs a BSBWidget.
 */
BSBWidget::BSBWidget( QWidget *parent )
    : QWidget( parent ),
    m_bsb(0), m_zoom(0),
    display_border(false),
    display_refs(false)
{
}

/**
 * Destructs a BSBWidget.
 */
BSBWidget::~BSBWidget()
{
}

/**
 * Sets current raster chart to view
 *
 * @param filename full path to the BSB file name to open
 */
void
BSBWidget::setChart(BSBImage* bsb)
{
    m_bsb = bsb;
    setZoom(2);
}

/**
 * Given current zoom level return image size
 */
QSize BSBWidget::getZoomedSize()
{
    if( m_bsb )
    {
        if( m_zoom >= 0 )
            return QSize( m_bsb->width*(m_zoom+1), m_bsb->height*(m_zoom+1) );
        else // if( m_zoom < 0 )
            return QSize( -m_bsb->width/(m_zoom-1), -m_bsb->height/(m_zoom-1) );
    }
    else
        return QSize(640,400);
}

/**
 * Sets zoom level
 *
 * @param zoom level of zoom
 */
void BSBWidget::setZoom(int zoom)
{
    if ( zoom > MAXZOOM ) zoom = MAXZOOM;
    if ( zoom < MINZOOM ) zoom = MINZOOM;
    if( m_zoom != zoom )
    {
        m_zoom = zoom;
        setMinimumSize(getZoomedSize());
    }
}

/**
 * Converts screen x,y to chart xc,yc
 *
 * @param xs screen (viewport) X coordinate
 * @param ys screen (viewport) Y coordinate
 * @param xc output chart X coordinate
 * @param yc output chart Y coordinate
 */
void BSBWidget::screenToChart(int xs, int ys, int* xc, int* yc)
{
    int zoom = getZoom();
    if( zoom < 0 )
    {
        zoom --;
        zoom = -zoom;
        *xc=xs*zoom;
        *yc=ys*zoom;
    }
    else
    {
        zoom ++;
        *xc=xs/zoom;
        *yc=ys/zoom;
    }
	//printf("s2c %d %d %d %d\n", xs, ys, *xc, *yc );
}


/**
 * Converts chart xc, yc to screen x,y
 *
 * @param xc chart X coordinate
 * @param yc chart Y coordinate
 * @param xs output screen (viewport) X coordinate
 * @param ys output screen (viewport) Y coordinate
 */
void BSBWidget::chartToScreen(int xc, int yc, int* xs, int* ys)
{
    int zoom = getZoom();
    if( zoom < 0 )
    {
        zoom --;
        zoom = -zoom;
        *xs=xc/zoom;
        *ys=yc/zoom;
    }
    else
    {
        zoom ++;
        *xs=xc*zoom;
        *ys=yc*zoom;
    }
	//printf("c2s %d %d %d %d\n" , xc, yc, *xs, *ys );
}

/**
 * Converts screen x,y to chart Lat/Lon and formats for output
 *
 * @param x screen (viewport) X coordinate
 * @param y screen (viewport) Y coordinate
 * @param s output text buffer
 */
void BSBWidget::screenToLLStr(int x, int y, char* s)
{
    double lat,lon;
    int xc,yc;
    screenToChart(x, y, &xc, &yc);
    if( bsb_XYtoLL( m_bsb, xc, yc, &lon, &lat) )
    {
        char sn = 'N';
        if( lat < 0 )
        {
            sn = 'S';
            lat = -lat;
        }
        char ew = 'E';
        if( lon < 0 )
        {
            ew = 'W';
            lon = -lon;

        }
        sprintf(s, "Lat %7.3f%c Lon %7.3f%c", lat,sn,lon,ew);
    }
    else
    {
        strcpy(s, "Lat ? Lon ?");
    }
}

/**
 * Creates a single tile (part of image) of given coordinates and zoom.
 * Uses smooth (and slow) algorithm for scaling.
 *
 * @param xc chart offset that tile should include
 * @param yc chart offset that tile should include
 * @param zoom level of zoom of tile
 * @return pointer to QImage of tile
 */
 QImage* BSBWidget::makeTileSmooth(int xc, int yc, int zoom, const int TILESIZEX, const int TILESIZEY)
{
    //printf("mts %d %d %d %08x\n", xc, yc, zoom, tileID(xc,yc,zoom) );	
    QImage img( TILESIZEX*zoom, TILESIZEY*zoom, QImage::Format_Indexed8 );
    if( img.isNull() )
    {
        printf("QImage isNull: (x,y,w,h,z)=%d,%d,%d,%d,%d\n", xc, yc, TILESIZEX, TILESIZEY, zoom);
        return 0;
    }
	img.setNumColors( m_bsb->num_colors );
	for ( int col = 0; col < m_bsb->num_colors; col++ )
	{
		img.setColor( col, qRgb( m_bsb->red[col], m_bsb->green[col], m_bsb->blue[col] ) );
	}
	//img.fill(0);
	int xo = xc/*(xc/TILESIZEX)*TILESIZEY*/;
	int yo = yc/*(yc/TILESIZEY)*TILESIZEY*/;
	if( xo >= m_bsb->width ) printf("xo=%d width=%d\n", xo, m_bsb->width );	
	for ( int y = 0; y < img.height(); y++ )
	{
		uchar* ppix = img.scanLine(y);		
		if( yo+y < m_bsb->height )
		{		
			// tile goes over the chart on the right
			bsb_read_row_part( m_bsb, yo+y, ppix, xo, TILESIZEX*zoom );
		}
		else // tile goes over the chart on the bottom
		    memcpy(ppix, img.scanLine(y-1), img.width());
			//memset( ppix, 0, img.width() );
	}
    return new QImage(img.scaled(TILESIZEX,TILESIZEY,
                                 Qt::IgnoreAspectRatio, 
                                 Qt::SmoothTransformation));
}


/**
 * Creates a single tile (part of image) of given coordinates and zoom.
 * Uses quick (subsample) algorithm for scaling.
 *
 * @param xo widget offset that tile should include
 * @param yo widget offset that tile should include
 * @param zoom level of zoom of tile
 * @param TILESIZEX x size of tile
 * @param TILESIZEY y size of tile
 * @return pointer to QImage of tile
 */
QImage* BSBWidget::makeTileQuick(int xo, int yo, int zoom, const int TILESIZEX, const int TILESIZEY)
{
    //printf("mt %d %d %d\n", xc, yc, zoom );
    if( zoom < 0 )
    {
        // zoom of 0 means 1:1
        // negative zoom --> shrink it by subsampling
        zoom--;
        zoom = -zoom;
        int xc = xo*zoom;
        int yc = yo*zoom;
        QImage img( TILESIZEX, TILESIZEY, QImage::Format_Indexed8 );
        if( img.isNull() )
        {
            printf("QImage isNull: (x,y,w,h,z)=%d,%d,%d,%d,%d\n", xc, yc, TILESIZEX, TILESIZEY, zoom);
            return 0;
        }
        img.fill(0);
        img.setNumColors( m_bsb->num_colors );
        for ( int col = 0; col < m_bsb->num_colors; col++ )
        {
            img.setColor( col, qRgb( m_bsb->red[col], m_bsb->green[col], m_bsb->blue[col] ) );
        }
        int xs = TILESIZEX*zoom;
        uint8_t* row = new uint8_t[xs];
        memset( row, 0, xs );
        for ( int y = 0; y < TILESIZEY; y++ )
        {
            int yy = yc+y*zoom;
            if( yy < m_bsb->height )
                bsb_read_row_part( m_bsb, yy, row, xc, xs );
            else
                memset( row, 0, xs );
            uchar* ppix = img.scanLine(y);
            for ( int x = 0; x < TILESIZEX; x++ )
            {
                int xx = x*zoom;
                int col = row && xx >=0 && xx < m_bsb->width ? row[xx] : 0;
                *ppix++ = col;
            }
        }
        delete[] row;
        return new QImage(img);
    }
    else // enlarge
    {
        // zoom of 0 means 1:1
        zoom ++;
        int xc = xo/zoom;
        int yc = yo/zoom;
        int tw = TILESIZEX/zoom;
        tw = tw < 1 ? 1 : tw;
        int th = TILESIZEY/zoom;
        th = th < 1 ? 1 : th;
        QImage img( tw, th, QImage::Format_Indexed8 );
        if( img.isNull() )
        {
            printf("QImage isNull: (x,y,w,h,z)=%d,%d,%d,%d,%d\n", xc, yc, TILESIZEX, TILESIZEY, zoom);
            return 0;
        }
        img.fill(0);
        img.setNumColors( m_bsb->num_colors );
        for ( int col = 0; col < m_bsb->num_colors; col++ )
        {
            img.setColor( col, qRgb( m_bsb->red[col], m_bsb->green[col], m_bsb->blue[col] ) );
        }
        for ( int y = 0; y < TILESIZEY/zoom; y++ )
        {
            uchar* ppix = img.scanLine(y);
            int yy = yc+y;
            if( yy < m_bsb->height )
                bsb_read_row_part( m_bsb, yy, ppix, xc, img.width() );
            else
                memset(ppix,0, img.width());
        }
        // scale it up
        return new QImage(img.scaled(TILESIZEX,TILESIZEY,
                                     Qt::IgnoreAspectRatio, 
                                     Qt::FastTransformation));
    }
}

/**
 * Handles paint events for the connect widget.
 */
void BSBWidget::paintEvent(QPaintEvent* pe)
{
    QRect r = pe->rect();
    int cx = r.left();
    int cy = r.top();
    int cw = r.width();
    int ch = r.height();
    QPainter p( this );
    
    //printf("dc %d %d %d %d\n", cx, cy, cw, ch );
    // do we have anything to draw?
    if ( m_bsb )
    {
        QImage* img = makeTileQuick( cx, cy, m_zoom, cw, ch );
        if( img )
        {
            p.drawImage( cx, cy, *img );
            delete img;
        }
        if ( display_refs )
        {
            for ( int r = 0; r < m_bsb->num_refs; r++ )
            {
                const int size = 10;

                int xc, yc, x, y;
                chartToScreen(m_bsb->ref[r].x, m_bsb->ref[r].y, &x, &y);
                //printf( "(%f,%f) -> (%d,%d) -> (%d,%d)\n",
                //            m_bsb->ref[r].lon, m_bsb->ref[r].lat, xc, yc, x, y );
                if ( x >= cx && x < cx+cw && y >= cy && y < cy+ch )
                {
                    p.setPen(QColor(64,255,64));
                    p.drawRect( x-size/2, y-size/2, size, size );
                }

                bsb_LLtoXY( m_bsb, m_bsb->ref[r].lon, m_bsb->ref[r].lat, &xc, &yc );
                chartToScreen(xc, yc, &x, &y);
                //printf( "(%f,%f) -> (%d,%d) -> (%d,%d)\n",
                //            m_bsb->ref[r].lon, m_bsb->ref[r].lat, xc, yc, x, y );
                if ( x >= cx && x < cx+cw && y >= cy && y < cy+ch )
                {
                    p.setPen(QColor(255,64,64));
                    p.drawRect( x-size/2, y-size/2, size, size );
                }
            }
        }
        if ( display_border )
        {
            p.setPen(QColor(255,64,64));
            QPolygon pg(m_bsb->num_plys);
            for ( int r = 0; r < m_bsb->num_plys; r++ )
            {
                int xc, yc, x, y;
                bsb_LLtoXY( m_bsb, m_bsb->ply[r].lon, m_bsb->ply[r].lat, &xc, &yc );
                chartToScreen(xc, yc, &x, &y);
                pg.setPoint(r, x, y);
            }
            p.drawPolygon(pg);
        }
    }
    else
    {
        QFont font( "Helvetica", 12 );
        p.setFont( font );
        QFontMetrics fm = p.fontMetrics();
        //p.drawText( 10, 10+fm.ascent(), bsbFileName );
        p.drawText( 10, 10+2*fm.ascent(), "The chart did not load!");
    }
	//printf("dce\n");
}
