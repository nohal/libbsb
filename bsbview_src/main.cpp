/*
*  main.cpp	- implementation of main() for qchart a marine BSB chart viewer
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
*  $Id: main.cpp,v 1.1 2007/02/18 07:50:31 mikrom Exp $
*
*/

#include <unistd.h>

#include <QApplication>
#include <BSBMainWindow.h>

//
// Create and display a BSBWidget.
//
int main( int argc, char **argv )
{
    QApplication a( argc, argv );

    BSBMainWindow *mw = new BSBMainWindow();

    //bsbsa.setCaption("QCharts Viewer");
    char *startchart = (char*)"/home/m/BSB_ROOT/World.KAP";
    //char *startchart = (char*)"d:\\home\\BSB_ROOT\\World.KAP";

    //extern char *optarg;
    extern int optind, optopt;
    bool test = false;
    bool verbose = false;
    bool quit = false;
    int c;
    while ((c = getopt(argc, argv, ":tvq")) != -1) {
        switch (c) {
            case 't': test = true; break;
            case 'v': verbose = true; break;
            case 'q': quit = true; break;
            default : printf("ignoring option %c\n", optopt); break;
        }
    }
    // see if there is any file name
    for ( ; optind < argc; optind++) {
        startchart = argv[optind];
    }
    if( startchart )
    {
        mw->setChartFile(startchart);
    }
    if( !quit )
    {
        mw->resize(640,480);
        mw->show();
        a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
        return a.exec();
    }
   }
