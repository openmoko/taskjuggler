/*
 * TaskDependency.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _TaskDependency_h_
#define _TaskDependency_h_

#include <qstring.h>

class Task;

/**
 * Besides a reference to the dependent task it also stores information about
 * a mininum required time gap to this task. The gap can be specified in
 * calendar time (duration) or working time (length). If more than one gap
 * criteria is specified all are honored.
 *
 * @short The class is used to store a dependency of a task.
 * @see Task
 * @author Chris Schlaeger <cs@suse.de>
 */
class TaskDependency
{
public:
    TaskDependency(QString tri, int maxScenarios) : taskRefId(tri)
    {
        gapDuration = new long[maxScenarios];
        gapLength = new long[maxScenarios];
        for (int sc = 0; sc < maxScenarios; ++sc)
            gapDuration[sc] = gapLength[sc] = 0;
    }
    ~TaskDependency()
    {
        delete [] gapDuration;
        delete [] gapLength;
    }

    const QString getTaskRefId() const { return taskRefId; }

    void setTaskRef(const Task* tr) { taskRef = tr; }
    const Task* getTaskRef() const { return taskRef; }

    void setGapDuration(int sc, long d) { gapDuration[sc] = d; }
    long getGapDuration(int sc) const { return gapDuration[sc]; }

    void setGapLength(int sc, long l) { gapLength[sc] = l; }
    long getGapLength(int sc) const { return gapLength[sc]; }

private:
    TaskDependency() { }    // Don't use this!
    /**
     * When we read in the project file, we don't know all functions yet. So
     * we need to store the ID of the dependency and later resolve this to the
     * pointer to the Task.
     */
    QString taskRefId;
    // The pointer to the dependent Task.
    const Task* taskRef;
    // Mininum required gap in calendar time.
    long* gapDuration;
    // Mininum required gap in working time.
    long* gapLength;
} ;

#endif

