/*
 * ktvcanvasitem.h - TaskJuggler Viewer
 *
 * Copyright (c) 2001, 2002 by Klaas Freitag <freitag@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */
#ifndef _KTVCANVASITEM_H
#define _KTVCANVASITEM_H

#include <qpoint.h>
#include <qcanvas.h>
#include <Project.h>
#include <Task.h>

#include "ktvconnector.h"

class QFont;

/*
 * The base. Contains most of the functionality all others need.
 */
class KTVCanvasItemBase: public QObject
{
public:
    KTVCanvasItemBase();
    virtual ~KTVCanvasItemBase();

    virtual void  setTask( Task *t ){ m_task = t; }
    Task* getTask()         { return m_task; }


    virtual int  y(){ return 0; }
    virtual int  x(){ return 0; }

    virtual void setSize( int, int );
    virtual void move( double, double ){}
    virtual void moveBy( int, int );
    virtual void show(){}
    virtual void hide();
    virtual bool contains( QCanvasItem* ){ return false; }
    virtual int  height( ) { return m_height; }
    virtual QRect rect()  { return QRect();  }
    virtual bool isVisible() { return false; };

    static void setFont( const QFont& f){ smFont=f; };

    void addConnectIn( KTVConnector*, Task* );
    void addConnectOut( KTVConnector*, Task* );
    KTVConnector* connectorIn( Task* );
    KTVConnector* connectorOut( Task* );

    virtual QPoint getConnectorIn() const {return QPoint(); }
    virtual QPoint getConnectorOut() const {return QPoint(); }

    virtual void moveInConnectors ( double, double );
    virtual void moveOutConnectors( double, double );
    virtual void moveConnectors   ( double, double );

    virtual void moveInConnectorsBy ( int, int );
    virtual void moveOutConnectorsBy( int, int );
    virtual void moveConnectorsBy   ( int, int );

    int               m_height;
protected:
    Task             *m_task;
    /* Connectorlists */
    KTVConnectorList  m_conIn;
    KTVConnectorList  m_conOut;
    static QFont smFont;
};

/*
 * List of Pointers to Canvas Items
 */
class CanvasItemList: public QPtrList<KTVCanvasItemBase>
{
public:
   CanvasItemList() {}
};

typedef QPtrListIterator<KTVCanvasItemBase> CanvasItemListIterator;

/*
 * Task Item
 */
class KTVCanvasItemTask: public KTVCanvasItemBase
{
public:
    KTVCanvasItemTask( QCanvas* );
    ~KTVCanvasItemTask();

    void setSize( int, int );
    void setComplete(double);
    void move(double, double );
    void moveBy( int, int );
    void hide();
    void show();
    bool contains( QCanvasItem* );

    void setTask( Task* );
    QRect rect() { return cRect->rect(); }

    int y();
    int x();
    bool isVisible() { return cRect->isVisible();};
    QPoint getConnectorIn() const;
    QPoint getConnectorOut() const;

    static void setFont( const QFont& f);

private:
    QCanvasRectangle *cRect;
    QCanvasText      *m_cText;
    QCanvasLine      *m_cmplLine;
    int m_TaskTextXOffset;
};

/*
 * Milestone-Item
 */
class KTVCanvasItemMilestone: public KTVCanvasItemBase
{
public:
    KTVCanvasItemMilestone( QCanvas* );
    ~KTVCanvasItemMilestone();
    void setSize( int, int );
    void move(double, double );
    void moveBy( int, int );
    void hide();
    void show();
    bool contains( QCanvasItem* );
    int y();
    int x();
    bool isVisible() { return cPoly->isVisible();};

    QPoint getConnectorOut() const;
    QPoint getConnectorIn() const;

    QRect rect() { return cPoly->boundingRect(); }

private:
    QCanvasPolygon *cPoly;

};

/*
 * Container-Item
 */
class  KTVCanvasItemContainer: public KTVCanvasItemBase
{
public:
    KTVCanvasItemContainer( QCanvas* );
    ~KTVCanvasItemContainer();
    void setSize( int, int );
    void move(double, double );
    void moveBy( int, int );
    void hide();
    void show();
    bool contains( QCanvasItem* );
    int y();
    int x();
    QRect rect() { return cPoly->boundingRect(); }
    bool isVisible() { return cPoly->isVisible();};

    QPoint getConnectorIn() const;
    QPoint getConnectorOut() const;

private:
    QCanvasPolygon    *cPoly;
};

#endif
