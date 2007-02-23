/*
 * ResourceList.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _ResourceList_h_
#define _ResourceList_h_

#include "CoreAttributesList.h"

class QString;
class Resource;

/**
 * @short A list of resources.
 * @author Chris Schlaeger <cs@kde.org>
 */
class ResourceList : public CoreAttributesList
{
public:
    ResourceList();
    virtual ~ResourceList() { }

    Resource* getResource(const QString& id) const;

    static bool isSupportedSortingCriteria(int sc);

    virtual int compareItemsLevel(CoreAttributes* c1, CoreAttributes* c2,
                                  int level);
} ;

/**
 * @short Iterator class for ResourceList objects.
 * @author Chris Schlaeger <cs@kde.org>
 */
class ResourceListIterator : public virtual CoreAttributesListIterator
{
public:
    ResourceListIterator(const CoreAttributesList& l) :
        CoreAttributesListIterator(l)
    { }

    ~ResourceListIterator() { }

    Resource* operator*() { return static_cast<Resource*>(get()); }
} ;

#endif
