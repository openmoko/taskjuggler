/*
 * ktvtasktable.h - TaskJuggler Viewer
 *
 * Copyright (c) 2001, 2002 by Klaas Freitag <freitag@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */
#ifndef _KTVTASKTABLE_H
#define _KTVTASKTABLE_H


#include <klistview.h>
#include <Project.h>
#include <Task.h>

#include <qpixmap.h>

#include "ktvtasktableitem.h"

#define COL_NAME 0
#define COL_ID 1
#define COL_PLAN_LEN 2
#define COL_PRIORITY 3
#define COL_COMPLETE 4
#define COL_PLAN_START_DATE 5
#define COL_PLAN_START_TIME 6

#define COL_PLAN_END_DATE 7
#define COL_PLAN_END_TIME 8

class QPixmap;
class KTVTaskCanvas;
class QListViewItem;

class KTVTaskTable: public KListView
{
   friend class KTVTaskCanvas;
   friend class KTVTaskCanvasView;
    Q_OBJECT
public:
    KTVTaskTable( QWidget *parent, const char *name = 0 );
    virtual ~KTVTaskTable(){}

   void showProject( Project * );
   void setItemHeight(int h);
   void setCanvasView( KTVTaskCanvasView* );
   
   
public slots:
   void resizeContents( int, int );
   
protected:
   virtual void wheelEvent( QWheelEvent * ) {};

protected slots:
   void slUpdateCanvas();
   void slExpanded( QListViewItem* );
   void slCollapsed( QListViewItem* );
   void slSelectionChanged( QListViewItem* );
   void slScrollTo( int, int );
   
signals:
   void itemHeightChanged(int);
   void topOffsetChanged(int);
   void newTaskAdded( Task*, KTVTaskTableItem* );
   void moveItems( int y, int dy);  // move items on canvas from int y by dy
   void showTaskByItem( KTVTaskTableItem* );
   void hideTaskByItem( KTVTaskTableItem* );
   void needUpdate();
   void moveMarker(int);
   void heightChanged( int );
   void scrolledBy( int, int );

private:
   void addTask( KTVTaskTableItem *parent, Task *t );
   QString beautyTimeSpan( time_t tStart, time_t tEnd ) const;
   int     m_itemHeight;

   KTVTaskTableItem *m_root;

   KTVTaskCanvasView *m_canvasView;
   
   QPixmap m_milestonePix;
   QPixmap m_taskPix;
   QPixmap m_containerPix;
   QPixmap m_menPix;
};


#endif

