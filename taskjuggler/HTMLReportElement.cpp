/*
 * Report.cpp - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include <config.h>

#include "HTMLReportElement.h"
#include "TjMessageHandler.h"
#include "tjlib-internal.h"
#include "Project.h"
#include "Resource.h"
#include "Report.h"
#include "Booking.h"
#include "BookingList.h"
#include "Utility.h"
#include "MacroTable.h"

#define KW(a) a

HTMLReportElement::HTMLReportElement(Project* p, Report* r,
                                     const QString& df, int dl) :
   ReportElement(p, r, df, dl)
{
    colDefault = 0xf3ebae;
    colDefaultLight = 0xfffadd;
    colWeekend = 0xffec80;
    colVacation = 0xfffc60;
    colAvailable = 0xa4ff8d;
    colBooked = 0xff5a5d;
    colBookedLight = 0xffbfbf;
    colHeader = 0xa5c2ff;
    colMilestone = 0xff2a2a;
    colCompleted = 0x87ff75;
    colCompletedLight = 0xa1ff9a;
    colToday = 0xa387ff;

    barLabels = BLT_LOAD;

    registerUrl(KW("dayheader"));
    registerUrl(KW("monthheader"));
    registerUrl(KW("resourcename"));
    registerUrl(KW("taskname"));
    registerUrl(KW("weekheader"));
    registerUrl(KW("yearheader"));
}

void
HTMLReportElement::generatePlanTask(const Task* t, const Resource* r, uint no)
{
    s() << "<tr valign=\"middle\">";
    const QString blank( "&nbsp;");
    
    for (QStringList::Iterator it = columns.begin(); it != columns.end();
         ++it )
    {
        if (*it == KW("seqno"))
            textTwoRows((r == 0 ? QString().sprintf("%d.", t->getSequenceNo()) :
                         QString("")), r != 0, "");
        else if (*it == KW("no"))
            textTwoRows((r == 0 ? QString().sprintf("%d.", no) :
                         QString("")), r != 0, "");
        else if (*it == KW("index"))
            textTwoRows((r == 0 ? QString().sprintf("%d.", t->getIndex()) :
                         QString("")), r != 0, "");
        else if (*it == KW("id"))
            textTwoRows(htmlFilter(t->getId()), r != 0, "left");
        else if (*it == KW("name"))
            taskName(t, r, r == 0);
        else if (*it == KW("start"))
            s() << "<td class=\""
                << (t->isStartOk(Task::Plan) ?
                    (r == 0 ? "default" : "defaultlight") : "milestone")
                << "\" style=\"text-align:left white-space:nowrap\">"
                << time2user(t->getStart(Task::Plan), timeFormat)
                << "</td>" << endl;
        else if (*it == KW("end"))
            s() << "<td class=\""
                << (t->isEndOk(Task::Plan) ?
                    (r == 0 ? "default" : "defaultlight") : "milestone")
                << "\" style=\"text-align:left white-space:nowrap\">"
                << time2user(t->getEnd(Task::Plan) + 1, timeFormat)
                << "</td>" << endl;
        else if (*it == KW("minstart"))
            textTwoRows(t->getMinStart() == 0 ? blank :
                        time2user(t->getMinStart(), timeFormat), r != 0, "");
        else if (*it == KW("maxstart"))
            textTwoRows(t->getMaxStart() == 0 ? blank :
                        time2user(t->getMaxStart(), timeFormat), r != 0, "");
        else if (*it == KW("minend"))
            textTwoRows(t->getMinEnd() == 0 ? blank :
                        time2user(t->getMinEnd(), timeFormat), r != 0, "");
        else if (*it == KW("maxend"))
            textTwoRows(t->getMaxEnd() == 0 ? blank :
                        time2user(t->getMaxEnd(), timeFormat), r != 0, "");
        else if (*it == KW("startbuffer"))
            textTwoRows(QString().sprintf
                        ("%3.0f", t->getStartBuffer(Task::Plan)), r != 0, 
                        "right");
        else if (*it == KW("endbuffer"))
            textTwoRows(QString().sprintf
                        ("%3.0f", t->getEndBuffer(Task::Plan)), r != 0, 
                        "right");
        else if (*it == KW("startbufferend"))
            textOneRow(time2user(t->getStartBufferEnd(Task::Plan) + 1,
                                 timeFormat), r != 0, "left");
        else if (*it == KW("endbufferstart"))
            textOneRow(time2user(t->getEndBufferStart(Task::Plan), timeFormat),
                       r != 0, "left");
        else if (*it == KW("duration"))
        {
            s() << "<td class=\""
                << (r == 0 ? "default" : "defaultlight")
                << "\" style=\"text-align:right white-space:nowrap\">"
                << scaledLoad(t->getCalcDuration(Task::Plan))
                << "</td>" << endl;
        }
        else if (*it == KW("effort"))
            taskLoadValue(t, r, t->getLoad(Task::Plan, 
                                           Interval(start, end), r)); 
        else if (*it == KW("projectid"))
            textTwoRows(t->getProjectId() + " (" +
                        project->getIdIndex(t->getProjectId()) + ")", r != 0,
                        "left");
        else if (*it == KW("resources"))
            scenarioResources(Task::Plan, t, r != 0);
        else if (*it == KW("responsible"))
            if (t->getResponsible())
                textTwoRows(htmlFilter(t->getResponsible()->getName()), r != 0,
                            "left");
            else
                textTwoRows("&nbsp", r != 0, "left");
        else if (*it == KW("responsibilities"))
            emptyPlan(r != 0);
        else if (*it == KW("depends"))
            generateDepends(t, r != 0);
        else if (*it == KW("follows"))
            generateFollows(t, r != 0);
        else if (*it == KW("schedule"))
            emptyPlan(r != 0);
        else if (*it == KW("mineffort"))
            emptyPlan(r != 0);
        else if (*it == KW("maxeffort"))
            emptyPlan(r != 0);
        else if (*it == KW("rate"))
            emptyPlan(r != 0);
        else if (*it == KW("kotrusid"))
            emptyPlan(r != 0);
        else if (*it == KW("note"))
        {
            s() << "<td class=\""
                << (r == 0 ? "default" : "defaultlight")
                << "\" rowspan=\""
                << QString("%1").arg(scenarios.count())
                << "\" style=\"text-align:left\">"
                << "<span style=\"font-size:100%\">";
            if (t->getNote().isEmpty())
                s() << "&nbsp;";
            else
                s() << htmlFilter(t->getNote());
            s() << "</span></td>" << endl;
        }
        else if (*it == KW("statusnote"))
        {
            s() << "<td class=\""
                << (r == 0 ? "default" : "defaultlight")
                << "\" style=\"text-align:left\">"
                << "<span style=\"font-size:100%\">";
            if (t->getStatusNote(Task::Plan).isEmpty())
                s() << "&nbsp;";
            else
                s() << htmlFilter(t->getStatusNote(Task::Plan));
            s() << "</span></td>" << endl;
        }
        else if (*it == KW("cost") || *it == "costs")
            taskCostRev(t, r, t->getCredits(Task::Plan,
                                            Interval(start, end), Cost, r));
        else if (*it == KW("revenue"))
            taskCostRev(t, r, t->getCredits(Task::Plan,
                                            Interval(start, end),
                                            Revenue, r));
        else if (*it == KW("profit"))
            taskCostRev(t, r,
                        t->getCredits(Task::Plan, Interval(start, end),
                                      Revenue, r) -
                        t->getCredits(Task::Plan, Interval(start, end),
                                      Cost, r));
        else if (*it == KW("priority"))
            textTwoRows(QString().sprintf("%d", t->getPriority()), r != 0,
                        "right");
        else if (*it == KW("flags"))
            flagList(t, r);
        else if (*it == KW("completed"))
            if (t->getCompletionDegree(Task::Plan) ==
                t->getCalcedCompletionDegree(Task::Plan))
            {
                textOneRow(QString("%1%")
                           .arg((int) t->getCompletionDegree(Task::Plan)),
                           r != 0, "right");
            }
            else
            {
                textOneRow(QString("%1% (%2%)")
                           .arg((int) t->getCompletionDegree(Task::Plan))
                           .arg((int) t->getCalcedCompletionDegree(Task::Plan)),
                           r != 0, "right");
            }
        else if (*it == KW("status"))
            generateTaskStatus(t->getStatus(Task::Plan), r != 0);
        else if (*it == KW("reference"))
        {
            s() << "<td class=\""
                << (r == 0 ? "default" : "defaultlight")
                << "\" rowspan=\""
                << QString("%1").arg(scenarios.count())
                << "\" style=\"text-align:left\">"
                << "<span style=\"font-size:100%\">";
            if (t->getReference().isEmpty())
                s() << "&nbsp;";
            else
            {
                s() << "<a href=\"" << t->getReference() << "\">";
                if (t->getReferenceLabel().isEmpty())
                    s() << htmlFilter(t->getReference());
                else
                    s() << htmlFilter(t->getReferenceLabel());
                s() << "<a>";
            }
            s() << "</span></td>" << endl;
        }
        else if (*it == KW("daily"))
            dailyTaskPlan(t, r);
        else if (*it == KW("weekly"))
            weeklyTaskPlan(t, r);
        else if (*it == KW("monthly"))
            monthlyTaskPlan(t, r);
        else
            qFatal("generatePlanTask: Unknown Column %s",
                   (*it).latin1());
    }
    s() << "</tr>" << endl;
}

void
HTMLReportElement::generateActualTask(const Task* t, const Resource* r)
{
    s() << "<tr>" << endl;
    for (QStringList::Iterator it = columns.begin();
         it != columns.end();
         ++it )
    {
        if (*it == KW("start"))
        {
            s() << "<td class=\""
                << (t->isStartOk(Task::Actual) ?
                    (r == 0 ? "default" : "defaultlight") : "milestone")
                << "\" style=\"text-align:left white-space:nowrap\">"
                << time2user(t->getStart(Task::Actual), timeFormat)
                << "</td>" << endl;
        }
        else if (*it == KW("end"))
        {
            s() << "<td class=\""
                << (t->isEndOk(Task::Actual) ?
                    (r == 0 ? "default" : "defaultlight") : "milestone")
                << "\" style=\"white-space:nowrap\">"
                << time2user(t->getEnd(Task::Actual) + 1, timeFormat)
                << "</td>" << endl;
        }
        else if (*it == KW("startbufferend"))
            textOneRow(time2user(t->getStartBufferEnd(Task::Actual) + 1, 
                                 timeFormat), r != 0, "left");
        else if (*it == KW("endbufferstart"))
            textOneRow(time2user(t->getEndBufferStart(Task::Actual), 
                                 timeFormat), r != 0, "left");
        else if (*it == KW("duration"))
        {
            s() << "<td class=\""
                << (r == 0 ? "default" : "defaultlight")
                << "\" style=\"text-align:right white-space:nowrap\">"
                << scaledLoad(t->getCalcDuration(Task::Actual))
                << "</td>" << endl;
        }
        else if (*it == KW("effort"))
            taskLoadValue(t, r, t->getLoad(Task::Actual, 
                                           Interval(start, end), r));
        else if (*it == KW("resources"))
            scenarioResources(Task::Actual, t, r != 0);
        else if (*it == KW("cost") || *it == "costs")
            taskCostRev(t, r, t->getCredits(Task::Actual,
                                            Interval(start, end), Cost, r));
        else if (*it == KW("revenue"))
            taskCostRev(t, r, t->getCredits(Task::Actual,
                                            Interval(start, end), Revenue, r));
        else if (*it == KW("profit"))
            taskCostRev(t, r,
                        t->getCredits(Task::Actual, Interval(start, end),
                                      Revenue, r) -
                        t->getCredits(Task::Actual, Interval(start, end),
                                      Cost, r));
        else if (*it == KW("completed"))
            if (t->getCompletionDegree(Task::Actual) ==
                t->getCalcedCompletionDegree(Task::Actual))
            {
                textOneRow(QString("%1%")
                           .arg((int) t->getCompletionDegree(Task::Actual)),
                           r != 0, "right");
            }
            else
            {
                textOneRow(QString("%1% (%2%)")
                           .arg((int) t->getCompletionDegree(Task::Actual))
                           .arg((int)
                                t->getCalcedCompletionDegree(Task::Actual)),
                           r != 0, "right");
            }
        else if (*it == KW("status"))
            generateTaskStatus(t->getStatus(Task::Actual), r != 0);
        else if (*it == KW("statusnote"))
        {
            s() << "<td class=\""
                << (r == 0 ? "default" : "defaultlight")
                << "\" style=\"text-align:left\">"
                << "<span style=\"font-size:100%\">";
            if (t->getStatusNote(Task::Actual).isEmpty())
                s() << "&nbsp;";
            else
                s() << htmlFilter(t->getStatusNote(Task::Actual));
            s() << "</span></td>" << endl;
        }
        else if (*it == KW("daily"))
            dailyTaskActual(t, r);
        else if (*it == KW("weekly"))
            weeklyTaskActual(t, r);
        else if (*it == KW("monthly"))
            monthlyTaskActual(t, r);
    }
    s() << "</tr>" << endl;
}

void
HTMLReportElement::generatePlanResource(const Resource* r, const Task* t, 
                                        uint no)
{
    s() << "<tr valign=\"middle\">";
    for (QStringList::Iterator it = columns.begin(); it != columns.end();
         ++it )
    {
        if (*it == KW("seqno"))
            textTwoRows((t == 0 ? QString().sprintf("%d.", r->getSequenceNo()) :
                         QString("")), t != 0, "");
        else if (*it == KW("no"))
            textTwoRows((t == 0 ? QString().sprintf("%d.", no) :
                         QString("")), t != 0, "");
        else if (*it == KW("index"))
            textTwoRows((t == 0 ? QString().sprintf("%d.", r->getIndex()) :
                         QString("")), t != 0, "");
        else if (*it == KW("id"))
            textTwoRows(htmlFilter(r->getId()), t != 0, "left");
        else if (*it == KW("name"))
            resourceName(r, t, FALSE);
        else if (*it == KW("start"))
            emptyPlan(t != 0);
        else if (*it == KW("end"))
            emptyPlan(t != 0);
        else if (*it == KW("minstart"))
            emptyPlan(t != 0);
        else if (*it == KW("maxstart"))
            emptyPlan(t != 0);
        else if (*it == KW("minend"))
            emptyPlan(t != 0);
        else if (*it == KW("maxend"))
            emptyPlan(t != 0);
        else if (*it == KW("startbuffer"))
            emptyPlan(t != 0);
        else if (*it == KW("endbuffer"))
            emptyPlan(t != 0);
        else if (*it == KW("startbufferend"))
            emptyPlan(t != 0);
        else if (*it == KW("endbufferstart"))
            emptyPlan(t != 0);
        else if (*it == KW("duration"))
            emptyPlan(t != 0);
        else if (*it == KW("effort"))
            resourceLoadValue(t, r, r->getLoad(Task::Plan, Interval(start, end), 
                                               AllAccounts, t));
        else if (*it == KW("projectid"))
            emptyPlan(t != 0);
        else if (*it == KW("resources"))
            emptyPlan(t != 0);
        else if (*it == KW("responsible"))
            emptyPlan(t != 0);
        else if (*it == KW("responsibilities"))
            generateResponsibilities(r, t != 0);
        else if (*it == KW("depends"))
            emptyPlan(t != 0);
        else if (*it == KW("follows"))
            emptyPlan(t != 0);
        else if (*it == KW("schedule"))
            generateSchedule(Task::Plan, r, t);
        else if (*it == KW("mineffort"))
            textTwoRows(QString().sprintf("%.2f", r->getMinEffort()), t != 0,
                        "right");
        else if (*it == KW("maxeffort"))
            textTwoRows(QString().sprintf("%.2f", r->getMaxEffort()), t != 0,
                        "right");
        else if (*it == KW("rate"))
            textTwoRows(QString().sprintf("%.*f", project->getCurrencyDigits(),
                                          r->getRate()), t != 0,
                        "right");
        else if (*it == KW("kotrusid"))
            textTwoRows(r->getKotrusId(), t != 0, "left");
        else if (*it == KW("note"))
            emptyPlan(t != 0);
        else if (*it == KW("cost") || *it == "costs")
            resourceCostRev(t, r, r->getCredits(Task::Plan,
                                                Interval(start, end), Cost, t));
        else if (*it == KW("revenue"))
            resourceCostRev(t, r, r->getCredits(Task::Plan,
                                                Interval(start, end), 
                                                Revenue, t));
        else if (*it == KW("profit"))
            resourceCostRev(t, r,
                            r->getCredits(Task::Plan, Interval(start, end),
                                          Revenue, t) -
                            r->getCredits(Task::Plan, Interval(start, end),
                                          Cost, t));
        else if (*it == KW("priority"))
            emptyPlan(t != 0);
        else if (*it == KW("flags"))
            flagList(r, t);
        else if (*it == KW("completed"))
            emptyPlan(t != 0);
        else if (*it == KW("status"))
            emptyPlan(t != 0);
        else if (*it == KW("reference"))
            emptyPlan(t != 0);
        else if (*it == KW("daily"))
            dailyResourcePlan(r, t);
        else if (*it == KW("weekly"))
            weeklyResourcePlan(r, t);
        else if (*it == KW("monthly"))
            monthlyResourcePlan(r, t);
        else
            qFatal("generatePlanResource: Unknown Column %s",
                   (*it).latin1());
    }
    s() << "</tr>" << endl;
}

void
HTMLReportElement::generateActualResource(const Resource* r, const Task* t)
{
    s() << "<tr valign=\"middle\">";
    for (QStringList::Iterator it = columns.begin(); it != columns.end();
         ++it )
    {
        if (*it == KW("effort"))
            resourceLoadValue(t, r, r->getLoad(Task::Actual, 
                                               Interval(start, end),
                                               AllAccounts, t));
        else if (*it == KW("schedule"))
            generateSchedule(Task::Actual, r, t);
        else if (*it == KW("cost") || *it == "costs")
            resourceCostRev(t, r, r->getCredits(Task::Actual,
                                                Interval(start, end), Cost, t));
        else if (*it == KW("revenue"))
            resourceCostRev(t, r, r->getCredits(Task::Actual,
                                                Interval(start, end), 
                                                Revenue, t));
        else if (*it == KW("profit"))
            resourceCostRev(t, r,
                            r->getCredits(Task::Actual, Interval(start, end),
                                          Revenue, t) -
                            r->getCredits(Task::Actual, Interval(start, end),
                                          Cost, t));
        else if (*it == KW("daily"))
            dailyResourceActual(r, t);
        else if (*it == KW("weekly"))
            weeklyResourceActual(r, t);
        else if (*it == KW("monthly"))
            monthlyResourceActual(r, t);
    }
    s() << "</tr>" << endl;
}

void
HTMLReportElement::reportHTMLHeader()
{
    s() << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">"
      << endl
      << "<!-- Generated by TaskJuggler v"VERSION" -->" << endl
      << "<!-- For details about TaskJuggler see "
      << TJURL << " -->" << endl
      << "<html>" << endl
      << "<head>" << endl
      << "<title>Task Report</title>" << endl
      << "<meta http-equiv=\"Content-Type\" "
      << "content=\"text/html; charset=utf-8\">" << endl
        << "<style type=\"text/css\"><!--" << endl;
    if (rawStyleSheet.isEmpty())
    {
        s().reset();
        s().setf(QTextStream::hex);
        s() << ".default { background-color:#" << colDefault
            << "; font-size:70%; text-align:center }" << endl
            << ".defaultlight { background-color:#" << colDefaultLight
            << "; font-size:70%; text-align:center }" << endl
            << ".task { background-color:#" << colDefault
            << "; font-size:100%; text-align:left }" << endl
            << ".tasklight { background-color:#" << colDefaultLight
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
            << ".bookedlight { background-color:#" << colBookedLight
            << "; font-size:70%; text-align:center }" << endl
            << ".headersmall { background-color:#" << colHeader
            << "; font-size:70%; text-align:center }" << endl
            << ".headerbig { background-color:#" << colHeader
            << "; font-size:110%; font-weight:bold; text-align:center }" << endl
            << ".completed { background-color:#" << colCompleted
            << "; font-size:70%; text-align:center }" << endl
            << ".completedlight { background-color:#" << colCompletedLight
            << "; font-size:70%; text-align:center }" << endl
            << ".today { background-color:#" << colToday
            << "; font-size:70%; text-align:center }" << endl;
    }
    else
        s() << rawStyleSheet << endl;
    s() << "--></style>" << endl;
    s() << "</head>" << endl
      << "<body>" << endl;

#if 0
    if (!headline.isEmpty())
        s() << "<h1>" << htmlFilter(headline) << "</h1>" << endl;
    if (!caption.isEmpty())
        s() << "<p>" << htmlFilter(caption) << "</p>" << endl;
#endif            
    if (!rawHead.isEmpty())
        s() << rawHead << endl;
}

void
HTMLReportElement::reportHTMLFooter()
{
    if (!rawTail.isEmpty())
        s() << rawTail << endl;

    s() << "<p><span style=\"font-size:0.7em\">";
    if (!project->getCopyright().isEmpty())
        s() << htmlFilter(project->getCopyright()) << " - ";
    s() << "Version " << htmlFilter(project->getVersion())
      << " - Created with <a HREF=\"" << TJURL <<
      "\">TaskJuggler v"
      << VERSION << "</a></span></p>" << endl << "</body>\n";
}

bool
HTMLReportElement::generateTableHeader()
{
    // Header line 1
    s() << "<table align=\"center\" cellpadding=\"1\" "
        << "style=\"padding-left:2px; padding-right:2px\">\n" << endl;
    s() << "<thead><tr>" << endl;
    for (QStringList::Iterator it = columns.begin(); it != columns.end();
         ++it )
    {
        if (*it == KW("seqno"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">" 
                << i18n("Seq. No.") << "</td>";
        else if (*it == KW("no"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("No.") << "</td>";
        else if (*it == KW("index"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Index No.") << "</td>";
        else if (*it == KW("id"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("ID") << "</td>";
        else if (*it == KW("name"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Name") << "</td>";
        else if (*it == KW("start"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Start") << "</td>";
        else if (*it == KW("end"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("End") << "</td>";
        else if (*it == KW("minstart"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Min. Start") << "</td>";
        else if (*it == KW("maxstart"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Max. Start") << "</td>";
        else if (*it == KW("minend"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Min. End") << "</td>";
        else if (*it == KW("maxend"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Max. End") << "</td>";
        else if (*it == KW("startbufferend"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Start Buf. End") << "</td>";
        else if (*it == KW("endbufferstart"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("End Buf. Start") << "</td>";
        else if (*it == KW("startbuffer"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Start Buf.") << "</td>";
        else if (*it == KW("endbuffer"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("End Buf.") << "</td>";
        else if (*it == KW("duration"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Duration") << "</td>";
        else if (*it == KW("effort"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Effort") << "</td>";
        else if (*it == KW("projectid"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Project ID") << "</td>";
        else if (*it == KW("resources"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Resources") << "</td>";
        else if (*it == KW("responsible"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Responsible") << "</td>";
        else if (*it == KW("responsibilities"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Responsibilities") << "</td>";
        else if (*it == KW("depends"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Dependencies") << "</td>";
        else if (*it == KW("follows"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Followers") << "</td>";
        else if (*it == KW("schedule"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Schedule") << "</td>";
        else if (*it == KW("mineffort"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Min. Effort") << "</td>";
        else if (*it == KW("maxeffort"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Max. Effort") << "</td>";
        else if (*it == KW("flags"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Flags") << "</td>";
        else if (*it == KW("completed"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Completed") << "</td>";
        else if (*it == KW("status"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Status") << "</td>";
        else if (*it == KW("rate"))
        {
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Rate");
            if (!project->getCurrency().isEmpty())
                s() << " " << htmlFilter(project->getCurrency());
            s() << "</td>";
        }
        else if (*it == KW("kotrusid"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Kotrus ID") << "</td>";
        else if (*it == KW("note"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Note") << "</td>";
        else if (*it == KW("statusnote"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Status Note") << "</td>";
        else if (*it == KW("cost") || *it == "costs")
        {
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Costs");
            if (!project->getCurrency().isEmpty())
                s() << " " << htmlFilter(project->getCurrency());
            s() << "</td>";
        }
        else if (*it == KW("revenue"))
        {
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Revenue");
            if (!project->getCurrency().isEmpty())
                s() << " " << htmlFilter(project->getCurrency());
            s() << "</td>";
        }
        else if (*it == KW("profit"))
        {
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Profit");
            if (!project->getCurrency().isEmpty())
                s() << " " << htmlFilter(project->getCurrency());
            s() << "</td>";
        }
        else if (*it == KW("priority"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Priority") << "</td>";
        else if (*it == KW("reference"))
            s() << "<td class=\"headerbig\" rowspan=\"2\">"
                << i18n("Ref.") << "</td>";
        else if (*it == KW("daily"))
            htmlDailyHeaderMonths();
        else if (*it == KW("weekly"))
            htmlWeeklyHeaderMonths();
        else if (*it == KW("monthly"))
            htmlMonthlyHeaderYears();
        else
        {
            TJMH.errorMessage
                (i18n("Unknown Column '%1' for HTML Task Report")
                 .arg(*it));
            return FALSE;
        }
    }
    s() << "</tr>" << endl;

    // Header line 2
    bool td = FALSE;
    s() << "<tr>" << endl;
    for (QStringList::Iterator it = columns.begin(); it != columns.end();
         ++it )
    {
        if (*it == KW("daily"))
        {
            td = TRUE;
            htmlDailyHeaderDays();
        }
        else if (*it == KW("weekly"))
        {
            td = TRUE;
            htmlWeeklyHeaderWeeks();
        }
        else if (*it == KW("monthly"))
        {
            td = TRUE;
            htmlMonthlyHeaderMonths();
        }
    }
    if (!td)
        s() << "<td>&nbsp;</td>";
    s() << "</tr></thead>\n" << endl;

    return TRUE;
}

void
HTMLReportElement::htmlDailyHeaderDays(bool highlightNow)
{
    // Generates the 2nd header line for daily calendar views.
    for (time_t day = midnight(start); day < end; day = sameTimeNextDay(day))
    {
        int dom = dayOfMonth(day);
        s() << "<td class=\"" <<
            (highlightNow && isSameDay(project->getNow(), day) ?
             "today" : isWeekend(day) ? "weekend" : "headersmall")
          << "\"><span style=\"font-size:0.8em\">&nbsp;";
        mt.clear();
        mt.addMacro(new Macro(KW("day"), QString().sprintf("%02d", dom),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(day)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(day)),
                              defFileName, defFileLine));
        if (dom < 10)
            s() << "&nbsp;";
        s() << generateUrl(KW("dayheader"), QString().sprintf("%d", dom));
        s() << "</span></td>";
    }
}

void
HTMLReportElement::htmlDailyHeaderMonths()
{
    // Generates the 1st header line for daily calendar views.
    for (time_t day = midnight(start); day < end;
         day = beginOfMonth(sameTimeNextMonth(day)))
    {
        int left = daysLeftInMonth(day);
        if (left > daysBetween(day, end))
            left = daysBetween(day, end);
        s() << "<td class=\"headerbig\" colspan=\""
            << QString().sprintf("%d", left) << "\">"; 
        mt.clear();
        mt.addMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(day)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(day)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(day)),
                              defFileName, defFileLine));
        s() << generateUrl(KW("monthheader"), monthAndYear(day)); 
        s() << "</td>" << endl;
    }
}

void
HTMLReportElement::htmlWeeklyHeaderWeeks(bool highlightNow)
{
    // Generates the 2nd header line for weekly calendar views.
    bool wsm = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, wsm); week < end; 
         week = sameTimeNextWeek(week))
    {
        int woy = weekOfYear(week, wsm);
        s() << "<td class=\"" <<
            (highlightNow && isSameWeek(project->getNow(), week, wsm) ?
             "today" : "headersmall")
          << "\"><span style=\"font-size:0.8em\">&nbsp;";
        if (woy < 10)
            s() << "&nbsp;";
        mt.clear();
        mt.addMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(woy)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(woy)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(woy)),
                              defFileName, defFileLine));
        s() << generateUrl(KW("weekheader"), QString().sprintf("%d", woy));
        s() << "</span></td>";
    }
}

void
HTMLReportElement::htmlWeeklyHeaderMonths()
{
    // Generates the 1st header line for weekly calendar views.
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, weekStartsMonday); week < end; )
    {
        int currMonth = monthOfWeek(week, weekStartsMonday);
        int left;
        time_t wi = sameTimeNextWeek(week);
        for (left = 1 ; wi < end &&
             monthOfWeek(wi, weekStartsMonday) == currMonth;
             wi = sameTimeNextWeek(wi))
            left++;
             
        s() << "<td class=\"headerbig\" colspan=\""
          << QString().sprintf("%d", left) << "\">";
        mt.clear();
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfWeek(week,
                                                            weekStartsMonday)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", yearOfWeek(week,
                                                            weekStartsMonday)),
                              defFileName, defFileLine));
        s() << generateUrl(KW("monthheader"), 
                         QString("%1 %2").
                         arg(shortMonthName(monthOfWeek(week, weekStartsMonday)
                                            - 1)).
                         arg(yearOfWeek(week, weekStartsMonday)));
        s() << "</td>" << endl;
        week = wi;
    }
}

void
HTMLReportElement::htmlMonthlyHeaderMonths(bool highlightNow)
{
    // Generates 2nd header line of monthly calendar view.
    for (time_t month = beginOfMonth(start); month < end;
         month = sameTimeNextMonth(month))
    {
        int moy = monthOfYear(month);
        s() << "<td class=\"" <<
            (highlightNow && isSameMonth(project->getNow(), month) ?
             "today" : "headersmall")
          << "\"><span style=\"font-size:0.8em\">&nbsp;";
        mt.clear();
        mt.addMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(moy)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(moy)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(moy)),
                              defFileName, defFileLine));
        s() << generateUrl(KW("monthheader"), shortMonthName(moy - 1));
        s() << "</span></td>";
    }
}

void
HTMLReportElement::htmlMonthlyHeaderYears()
{
    // Generates 1st header line of monthly calendar view.
    for (time_t year = midnight(start); year < end;
         year = beginOfYear(sameTimeNextYear(year)))
    {
        int left = monthLeftInYear(year);
        if (left > monthsBetween(year, end))
            left = monthsBetween(year, end);
        s() << "<td class=\"headerbig\" colspan=\""
          << QString().sprintf("%d", left) << "\">";
        mt.clear();
        mt.addMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(year)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(year)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", ::year(year)),
                              defFileName, defFileLine));
        s() << generateUrl(KW("yearheader"),
                         QString().sprintf("%d", ::year(year)));
        s() << "</td>" << endl;
    }
}

void
HTMLReportElement::htmlQuarterlyHeaderQuarters(bool highlightNow)
{
    // Generates 2nd header line of quarterly calendar view.
    static const char* qnames[] =
    {
        I18N_NOOP("1st Quarter"), I18N_NOOP("2nd Quarter"),
        I18N_NOOP("3rd Quarter"), I18N_NOOP("4th Quarter")
    };

    for (time_t quarter = beginOfQuarter(start); quarter < end;
         quarter = sameTimeNextQuarter(quarter))
    {
        int qoy = quarterOfYear(quarter);
        s() << "<td class=\"" <<
            (highlightNow && isSameQuarter(project->getNow(), quarter) ?
             "today" : "headersmall")
          << "\"><span style=\"font-size:0.8em\">&nbsp;";
        mt.clear();
        mt.addMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(qoy)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(qoy)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("quarter"),
                              QString().sprintf("%d", quarterOfYear(qoy)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(qoy)),
                              defFileName, defFileLine));
        s() << generateUrl(KW("quarterheader"), i18n(qnames[qoy - 1]));
        s() << "</span></td>";
    }
}

void
HTMLReportElement::htmlQuarterlyHeaderYears()
{
    // Generates 1st header line of quarterly calendar view.
    for (time_t year = midnight(start); year < end;
         year = beginOfYear(sameTimeNextYear(year)))
    {
        int left = quartersLeftInYear(year);
        if (left > quartersBetween(year, end))
            left = quartersBetween(year, end);
        s() << "<td class=\"headerbig\" colspan=\""
          << QString().sprintf("%d", left) << "\">";
        mt.clear();
        mt.addMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(year)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(year)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("quarter"),
                              QString().sprintf("%d", quarterOfYear(year)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", ::year(year)),
                              defFileName, defFileLine));
        s() << generateUrl(KW("yearheader"),
                         QString().sprintf("%d", ::year(year)));
        s() << "</td>" << endl;
    }
}

void
HTMLReportElement::htmlYearHeader()
{
    // Generates 1st header line of monthly calendar view.
    for (time_t year = beginOfYear(start); year < end;
         year = sameTimeNextYear(year))
    {
        s() << "<td class=\"headerbig\" rowspan=\"2\">";
        mt.clear();
        mt.addMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(year)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(year)),
                              defFileName, defFileLine));
        mt.addMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", ::year(year)),
                              defFileName, defFileLine));
        s() << generateUrl(KW("yearheader"),
                         QString().sprintf("%d", ::year(year)));
        s() << "</td>" << endl;
    }
}

void
HTMLReportElement::emptyPlan(bool light)
{
    s() << "<td class=\""
      << (light ? "defaultlight" : "default")
      << "\" rowspan=\""
      << QString("%1").arg(scenarios.count())
      << "\">&nbsp;</td>";
}

void
HTMLReportElement::emptyActual(bool light)
{
    s() << "<td class=\""
      << (light ? "defaultlight" : "default")
      << "\">&nbsp;</td>";
}

void
HTMLReportElement::textOneRow(const QString& text, bool light, 
                              const QString& align)
{
    s() << "<td class=\""
      << (light ? "defaultlight" : "default") << "\"";
    if (!align.isEmpty())
        s() << " style=\"text-align:" << align << "; white-space:nowrap\"";
    s() << ">" << text << "</td>";
}

void
HTMLReportElement::textTwoRows(const QString& text, bool light, 
                               const QString& align)
{
    s() << "<td class=\""
        << (light ? "defaultlight" : "default")
        << "\" rowspan=\""
        << QString("%1").arg(scenarios.count());
    if (!align.isEmpty())
        s() << " style=\"text-align:" << align << "; white-space:nowrap\"";
    s() << ">" << text << "</td>";
}

void
HTMLReportElement::dailyResourcePlan(const Resource* r, const Task* t)
{
    for (time_t day = midnight(start); day < end;
         day = sameTimeNextDay(day))
    {
        double load = r->getLoad(Task::Plan, Interval(day).firstDay(), 
                                 AllAccounts, t);
        QString bgCol = 
            load > r->getMinEffort() * r->getEfficiency() ?
            (t == 0 ? "booked" : "bookedlight") :
            isSameDay(project->getNow(), day) ? "today" :
            isWeekend(day) ? "weekend" :
            project->isVacation(day) || r->hasVacationDay(day) ?
            "vacation" :
            (t == 0 ? "default" : "defaultlight");
        reportLoad(load, bgCol, !r->isGroup());
    }
}

void
HTMLReportElement::dailyResourceActual(const Resource* r, const Task* t)
{
    for (time_t day = midnight(start); day < end;
         day = sameTimeNextDay(day))
    {
        double load = r->getLoad(Task::Actual, Interval(day).firstDay(),
                                AllAccounts, t);
        QString bgCol = 
            load > r->getMinEffort() * r->getEfficiency() ?
            (t == 0 ? "booked" :
             (t->isCompleted(Task::Plan, sameTimeNextDay(day) - 1) ?
              "completedlight" : "bookedlight")) :
            isSameDay(project->getNow(), day) ? "today" :
            isWeekend(day) ? "weekend" :
            project->isVacation(day) || r->hasVacationDay(day) ?
            "vacation" :
            (t == 0 ? "default" : "defaultlight");
        reportLoad(load, bgCol, !r->isGroup());
    }
}

void 
HTMLReportElement::dailyTaskPlan(const Task* t, const Resource* r)
{
    for (time_t day = midnight(start); day < end;
         day = sameTimeNextDay(day))
    {
        double load = t->getLoad(Task::Plan, Interval(day).firstDay(), r);
        QString bgCol = 
            t->isActive(Task::Plan, Interval(day).firstDay()) ?
            (t->isMilestone() ? "milestone" :
             (r == 0 && !t->isBuffer(Task::Plan, Interval(day).firstDay()) ?
              "booked" : "bookedlight")) :
            isSameDay(project->getNow(), day) ? "today" :
            isWeekend(day) ? "weekend" :
            project->isVacation(day) ? "vacation" :
            (r == 0 ? "default" : "defaultlight");
        if (showPIDs)
        {
            QString pids = r->getProjectIDs(Task::Plan,
                                            Interval(day).firstDay(), t);
            reportPIDs(pids, bgCol, !r->isGroup());
        }
        else
            reportLoad(load, bgCol, !t->isContainer(), 
                       t->isActive(Task::Plan, Interval(day).firstDay()) &&
                       t->isMilestone());
    }
}

void
HTMLReportElement::dailyTaskActual(const Task* t, const Resource* r)
{
    for (time_t day = midnight(start); day < end;
         day = sameTimeNextDay(day))
    {
        double load = t->getLoad(Task::Actual, Interval(day).firstDay(), r);
        QString bgCol = 
            t->isActive(Task::Actual, Interval(day).firstDay()) ? 
            (t->isCompleted(Task::Plan, sameTimeNextDay(day) - 1) ?
             (r == 0 ? "completed" : "completedlight") :
             t->isMilestone() ? "milestone" :
             (r == 0 && !t->isBuffer(Task::Actual, Interval(day).firstDay())
              ? "booked" : "bookedlight")) :
            isSameDay(project->getNow(), day) ? "today" :
            isWeekend(day) ? "weekend" :
            project->isVacation(day) ? "vacation" :
            (r == 0 ? "default" : "defaultlight");
        if (showPIDs)
        {
            QString pids = r->getProjectIDs(Task::Actual,
                                            Interval(day).firstDay(), t);
            reportPIDs(pids, bgCol, !r->isGroup());
        }
        else
            reportLoad(load, bgCol, !t->isContainer(),
                       t->isActive(Task::Actual, Interval(day).firstDay()) &&
                       t->isMilestone());
    }
}

void
HTMLReportElement::weeklyResourcePlan(const Resource* r, const Task* t)
{
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, weekStartsMonday); week < end;
         week = sameTimeNextWeek(week))
    {
        double load =
            r->getLoad(Task::Plan, 
                       Interval(week).firstWeek(weekStartsMonday),
                      AllAccounts, t);
        QString bgCol =
            load > r->getMinEffort() * r->getEfficiency() ?
            (t == 0 ? "booked" : "bookedlight") :
            isSameWeek(project->getNow(), week, weekStartsMonday) ? "today" :
            (t == 0 ? "default" : "defaultlight");
        if (showPIDs)
        {
            QString pids =
                r->getProjectIDs(Task::Plan, Interval(week).
                                 firstWeek(weekStartsMonday), t);
            reportPIDs(pids, bgCol, !r->isGroup());
        }
        else
            reportLoad(load, bgCol, !r->isGroup());
    }
}

void
HTMLReportElement::weeklyResourceActual(const Resource* r, const Task* t)
{
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, weekStartsMonday); week < end;
         week = sameTimeNextWeek(week))
    {
        double load = 
            r->getLoad(Task::Actual, 
                       Interval(week).firstWeek(weekStartsMonday),
                      AllAccounts, t);
        QString bgCol =
            load > r->getMinEffort() * r->getEfficiency() ?
            (t == 0 ? "booked" :
             (t->isCompleted(Task::Plan, sameTimeNextWeek(week) - 1) ?
              "completedlight" : "bookedlight")) :
            isSameWeek(project->getNow(), week, weekStartsMonday) ? "today" :
            (t == 0 ? "default" : "defaultlight");
        if (showPIDs)
        {
            QString pids = r->getProjectIDs(Task::Actual, Interval(week).
                                            firstWeek(weekStartsMonday), t);
            reportPIDs(pids, bgCol, !r->isGroup());
        }
        else
            reportLoad(load, bgCol, !r->isGroup());
    }
}

void 
HTMLReportElement::weeklyTaskPlan(const Task* t, const Resource* r)
{
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, weekStartsMonday); week < end;
         week = sameTimeNextWeek(week))
    {
        double load = t->getLoad(Task::Plan, Interval(week).
                                 firstWeek(weekStartsMonday), r);
        QString bgCol = 
            t->isActive(Task::Plan,
                        Interval(week).firstWeek(weekStartsMonday)) ?
            (t->isMilestone() ? "milestone" :
             (r == 0 && !t->isBuffer(Task::Plan, Interval(week).
                                     firstWeek(weekStartsMonday))
              ? "booked" : "bookedlight")) :
            isSameWeek(project->getNow(), week, weekStartsMonday) ? "today" :
            (r == 0 ? "default" : "defaultlight");
        reportLoad(load, bgCol, !t->isContainer(), 
                   t->isActive(Task::Plan, 
                               Interval(week).firstWeek(weekStartsMonday)) &&
                   t->isMilestone());
    }
}

void
HTMLReportElement::weeklyTaskActual(const Task* t, const Resource* r)
{
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, weekStartsMonday); week < end;
         week = sameTimeNextWeek(week))
    {
        double load = t->getLoad(Task::Actual, Interval(week).
                                 firstWeek(weekStartsMonday), r);
        QString bgCol = 
            t->isActive(Task::Actual,
                        Interval(week).firstWeek(weekStartsMonday)) ?
            (t->isCompleted(Task::Plan, sameTimeNextWeek(week) - 1) ?
             (r == 0 ? "completed" : "completedlight") :
             t->isMilestone() ? "milestone" :
             (r == 0 && !t->isBuffer(Task::Actual, Interval(week).
                                           firstWeek(weekStartsMonday))
              ? "booked" : "bookedlight")) :
            isSameWeek(project->getNow(), week, weekStartsMonday) ? "today" :
            (r == 0 ? "default" : "defaultlight");
        reportLoad(load, bgCol, !t->isContainer(), 
                   t->isActive(Task::Actual, 
                               Interval(week).firstWeek(weekStartsMonday)) && 
                   t->isMilestone());
    }
}

void
HTMLReportElement::monthlyResourcePlan(const Resource* r, const Task* t)
{
    for (time_t month = beginOfMonth(start); month < end;
         month = sameTimeNextMonth(month))
    {
        double load = r->getLoad(Task::Plan, Interval(month).firstMonth(),
                                AllAccounts, t);
        QString bgCol =
            load > r->getMinEffort() * r->getEfficiency() ?
            (t == 0 ? "booked" : "bookedlight") :
            isSameMonth(project->getNow(), month) ? "today" :
            (t == 0 ? "default" : "defaultlight");
        reportLoad(load, bgCol, !r->isGroup());
    }
}

void
HTMLReportElement::monthlyResourceActual(const Resource* r, const Task* t)
{
    for (time_t month = beginOfMonth(start); month < end;
         month = sameTimeNextMonth(month))
    {
        double load = r->getLoad(Task::Actual, Interval(month).firstMonth(),
                                AllAccounts, t);
        QString bgCol =
            load > r->getMinEffort() * r->getEfficiency() ?
            (t == 0 ? "booked" :
             (t->isCompleted(Task::Plan, sameTimeNextMonth(month) - 1) ?
              "completedlight" : "bookedlight")) :
            isSameMonth(project->getNow(), month) ? "today" :
            (t == 0 ? "default" : "defaultlight");
        reportLoad(load, bgCol, !r->isGroup());
    }
}

void
HTMLReportElement::monthlyTaskPlan(const Task* t, const Resource* r)
{
    for (time_t month = beginOfMonth(start); month < end;
         month = sameTimeNextMonth(month))
    {
        double load = t->getLoad(Task::Plan, Interval(month).firstMonth(), r);
        QString bgCol = 
            t->isActive(Task::Plan, Interval(month).firstMonth()) ?
            (t->isMilestone() ? "milestone" :
             (r == 0 && !t->isBuffer(Task::Plan, Interval(month).firstMonth())
              ? "booked" : "bookedlight")) :
            isSameMonth(project->getNow(), month) ? "today" :
            (r == 0 ? "default" : "defaultlight");
        reportLoad(load, bgCol, !t->isContainer(), 
                   t->isActive(Task::Plan, Interval(month).firstMonth()) &&
                   t->isMilestone());
    }
}

void
HTMLReportElement::monthlyTaskActual(const Task* t, const Resource* r)
{
    for (time_t month = beginOfMonth(start); month < end;
         month = sameTimeNextMonth(month))
    {
        double load = t->getLoad(Task::Actual, Interval(month).firstMonth(), r);
        QString bgCol = 
            t->isActive(Task::Actual, Interval(month).firstMonth()) ?
            (t->isCompleted(Task::Plan, sameTimeNextMonth(month) - 1) ?
             (r == 0 ? "completed" : "completedlight"):
             t->isMilestone() ? "milestone" :
             (r == 0 && !t->isBuffer(Task::Actual, 
                                     Interval(month).firstMonth())
              ? "booked" : "bookedlight")) :
            isSameMonth(project->getNow(), month) ? "today" :
            (r == 0 ? "default" : "defaultlight");
        reportLoad(load, bgCol, !t->isContainer(), 
                   t->isActive(Task::Actual, Interval(month).firstMonth()) && 
                   t->isMilestone());
    }
}

void
HTMLReportElement::taskName(const Task* t, const Resource* r, bool big)
{
    int lPadding = 0;
    int fontSize = big ? 100 : 90; 
    if (resourceSortCriteria[0] == CoreAttributesList::TreeMode)
        for (const Resource* rp = r ; rp != 0; rp = rp->getParent())
            lPadding++;

    mt.clear();
    mt.addMacro(new Macro(KW("taskid"), t->getId(), defFileName,
                          defFileLine));

    if (taskSortCriteria[0] == CoreAttributesList::TreeMode)
    {
        lPadding += t->treeLevel();
        fontSize = fontSize + 5 * (maxDepthTaskList - 1 - t->treeLevel()); 
        s() << "<td class=\""
          << (r == 0 ? "task" : "tasklight") << "\" rowspan=\""
          << QString("%1").arg(scenarios.count())
          << "\" style=\"white-space:nowrap;"
          << " padding-left: " << QString("%1").arg(lPadding * 15) << "px;\">"
          << "<span style=\"font-size:" 
          << QString().sprintf("%d", fontSize) << "%\">";
        if (r == 0)
            s() << "<a name=\"task_" << t->getId() << "\"></a>";
        s() << generateUrl(KW("taskname"), t->getName());
        s() << "</span></td>" << endl;
    }
    else
    {
        s() << "<td class=\""
          << (r == 0 ? "task" : "tasklight") << "\" rowspan=\""
          << QString("%1").arg(scenarios.count())
          << "\" style=\"white-space:nowrap;"
          << " padding-left: " << QString("%1").arg(lPadding * 15)
          << "px;\">";
        if (r == 0)
            s() << "<a name=\"task_" << t->getId() << "\"></a>";
        s() << generateUrl(KW("taskname"), t->getName());
        s() << "</td>" << endl;
    }
}

void
HTMLReportElement::resourceName(const Resource* r, const Task* t, bool big)
{
    int lPadding = 0;
    int fontSize = big ? 100 : 90;
    if (taskSortCriteria[0] == CoreAttributesList::TreeMode)
        for (const Task* tp = t; tp != 0; tp = tp->getParent())
            lPadding++;

    mt.clear();
    mt.addMacro(new Macro(KW("resourceid"), r->getId(), defFileName,
                          defFileLine));
    
    if (resourceSortCriteria[0] == CoreAttributesList::TreeMode)
    {
        for (uint i = 0; i < r->treeLevel(); i++)
            lPadding++;
        fontSize = fontSize + 5 * (maxDepthResourceList - 1 - r->treeLevel());
        s() << "<td class=\""
          << (t == 0 ? "task" : "tasklight") << "\" rowspan=\""
          << QString("%1").arg(scenarios.count())
          << "\" style=\"white-space:nowrap;"
          << " padding-left: " << QString("%1").arg(lPadding * 15) << "px;\">"
          << "<span style=\"font-size:" 
          << QString().sprintf("%d", fontSize) << "%\">";
        if (t == 0)
            s() << "<a name=\"resource_" << r->getFullId() << "\"></a>";
        s() << generateUrl(KW("resourcename"), r->getName());
        s() << "</span></td>" << endl;
    }
    else
    {
        s() << "<td class=\""
          << (t == 0 ? "task" : "tasklight") << "\" rowspan=\""
          << QString("%1").arg(scenarios.count())
          << "\" style=\"white-space:nowrap;"
          << " padding-left: " << QString("%1").arg(lPadding * 15)
          << "px;\">";
        if (t == 0)
            s() << "<a name=\"resource_" << r->getFullId() << "\"></a>";
        s() << generateUrl(KW("resourcename"), r->getName());
        s() << "</td>" << endl;
    }
}

void
HTMLReportElement::taskCostRev(const Task* t, const Resource* r, double val)
{
    int resIndent = 0, taskIndent = 0;
    if (taskSortCriteria[0] == CoreAttributesList::TreeMode)
        taskIndent = r == 0 ? 8 : 5;
    if (resourceSortCriteria[0] == CoreAttributesList::TreeMode)
        resIndent = (r != 0 ? 0 : 5) * maxDepthResourceList;
        
    s() << "<td class=\"default" << (r != 0 ? "light" : "") << "\""
        << " style=\"text-align:right; white-space:nowrap;"
        << " padding-right:" 
        << QString("%1").arg(resIndent +
                             (maxDepthTaskList - 1 - t->treeLevel()) * 
                             taskIndent)
        << "\">"
        << QString().sprintf("%.*f", project->getCurrencyDigits(), val)
        << "</td>";
}

void
HTMLReportElement::resourceCostRev(const Task* t, const Resource* r, double val)
{
    int resIndent = 0, taskIndent = 0;
    if (resourceSortCriteria[0] == CoreAttributesList::TreeMode)
        resIndent = t == 0 ? 8 : 5;
    if (taskSortCriteria[0] == CoreAttributesList::TreeMode)
        taskIndent = (t != 0 ? 0 : 5) * maxDepthTaskList;
    
    s() << "<td class=\"default" << (t != 0 ? "light" : "") << "\""
        << " style=\"text-align:right; white-space:nowrap;"
        << " padding-right:" 
        << QString("%1").arg(taskIndent +
                             (maxDepthResourceList - 1 - r->treeLevel()) * 
                             resIndent)
        << "\">"
        << QString().sprintf("%.*f", project->getCurrencyDigits(), val)
        << "</td>";
}

void
HTMLReportElement::taskLoadValue(const Task* t, const Resource* r, double val)
{
    int resIndent = 0, taskIndent = 0;
    if (taskSortCriteria[0] == CoreAttributesList::TreeMode)
        taskIndent = r == 0 ? 8 : 5;
    if (resourceSortCriteria[0] == CoreAttributesList::TreeMode)
        resIndent = (r != 0 ? 0 : 5) * maxDepthResourceList;
    
    s() << "<td class=\"default" << (r == 0 ? "" : "light")
        << "\" style=\"text-align:right; white-space:nowrap;"
        << " padding-right:" 
        << QString("%1").arg(resIndent +
                             (maxDepthTaskList - 1 - t->treeLevel()) * 
                             taskIndent)
        << "\">"
        << scaledLoad(val)
        << "</td>" << endl;
}

void
HTMLReportElement::resourceLoadValue(const Task* t, const Resource* r, double val)
{
    int resIndent = 0, taskIndent = 0;
    if (resourceSortCriteria[0] == CoreAttributesList::TreeMode)
        resIndent = t == 0 ? 8 : 5;
    if (taskSortCriteria[0] == CoreAttributesList::TreeMode)
        taskIndent = (t != 0 ? 0 : 5) * maxDepthTaskList;
    
    s() << "<td class=\"default" << (t == 0 ? "" : "light")
        << "\" style=\"text-align:right; white-space:nowrap;"
        << " padding-right:" 
        << QString("%1").arg(taskIndent +
                             (maxDepthResourceList - 1 - r->treeLevel()) *
                             resIndent)
        << "\">"
        << scaledLoad(val)
        << "</td>" << endl;
}

void
HTMLReportElement::scenarioResources(int sc, const Task* t, bool light)
{
    s() << "<td class=\""
      << (light ? "defaultlight" : "default")
      << "\" style=\"text-align:left\">"
      << "<span style=\"font-size:100%\">";
    bool first = TRUE;
    for (ResourceListIterator rli(t->getBookedResourcesIterator(sc));
        *rli != 0; ++rli)
    {
        if (!first)
            s() << ", ";
                    
        s() << htmlFilter((*rli)->getName());
        first = FALSE;
    }
    s() << "</span></td>" << endl;
}

void
HTMLReportElement::generateDepends(const Task* t, bool light)
{
    s() << "<td class=\""
      << (light ? "defaultlight" : "default")
      << "\" rowspan=\""
      << QString("%1").arg(scenarios.count())
      << "\" style=\"text-align:left\"><span style=\"font-size:100%\">";
    bool first = TRUE;
    for (TaskListIterator tli(t->getPreviousIterator()); *tli != 0; ++tli)
    {
        if (!first)
            s() << ", ";
        else
            first = FALSE;
        s() << (*tli)->getId();
    }
    s() << "</span</td>" << endl;
}

void
HTMLReportElement::generateFollows(const Task* t, bool light)
{
    s() << "<td class=\""
      << (light ? "defaultlight" : "default")
      << "\" rowspan=\""
      << QString("%1").arg(scenarios.count())
      << "\" style=\"text-align:left\">"
        "<span style=\"font-size:100%\">";
    bool first = TRUE;
    for (TaskListIterator tli(t->getFollowersIterator()); *tli != 0; ++tli)
    {
        if (!first)
            s() << ", ";
        s() << (*tli)->getId();
        first = FALSE;
    }
    s() << "</span</td>" << endl;
}

void
HTMLReportElement::generateResponsibilities(const Resource*r, bool light)
{
    s() << "<td class=\""
      << (light ? "defaultlight" : "default")
      << "\" rowspan=\""
      << QString("%1").arg(scenarios.count())
      << "\" style=\"text-align:left\">"
        "<span style=\"font-size:100%\">";
    bool first = TRUE;
    for (TaskListIterator tli(project->getTaskListIterator()); *tli != 0;
         ++tli)
    {
        if ((*tli)->getResponsible() == r)
        {
            if (!first)
                s() << ", ";
            s() << (*tli)->getId();
            first = FALSE;
        }
    }
    s() << "</span></td>" << endl;
}

void
HTMLReportElement::generateSchedule(int sc, const Resource* r, const Task* t)
{
    s() << "<td class=\""
      << (t == 0 ? "default" : "defaultlight") 
      << "\" style=\"text-align:left\">";

    if (r)
    {
        BookingList jobs = r->getJobs(sc);
        jobs.setAutoDelete(TRUE);
        time_t prevTime = 0;
        Interval reportPeriod(start, end);
        s() << "<table style=\"width:150px; font-size:100%; "
           "text-align:left\"><tr><th style=\"width:35%\"></th>"
           "<th style=\"width:65%\"></th></tr>" << endl;
        for (BookingListIterator bli(jobs); *bli != 0; ++bli)
        {
            if ((t == 0 || t == (*bli)->getTask()) && 
                reportPeriod.overlaps(Interval((*bli)->getStart(), 
                                               (*bli)->getEnd())))
            {
                /* If the reporting interval is not more than a day, we
                 * do not print the day since this information is most
                 * likely given by the context of the report. */
                if (!isSameDay(prevTime, (*bli)->getStart()) &&
                    !isSameDay(start, end - 1))
                {
                    s() << "<tr><td colspan=\"2\" style=\"font-size:120%\">"
                        << time2weekday((*bli)->getStart()) << ", "
                        << time2date((*bli)->getStart()) << "</td></tr>" 
                        << endl;
                }
                s() << "<tr><td>";
                Interval workPeriod((*bli)->getStart(), (*bli)->getEnd());
                workPeriod.overlap(reportPeriod);
                s() << time2user(workPeriod.getStart(), shortTimeFormat)
                    << "&nbsp;-&nbsp;"
                    << time2user(workPeriod.getEnd() + 1, shortTimeFormat);
                s() << "</td><td>";
                if (t == 0)
                    s() << " " << htmlFilter((*bli)->getTask()->getName());
                s() << "</td>" << endl;
                prevTime = (*bli)->getStart();
                s() << "</tr>" << endl;
            }
        }
        s() << "</table>" << endl;
    }
    else
        s() << "&nbsp;";

    s() << "</td>" << endl;
}

void
HTMLReportElement::flagList(const CoreAttributes* c1, const CoreAttributes* c2)
{
    FlagList allFlags = c1->getFlagList();
    QString flagStr;
    for (QStringList::Iterator it = allFlags.begin();
         it != allFlags.end(); ++it)
    {
        if (it != allFlags.begin())
            flagStr += ", ";
        flagStr += htmlFilter(*it);
    }
    if (flagStr.isEmpty())
        flagStr = "&nbsp";
    textTwoRows(flagStr, c2 != 0, "left");
}

void
HTMLReportElement::generateTaskStatus(TaskStatus status, bool light)
{
    QString text;
    switch (status)
    {
    case NotStarted:
        text = "Not yet started";
        break;
    case InProgressLate:
        text = "Behind schedule";
        break;
    case InProgress:
        text = "Work in progress";
        break;
    case OnTime:
        text = "On schedule";
        break;
    case InProgressEarly:
        text = "Ahead of schedule";
        break;
    case Finished:
        text = "Finished";
        break;
    default:
        text = "Unknown status";
        break;
    }
    textOneRow(text, light, "center");
}

void
HTMLReportElement::reportLoad(double load, const QString& bgCol, bool bold,
                       bool milestone)
{
    if ((load > 0.0 || milestone) && barLabels != BLT_EMPTY)
    {
        s() << "<td class=\""
          << bgCol << "\">";
        if (bold)
            s() << "<b>";
        if (milestone)
            s() << "*";
        else
            s() << scaledLoad(load);
        if (bold)
            s() << "</b>";
        s() << "</td>" << endl;
    }
    else
        s() << "<td class=\""
          << bgCol << "\">&nbsp;</td>" << endl;
}

void
HTMLReportElement::reportPIDs(const QString& pids, const QString bgCol, 
                              bool bold)
{
    s() << "<td class=\""
      << bgCol << "\" style=\"white-space:nowrap\">";
    if (bold)
        s() << "<b>";
    s() << pids;
    if (bold)
        s() << "</b>";
    s() << "</td>" << endl;
}

bool
HTMLReportElement::setUrl(const QString& key, const QString& url)
{
    if (urls.find(key) == urls.end())
        return FALSE;

    urls[key] = url;
    return TRUE;
}

const QString*
HTMLReportElement::getUrl(const QString& key) const
{
    if (urls.find(key) == urls.end() || urls[key] == "")
        return 0;
    return &urls[key];
}

QString
HTMLReportElement::generateUrl(const QString& key, const QString& txt)
{
    if (getUrl(key))
    {
        mt.setLocation(defFileName, defFileLine);
        return QString("<a href=\"") + mt.expand(*getUrl(key))
            + "\">" + htmlFilter(txt) + "</a>";
    }
    else
        return htmlFilter(txt);
}

