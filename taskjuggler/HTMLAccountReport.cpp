/*
 * HTMLAccountReport.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include <qfile.h>

#include "HTMLAccountReport.h"
#include "Account.h"
#include "Interval.h"
#include "Utility.h"

HTMLAccountReport::HTMLAccountReport(Project* p, const QString& f, time_t s,
							   time_t e) :
	ReportHtml(p, f, s, e)
{
	columns.append("no");
	columns.append("name");
	columns.append("total");

	accumulate = FALSE;
}

bool
HTMLAccountReport::generate()
{
	QFile f(fileName);
	if (!f.open(IO_WriteOnly))
	{
		qWarning("Cannot open report file %s!\n",
				 fileName.latin1());
		return FALSE;
	}
	s.setDevice(&f);
	reportHTMLHeader();
	s << "<table border=\"0\" cellpadding=\"1\">\n" << endl;

	if (!generateTableHeader())
		return FALSE;

	AccountList filteredList;
	filterAccountList(filteredList, Account::Cost);
	sortAccountList(filteredList);

	for (Account* a = filteredList.first(); a != 0; a = filteredList.next())
	{
		if (!hidePlan)
			generatePlanAccount(a);
		if (showActual)
			generateActualAccount(a);
	}
	generateTotals("Subtotal Costs", "headersmall");
	planTotalsCosts = planTotals;
	actualTotalsCosts = actualTotals;
	planTotals.clear();
	actualTotals.clear();

	filterAccountList(filteredList, Account::Revenue);
	sortAccountList(filteredList);

	for (Account* a = filteredList.first(); a != 0; a = filteredList.next())
	{
		if (!hidePlan)
			generatePlanAccount(a);
		if (showActual)
			generateActualAccount(a);
	}
	generateTotals("Subtotal Revenue", "headersmall");
	planTotalsRevenue = planTotals;
	actualTotalsRevenue = actualTotals;

	QMap<QString, double>::Iterator tc;
	QMap<QString, double>::Iterator rc;
	QMap<QString, double>::Iterator it;
	for (tc = planTotalsCosts.begin(),
			 rc = planTotalsRevenue.begin(),
			 it = planTotals.begin();
		 tc != planTotalsCosts.end(); ++tc, ++rc, ++it)
	{
		*it = *rc - *tc;
	}
	for (tc = actualTotalsCosts.begin(),
			 rc = actualTotalsRevenue.begin(),
			 it = actualTotals.begin();
		  tc != actualTotalsCosts.end(); ++tc, ++rc, ++it)
	{
		*it = *rc - *tc;
	}
	generateTotals("Total", "default");
		
	s << "</table>" << endl;
	reportHTMLFooter();

	f.close();
	return TRUE;
}

void
HTMLAccountReport::generatePlanAccount(Account* a)
{
	s << "<tr valign=\"center\">";
	for (QStringList::Iterator it = columns.begin(); it != columns.end();
		 ++it )
	{
		if (*it == "no")
			textTwoRows(QString().sprintf("%d.", a->getIndex()), FALSE, "");
		else if (*it == "id")
			textTwoRows(htmlFilter(a->getId()), FALSE, "left");
		else if (*it == "name")
			accountName(a);
		else if (*it == "total")
		{
			double value = a->getPlanVolume(Interval(start, end));
			planTotals["total"] += value;
			textOneRow(QString().sprintf("%1.3f", value), FALSE, "right");
		}
		else if (*it == "daily")
			dailyAccountPlan(a, "default");
		else if (*it == "weekly")
			weeklyAccountPlan(a, "default");
		else if (*it == "monthly")
			monthlyAccountPlan(a, "default");
		else
			qFatal("generatePlanAccount: Unknown Column %s",
				   (*it).latin1());
	}
	s << "</tr>" << endl;
}

void
HTMLAccountReport::generateActualAccount(Account* a)
{
	s << "<tr bgcolor=\"" << colAvailable << "\">" << endl;
	for (QStringList::Iterator it = columns.begin();
		 it != columns.end();
		 ++it )
	{
		if (*it == "no")
		{
			if (hidePlan)
				textOneRow(QString().sprintf("%d.", a->getIndex()), FALSE, "");
		}
		else if (*it == "id")
		{
			if (hidePlan)
				textOneRow(htmlFilter(a->getId()), FALSE, "left");
		}
		else if (*it == "name")
		{
			if (hidePlan)
				accountName(a);
		}
		else if (*it == "total")
		{
			double value = a->getActualVolume(Interval(start, end));
			actualTotals["total"] += value;
			textOneRow(QString().sprintf("%1.3f", value), FALSE, "right");
		}
		else if (*it == "daily")
			dailyAccountActual(a, "default");
		else if (*it == "weekly")
			weeklyAccountActual(a, "default");
		else if (*it == "monthly")
			monthlyAccountActual(a, "default");
	}
	s << "</tr>" << endl;
}

bool
HTMLAccountReport::generateTableHeader()
{
	// Header line 1
	s << "<tr>" << endl;
	for (QStringList::Iterator it = columns.begin(); it != columns.end();
		 ++it )
	{
		if (*it == "no")
			s << "<td class=\"headerbig\" rowspan=\"2\">No.</td>";
		else if (*it == "id")
			s << "<td class=\"headerbig\" rowspan=\"2\">ID</td>";
		else if (*it == "name")
			s << "<td class=\"headerbig\" rowspan=\"2\">Name</td>";
		else if (*it == "total")
			s << "<td class=\"headerbig\" rowspan=\"2\">Total</td>";
		else if (*it == "daily")
			htmlDayHeaderMonths();
		else if (*it == "weekly")
			htmlWeekHeaderMonths();
		else if (*it == "monthly")
			htmlMonthHeaderYears();
		else
		{
			qWarning("Unknown Column '%s' for HTML Task Report\n",
					(*it).latin1());
			return FALSE;
		}
	}
	s << "</tr>" << endl;

	// Header line 2
	s << "<tr>" << endl;
	for (QStringList::Iterator it = columns.begin(); it != columns.end();
		 ++it )
	{
		if (*it == "daily")
			htmlDayHeaderDays(FALSE);
		else if (*it == "weekly")
			htmlWeekHeaderWeeks(FALSE);
		else if (*it == "monthly")
			htmlMonthHeaderMonths(FALSE);
	}
	s << "</tr>\n" << endl;

	return TRUE;
}

void
HTMLAccountReport::generateTotals(const QString& label, const QString& style)
{
	// Total plan values
	if (!hidePlan)
	{
		s << "<tr>" << endl;
		for (QStringList::Iterator it = columns.begin(); it != columns.end();
			 ++it )
		{
			if (*it == "no")
				s << "<td class=\"" << style << "\" rowspan=\""
				  << (!hidePlan && showActual ? "2" : "1")
				  << "\">&nbsp;</td>";
			else if (*it == "id")
				s << "<td class=\"" << style << "\" rowspan=\""
				  << (!hidePlan && showActual ? "2" : "1")
				  << "\">&nbsp;</td>";
			else if (*it == "name")
				s << "<td class=\"" << style << "\" rowspan=\""
				  << (!hidePlan && showActual ? "2" : "1")
				  << "\" nowrap><b>" << label << "</b></td>";
		else if (*it == "total")
				s << "<td class=\"" << style
				  << "\" style=\"text-align:right\">"
				  << "<b>"
				  << QString().sprintf("%1.3f", planTotals["total"])
				  << "</b></td>";
			else if (*it == "daily")
				dailyAccountPlan(0, style);
			else if (*it == "weekly")
				weeklyAccountPlan(0, style);
			else if (*it == "monthly")
				monthlyAccountPlan(0, style);
		}
		s << "</tr>" << endl;
	}

	if (showActual)
	{
		// Total actual values
		s << "<tr>" << endl;
		for (QStringList::Iterator it = columns.begin(); it != columns.end();
			 ++it )
		{
			if (*it == "no")
			{
				if (hidePlan)
					s << "<td class=\"" << style << "\">&nbsp;</td>";
			}
			else if (*it == "id")
			{
				if (hidePlan)
					s << "<td class=\"" << style << "\">&nbsp;</td>";
			}
			else if (*it == "name")
			{
				if (hidePlan)
					s << "<td class=\"" << style << "\" nowrap><b>"
					  << label << "</b></td>";
			}
			else if (*it == "total")
				s << "<td class=\"" << style
				  << "\" style=\"text-align:right\">"
				  << "<b>"
				  << QString().sprintf("%1.3f", actualTotals["total"])
				  << "</b></td>";
			else if (*it == "daily")
				dailyAccountActual(0, style);
			else if (*it == "weekly")
				weeklyAccountActual(0, style);
			else if (*it == "monthly")
				monthlyAccountActual(0, style);
		}
		s << "</tr>\n" << endl;
	}
}

void 
HTMLAccountReport::dailyAccountPlan(Account* a, const QString& style)
{
	if (hidePlan)
		return;

	if (showActual)
	{
		s << "<td class=\"headersmall\">";
		if (!a)
			s << "<b>";
		s << "Plan";
		if (!a)
			s << "</b>";
		s << "</td>" << endl;
	}
	for (time_t day = midnight(start); day < end;
		 day = sameTimeNextDay(day))
	{
		if (a)
		{
			double volume = a->getPlanVolume(Interval(day).firstDay());
			planTotals[time2ISO(day)] += volume;
			reportValue(volume, style, FALSE);
		}
		else
			reportValue(planTotals[time2ISO(day)], style, TRUE);
	}
}

void
HTMLAccountReport::dailyAccountActual(Account* a, const QString& style)
{
	if (!hidePlan)
	{
		s << "<td class=\"headersmall\">";
		if (!a)
			s << "<b>";
		s << "Actual";
		if (!a)
			s << "</b>";
		s << "</td>" << endl;
	}
	for (time_t day = midnight(start); day < end;
		 day = sameTimeNextDay(day))
	{
		if (a)
		{
			double volume = a->getActualVolume(Interval(day).firstDay());
			actualTotals[time2ISO(day)] += volume;
			reportValue(volume, "style", FALSE);
		}
		else
			reportValue(actualTotals[time2ISO(day)], style, TRUE);
	}
}

void 
HTMLAccountReport::weeklyAccountPlan(Account* a, const QString& style)
{
	if (hidePlan)
		return;

	if (showActual)
	{
		s << "<td class=\"headersmall\">";
		if (!a)
			s << "<b>";
		s << "Plan";
		if (!a)
			s << "</b>";
		s << "</td>" << endl;
	}
	for (time_t week = beginOfWeek(start); week < end;
		 week = sameTimeNextWeek(week))
	{
		if (a)
		{
			double volume = a->getPlanVolume(
				accumulate ? 
				Interval(start, sameTimeNextWeek(week) - 1) :
				Interval(week).firstWeek());
			planTotals[time2ISO(week)] += volume;
			reportValue(volume, style, FALSE);
		}
		else
			reportValue(planTotals[time2ISO(week)], style, TRUE);
	}
}

void
HTMLAccountReport::weeklyAccountActual(Account* a, const QString& style)
{
	if (!hidePlan)
	{
		s << "<td class=\"headersmall\">";
		if (!a)
			s << "<b>";
		s << "Actual";
		if (!a)
			s << "</b>";
		s << "</td>" << endl;
	}
	for (time_t week = beginOfWeek(start); week < end;
		 week = sameTimeNextWeek(week))
	{
		if (a)
		{
			double volume = a->getActualVolume(
				accumulate ?
				Interval(start, sameTimeNextWeek(week) - 1) :
				Interval(week).firstWeek());
			actualTotals[time2ISO(week)] += volume;
			reportValue(volume, style, FALSE);
		}
		else
			reportValue(actualTotals[time2ISO(week)], style, TRUE);
	}
}

void
HTMLAccountReport::monthlyAccountPlan(Account* a, const QString& style)
{
	if (hidePlan)
		return;

	if (showActual)
	{
		s << "<td class=\"headersmall\">";
		if (!a)
			s << "<b>";
		s << "Plan";
		if (!a)
			s << "</b>";
		s << "</td>" << endl;
	}
	for (time_t month = beginOfMonth(start); month < end;
		 month = sameTimeNextMonth(month))
	{
		if (a)
		{
			double volume = a->getPlanVolume(
				accumulate ?
				Interval(start, sameTimeNextMonth(month) - 1) :
				Interval(month).firstMonth());
			planTotals[time2ISO(month)] += volume;
			reportValue(volume, style, FALSE);
		}
		else
			reportValue(planTotals[time2ISO(month)], style, TRUE);
	}
}

void
HTMLAccountReport::monthlyAccountActual(Account* a, const QString& style)
{
	if (!hidePlan)
	{
		s << "<td class=\"headersmall\">";
		if (!a)
			s << "<b>";
		s << "Actual";
		if (!a)
			s << "</b>";
		s << "</td>" << endl;
	}
	for (time_t month = beginOfMonth(start); month < end;
		 month = sameTimeNextMonth(month))
	{
		if (a)
		{
			double volume = a->getActualVolume(
				accumulate ?
				Interval(start, sameTimeNextMonth(month) - 1) :
				Interval(month).firstMonth());
			actualTotals[time2ISO(month)] += volume;
			reportValue(volume, style, FALSE);
		}
		else
			reportValue(actualTotals[time2ISO(month)], style, TRUE);
	}
}

void
HTMLAccountReport::accountName(Account* a)
{
	QString spaces = "";
	int fontSize = 0;

	if (accountSortCriteria == CoreAttributesList::TreeMode)
	{
		Account* ap = a->getParent();
		for ( ; ap != 0; --fontSize)
		{
			spaces += "&nbsp;&nbsp;&nbsp;&nbsp;";
			ap = ap->getParent();
		}
		s << "<td class=\"default\" style=\"text-align:left\" rowspan=\""
		  << (!hidePlan && showActual ? "2" : "1") << "\" nowrap>"
		  << spaces
		  << "<font size=\""
		  << (fontSize < 0 ? '-' : '+') 
		  << (fontSize < 0 ? -fontSize : fontSize) << "\">"
		  << htmlFilter(a->getName())
		  << "</font></td>" << endl;
	}
	else
		s << "<td class=\"default\" rowspan=\""
		  << (!hidePlan && showActual ? "2" : "1")
		  << "\" style=\"text-align:left\" nowrap>"
		  << spaces << htmlFilter(a->getName())
		  << "</td>" << endl;
}

void
HTMLAccountReport::reportValue(double value, const QString& bgCol, bool bold)
{
	s << "<td class=\""
	  << bgCol << "\" style=\"text-align:right\">";
	if (bold)
		s << "<b>";
	s << QString().sprintf("%1.3f", value);
//		scaleTime(value, FALSE);
	if (bold)
		s << "</b>";
	s << "</td>" << endl;
}