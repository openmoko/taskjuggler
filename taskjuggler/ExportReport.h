/*
 * ExportReport.h - TaskJuggler
 *
 * Copyright (c) 2002 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */
#ifndef _ExportReport_h_
#define _ExportReport_h_

#include <Report.h>

class Project;

class ExportReport : public Report
{
public:
	ExportReport(Project* p, const QString& f);
	virtual ~ExportReport() { }

	bool generate();

private:
	ExportReport() { }
};

#endif
