/*
 * Project.cpp - TaskJuggler
 *
 * Copyright (c) 2001 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include <stdio.h>

#include <qdict.h>

#include "Project.h"
#include "Utility.h"

Project::Project()
{
	taskList.setAutoDelete(TRUE);
	priority = 50;
	start = 0;
	end = MAXTIME;
	minEffort = 0.0;
	maxEffort = 1.0;
	rate = 0.0;
	/* The 'closed' flag may be used for container classes to hide all
	 * sub tasks. */
	addAllowedFlag("closed");
	// The 'hidden' flag may be used to hide the task in all reports.
	addAllowedFlag("hidden");
	htmlResourceReport = "";
	htmlTaskReport = "";
}

bool
Project::addTask(Task* t)
{
	taskList.append(t);
	return TRUE;
}

bool
Project::pass2()
{
	QDict<Task> idHash;
	bool error = FALSE;

	// Create hash to map task IDs to pointers.
	for (Task* t = taskList.first(); t != 0; t = taskList.next())
	{
		idHash.insert(t->getId(), t);
	}

	// Create cross links from dependency lists.
	for (Task* t = taskList.first(); t != 0; t = taskList.next())
	{
		if (!t->xRef(idHash))
			error = TRUE;
	}

	TaskList sortedTasks(taskList);
	sortedTasks.setSorting(TaskList::PrioDown);
	sortedTasks.sort();

	int uc = 0, puc = -1;
	while ((uc = unscheduledTasks()) > 0 && uc != puc)
	{
		for (Task* t = sortedTasks.first(); t != 0; t = sortedTasks.next())
			t->schedule(start);
		puc = uc;
	}

	if (uc > 0 && uc == puc)
	{
		fprintf(stderr, "Can't schedule some tasks. Giving up!\n");
	}
	else
		checkSchedule();

	return error;
}

bool
Project::reportHTMLHeader(FILE* f)
{
	fprintf(f, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n"
			"<html>\n"
			"<head>\n"
			"<title>Task Report</title>\n"
			"</head>\n"
			"<body>\n");
	return TRUE;
}

bool
Project::reportHTMLFooter(FILE* f)
{
	fprintf(f, "</body>\n"
			"</html>\n");
	return TRUE;
}

bool
Project::reportHTMLTaskList()
{
	/* TODO: This is pretty much a quick hack. Using FILE is bad. latin1()
	 * does not work for non ASCII characters! */
	if (htmlTaskReport == "")
		return TRUE;

	FILE* f;
	if ((f = fopen(htmlTaskReport, "w")) == NULL)
	{
		fprintf(stderr, "Cannot open report file %s!\n",
				htmlTaskReport.latin1());
		return FALSE;
	}
	reportHTMLHeader(f);
	fprintf(f, "<table border=\"1\" cellpadding=\"3\">\n");
	for (QStringList::Iterator it = htmlTaskReportColumns.begin();
		 it != htmlTaskReportColumns.end(); ++it )
	{
		if (*it == "no")
			fprintf(f, "<th><h2>No.</h2></th>");
		else if (*it == "taskId")
			fprintf(f, "<th><h2>Task ID</h2></th>");
		else if (*it == "task")
			fprintf(f, "<th><h2>Task</h2></th>");
		else if (*it == "start")
			fprintf(f, "<th><h2>Start</h2></th>");
		else if (*it == "end")
			fprintf(f, "<th><h2>End</h2></th>");
		else if (*it == "minStart")
			fprintf(f, "<th><h2>Min. Start</h2></th>");
		else if (*it == "maxStart")
			fprintf(f, "<th><h2>Max. Start</h2></th>");
		else if (*it == "resources")
			fprintf(f, "<th><h2>Resources</h2></th>");
		else if (*it == "depends")
			fprintf(f, "<th><h2>Dependencies</h2></th>");
		else if (*it == "follows")
			fprintf(f, "<th><h2>Following Tasks</h2></th>");
		else if (*it == "note")
			fprintf(f, "<th><h2>Note</h2></th>");
		else
		{
			fprintf(stderr, "Unknown Column '%s' for HTML Task Report\n",
					(*it).latin1());
			return FALSE;
		}
	}

	int i = 1;
	QDict<int> idxDict;
	idxDict.setAutoDelete(TRUE);
	for (Task* t = taskList.first(); t != 0; t = taskList.next(), ++i)
		idxDict.insert(t->getId(), new int(i));

	i = 1;
	for (Task* t = taskList.first(); t != 0; t = taskList.next(), ++i)
	{
		if (t->hasFlag("hidden"))
			continue;

		fprintf(f, "<tr valign=\"center\">");
		for (QStringList::Iterator it = htmlTaskReportColumns.begin();
			 it != htmlTaskReportColumns.end(); ++it )
		{
			if (*it == "no")
				fprintf(f, "<td>%d.</td>", i);
			else if (*it == "taskId")
				fprintf(f, "<td nowrap>%s</td>", t->getId().latin1());
			else if (*it == "task")
			{
				int indent;
				Task* tp = t->getParent();
				QString spaces = "";
				for (indent = 0; tp != 0; ++indent)
				{
					spaces += "&nbsp;&nbsp;&nbsp;&nbsp;";
					tp = tp->getParent();
				}
				fprintf(f, "<td nowrap>%s<font size=\"%c%d\">%s</font></td>\n",
						spaces.latin1(), 2 - indent < 0 ? '-' : '+',
						2 - indent < 0 ? -(2 - indent) : 2 - indent,
						t->getName().latin1());
			}
			else if (*it == "start")
				fprintf(f, "<td %s>%s</td>\n",
						t->isStartOk() ? "" : "bgcolor=\"red\"",
						t->getStartISO().latin1());
			else if (*it == "end")
				fprintf(f, "<td %s>%s</td>\n",
						t->isEndOk() ? "" : "bgcolor=\"red\"",
						t->getEndISO().latin1());
			else if (*it == "minStart")
				fprintf(f, "<td>%s</td>\n",
						time2ISO(t->getMinStart()).latin1());
			else if (*it == "maxStart")
				fprintf(f, "<td>%s</td>\n",
						time2ISO(t->getMaxStart()).latin1());
			else if (*it == "resources")
			{
				fprintf(f, "<td><font size=\"-2\">");
				bool first = TRUE;
				for (Resource* r = t->firstBookedResource(); r != 0;
					 r = t->nextBookedResource())
					fprintf(f, "%s%s,", first ? "" : ", ",
							r->getName().latin1());
				fprintf(f, "</font></td>\n");
			}
			else if (*it == "depends")
			{
				fprintf(f, "<td><font size=\"-2\">");
				bool first = TRUE;
				for (Task* d = t->firstPrevious(); d != 0;
					 d = t->nextPrevious())
				{
					fprintf(f, "%s%d", first ? "" : ", ",
							*(idxDict[d->getId()]));
					first = FALSE;
				}
				fprintf(f, "</font></td>\n");
			}
			else if (*it == "follows")
			{
				fprintf(f, "<td><font size=\"-2\">");
				bool first = TRUE;
				for (Task* d = t->firstFollower(); d != 0;
					 d = t->nextFollower())
				{
					fprintf(f, "%s%d", first ? "" : ", ",
							*(idxDict[d->getId()]));
					first = FALSE;
				}
				fprintf(f, "</font></td>\n");
			}
			else if (*it == "note")
			{
				fprintf(f, "<td><font size=\"-2\">%s</font></td>\n",
						htmlFilter(t->getNote()).latin1());
			}
		}
		fprintf(f, "</tr>");
	}
	fprintf(f, "</table>");
	reportHTMLFooter(f);

	fclose(f);
	return TRUE;
}

bool
Project::reportHTMLResourceList()
{
	/* TODO: This is pretty much a quick hack. Using FILE is bad. latin1()
	 * does not work for non ASCII characters! */
	if (htmlResourceReport == "")
		return TRUE;
	FILE* f;
	if ((f = fopen(htmlResourceReport, "w")) == NULL)
	{
		fprintf(stderr, "Cannot open report file %s!\n",
				htmlResourceReport.latin1());
		return FALSE;
	}
	reportHTMLHeader(f);
	fprintf(f, "<table border=\"1\" cellpadding=\"3\">\n");
	const int oneDay = 60 * 60 * 24;
	fprintf(f, "<tr>");
	fprintf(f, "<td rowspan=\"2\"></td>");
	for (time_t day = htmlResourceReportStart; day < htmlResourceReportEnd;
		 day += oneDay * daysLeftInMonth(day))
	{
		int left = daysLeftInMonth(day);
		if (left > (htmlResourceReportEnd - day) / oneDay)
			left = (htmlResourceReportEnd - day) / oneDay;
		fprintf(f, "<td colspan=\"%d\" align=\"center\">%s</td>",
				left, monthAndYear(day));
	}
	fprintf(f, "</tr>\n");

	fprintf(f, "<tr>");
	for (time_t day = htmlResourceReportStart; day < htmlResourceReportEnd;
		 day += oneDay)
	{
		fprintf(f, "<td>%d</td>", dayOfMonth(day));
	}
	fprintf(f, "</tr>\n");

	for (Resource* r = resourceList.first(); r != 0;
		 r = resourceList.next())
	{
		fprintf(f, "<tr>");
		fprintf(f, "<td nowrap><b>%s</b></td>", r->getName().latin1());
		for (time_t day = htmlResourceReportStart; day < htmlResourceReportEnd;
			 day += oneDay)
		{
			double load = r->getLoadOnDay(day);
			QString s = "";
			if (load > 0.0)
				s.sprintf("%3.1f", load);
			fprintf(f, "<td %s>%s</td>",
					isWeekend(day) ? "bgcolor=\"yellow\"" : "",
					s.latin1());
		}
		fprintf(f, "</tr>\n");

		for (Task* t = taskList.first(); t != 0; t = taskList.next())
			if (r->isBusyWith(t))
			{
				fprintf(f, "<tr>");
				fprintf(f, "<td nowrap>&nbsp;&nbsp;&nbsp;&nbsp;<font size=\"-1\">%s</font></td>",
						t->getName().latin1());
				for (time_t day = htmlResourceReportStart;
					 day < htmlResourceReportEnd; day += oneDay)
				{
					double load = r->getLoadOnDay(day, t);
					QString s = "";
					if (load > 0.0)
						s.sprintf("%3.1f", load);
					fprintf(f, "<td %s><font size=\"-1\">%s</font></td>",
							isWeekend(day) ? "bgcolor=\"yellow\"" : "",
							s.latin1());
				}
				fprintf(f, "</tr>\n");
			}
	}

	fprintf(f, "</table>");
	reportHTMLFooter(f);

	fclose(f);
	return TRUE;
}

void
Project::printText()
{
	printf("ID  Task Name    Start      End\n");
	int i = 0;
	for (Task* t = taskList.first(); t != 0; t = taskList.next(), ++i)
	{
		printf("%2d. %-12s %10s %10s\n",
			   i, t->getName().latin1(),
			   t->getStartISO().latin1(), t->getEndISO().latin1());
	}

	printf("\n\n");
	resourceList.printText();
}

int
Project::unscheduledTasks()
{
	int cntr = 0;
	for (Task* t = taskList.first(); t != 0; t = taskList.next())
		if (!t->isScheduled())
			cntr++;

	return cntr;
}

bool
Project::checkSchedule()
{
	for (Task* t = taskList.first(); t != 0; t = taskList.next())
		if (!t->scheduleOK())
			return FALSE;

	return TRUE;
}

QString
Project::htmlFilter(const QString& s)
{
	// TODO: All special characters must be html-ized.
	return s;
}
