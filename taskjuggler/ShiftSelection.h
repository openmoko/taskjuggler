/*
 * ShiftSelection.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */
#ifndef _ShiftSelection_h_
#define _ShiftSelection_h_

#include <time.h>

#include "ShiftSelectionList.h"
#include "Shift.h"
#include "Interval.h"

/**
 * @short Stores shift selection related information.
 * @author Chris Schlaeger <cs@suse.de>
 */
class ShiftSelection
{
	friend int ShiftSelectionList::compareItems(QCollection::Item i1, 
												QCollection::Item i2);

public:
	ShiftSelection(const Interval& p, Shift* s) : period(p), shift(s) { }
	~ShiftSelection() { }

	const Interval& getPeriod() const { return period; }
	Shift* getShift() const { return shift; }

	bool isVacationDay(time_t day) const;

private:
	Interval period;
	Shift* shift;
};

#endif
