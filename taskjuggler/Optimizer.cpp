/*
 * Optimizer.cpp - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include "Optimizer.h"
#include "DecisionNode.h"
#include "OptimizerRun.h"

Optimizer::Optimizer()
{
    runs.setAutoDelete(TRUE);
    decisionTree = 0;
    minimize = TRUE;
}

Optimizer::~Optimizer()
{
    delete decisionTree;
}

OptimizerRun*
Optimizer::startNewRun()
{
    OptimizerRun* run = new OptimizerRun(this);
    runs.append(run);

    return run;
}

void
Optimizer::finishRun(OptimizerRun* run)
{
    runs.remove(run);
}

