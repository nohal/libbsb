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
*  $Id: BSBWidget.cpp,v 1.1 2007/02/06 16:05:25 mikrom Exp $
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <bsb.h>

#include <qwidget.h>
#include <qpainter.h>
#include <qimage.h>
#include <qapplication.h>
#include <qfiledialog.h>
#include <qcursor.h>
#include <qpopupmenu.h>
#include <qprogressdialog.h>
#include <qmessagebox.h>

// set preload to 1 if you want the entire chart to be preloaded (faster)
// note: this takes gobs of memory as chart rasters can be tens MB
#define PRELOAD 0

#include "BSBWidget.h"

/**
 * Constructs a BSBWidget.
 */
BSBWidget::BSBWidget( QWidget *parent, const char *name )
: QWidget( parent, name, WNorthWestGravity|WNoAutoErase ),
    mseDown(false),
    bsbFileName(""),
    bsbLoaded(false),
    bsbBuf(0),
    xc_ofs(0),
    yc_ofs(0),
    zoom(1),
    display_border(false),
    display_refs(false)
{
    cursor.setShape( Qt::CrossCursor );
    setCursor( cursor );
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
BSBWidget::setChartFile(const char* filename)
{
    if ( bsbLoaded )
    {
        bsb_close( &bsb );
        bsbLoaded = false;
        free((void*)bsbFileName);
    }
    bsbFileName = strdup(filename);
    if ( bsb_open_header((char*)filename, &bsb) )
    {
        for ( int i=0; i<bsb.num_colors; i++ )           // init color array
            colors[i] = QColor( bsb.red[i], bsb.green[i], bsb.blue[i] ).pixel();
        xc_ofs = 0;
        yc_ofs = 0;
        zoom = MAXZOOM;
        bsbLoaded = true;
        QString cap("QCharts: ");
        cap += bsb.name;
        setCaption(cap);
    }
    else
    {
        printf("Failed to load %s\n",bsbFileName);
    }
    delete[] bsbBuf;
    bsbBuf = 0;
    if ( bsbLoaded )
    {
#if PRELOAD
        QProgressDialog prog(this);
        prog.setCaption("Loading chart...");
        prog.setTotalSteps(100);
        bsbBuf = new uint8_t[bsb.height*bsb.width];
        //if( bsb_seek_to_row(&bsb, 0) )
        int prg= 0;
        for ( int y = 0; y <  bsb.height; y++ )
        {
            if ( y*100/bsb.height-prg > 0 )
            {
                prg = y*100/bsb.height;
                prog.setProgress( prg );
            }
            qApp->processEvents();
            {
                //bsb_read_row(&bsb, bsbBuf+y*bsb.width);
                bsb_read_row_at(&bsb, y, bsbBuf+y*bsb.width);
            }
        }
#else
        bsbBuf = new uint8_t[bsb.width];
#endif
        zoomToFitChart();
    }
}

/**
 * test the Geo Transform using BSB reference points
 *
 * @param verbose set to true for point-by-point print
 */
void
BSBWidget::testGeoTransform(bool verbose)
{
    if ( bsbLoaded )
    {
        printf("%s: NA=%s PR=%s GD=%s\nrefs=%d plys=%d wpx=%d/%d wpy=%d/%d pwx=%d/%d pwy=%d/%d cph=%f ",
               bsbFileName,
               bsb.name, bsb.projection, bsb.datum,
               bsb.num_refs, bsb.num_plys,
               bsb.num_wpxs, bsb.wpx_level, bsb.num_wpys, bsb.wpy_level,
               bsb.num_pwxs, bsb.pwx_level, bsb.num_pwys, bsb.pwy_level,
               bsb.cph
              );
        if ( verbose ) printf("\n");
        int maxXerr = 0;
        int maxYerr = 0;
        double maxLatErr = 0;
        double maxLonErr = 0;
        for ( int r = 0; r < bsb.num_refs; r++ )
        {
            int xc, yc;
            bsb_LLtoXY( &bsb, bsb.ref[r].lon, bsb.ref[r].lat, &xc, &yc );
            int xerr = abs(bsb.ref[r].x-xc);
            int yerr = abs(bsb.ref[r].y-yc);
            if ( maxXerr < xerr ) maxXerr = xerr;
            if ( maxYerr < yerr ) maxYerr = yerr;
            double lat, lon;
            bsb_XYtoLL( &bsb, bsb.ref[r].x, bsb.ref[r].y, &lon, &lat );
            double latErr = fabs(bsb.ref[r].lat-lat);
            double lonErr = fabs(bsb.ref[r].lon-lon);
            if ( maxLonErr < lonErr ) maxLonErr = lonErr;
            if ( maxLatErr < latErr ) maxLatErr = latErr;
            if ( verbose )
            {
                printf("Ref#%3d (%8.3f,%8.3f} {%5d,%5d} --> ",
                       r+1, bsb.ref[r].lon, bsb.ref[r].lat, bsb.ref[r].x, bsb.ref[r].y );
                printf("{%5d,%5d} err {%3d,%3d} ", xc, yc,  bsb.ref[r].x - xc, bsb.ref[r].y - yc );
                printf("{%8.3f,%8.3f} err {%8.3f,%8.3f} ", lon, lat,  bsb.ref[r].lon - lon, bsb.ref[r].lat - lat );
                printf("\n");
            }
        }
        printf("err xy=%d/%d ll=%f/%f\n", maxXerr, maxYerr, maxLonErr, maxLatErr );
    }
}

/*
 * Open dialog and let the user to select the chart file
 */
void
BSBWidget::selectChartFile()
{
    QFileDialog fd;
    fd.addFilter("BSB Charts (*.KAP)");
    fd.setCaption("Select BSB chart file");
    fd.setDir("~/BSB_ROOT");
    if( fd.exec() == QDialog::Accepted )
    {
       QString qfn = fd.selectedFile();
       setChartFile( qfn.ascii() );
    }
}

/**
 * Adjust the zoom to fit the chart in the current window
 */
void
BSBWidget::zoomToFitChart()
{
    int w = width();
    int h = height();
    int wc = bsb.width;
    int hc = bsb.height;
    int zw = (wc+w/2)/w;
    int zh = (hc+h/2)/h;
    zoom = zw > zh ? zw : zh;
    if ( zoom < 1 ) zoom = 1;
    if ( zoom > MAXZOOM ) zoom = MAXZOOM;
}

/**
 * Returns the pointer to memory buffer with given image row.
 * If no preload is selected than the buffer if always the same
 * but the function reads the specified row into it first.
 *
 * @param row row number to get the pointer to
 */
uint8_t* BSBWidget::getRow(int row)
{
    uint8_t* p = 0;
    if ( row >=0 && row < bsb.height )
    {
#if PRELOAD
        p = bsbBuf+row*bsb.width;
#else
        //if( bsb_seek_to_row(&bsb, row) )
        {
            bsb_read_row_at(&bsb, row, bsbBuf);
            p = bsbBuf;
        }
#endif
    }
    return p;
}

/**
 * Converts screen x,y to chart xc,yc
 *
 * @param x screen X coordinate
 * @param y screen Y coordinate
 * @param xc output chart X coordinate
 * @param yc output chart Y coordinate
 */
void BSBWidget::screenToChart(int x, int y, int* xc, int* yc)
{
    // relative to origin of the screen and zoomed
    int xz=x*zoom;
    int yz=y*zoom;
    // absolute to the origin in chart's units
    *xc = xc_ofs+xz;
    *yc = yc_ofs+yz;
}

/**
 * Converts screen x,y to chart Lat/Lon and formats for output
 *
 * @param x screen X coordinate
 * @param y screen Y coordinate
 * @param s output text buffer
 */
void BSBWidget::screenToLLStr(int x, int y, char* s)
{
    double lat,lon;
    int xc,yc;
    screenToChart(x, y, &xc, &yc);
    if( bsb_XYtoLL(&bsb, xc, yc, &lon, &lat) )
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
 * Converts chart xc, yc to screen x,y
 *
 * @param xc chart X coordinate
 * @param yc chart Y coordinate
 * @param x output screen X coordinate
 * @param y output screen Y coordinate
 */
void BSBWidget::chartToScreen(int xc, int yc, int* x, int* y)
{
    // relative to the chart's display offset
    xc -= xc_ofs;
    yc -= yc_ofs;
    *x = xc / zoom;
    *y = yc / zoom;
}

/**
 * Handles paint events for the connect widget.
 */
void BSBWidget::paintEvent( QPaintEvent * )
{
    QPainter paint( this );
    // do we have anything to print?
    if ( bsbLoaded )
    {
        int ws = width();
        int hs = height();
        QImage img( ws, hs, 8 );
        img.setNumColors( bsb.num_colors );
        for ( int col = 0; col < bsb.num_colors; col++ )
        {
            img.setColor( col, qRgb( bsb.red[col], bsb.green[col], bsb.blue[col] ) );
        }

#if 0 /* keep max of chart visible - limit the chart offset */
        // chart visible width & height
        int wz = ws*zoom;
        int hz = hs*zoom;
        // correct chart center
        if ( x_cent - wz/2 < 0 )
            x_cent = wz < bsb.width ? wz/2 : bsb.width/2;
        if ( x_cent + wz/2 > bsb.width )
            x_cent = wz < bsb.width ? bsb.width-wz/2 : bsb.width/2;
        if ( y_cent - hz/2 < 0 )
            y_cent = hz < bsb.height ? hz/2 : bsb.height/2;
        if ( y_cent + hz/2 > bsb.height )
            y_cent = hz < bsb.width ? bsb.height-hz/2 : bsb.height/2;
#endif
        // chart unit offsets
        int xo = xc_ofs;
        int yo = yc_ofs;
#define FILTER 0 /* simple average filter for filtering down */
#if FILTER
        // screen offsets and coordinates
        for ( int y = 0; y < hs; y++ )
        {
            for ( int x = 0; x < ws; x++ )
            {
                int r = 0, g = 0, b = 0;
                int yy = yo+y*zoom;
                for ( int yc = yy ; yc < yy+zoom; yc ++ )
                {
                    uint8_t* row = getRow(yc);
                    for ( int xc = xo+x*zoom; row && (xc < xo+x*zoom+zoom); xc ++ )
                    {
                        int c = xc >=0 && xc < bsb.width ? row[xc] : 0;
                        r += bsb.red[c];
                        g += bsb.green[c];
                        b += bsb.blue[c];
                    }
                }
                img.setPixel( x, y, QColor(r/zoom,g/zoom,b/zoom).pixel() );
            }
        }
#else
        // screen offsets and coordinates
        for ( int y = 0; y < hs; y++ )
        {
            int yc = yo+y*zoom;
            uint8_t* row = getRow(yc);
            uchar* ppix = img.scanLine(y);
            for ( int x = 0; x < ws; x++ )
            {
                int xc = xo+x*zoom;
//#define HORFILTER
#ifdef HORFILTER
                int col = 0;
                for ( int i = 0; i < zoom; i++, xc++ )
                    col += row && xc >=0 && xc < bsb.width ? row[xc] : 0;
                col /= zoom;
#else
                int col = row && xc >=0 && xc < bsb.width ? row[xc] : 0;
#endif
                *ppix++ = col;
                //img.setPixel( x, y, col );
            }
        }
#endif
        paint.drawImage( 0, 0, img );
        if ( display_refs )
        {
            for ( int r = 0; r < bsb.num_refs; r++ )
            {
                const int size = 10;

                int xc, yc;
                int x,y;
                chartToScreen( bsb.ref[r].x, bsb.ref[r].y, &x, &y );
                //printf( "(%f,%f) -> (%d,%d) -> (%d,%d)\n",
                //            bsb.ref[r].lon, bsb.ref[r].lat, xc, yc, x, y );
                if ( x >= 0 && x < width() && y >= 0 && y < height() )
                {
                    paint.setPen(QColor(64,255,64));
                    paint.drawRect( x-size/2, y-size/2, size, size );
                }

                bsb_LLtoXY( &bsb, bsb.ref[r].lon, bsb.ref[r].lat, &xc, &yc );
                chartToScreen( xc, yc, &x, &y );
                //printf( "(%f,%f) -> (%d,%d) -> (%d,%d)\n",
                //            bsb.ref[r].lon, bsb.ref[r].lat, xc, yc, x, y );
                if ( x >= 0 && x < width() && y >= 0 && y < height() )
                {
                    paint.setPen(QColor(255,64,64));
                    paint.drawRect( x-size/2, y-size/2, size, size );
                }
            }
        }
        if ( display_border )
        {
            paint.setPen(QColor(255,64,64));
            QPointArray pa(bsb.num_plys);
            for ( int r = 0; r < bsb.num_plys; r++ )
            {
                int xc, yc;
                bsb_LLtoXY( &bsb, bsb.ply[r].lon, bsb.ply[r].lat, &xc, &yc );
                int x,y;
                chartToScreen( xc, yc, &x, &y );
                pa.setPoint(r,x,y);
            }
            paint.drawPolygon(pa);
        }

    }
    else
    {
        QFont font( "Helvetica", 12 );
        paint.setFont( font );
        QFontMetrics fm = paint.fontMetrics();
        paint.drawText( 10, 10+fm.ascent(), bsbFileName );
        paint.drawText( 10, 10+2*fm.ascent(), "The chart did not load!");
    }
}

//
// Handles mouse press events for the connect widget.
//
void BSBWidget::mousePressEvent( QMouseEvent *e )
{
    msedwntme.start();
    msedwnpos = e->pos();
    x_dwn = xc_ofs;
    y_dwn = yc_ofs;
    switch ( e->button() )
    {
    case Qt::LeftButton : {
            cursor.setShape( Qt::SizeAllCursor );
            setCursor( cursor );
            mseDown = TRUE;
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
 * Handles mouse double click press events for the connect widget.
 * Here execute ZOOM IN action at the click point.
 */
void BSBWidget::mouseDoubleClickEvent ( QMouseEvent * e )
{
    doAction(A_ZOOMIN, e->x(), e->y() );
}

/**
 * Show context menu at given point.
 *
 * @param p clicked point to show the menu at
 */
void
BSBWidget::doContextMenu( QPoint p )
{
    char s[100];
    screenToLLStr( p.x(), p.y(), s );

    QPopupMenu pup(this);
    pup.setCheckable(true);
    pup.setCaption("Actions");
    pup.insertItem("Zoom In",A_ZOOMIN);
    pup.insertItem("Zoom Out",A_ZOOMOUT);
    pup.insertItem("Full zoom",A_ZOOMMAX);
    pup.insertItem("Show all",A_ZOOMMIN);
    pup.insertItem("Center",A_CENTER);
    pup.insertSeparator();
    pup.insertItem("Open chart",A_OPEN);
    pup.insertItem("Chart info",A_CHARTINFO);
    pup.insertItem("Display border",A_DISPBORDER);
    pup.setItemChecked(A_DISPBORDER, display_border);
    pup.insertItem("Display refs",A_DISPREFS);
    pup.setItemChecked(A_DISPREFS, display_refs);
    pup.insertSeparator();
    pup.insertItem("Toggle Full Screen",A_FULLSCREEN);
    pup.insertItem("About",A_ABOUT);
    pup.insertItem("Exit",A_EXIT);
    pup.insertSeparator();
    pup.insertItem(s);

    doAction((Action)pup.exec(mapToGlobal(p)),p.x(),p.y());
}

/**
 * Handles main UI actions.
 *
 * @param action the action to perform
 * @param par1 first parameter (action specific)
 * @param par2 second parameter (action specific)
 */
void BSBWidget::doAction(enum Action action, int par1, int par2)
{
    int xc,yc;
    screenToChart( par1, par2, &xc, &yc );
    bool applyShift = false;

    switch ( action )
    {
    case A_ZOOMIN:
        {
            zoom/=2;
            if (zoom<1) zoom=1;
            applyShift = true;
            break;
        }
    case A_ZOOMOUT:
        {
            zoom*=2;
            if (zoom>MAXZOOM) zoom=MAXZOOM;
            applyShift = true;
            break;
        }
    case A_ZOOMMAX:
        {
            zoom=1;
            applyShift = true;
            break;
        }
    case A_ZOOMMIN:
        zoomToFitChart(); /* continue to CENTER */
    case A_CENTER:
        {
            xc = bsb.width/2;
            yc = bsb.height/2;
            applyShift = true;
            break;
        }
    case A_FULLSCREEN:
        if ( isFullScreen() )
            showNormal();
        else
            showFullScreen();
        break;
    case A_OPEN:
        selectChartFile();
        update();
        break;
    case A_ABOUT:
        QMessageBox::information(this,
                                 "About QCharts",
                                 "QCharts written by Michal Krombholz (C)2006"
                                 "<p>With help from libbsb code by Stuart Cunningham"
                                 "<p>Use popup menu for commands."
                                 "<p>Keys:<br>"
        			 "Z+ zoom in<br>"
        			 "X- zoom out<br>"
        			 "C center<br>"
        			 "A fit to screen<br>"
        			 "S 1:1 zoom<br>"
        			 "F toggle full screen<br>"
        			 "<p> also use:<br>"
        			 "- left mouse button for panning (grab the chart)<br>"
        			 "- right button for mouse gestures:<br>"
        			 "up to zoom into an area<br>"
        			 "down to zoom out<br>"
                                 , QMessageBox::Ok);
        break;
    case A_CHARTINFO:
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
                    bsb.name, bsb.version,
                    bsb.projection, bsb.datum,
                    bsb.scale, bsb.projectionparam,
                    bsb.xresolution, bsb.yresolution,
                    bsb.num_refs, bsb.num_plys,
                    bsb.num_wpxs, bsb.num_wpys, bsb.num_pwxs, bsb.num_pwys,
                    bsb.wpx_level, bsb.wpy_level, bsb.pwx_level, bsb.pwy_level, bsb.cph );
            QMessageBox::information(this, "About This Chart", txt, QMessageBox::Ok);
        }
        break;
    case A_DISPBORDER:
        display_border = !display_border;
        update();
        break;
    case A_DISPREFS:
        display_refs = !display_refs;
        update();
        break;
    case A_EXIT:
        QApplication::exit( 0 );
        break;
    }
    // apply image shift (if any) and redraw the image
    if ( applyShift )
    {
        xc_ofs = xc-width()*zoom/2;
        yc_ofs = yc-height()*zoom/2;
        update();
    }
}

/**
 * Handles mouse release events for the connect widget.
 * The code is smart on right click:
 * - if it was a short click and mouse did not move much then it's a popup menu
 * - if the time was longer and mouse move then it is a gesture
 */
void BSBWidget::mouseReleaseEvent( QMouseEvent *e )
{
    cursor.setShape( Qt::CrossCursor );
    setCursor( cursor );
    QPoint mseuppos = e->pos();
    switch ( e->button() )
    {
    case Qt::LeftButton :
        if ( mseDown )
        {
            mseDown = false;
            xc_ofs = x_dwn - (mseuppos.x()-msedwnpos.x())*zoom;
            yc_ofs = y_dwn - (mseuppos.y()-msedwnpos.y())*zoom;
            update();
        }
        break;
    case Qt::RightButton : {
            int dx = mseuppos.x()-msedwnpos.x();
            int dy = mseuppos.y()-msedwnpos.y();
            int adx = dx > 0 ? dx : -dx;
            int ady = dy > 0 ? dy : -dy;
            if ( msedwntme.elapsed() < 300 /* ms */ && adx < 10 && ady < 10 )
            {
                doContextMenu( msedwnpos );
            }
            else /* its a gesture - up (zoom in) or down (zoom out) */
            {
                int xc,yc;
                screenToChart( mseuppos.x()-dx/2, mseuppos.y()-dy/2, &xc, &yc );
                if ( dy && dx/dy == 0 )
                    zoom = (dy>0) ? zoom*2 : zoom/2;
                else if ( dx )
                    zoom = dx>0 ? (dx*zoom)/width() : zoom*2;
                if ( zoom > MAXZOOM ) zoom = MAXZOOM;
                if ( zoom < 1 ) zoom = 1;
                xc_ofs = xc - width()*zoom/2;
                yc_ofs = yc - height()*zoom/2;
                update();
            }
        }
        break;
    default:
        break;
    }
}

/**
 * Handles mouse move events for the widget.
 */
void BSBWidget::mouseMoveEvent( QMouseEvent *e )
{
    if ( mseDown )
    {
        QPoint mseuppos = e->pos();
        xc_ofs = x_dwn - (mseuppos.x()-msedwnpos.x())*zoom;
        yc_ofs = y_dwn - (mseuppos.y()-msedwnpos.y())*zoom;
        update();                                   // draw the lines
    }
}

/**
 * Handles key events for the widget.
 */
void BSBWidget::keyPressEvent( QKeyEvent *e )
{
    if ( !(e->state() & Qt::KeyButtonMask) )
    {
        Action action;
        int x = width()/2;
        int y = height()/2;
        switch ( e->ascii() )
        {
        case '+':
        case 'z':
        case 'Z': action = A_ZOOMIN; break;
        case '-':
        case 'x':
        case 'X': action = A_ZOOMOUT; break;
        case 'c':
        case 'C': action = A_CENTER; break;
        case 'a':
        case 'A': action = A_ZOOMMIN; break;
        case 's':
        case 'S': action = A_ZOOMMAX; break;
        case 'f':
        case 'F': action = A_FULLSCREEN; break;
        default: return;
        }
        doAction(action,x,y);
    }
}

