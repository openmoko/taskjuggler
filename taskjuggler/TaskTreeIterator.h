/*
 * TaskTreeIterator.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _TaskTreeIterator_h_
#define _TaskTreeIterator_h_

#include "CoreAttributesTreeIterator.h"

class TaskTreeIterator : public virtual CoreAttributesTreeIterator
{
public:
	TaskTreeIterator(Task* r, CoreAttributesTreeIterator::IterationMode m =
					 CoreAttributesTreeIterator::leavesOnly)
	   	: CoreAttributesTreeIterator(r, m) { } 
	virtual ~TaskTreeIterator() { }

	Task* operator*() { return (Task*) current; }
	Task* operator++() 
	{
	   	return (Task*) CoreAttributesTreeIterator::operator++();
	}
} ;

#endif
