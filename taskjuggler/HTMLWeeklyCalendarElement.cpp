/*
 * HTMLWeeklyCalendarElement.cpp - TaskJuggler
 *
 * Copyright (c) 2002, 2003 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include "HTMLWeeklyCalendarElement.h"
#include "Project.h"
#include "Report.h"
#include "ExpressionTree.h"
#include "Operation.h"
#include "CoreAttributesList.h"
#include "Task.h"
#include "TaskList.h"
#include "Resource.h"
#include "ResourceList.h"
#include "Utility.h"
#include "Interval.h"
#include "TableColumnInfo.h"
#include "TableLineInfo.h"

HTMLWeeklyCalendarElement::HTMLWeeklyCalendarElement(Report* r, 
                                                     const QString& df,
                                                     int dl) :
    HTMLReportElement(r, df, dl)
{
    uint sc = r->getProject()->getMaxScenarios();
    columns.append(new TableColumnInfo(sc, "name"));

    // show all tasks
    setHideTask(new ExpressionTree(new Operation(0)));
    // hide all resources
    setHideResource(new ExpressionTree(new Operation(1)));

    taskSortCriteria[0] = CoreAttributesList::TreeMode;
    taskSortCriteria[1] = CoreAttributesList::StartUp;
    taskSortCriteria[2] = CoreAttributesList::EndUp;

    resourceSortCriteria[0] = CoreAttributesList::TreeMode;
    resourceSortCriteria[1] = CoreAttributesList::NameUp;
    resourceSortCriteria[2] = CoreAttributesList::IdUp;
}

HTMLWeeklyCalendarElement::~HTMLWeeklyCalendarElement()
{
}

void
HTMLWeeklyCalendarElement::generate()
{
    TaskList filteredTaskList;
    filterTaskList(filteredTaskList, 0, hideTask, rollUpTask);
    sortTaskList(filteredTaskList);
    maxDepthTaskList = filteredTaskList.maxDepth();

    ResourceList filteredResourceList;
    filterResourceList(filteredResourceList, 0, hideResource, rollUpResource);
    sortResourceList(filteredResourceList);
    maxDepthResourceList = filteredResourceList.maxDepth();
    
    bool weekStartsMonday = report->getProject()->getWeekStartsMonday();
    s() << "<table align=\"center\" cellpadding=\"1\">\n" << endl;
    for (time_t week = beginOfWeek(start, weekStartsMonday);
         week <= sameTimeNextWeek(beginOfWeek(end, weekStartsMonday)); )
    {
        time_t wd = week;
        /* Generate table row that contains the day of the month, the month
         * and the year. The row starts with a cell that shows the week number
         * of the first day of the week. */
        s() << "  <tr>" << endl;
        s() << "   <td width=\"5.5%\" class=\"headersmall\" "
            "style=\"font-size:110%\">"
            << "Week " << QString().sprintf("%d",
                                            weekOfYear(wd, weekStartsMonday))
            << "</td>" << endl;
        for (int day = 0; day < 7; ++day, wd = sameTimeNextDay(wd))
        {
            s() << "   <td width=\"13.5%\" class=\"";
            s() << (isSameDay(report->getProject()->getNow(), wd) ? "today" :
                    isWeekend(wd) ? "weekend" : "headersmall");
            s() << "\">" << endl
                << "<table width=\"100%\" style=\"text-align:center\">"
                << "  <tr>"
                << "   <td width=\"25%\" rowspan=\"2\" "
                   "style=\"font-size:280%; text-align:center\">" 
                << QString().sprintf("%d", dayOfMonth(wd)) << "</td>" << endl
                << "<td width=\"75%\" style=\"font-size:90%\">" 
                << htmlFilter(dayOfWeekName(wd)) << "</td>" << endl
                << "  </tr>"
                << "  <tr>"
                << "   <td style=\"font-size:100%\">" 
                << monthAndYear(wd) << "</td>"
                << "  </tr>"
                << "</table></td>" << endl;
        }
        s() << "  </tr>" << endl;

        if (!filteredTaskList.isEmpty())
        {
            // Generate a row with lists the tasks for each day.
            s() << "  <tr>" << endl
                << "    <td width=\"5.5%\" class=\"default\">&nbsp;</td>" 
                << endl;
            wd = week;
            for (int day = 0; day < 7; ++day, wd = sameTimeNextDay(wd))
            {
                /* Misuse the class member start and end to limit the scope of
                 * the information listed. */
                time_t savedStart = start;
                time_t savedEnd = end;
                start = wd;
                end = sameTimeNextDay(wd);
                s() << "   <td width=\"13.5%\" class=\"default\" "
                    "style=\"vertical-align:top\">" << endl;
                bool first = TRUE;
                int no = 1;
                for (TaskListIterator tli(filteredTaskList); *tli != 0;
                     ++tli, ++no)
                {
                    if ((*tli)->isActive(Task::Plan,
                                         Interval(wd, sameTimeNextDay(wd))))
                    {
                        if (first)
                        {
                            s() << "<table width=\"100%\" "
                                "style=\"font-size:150%\">" << endl;
                            first = FALSE;
                        }
                        TableLineInfo tli1;
                        tli1.bgCol = colors.getColor("default");
                        tli1.ca1 = tli1.task = *tli;
                        tli1.idxNo = no;
                        generateLine(&tli1, 2);
                    }
                }
                if (!first)
                    s() << "</table>" << endl;
                else
                    s() << "<p>&nbsp;</p>" << endl;   
                s() << "    </td>";
                start = savedStart;
                end = savedEnd;
            }
            s() << "  </tr>";
        }

        if (!filteredResourceList.isEmpty())
        {
            // Generate a table row which lists the resources for each day. 
            s() << "  <tr>" << endl
                << "   <td width=\"5.5%\" class=\"default\">&nbsp;</td>"
                << endl;
            wd = week;
            for (int day = 0; day < 7; ++day, wd = sameTimeNextDay(wd))
            {
                /* Misuse the class member start and end to limit the scope of
                 * the information listed. */
                time_t savedStart = start;
                time_t savedEnd = end;
                start = wd;
                end = sameTimeNextDay(wd);
                s() << "   <td width=\"13.5%\" class=\"default\" "
                    "style=\"vertical-align:top\">" << endl;
                bool first = TRUE;
                int no = 1;
                for (ResourceListIterator rli(filteredResourceList); 
                     *rli != 0; ++rli, ++no) 
                {
                    if ((*rli)->getLoad(Task::Plan,
                                        Interval(wd, 
                                                 sameTimeNextDay(wd))) > 0.0)
                    {
                        if (first)
                        {
                            s() << "<table width=\"100%\" "
                                "style=\"font-size:150%\">" << endl;
                            first = FALSE;
                        }
                        TableLineInfo tli2;
                        tli2.bgCol = colors.getColor("default");
                        tli2.ca1 = tli2.resource = *rli;
                        tli2.idxNo = no;
                        generateLine(&tli2, 4);
                    }
                }
                if (!first)
                    s() << "</table>" << endl;
                else
                    s() << "<p>&nbsp;</p>" << endl;   
                s() << "   </td>";
                start = savedStart;
                end = savedEnd;
            }
            s() << "  </tr>";
        }

        week = wd;  
    }

    s() << "</table>" << endl;
}
