/*
 * Scenario.cpp - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include "Scenario.h"
#include "ScenarioList.h"
#include "Project.h"

Scenario::Scenario(Project* p, const QString& i, const QString& n,
				   Scenario* pr)
	: CoreAttributes(p, i, n, pr)
{
	p->addScenario(this);
}

Scenario::~Scenario()
{
}

ScenarioListIterator 
Scenario::getSubListIterator() const
{
	return ScenarioListIterator(sub);
}
