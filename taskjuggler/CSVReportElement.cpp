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

#include "CSVReportElement.h"
#include "TjMessageHandler.h"
#include "tjlib-internal.h"
#include "Project.h"
#include "Task.h"
#include "Resource.h"
#include "Account.h"
#include "Report.h"
#include "Booking.h"
#include "BookingList.h"
#include "Utility.h"
#include "MacroTable.h"
#include "TableColumnFormat.h"
#include "TableLineInfo.h"
#include "TableColumnInfo.h"
#include "TableCellInfo.h"
#include "ReferenceAttribute.h"
#include "TextAttribute.h"
#include "CSVReport.h"

#define KW(a) a

CSVReportElement::CSVReportElement(Report* r, const QString& df, int dl) :
   ReportElement(r, df, dl)
{
    fieldSeparator = ",";
}

void
CSVReportElement::generateHeader()
{
}

void
CSVReportElement::generateFooter()
{
}

void
CSVReportElement::generateTableHeader()
{
    bool first = TRUE;
    for (QPtrListIterator<TableColumnInfo> it(columns); it; ++it )
    {
        if (!first)
            s() << fieldSeparator;
        else
            first = FALSE;
        
        if (columnFormat[(*it)->getName()])
        {
            TableCellInfo tci(columnFormat[(*it)->getName()], 0, *it);
            (*this.*(columnFormat[(*it)->getName()]->genHeadLine1))
                (&tci);
        }
        else if ((*it)->getName() == "costs")
        {
            TJMH.errorMessage
                (i18n("'costs' has been deprecated. Use 'cost' instead."));
            return;
        }
        else
        {
            TJMH.errorMessage
                (i18n("Unknown Column '%1' for CSV Report")
                 .arg((*it)->getName()));
            return;
        }
    }
    
    if (!first)
        s() << endl;
}

void
CSVReportElement::generateLine(TableLineInfo* tli, int funcSel)
{
    setMacros(tli);

    bool first = TRUE;
    for (QPtrListIterator<TableColumnInfo> it(columns); it; ++it )
    {
        TableCellInfo tci(columnFormat[(*it)->getName()], tli, *it);
        if (columnFormat[(*it)->getName()])
        {
            if (!first)
                s() << fieldSeparator;
            else
                first = FALSE;

            GenCellPtr gcf;
            switch (funcSel)
            {
                case 0:
                    gcf = columnFormat[(*it)->getName()]->genHeadLine1;
                    break;
                case 1:
                    gcf = columnFormat[(*it)->getName()]->genHeadLine2;
                    break;
                case 2:
                    gcf = columnFormat[(*it)->getName()]->genTaskLine1;
                    break;
                case 3:
                    gcf = columnFormat[(*it)->getName()]->genTaskLine2;
                    break;
                case 4:
                    gcf = columnFormat[(*it)->getName()]->genResourceLine1;
                    break;
                case 5:
                    gcf = columnFormat[(*it)->getName()]->genResourceLine2;
                    break;
                case 6:
                    gcf = columnFormat[(*it)->getName()]->genAccountLine1;
                    break;
                case 7:
                    gcf = columnFormat[(*it)->getName()]->genAccountLine2;
                    break;
                case 8:
                    gcf = columnFormat[(*it)->getName()]->genSummaryLine1;
                    break;
                case 9:
                    gcf = columnFormat[(*it)->getName()]->genSummaryLine2;
                    break;
            }
            if (gcf)
            {
                (*this.*(gcf))(&tci);
            }
        }
    }
    if (!first)
        s() << endl;
}

void
CSVReportElement::genCell(const QString& text, TableCellInfo* tci, 
                           bool, bool filterText)
{
    QString cellText = filterText ? filter(text) : text;
    if (tci->tli->ca1 && !tci->tci->getCellText().isEmpty())
    {
        QStringList* sl = new QStringList();
        sl->append(text);
        mt.pushArguments(sl);
        cellText = mt.expand(tci->tci->getCellText());
        QString cellURL = mt.expand(tci->tci->getCellURL());
        mt.popArguments();
    }
    s() << "\"" << cellText << "\"";
}

void
CSVReportElement::reportTaskLoad(double load, TableCellInfo* tci,
                                  const Interval& period)
{
    QString text;
    if (tci->tli->task->isActive(tci->tli->sc, period))
    {
        if (tci->tli->task->isContainer())
        {
            QString pre, post;
            if (period.contains(tci->tli->task->getStart(tci->tli->sc)))
                pre = "v=";
            if (period.contains(tci->tli->task->getEnd(tci->tli->sc)))
                post += "=v";
            if (load > 0.0 && barLabels != BLT_EMPTY)
                text = scaledLoad(load, tci);
            else if (pre.isEmpty() && post.isEmpty())
                text = "==";
            else if (!pre.isEmpty() && !post.isEmpty())
            {
                pre = post = "v";
                text = "=";
            }
            text = pre + text + post;
            tci->setBoldText(true);
        }
        else
        {
            if (tci->tli->task->isMilestone())
            {
                text += "<>";
                tci->setBoldText(true);
            }
            else
            {
                QString pre, post;
                if (period.contains(tci->tli->task->
                                    getStart(tci->tli->sc)))
                    pre = "[=";
                if (period.contains(tci->tli->task->
                                    getEnd(tci->tli->sc)))
                    post = "=]";
                if (!pre.isEmpty() && !post.isEmpty())
                {
                    pre = "[";
                    post = "]";
                }
                if (load > 0.0 && barLabels != BLT_EMPTY)
                    text = scaledLoad(load, tci);
                else if (pre.isEmpty() && post.isEmpty())
                    text = "==";
                else if (pre == "[")
                   text = "="; 
                text = pre + text + post;
            }
        }
        tci->setHAlign("center");
        tci->setStatusText(time2user(period.getStart(), "%Y-%m-%d / [") +
                           tci->tli->task->getId() + "] " +
                           filter(tci->tli->task->getName()));
    }
    else
    {
        tci->setStatusText("");
    }
    genCell(text, tci, FALSE);
}

void
CSVReportElement::reportResourceLoad(double load, TableCellInfo* tci,
                                      const Interval& period)
{
    QString text;
    if (load > 0.0)
    {
        if (barLabels != BLT_EMPTY)
            text += scaledLoad(load, tci);
        if (tci->tli->resource->hasSubs())
            tci->setBoldText(true);
        tci->setHAlign("center");
        tci->setStatusText(time2user(period.getStart(), "%Y-%m-%d / [") +
                           tci->tli->resource->getId() + "] " +
                           filter(tci->tli->resource->getName()));
    }
    else
    {
        tci->setStatusText("");
    }
    genCell(text, tci, FALSE);
}

void
CSVReportElement::reportCurrency(double value, TableCellInfo* tci, 
                                  time_t iv_start)
{
    tci->setStatusText(time2user(iv_start, "%Y-%m-%d / [") +
                       tci->tli->account->getId() + "] " +
                       filter(tci->tli->account->getName()));
    genCell(tci->tcf->realFormat.format(value, tci), tci, FALSE);
}

void
CSVReportElement::generateTitle(TableCellInfo* tci, const QString& str)
{
    QStringList* sl = new QStringList();
    sl->append(str);
    mt.pushArguments(sl);
    QString cellText;
    if (!tci->tci->getTitle().isEmpty())
    {
        cellText = mt.expand(tci->tci->getTitle());
        if (!tci->tci->getSubTitle().isEmpty())
            cellText += " " + mt.expand(tci->tci->getSubTitle());
    }
    else
        cellText = str;
    cellText = filter(cellText);
    mt.popArguments();

    s() << "\"" << cellText << "\"";
}

void
CSVReportElement::generateSubTitle(TableCellInfo*, const QString&)
{
}

void
CSVReportElement::generateRightIndented(TableCellInfo* tci, const QString& str)
{
    int topIndent = 0, maxDepth = 0;
    if (tci->tli->ca1->getType() == CA_Task)
    {
        if (resourceSortCriteria[0] == CoreAttributesList::TreeMode)
            topIndent = (tci->tli->ca2 != 0 ? 0 : 1) * maxDepthResourceList;
        maxDepth = maxDepthTaskList;
    }
    else if (tci->tli->ca1->getType() == CA_Resource)
    {
        if (taskSortCriteria[0] == CoreAttributesList::TreeMode)
            topIndent = (tci->tli->ca2 != 0 ? 0 : 1) * maxDepthTaskList;
        maxDepth = maxDepthResourceList;
    }

    genCell(str + QString().fill(' ', topIndent + (maxDepth - 1 -
                                                   tci->tli->ca1->treeLevel())),
            tci, FALSE);
}

void
CSVReportElement::genHeadDefault(TableCellInfo* tci)
{
    generateTitle(tci, tci->tcf->getTitle());
}

void
CSVReportElement::genHeadCurrency(TableCellInfo* tci)
{

    generateTitle(tci, tci->tcf->getTitle() +
                  (!report->getProject()->getCurrency().isEmpty() ?
                   QString(" ") + report->getProject()->getCurrency() :
                   QString()));
}

void
CSVReportElement::genHeadDaily1(TableCellInfo* tci)
{
    // Generates the 1st header line for daily calendar views.
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t day = midnight(start); day < end;
         day = sameTimeNextMonth(beginOfMonth(day)))
    {
        int left = daysLeftInMonth(day);
        if (left > daysBetween(day, end))
            left = daysBetween(day, end);
        mt.setMacro(new Macro(KW("day"), "01",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(day)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"),
                              QString().sprintf
                              ("%02d", quarterOfYear(day)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"),
                              QString().sprintf
                              ("%02d", weekOfYear(day, weekStartsMonday)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(day)),
                              defFileName, defFileLine));
        generateTitle(tci, monthAndYear(day));
    }
}

void
CSVReportElement::genHeadDaily2(TableCellInfo* tci)
{
    // Generates the 2nd header line for daily calendar views.
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t day = midnight(start); day < end; day = sameTimeNextDay(day))
    {
        int dom = dayOfMonth(day);
        mt.setMacro(new Macro(KW("day"), QString().sprintf("%02d", dom),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(day)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"),
                              QString().sprintf
                              ("%02d", quarterOfYear(day)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"),
                              QString().sprintf
                              ("%02d", weekOfYear(day, weekStartsMonday)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(day)),
                              defFileName, defFileLine));
        generateSubTitle(tci, QString().sprintf("%d", dom));
    }
}

void
CSVReportElement::genHeadWeekly1(TableCellInfo* tci)
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
             
        mt.setMacro(new Macro(KW("day"), QString().sprintf
                              ("%02d", dayOfMonth(week)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"),
                              QString().sprintf
                              ("%02d", monthOfWeek(week, weekStartsMonday)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"),
                              QString().sprintf
                              ("%02d", quarterOfYear(week)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"),
                              QString().sprintf
                              ("%02d", weekOfYear(week, weekStartsMonday)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf
                              ("%04d", yearOfWeek(week, weekStartsMonday)),
                              defFileName, defFileLine));
        generateTitle(tci, 
                      QString("%1 %2").arg(shortMonthName(monthOfWeek(week, 
                                                          weekStartsMonday) 
                                                          - 1)).
                      arg(yearOfWeek(week, weekStartsMonday)));
        week = wi;
    }
}

void
CSVReportElement::genHeadWeekly2(TableCellInfo* tci)
{
    // Generates the 2nd header line for weekly calendar views.
    bool wsm = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, wsm); week < end; 
         week = sameTimeNextWeek(week))
    {
        int woy = weekOfYear(week, wsm);
        mt.setMacro(new Macro(KW("day"), QString().sprintf
                              ("%02d", dayOfMonth(week)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", 
                                                monthOfWeek(week, wsm)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"),
                              QString().sprintf
                              ("%02d", quarterOfYear(week)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"),
                              QString().sprintf("%02d", woy),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", yearOfWeek(week, wsm)),
                              defFileName, defFileLine));
        generateSubTitle(tci, QString().sprintf("%d", woy));
    }
}

void
CSVReportElement::genHeadMonthly1(TableCellInfo* tci)
{
    // Generates 1st header line of monthly calendar view.
    for (time_t year = beginOfMonth(start); year < end;
         year = sameTimeNextYear(beginOfMonth(year)))
    {
        int left = monthLeftInYear(year);
        if (left > monthsBetween(year, end))
            left = monthsBetween(year, end);
        mt.setMacro(new Macro(KW("day"), "01",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"), "01",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"), "1",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"), "01",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", ::year(year)),
                              defFileName, defFileLine));
        generateTitle(tci, QString().sprintf("%d", ::year(year)));
    }
}

void
CSVReportElement::genHeadMonthly2(TableCellInfo* tci)
{
    // Generates 2nd header line of monthly calendar view.
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t month = beginOfMonth(start); month < end;
         month = sameTimeNextMonth(month))
    {
        int moy = monthOfYear(month);
        mt.setMacro(new Macro(KW("day"), "01",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", moy),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"),
                              QString().sprintf
                              ("%02d", quarterOfYear(month)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"),
                              QString().sprintf
                              ("%02d", weekOfYear(month, weekStartsMonday)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(month)),
                              defFileName, defFileLine));
        generateSubTitle(tci, shortMonthName(moy - 1));
    }
}

void
CSVReportElement::genHeadQuarterly1(TableCellInfo* tci)
{
    // Generates 1st header line of quarterly calendar view.
    for (time_t year = beginOfQuarter(start); year < end;
         year = sameTimeNextYear(beginOfQuarter(year)))
    {
        int left = quartersLeftInYear(year);
        if (left > quartersBetween(year, end))
            left = quartersBetween(year, end);
        mt.setMacro(new Macro(KW("day"), "01",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"), "01",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"), "1",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"), "01", 
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", ::year(year)),
                              defFileName, defFileLine));
        generateTitle(tci, QString().sprintf("%d", ::year(year)));
    }
}

void
CSVReportElement::genHeadQuarterly2(TableCellInfo* tci)
{
    // Generates 2nd header line of quarterly calendar view.
    static const char* qnames[] =
    {
        I18N_NOOP("1st Quarter"), I18N_NOOP("2nd Quarter"),
        I18N_NOOP("3rd Quarter"), I18N_NOOP("4th Quarter")
    };

    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t quarter = beginOfQuarter(start); quarter < end;
         quarter = sameTimeNextQuarter(quarter))
    {
        int qoy = quarterOfYear(quarter);
        mt.setMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(quarter)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(quarter)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"),
                              QString().sprintf("%d", qoy),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"),
                              QString().sprintf
                              ("%02d", weekOfYear(quarter, weekStartsMonday)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", year(quarter)),
                              defFileName, defFileLine));
        generateSubTitle(tci, i18n(qnames[qoy - 1]));
    }
}

void
CSVReportElement::genHeadYear(TableCellInfo* tci)
{
    // Generates 1st header line of monthly calendar view.
    for (time_t year = beginOfYear(start); year < end;
         year = sameTimeNextYear(year))
    {
        mt.setMacro(new Macro(KW("day"), QString().sprintf("%02d",
                                                           dayOfMonth(year)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("month"),
                              QString().sprintf("%02d", monthOfYear(year)),
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("quarter"), "1",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("week"), "01",
                              defFileName, defFileLine));
        mt.setMacro(new Macro(KW("year"),
                              QString().sprintf("%04d", ::year(year)),
                              defFileName, defFileLine));
        generateTitle(tci, QString().sprintf("%d", ::year(year)));
    }
}

void
CSVReportElement::genCellEmpty(TableCellInfo* tci)
{
    genCell("", tci, TRUE);
}

void
CSVReportElement::genCellSequenceNo(TableCellInfo* tci)
{
    genCell(tci->tli->ca2 == 0 ? 
            QString().sprintf("%d.", tci->tli->ca1->getSequenceNo()) :
            QString(""), tci, TRUE);
}

void
CSVReportElement::genCellNo(TableCellInfo* tci)
{
    genCell(tci->tli->ca2 == 0 ? QString().sprintf("%d.", tci->tli->idxNo) :
            QString(""), tci, TRUE);
}

void
CSVReportElement::genCellHierarchNo(TableCellInfo* tci)
{
    genCell(tci->tli->ca2 == 0 ?
            tci->tli->ca1->getHierarchNo() : QString(""), tci, TRUE);
}

void
CSVReportElement::genCellIndex(TableCellInfo* tci)
{
    genCell(tci->tli->ca2 == 0 ? 
            QString().sprintf("%d.", tci->tli->ca1->getIndex()) :
            QString(""), tci, TRUE);
}

void
CSVReportElement::genCellHierarchIndex(TableCellInfo* tci)
{
    genCell(tci->tli->ca2 == 0 ?
            tci->tli->ca1->getHierarchIndex() : QString(""), tci, TRUE);
}

void
CSVReportElement::genCellId(TableCellInfo* tci)
{
    genCell(tci->tli->ca1->getId(), tci, TRUE); 
}

void
CSVReportElement::genCellName(TableCellInfo* tci)
{
    int lPadding = 0;
    if ((tci->tli->ca2 && (tci->tli->ca2->getType() == CA_Resource &&
          resourceSortCriteria[0] == CoreAttributesList::TreeMode)) ||
        (tci->tli->ca2 && tci->tli->ca2->getType() == CA_Task &&
         taskSortCriteria[0] == CoreAttributesList::TreeMode))
        for (const CoreAttributes* cp = tci->tli->ca2 ; cp != 0;
             cp = cp->getParent())
            lPadding++;

    QString text;
    if (tci->tli->specialName.isNull())
    {
        if (tci->tli->task)
            mt.setMacro(new Macro(KW("taskid"), tci->tli->task->getId(), 
                                  defFileName, defFileLine));
        if (tci->tli->resource)
            mt.setMacro(new Macro(KW("resourceid"), tci->tli->resource->getId(),
                                  defFileName, defFileLine));
        if (tci->tli->account)
            mt.setMacro(new Macro(KW("accountid"), tci->tli->account->getId(),
                                  defFileName, defFileLine));

        if ((tci->tli->ca1->getType() == CA_Resource &&
             resourceSortCriteria[0] == CoreAttributesList::TreeMode) ||
            (tci->tli->ca1->getType() == CA_Task &&
             taskSortCriteria[0] == CoreAttributesList::TreeMode) ||
            (tci->tli->ca1->getType() == CA_Account &&
             accountSortCriteria[0] == CoreAttributesList::TreeMode))
        {
            lPadding += tci->tli->ca1->treeLevel();
        }
        text = QString().fill(' ', lPadding) + tci->tli->ca1->getName(); 
    }
    else
        text = tci->tli->specialName;
    genCell(text, tci, TRUE);
}

void
CSVReportElement::genCellStart(TableCellInfo* tci)
{
    if (!tci->tli->task->isStartOk(tci->tli->sc))
        tci->setBgColor(colors.getColor("error"));
    genCell(time2user(tci->tli->task->getStart(tci->tli->sc), timeFormat),
            tci, FALSE);
}

void
CSVReportElement::genCellEnd(TableCellInfo* tci)
{
    if (!tci->tli->task->isEndOk(tci->tli->sc))
        tci->setBgColor(colors.getColor("error"));
    genCell(time2user(tci->tli->task->getEnd(tci->tli->sc) +
                      (tci->tli->task->isMilestone() ? 1 : 0), timeFormat),
            tci, FALSE);
}

#define GCMMSE(a) \
void \
CSVReportElement::genCell##a(TableCellInfo* tci) \
{ \
    genCell(tci->tli->task->get##a(tci->tli->sc) == 0 ? QString() : \
            time2user(tci->tli->task->get##a(tci->tli->sc), timeFormat), \
            tci, FALSE); \
}

GCMMSE(MinStart)
GCMMSE(MaxStart)
GCMMSE(MinEnd)
GCMMSE(MaxEnd)

#define GCSEBUFFER(a) \
void \
CSVReportElement::genCell##a##Buffer(TableCellInfo* tci) \
{ \
    genCell(QString().sprintf \
            ("%3.0f", tci->tli->task->get##a##Buffer(tci->tli->sc)), \
            tci, FALSE); \
}

GCSEBUFFER(Start)
GCSEBUFFER(End)

void
CSVReportElement::genCellStartBufferEnd(TableCellInfo* tci)
{
    genCell(time2user(tci->tli->task->getStartBufferEnd
                         (tci->tli->sc), timeFormat), tci, FALSE);
}

void
CSVReportElement::genCellEndBufferStart(TableCellInfo* tci)
{
    genCell(time2user(tci->tli->task->getStartBufferEnd
                      (tci->tli->sc) + 1, timeFormat), tci, FALSE); 
}

void
CSVReportElement::genCellDuration(TableCellInfo* tci)
{
    genCell(scaledLoad(tci->tli->task->getCalcDuration(tci->tli->sc), tci),
            tci, FALSE);
}

void
CSVReportElement::genCellEffort(TableCellInfo* tci)
{
    double val = 0.0;
    if (tci->tli->ca1->getType() == CA_Task)
    {
        val = tci->tli->task->getLoad(tci->tli->sc, Interval(start, end),
                                      tci->tli->resource);
    }
    else if (tci->tli->ca1->getType() == CA_Resource)
    {
        val = tci->tli->resource->getLoad(tci->tli->sc, Interval(start, end), 
                                          AllAccounts, tci->tli->task);
    }
    
    generateRightIndented(tci, scaledLoad(val, tci));
}

void
CSVReportElement::genCellProjectId(TableCellInfo* tci)
{
    genCell(tci->tli->task->getProjectId() + " (" +
            report->getProject()->getIdIndex(tci->tli->task->
                                             getProjectId()) + ")", tci,
            TRUE);
}

void
CSVReportElement::genCellResources(TableCellInfo* tci)
{
    QString text;
    for (ResourceListIterator rli(tci->tli->task->
                                  getBookedResourcesIterator(tci->tli->sc));
        *rli != 0; ++rli)
    {
        if (!text.isEmpty())
            text += ", ";
                    
        text += (*rli)->getName();
    }
    genCell(text, tci, FALSE);
}

void
CSVReportElement::genCellResponsible(TableCellInfo* tci)
{
    if (tci->tli->task->getResponsible())
        genCell(tci->tli->task->getResponsible()->getName(),
                tci, TRUE);
    else
        genCell("", tci, TRUE);
}

void
CSVReportElement::genCellText(TableCellInfo* tci)
{
    if (tci->tcf->getId() == "note")
    {
        if (tci->tli->task->getNote().isEmpty())
            genCell("", tci, TRUE);
        else
            genCell(tci->tli->task->getNote(), tci, TRUE);
        return;
    }
   
    const TextAttribute* ta = (const TextAttribute*) 
        tci->tli->ca1->getCustomAttribute(tci->tcf->getId());
    if (!ta || ta->getText().isEmpty())
        genCell("", tci, TRUE);
    else
        genCell(ta->getText(), tci, TRUE);
}

void
CSVReportElement::genCellStatusNote(TableCellInfo* tci)
{
    if (tci->tli->task->getStatusNote(tci->tli->sc).isEmpty())
        genCell("", tci, TRUE);
    else
        genCell(tci->tli->task->getStatusNote(tci->tli->sc),
                tci, TRUE);
}

void
CSVReportElement::genCellCost(TableCellInfo* tci)
{
    double val = 0.0;
    if (tci->tli->ca1->getType() == CA_Task)
    {
        val = tci->tli->task->getCredits(tci->tli->sc, Interval(start, end),
                                       Cost, tci->tli->resource);
    }
    else if (tci->tli->ca1->getType() == CA_Resource)
    {
        val = tci->tli->resource->getCredits(tci->tli->sc, Interval(start, end),
                                        Cost, tci->tli->task);
    }
    generateRightIndented(tci, tci->tcf->realFormat.format(val, tci)); 
}

void
CSVReportElement::genCellRevenue(TableCellInfo* tci)
{
    double val = 0.0;
    if (tci->tli->ca1->getType() == CA_Task)
    {
        val = tci->tli->task->getCredits(tci->tli->sc, Interval(start, end),
                                       Revenue, tci->tli->resource);
    }
    else if (tci->tli->ca1->getType() == CA_Resource)
    {
        val = tci->tli->resource->getCredits(tci->tli->sc, Interval(start, end),
                                        Revenue, tci->tli->task);
    }
    generateRightIndented(tci, tci->tcf->realFormat.format(val, tci)); 
}

void
CSVReportElement::genCellProfit(TableCellInfo* tci)
{
    double val = 0.0;
    if (tci->tli->ca1->getType() == CA_Task)
    {
        val = tci->tli->task->getCredits(tci->tli->sc, Interval(start, end),
                                    Revenue, tci->tli->resource) - 
            tci->tli->task->getCredits(tci->tli->sc, Interval(start, end),
                                  Cost, tci->tli->resource);
    }
    else if (tci->tli->ca1->getType() == CA_Resource)
    {
        val = tci->tli->resource->getCredits(tci->tli->sc, Interval(start, end),
                                        Revenue, tci->tli->task) -
            tci->tli->resource->getCredits(tci->tli->sc, Interval(start, end),
                                      Cost, tci->tli->task);
    }
    generateRightIndented(tci, tci->tcf->realFormat.format(val, tci)); 
}

void
CSVReportElement::genCellPriority(TableCellInfo* tci)
{
    genCell(QString().sprintf("%d", tci->tli->task->getPriority()),
            tci, TRUE);
}

void
CSVReportElement::genCellFlags(TableCellInfo* tci)
{
    FlagList allFlags = tci->tli->ca1->getFlagList();
    QString flagStr;
    for (QStringList::Iterator it = allFlags.begin();
         it != allFlags.end(); ++it)
    {
        if (it != allFlags.begin())
            flagStr += ", ";
        flagStr += *it;
    }
    genCell(flagStr, tci, TRUE);
}

void
CSVReportElement::genCellCompleted(TableCellInfo* tci)
{
    if (tci->tli->task->getCompletionDegree(tci->tli->sc) ==
        tci->tli->task->getCalcedCompletionDegree(tci->tli->sc))
    {
        genCell(QString("%1%").arg((int) tci->tli->task->
                                   getCompletionDegree(tci->tli->sc)),
                tci, FALSE);
    }
    else
    {
        genCell(QString("%1% (%2%)")
                .arg((int) tci->tli->task->getCompletionDegree(tci->tli->sc))
                .arg((int) tci->tli->task->
                     getCalcedCompletionDegree(tci->tli->sc)),
                tci, FALSE);
    }
}

void
CSVReportElement::genCellStatus(TableCellInfo* tci)
{
    QString text;
    switch (tci->tli->task->getStatus(tci->tli->sc))
    {
    case NotStarted:
        text = i18n("Not yet started");
        break;
    case InProgressLate:
        text = i18n("Behind schedule");
        break;
    case InProgress:
        text = i18n("Work in progress");
        break;
    case OnTime:
        text = i18n("On schedule");
        break;
    case InProgressEarly:
        text = i18n("Ahead of schedule");
        break;
    case Finished:
        text = i18n("Finished");
        break;
    default:
        text = i18n("Unknown status");
        break;
    }
    genCell(text, tci, FALSE);
}

void
CSVReportElement::genCellReference(TableCellInfo* tci)
{
    if (tci->tcf->getId() == "reference")
    {
        if (tci->tli->task->getReference().isEmpty())
            genCell("", tci, TRUE);
        else
        {
            QString text =tci->tli->task->getReference();
            if (tci->tli->task->getReferenceLabel().isEmpty())
                text += filter(tci->tli->task->getReference());
            else
                text += filter(tci->tli->task->getReferenceLabel());
            genCell(text, tci, TRUE, FALSE);
        }
        return;
    }
   
    const ReferenceAttribute* ra =  (const ReferenceAttribute*)
        tci->tli->ca1->getCustomAttribute(tci->tcf->getId());
    if (!ra || ra->getUrl().isEmpty())
        genCell("", tci, TRUE);
    else
    {
        QString text = ra->getUrl();
        if (ra->getLabel().isEmpty())
            text += filter(ra->getUrl());
        else
            text += filter(ra->getLabel());
        genCell(text, tci, TRUE, FALSE);
    }
}

void
CSVReportElement::genCellScenario(TableCellInfo* tci)
{
    genCell(report->getProject()->getScenarioName(tci->tli->sc), tci, FALSE);
}

#define GCDEPFOL(a, b) \
void \
CSVReportElement::genCell##a(TableCellInfo* tci) \
{ \
    QString text; \
    for (TaskListIterator it(tci->tli->task->get##b##Iterator()); *it != 0; \
         ++it) \
    { \
        if (!text.isEmpty()) \
            text += ", "; \
        text += (*it)->getId(); \
    } \
    genCell(text, tci, TRUE); \
}

GCDEPFOL(Depends, Previous)
GCDEPFOL(Follows, Followers)

QColor
CSVReportElement::selectTaskBgColor(TableCellInfo* tci, double load,
                                     const Interval& period, bool daily)
{
    QColor bgCol;
    if (tci->tli->task->isActive(tci->tli->sc, period) &&
        ((tci->tli->resource != 0 && load > 0.0) || tci->tli->resource == 0))
    { 
        if (tci->tli->task->isCompleted(tci->tli->sc, period.getEnd() - 1))
        {
            if (tci->tli->ca2 == 0)
                bgCol = colors.getColor("completed");
            else
                bgCol = colors.getColor("completed").light(130);
        }
        else 
        {
            if (tci->tli->ca2 == 0 && 
                !tci->tli->task->isBuffer(tci->tli->sc, period)) 
            {
                bgCol = colors.getColor("booked");
            }
            else
            {
                bgCol = colors.getColor("booked").light(130);
            }
        } 
    }
    else if (period.contains(report->getProject()->getNow()))
    {
        bgCol = colors.getColor("today");
    }
    else if (daily && (isWeekend(period.getStart()) ||
                       report->getProject()->isVacation(period.getStart())))
    {
            bgCol = colors.getColor("vacation");
    }

    return bgCol;
}

QColor
CSVReportElement::selectResourceBgColor(TableCellInfo* tci, double load,
                                         const Interval& period, bool daily)
{
    QColor bgCol;
    if (load > tci->tli->resource->getMinEffort() * 
            tci->tli->resource->getEfficiency())
    {
        if (tci->tli->ca2 == 0)
        {
            bgCol = colors.getColor("booked");
        }
        else
        {
            if (tci->tli->task->isCompleted(tci->tli->sc, period.getEnd() - 1))
                bgCol = colors.getColor("completed").light(130);
            else
                bgCol = colors.getColor("booked").light(130);
        }
    }
    else if (period.contains(report->getProject()->getNow()))
    {
        bgCol = colors.getColor("today");
    }
    else if (daily && (isWeekend(period.getStart()) ||
                       report->getProject()->isVacation(period.getStart()) ||
                       tci->tli->resource->hasVacationDay(period.getStart())))
    {
            bgCol = colors.getColor("vacation");
    }

    return bgCol;
}

void
CSVReportElement::genCellTaskFunc(TableCellInfo* tci, bool daily,
                                   time_t (*beginOfT)(time_t),
                                   time_t (*sameTimeNextT)(time_t))
{
    for (time_t t = (*beginOfT)(start); t < end; t = (*sameTimeNextT)(t))
    {
        Interval period = Interval(t, sameTimeNextT(t) - 1);
        double load = tci->tli->task->getLoad(tci->tli->sc, period, 
                                              tci->tli->resource);
        QColor bgCol = selectTaskBgColor(tci, load, period, daily);

        int runLength = 1;
        if (!tci->tli->task->isActive(tci->tli->sc, period))
        {
            time_t lastEndT = t;
            for (time_t endT = sameTimeNextT(t); endT < end;
                 endT = sameTimeNextT(endT))
            {
                Interval periodProbe = Interval(endT, sameTimeNextT(endT) - 1);
                double loadProbe = tci->tli->task->getLoad(tci->tli->sc,
                                                           periodProbe, 
                                                           tci->tli->resource);
                QColor bgColProbe = selectTaskBgColor(tci, loadProbe,
                                                      periodProbe, daily);
                if (load != loadProbe || bgCol != bgColProbe)
                    break;
                lastEndT = endT;
                runLength++;
            }
            t = lastEndT;
        }
        tci->setColumns(runLength);
        tci->setBgColor(bgCol);
        
        reportTaskLoad(load, tci, period);
    }
}

void
CSVReportElement::genCellResourceFunc(TableCellInfo* tci, bool daily,
                                   time_t (*beginOfT)(time_t),
                                   time_t (*sameTimeNextT)(time_t))
{
    for (time_t t = beginOfT(start); t < end; t = sameTimeNextT(t))
    {
        Interval period = Interval(t, sameTimeNextT(t) - 1);
        double load = tci->tli->resource->getLoad(tci->tli->sc, period,
                                                  AllAccounts, tci->tli->task);
        QColor bgCol = selectResourceBgColor(tci, load, period, daily); 

        int runLength = 1;
        if (load == 0.0)
        {
            time_t lastEndT = t;
            for (time_t endT = sameTimeNextT(t); endT < end;
                 endT = sameTimeNextT(endT))
            {
                Interval periodProbe = Interval(endT, sameTimeNextT(endT) - 1);
                double loadProbe = 
                    tci->tli->resource->getLoad(tci->tli->sc, periodProbe,
                                                AllAccounts, tci->tli->task);
                QColor bgColProbe = selectResourceBgColor(tci, loadProbe,
                                                          periodProbe, daily);

                if (load != loadProbe || bgCol != bgColProbe)
                    break;
                lastEndT = endT;
                runLength++;
            }
            t = lastEndT;
        }
        tci->setColumns(runLength);
        tci->setBgColor(bgCol);
        
        reportResourceLoad(load, tci, period);
    }
}

void
CSVReportElement::genCellDailyTask(TableCellInfo* tci)
{
    genCellTaskFunc(tci, TRUE, midnight, sameTimeNextDay);
}

void
CSVReportElement::genCellDailyResource(TableCellInfo* tci)
{
    genCellResourceFunc(tci, TRUE, midnight, sameTimeNextDay);
}

void
CSVReportElement::genCellDailyAccount(TableCellInfo* tci)
{
    for (time_t day = midnight(start); day < end;
         day = sameTimeNextDay(day))
    {
        double volume = tci->tli->account->getVolume(tci->tli->sc, 
                                                     Interval(day).firstDay());
        if ((accountSortCriteria[0] == CoreAttributesList::TreeMode &&
             tci->tli->account->isRoot()) ||
            (accountSortCriteria[0] != CoreAttributesList::TreeMode)) 
            tci->tci->addToSum(tci->tli->sc, time2ISO(day), volume);
        reportCurrency(volume, tci, day);
    }
}

void
CSVReportElement::genCellWeeklyTask(TableCellInfo* tci)
{
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, weekStartsMonday); week < end;
         week = sameTimeNextWeek(week))
    {
        Interval period = Interval(week).firstWeek(weekStartsMonday);
        double load = tci->tli->task->getLoad(tci->tli->sc, period, 
                                              tci->tli->resource);
        QColor bgCol = selectTaskBgColor(tci, load, period, FALSE);

        int runLength = 1;
        if (!tci->tli->task->isActive(tci->tli->sc, period))
        {
            time_t lastEndWeek = week;
            for (time_t endWeek = sameTimeNextWeek(week); endWeek < end;
                 endWeek = sameTimeNextWeek(endWeek))
            {
                Interval periodProbe = Interval(endWeek)
                    .firstWeek(weekStartsMonday);
                double loadProbe = tci->tli->task->getLoad(tci->tli->sc,
                                                           periodProbe, 
                                                           tci->tli->resource);
                QColor bgColProbe = selectTaskBgColor(tci, loadProbe,
                                                      periodProbe, FALSE);
                if (load != loadProbe || bgCol != bgColProbe)
                    break;
                lastEndWeek = endWeek;
                runLength++;
            }
            week = lastEndWeek;
        }
        tci->setColumns(runLength);
        tci->setBgColor(bgCol);

        reportTaskLoad(load, tci, period);
    }
}

void
CSVReportElement::genCellWeeklyResource(TableCellInfo* tci)
{
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, weekStartsMonday); week < end;
         week = sameTimeNextWeek(week))
    {
        Interval period = Interval(week).firstWeek(weekStartsMonday);
        double load = tci->tli->resource->getLoad(tci->tli->sc, period,
                                                  AllAccounts, tci->tli->task);
        QColor bgCol = selectResourceBgColor(tci, load, period, FALSE);

        int runLength = 1;
        if (load == 0.0)
        {
            time_t lastEndWeek = week;
            for (time_t endWeek = sameTimeNextWeek(week); endWeek < end;
                 endWeek = sameTimeNextWeek(endWeek))
            {
                Interval periodProbe = Interval(endWeek)
                    .firstWeek(weekStartsMonday);
                double loadProbe = 
                    tci->tli->resource->getLoad(tci->tli->sc, periodProbe,
                                                AllAccounts, tci->tli->task);
                QColor bgColProbe = selectResourceBgColor(tci, loadProbe,
                                                          periodProbe, FALSE);
                if (load != loadProbe || bgCol != bgColProbe)
                    break;
                lastEndWeek = endWeek;
                runLength++;
            }
            week = lastEndWeek;
        }
        tci->setColumns(runLength); 
        tci->setBgColor(bgCol);
        
        reportResourceLoad(load, tci, period);
    }
}

void
CSVReportElement::genCellWeeklyAccount(TableCellInfo* tci)
{
    bool weekStartsMonday = report->getWeekStartsMonday();
    for (time_t week = beginOfWeek(start, weekStartsMonday); week < end;
         week = sameTimeNextWeek(week))
    {
        double volume = tci->tli->account->
            getVolume(tci->tli->sc, Interval(week).firstWeek(weekStartsMonday));
        if ((accountSortCriteria[0] == CoreAttributesList::TreeMode && 
             tci->tli->account->isRoot()) ||
            (accountSortCriteria[0] != CoreAttributesList::TreeMode)) 
            tci->tci->addToSum(tci->tli->sc, time2ISO(week), volume);
        reportCurrency(volume, tci, week);
    }
}

void
CSVReportElement::genCellMonthlyTask(TableCellInfo* tci)
{
    genCellTaskFunc(tci, FALSE, beginOfMonth, sameTimeNextMonth);
}

void
CSVReportElement::genCellMonthlyResource(TableCellInfo* tci)
{
    genCellResourceFunc(tci, FALSE, beginOfMonth, sameTimeNextMonth);   
}

void
CSVReportElement::genCellMonthlyAccount(TableCellInfo* tci)
{
    tci->tcf->realFormat = currencyFormat;
    for (time_t month = beginOfMonth(start); month < end;
         month = sameTimeNextMonth(month))
    {
        double volume = tci->tli->account->
            getVolume(tci->tli->sc, Interval(month).firstMonth());
        if ((accountSortCriteria[0] == CoreAttributesList::TreeMode && 
             tci->tli->account->isRoot()) ||
            (accountSortCriteria[0] != CoreAttributesList::TreeMode)) 
            tci->tci->addToSum(tci->tli->sc, time2ISO(month), volume);
        reportCurrency(volume, tci, month);
    }
}

void
CSVReportElement::genCellQuarterlyTask(TableCellInfo* tci)
{
    genCellTaskFunc(tci, FALSE, beginOfQuarter, sameTimeNextQuarter);
}

void
CSVReportElement::genCellQuarterlyResource(TableCellInfo* tci)
{
    genCellResourceFunc(tci, FALSE, beginOfQuarter, sameTimeNextQuarter);
}

void
CSVReportElement::genCellQuarterlyAccount(TableCellInfo* tci)
{
    for (time_t quarter = beginOfQuarter(start); quarter < end;
         quarter = sameTimeNextQuarter(quarter))
    {
        double volume = tci->tli->account->
            getVolume(tci->tli->sc, Interval(quarter).firstQuarter());
        if ((accountSortCriteria[0] == CoreAttributesList::TreeMode && 
             tci->tli->account->isRoot()) ||
            (accountSortCriteria[0] != CoreAttributesList::TreeMode)) 
            tci->tci->addToSum(tci->tli->sc, time2ISO(quarter), volume);
        reportCurrency(volume, tci, quarter);
    }
}

void
CSVReportElement::genCellYearlyTask(TableCellInfo* tci)
{
    genCellTaskFunc(tci, FALSE, beginOfYear, sameTimeNextYear);
}

void
CSVReportElement::genCellYearlyResource(TableCellInfo* tci)
{
    genCellResourceFunc(tci, FALSE, beginOfYear, sameTimeNextYear);
}

void
CSVReportElement::genCellYearlyAccount(TableCellInfo* tci)
{
    for (time_t year = beginOfYear(start); year < end;
         year = sameTimeNextYear(year))
    {
        double volume = tci->tli->account->
            getVolume(tci->tli->sc, Interval(year).firstYear());
        if ((accountSortCriteria[0] == CoreAttributesList::TreeMode && 
             tci->tli->account->isRoot()) ||
            (accountSortCriteria[0] != CoreAttributesList::TreeMode)) 
            tci->tci->addToSum(tci->tli->sc, time2ISO(year), volume);
        reportCurrency(volume, tci, year);
    }
}

void
CSVReportElement::genCellResponsibilities(TableCellInfo* tci)
{
    QString text;
    for (TaskListIterator it(report->getProject()->getTaskListIterator());
         *it != 0; ++it)
    {
        if ((*it)->getResponsible() == tci->tli->resource)
        {
            if (!text.isEmpty())
                text += ", ";
            text += (*it)->getId();
        }
    }
    genCell(text, tci, TRUE);
}

void
CSVReportElement::genCellSchedule(TableCellInfo*)
{
}

#define GCEFFORT(a) \
void \
CSVReportElement::genCell##a##Effort(TableCellInfo* tci) \
{ \
    genCell(tci->tcf->realFormat.format \
            (tci->tli->resource->get##a##Effort(), FALSE), \
            tci, TRUE); \
}

GCEFFORT(Min)
GCEFFORT(Max)

void
CSVReportElement::genCellRate(TableCellInfo* tci)
{
    genCell(tci->tcf->realFormat.format(tci->tli->resource->getRate(), tci),
            tci, TRUE);
}

void
CSVReportElement::genCellKotrusId(TableCellInfo* tci)
{
    genCell(tci->tli->resource->getKotrusId(), tci, TRUE);
}

void
CSVReportElement::genCellTotal(TableCellInfo* tci)
{
    double value = tci->tli->account->getVolume(tci->tli->sc, 
                                                Interval(start, end));
    if (tci->tli->account->isLeaf())
        tci->tci->addToSum(tci->tli->sc, "total", value);
    genCell(tci->tcf->realFormat.format(value, tci), tci, FALSE);
}

void
CSVReportElement::genCellSummary(TableCellInfo* tci)
{
    QMap<QString, double>::ConstIterator it;
    const QMap<QString, double>* sum = tci->tci->getSum();
    if (sum)
    {
        uint sc = tci->tli->sc;
        double val = 0.0;
        for (it = sum[sc].begin(); it != sum[sc].end(); ++it)
        {
            if (accumulate)
                val += *it;
            else
                val = *it;
            genCell(tci->tcf->realFormat.format(val, tci), tci, FALSE);
        }
    }
    else
        genCell("", tci, FALSE);
}
