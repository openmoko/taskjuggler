/*
 * Report.cpp - TaskJuggler
 *
 * Copyright (c) 2001 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include <config.h>

#include "Project.h"
#include "Report.h"
#include "Utility.h"

ReportHtml::ReportHtml(Project* p, const QString& f, time_t s, time_t e) :
   Report(p, f, s, e )
{
	colDefault = 0xfffadd;
	colWeekend = 0xffec80;
	colVacation = 0xfffc60;
	colAvailable = 0xa4ff8d;
	colBooked = 0xffc0a3;
	colHeader = 0xa5c2ff;
	colMilestone = 0xff2a2a;
	colCompleted = 0xa1ff9a;
	colToday = 0xa387ff;
}

void
ReportHtml::reportHTMLHeader()
{
	s << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">" << endl
	  << "<! Generated by TaskJuggler v"VERSION">"
	  << "<html>" << endl
	  << "<head>" << endl
	  << "<title>Task Report</title>" << endl
	  << "<style type=\"text/css\"><!--" << endl;
	s.reset();
	s.setf(QTextStream::hex);
	s << ".default { background-color:#" << colDefault
	  << "; font-size:70%; text-align:center }" << endl
	  << ".task { background-color:#" << colDefault
	  << "; font-size:100%; text-align:left }" << endl
	  << ".available { background-color:#" << colAvailable
	  << "; font-size:70%; text-align:center }" << endl
	  << ".vacation { background-color:#" << colVacation
	  << "; font-size:70%; text-align:center }" << endl
	  << ".weekend { background-color:#" << colWeekend
	  << "; font-size:70%; text-align:center }" << endl
	  << ".milestone { background-color:#" << colMilestone
	  << "; font-size:70%; text-align:center }" << endl
	  << ".booked { background-color:#" << colBooked
	  << "; font-size:70%; text-align:center }" << endl
	  << ".headersmall { background-color:#" << colHeader
	  << "; font-size:70%; text-align:center }" << endl
	  << ".headerbig { background-color:#" << colHeader
	  << "; font-size:110%; font-weight:bold; text-align:center }" << endl
	  << ".completed { background-color:#" << colCompleted
	  << "; font-size:70%; text-align:center }" << endl
	  << ".today { background-color:#" << colToday
	  << "; font-size:70%; text-align:center }" << endl
	  << "--></style>" << endl
	  << "</head>" << endl
	  << "<body>" << endl;
}

void
ReportHtml::reportHTMLFooter()
{
	s << "<p><font size=\"-2\">"
	  << project->getCopyright()
	  << " - " << project->getVersion()
	  << " - TaskJuggler v"
	  << VERSION << "</font></p></body>\n";
}

void
ReportHtml::htmlDayHeaderDays()
{
	for (time_t day = midnight(start); day < end; day = sameTimeNextDay(day))
	{
		int dom = dayOfMonth(day);
		s << "<td class=\"" <<
			(isSameDay(project->getNow(), day) ? "today" :
			 isWeekend(day) ? "weekend" :
			 "headersmall")
		  << "\"><font size=\"-2\">&nbsp;";
		if (dom < 10)
			s << "&nbsp;";
		s << QString().sprintf("%d", dom) << "</font></td>";
	}
}

void
ReportHtml::htmlDayHeaderMonths()
{
	for (time_t day = midnight(start); day < end;
		 day = beginOfMonth(sameTimeNextMonth(day)))
	{
		int left = daysLeftInMonth(day);
		if (left > daysBetween(day, end))
			left = daysBetween(day, end);
		s << "<td class=\"headerbig\" colspan=\""
		  << QString().sprintf("%d", left)
		  << "\">"
		  << monthAndYear(day) << "</td>" << endl;
	}
}

void
ReportHtml::htmlWeekHeaderWeeks()
{
	for (time_t day = beginOfWeek(start); day < end;
		 day = sameTimeNextWeek(day))
	{
		int woy = weekOfYear(day);
		s << "<td class=\"" <<
			(isSameDay(project->getNow(), day) ? "today" :
			 "headersmall")
		  << "\"><font size=\"-2\">&nbsp;";
		if (woy < 10)
			s << "&nbsp;";
		s << QString().sprintf("%d", woy) << "</font></td>";
	}
}

void
ReportHtml::htmlWeekHeaderMonths()
{
	for (time_t week = beginOfWeek(start); week < end; )
	{
		int left = weeksLeftInMonth(week);
		if (left > weeksBetween(week, end))
			left = weeksBetween(week, end);
		s << "<td class=\"headerbig\" colspan=\""
		  << QString().sprintf("%d", left)
		  << "\">"
		  << monthAndYear(week) << "</td>" << endl;

		time_t newWeek = beginOfWeek(beginOfMonth(sameTimeNextMonth(week)));
		if (isSameMonth(newWeek, week))
			week = sameTimeNextWeek(newWeek);
		else
			week = newWeek;
	}
}

void
ReportHtml::htmlMonthHeaderMonths()
{
	for (time_t day = beginOfMonth(start); day < end;
		 day = sameTimeNextMonth(day))
	{
		int moy = monthOfYear(day);
		s << "<td class=\"" <<
			(isSameMonth(project->getNow(), day) ? "today" :
			 "headersmall")
		  << "\"><font size=\"-2\">&nbsp;";
		if (moy < 10)
			s << "&nbsp;";
		s << QString().sprintf("%d", moy) << "&nbsp;</font></td>";
	}
}

void
ReportHtml::htmlMonthHeaderYears()
{
	for (time_t day = midnight(start); day < end;
		 day = beginOfYear(sameTimeNextYear(day)))
	{
		int left = monthLeftInYear(day);
		if (left > monthsBetween(day, end))
			left = monthsBetween(day, end);
		s << "<td class=\"headerbig\" colspan=\""
		  << QString().sprintf("%d", left) << "\">"
		  << QString().sprintf("%d", year(day)) << "</td>" << endl;
	}
}

QString
ReportHtml::htmlFilter(const QString& s)
{
	QString out;
	bool parTags = FALSE;
	for (uint i = 0; i < s.length(); i++)
	{
		QString repl;
		if (s[i] == '&')
			repl = "&amp;";
		else if (s[i] == '<')
			repl = "&lt;";
		else if (s[i] == '>')
			repl = "&gt;";
		else if (s.mid(i, 2) == "\n\n")
		{
			repl = "</p><p>";
			parTags = TRUE;
			i++;
		}

		if (repl.isEmpty())
			out += s[i];
		else
			out += repl;
	}

	return parTags ? QString("<p>") + out + "</p>" : out;
}
