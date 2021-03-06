/*
 * The TaskJuggler Project Management Software
 *
 * Copyright (c) 2001, 2002, 2003, 2004, 2005 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _ManagedReportInfo_h_
#define _ManagedReportInfo_h_

#include <qstring.h>

class KListViewItem;
class ReportManager;
class TjUIReportBase;
class Report;

class ManagedReportInfo
{
public:
    ManagedReportInfo(ReportManager* rm, Report* const r);
    ~ManagedReportInfo();

    Report* getProjectReport() const { return projectReport; }

    const QString& getName() const;

    void setBrowserEntry(KListViewItem* lvi) { browserEntry = lvi; }
    KListViewItem* getBrowserEntry() const { return browserEntry; }

    void setReport(TjUIReportBase* r) { report = r; }
    TjUIReportBase* getReport() const { return report; }

    void setLoadingProject(bool lp);

    void print();

private:
    ManagedReportInfo() : projectReport(0) { }

    ReportManager* manager;
    Report* const projectReport;

    KListViewItem* browserEntry;
    TjUIReportBase* report;

    bool loadingProject;
} ;

#endif

