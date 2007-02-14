/*
 * TaskList.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */
#ifndef _TaskList_h_
#define _TaskList_h_

#include "CoreAttributesList.h"

class QString;
class Task;

/**
 * @short The class stores a list of tasks.
 * @see Task
 * @author Chris Schlaeger <cs@kde.org>
 */
class TaskList : public virtual CoreAttributesList
{
public:
    TaskList()
    {
        sorting[0] = CoreAttributesList::TreeMode;
        sorting[1] = CoreAttributesList::StartUp;
        sorting[2] = CoreAttributesList::EndUp;
        // By default we are always using the first scenario for sorting.
        sortScenario = 0;
    }
    TaskList(const TaskList& tl) : CoreAttributesList(tl)
    {
        sortScenario = tl.sortScenario;
    }
    virtual ~TaskList() { }

    Task* getTask(const QString& id) const;

    static bool isSupportedSortingCriteria(int sc);

    virtual int compareItemsLevel(CoreAttributes* c1, CoreAttributes* c2,
                                  int level);

    void setSortScenario(int sc) { sortScenario = sc; }

protected:
    int sortScenario;
} ;

/**
 * @short Iterator class for TaskList objects.
 * @see TaskList
 * @author Chris Schlaeger <cs@kde.org>
 */
class TaskListIterator : public virtual CoreAttributesListIterator
{
public:
    TaskListIterator(const CoreAttributesList& l) :
        CoreAttributesListIterator(l) { }
    virtual ~TaskListIterator() { }
    Task* operator*() { return (Task*) get(); }
} ;

#endif

