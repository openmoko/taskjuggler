/*
 * VacationList.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _VacationList_h_
#define _VacationList_h_

#include <qptrlist.h>

#include "VacationInterval.h"

class Interval;

/**
 * @short A list of vacations.
 * @author Chris Schlaeger <cs@kde.org>
 */
class VacationList : public QPtrList<VacationInterval>
{
public:
    VacationList() { setAutoDelete(TRUE); }
    virtual ~VacationList() {}

    typedef QPtrListIterator<VacationInterval> Iterator;

    void add(const QString& name, const Interval& i);
    void add(VacationInterval* vi);
    bool isVacation(time_t date) const;
    const QString& vacationName(time_t date) const;

protected:
    virtual int compareItems(QCollection::Item i1, QCollection::Item i2);
} ;

#endif
