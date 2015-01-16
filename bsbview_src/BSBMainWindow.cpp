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
*  $Id: BSBMainWindow.cpp,v 1.1 2007/02/18 07:50:31 mikrom Exp $
        *
*/

#include <QScrollArea>
#include <QMainWindow>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QScrollBar>

#include "BSBScrollArea.h"
#include "BSBMainWindow.h"
        
BSBMainWindow::BSBMainWindow(QWidget* parent)
    : QMainWindow(parent), bsbsa(0)
{
    bsbsa = new BSBScrollArea(this);
    setCentralWidget(bsbsa);
    connect( bsbsa, SIGNAL(chartChanged()), this, SLOT(chartChanged()) );
}

void BSBMainWindow::setChartFile(const char* filename)
{
    if( bsbsa ) 
        bsbsa->setChartFile(filename);
}

void BSBMainWindow::chartChanged()
{
    setWindowTitle( bsbsa->chartName() );
}
