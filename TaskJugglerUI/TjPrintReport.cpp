/*
 * The TaskJuggler Project Management Software
 *
 * Copyright (c) 2001, 2002, 2003, 2004, 2005 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id: taskjuggler.cpp 1085 2005-06-23 20:34:54Z cs $
 */

#include "TjPrintReport.h"

#include <config.h>
#include <assert.h>
#include <memory>

#include <qptrlist.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qfontmetrics.h>

#include <klocale.h>

#include "Report.h"
#include "QtReportElement.h"
#include "QtReport.h"
#include "TableColumnInfo.h"
#include "TableColumnFormat.h"
#include "ReportElement.h"
#include "Project.h"
#include "Task.h"
#include "Resource.h"
#include "TjReportCell.h"
#include "TjReportRow.h"
#include "TjReportColumn.h"
#include "TextAttribute.h"
#include "ReferenceAttribute.h"
#include "TjGanttChart.h"
#include "TjObjPosTable.h"
#include "UsageLimits.h"

TjPrintReport::TjPrintReport(const Report* rd, KPrinter* pr) :
    reportDef(rd), printer(pr)
{
    ganttChartObj = new QObject();
    ganttChart = new TjGanttChart(ganttChartObj);

    objPosTable = 0;

    showGantt = false;

    // Initialize the geometry variables with 0. This makes it a bit easier to
    // detect programming bugs.
    leftMargin = topMargin = pageWidth = pageHeight = 0;
    cellMargin = 0;
    headlineHeight = 0;
    tableRight = tableBottom = 0;

    // Set the sizes of all used fonts. We always use the default font.
    standardFont.setPixelSize(pointsToYPixels(7));
    tableHeaderFont.setPixelSize(pointsToYPixels(7));
    headlineFont.setPixelSize(pointsToYPixels(12));
    signatureFont.setPixelSize(pointsToYPixels(4));

    // Size of the indentation steps for nested attributes.
    indentSteps = mmToXPixels(3);

    // List of columns that are not generated by the generic routines.
    specialColumns.insert("hourly");
    specialColumns.insert("daily");
    specialColumns.insert("weekly");
    specialColumns.insert("monthly");
    specialColumns.insert("quarterly");
    specialColumns.insert("yearly");
}

TjPrintReport::~TjPrintReport()
{
    for (unsigned int i = 0; i < rows.size(); i++)
        delete rows[i];
    rows.clear();
    for (unsigned i = 0; i < columns.size(); i++)
        delete columns[i];
    columns.clear();

    delete ganttChartObj;
    delete ganttChart;
    delete objPosTable;
}

void
TjPrintReport::getNumberOfPages(int& xPages, int& yPages)
{
    assert(!columns.empty());
    xPages = columns.back()->getXPage() + 1;
    assert(!rows.empty());
    yPages = rows.back()->getYPage() + 1;
}

void
TjPrintReport::generateTableHeader()
{
    for (QPtrListIterator<TableColumnInfo>
         ci = reportElement->getColumnsIterator(); *ci; ++ci)
    {
        /* The calendar columns need special treatment. We just ignore them
         * for now. */
        if (specialColumns.find((*ci)->getName()) != specialColumns.end())
        {
            /* No matter how many calendar columns the user has specified. We
             * them all. */
            continue;
        }
        else if ((*ci)->getName() == "chart")
        {
            showGantt = true;
            continue;
        }

        TjReportColumn* col = new TjReportColumn;
        const TableColumnFormat* tcf =
            reportElement->getColumnFormat((*ci)->getName());
        col->setTableColumnFormat(tcf);

        if (!(*ci)->getTitle().isEmpty())
            col->setTitle((*ci)->getTitle());
        else
            col->setTitle(tcf->getTitle());

        if (tcf->getIndent())
        {
            /* This header should be generic for task and resource reports. We
             * need some clever magic code to find out wheter we are nesting
             * tasks in resources, resources in tasks or only have tasks or
             * resources. */
            int maxIndentLevel = 1;
            if (((tcf->genTaskLine1 &&
                  tcf->genTaskLine1 != &ReportElement::genCellEmpty) &&
                 tcf->genResourceLine2) ||
                ((tcf->genResourceLine1 &&
                  tcf->genResourceLine1 != &ReportElement::genCellEmpty) &&
                 tcf->genTaskLine2))
            {
                // Resources nested in tasks or vice versa
                maxIndentLevel = maxDepthTaskList + maxDepthResourceList;
            }
            else if ((tcf->genTaskLine1 &&
                      tcf->genTaskLine1 != &ReportElement::genCellEmpty) &&
                     !tcf->genResourceLine2)
            {
                // Just tasks
                maxIndentLevel = maxDepthTaskList;
            }
            else if ((tcf->genResourceLine1 &&
                      tcf->genResourceLine1 != &ReportElement::genCellEmpty) &&
                     !tcf->genTaskLine2)
            {
                // Just resources
                maxIndentLevel = maxDepthResourceList;
            }
            col->setMaxIndentLevel(maxIndentLevel);
        }

        columns.push_back(col);
    }

    if (showGantt)
    {
        TjReportColumn* col = new TjReportColumn;
        col->setIsGantt(TRUE);
        columns.push_back(col);
    }
}

void
TjPrintReport::generateTaskListRow(TjReportRow* row, const Task* task,
                                   const Resource* resource)
{
    int colIdx= 0;
    for (QPtrListIterator<TableColumnInfo>
         ci = reportElement->getColumnsIterator(); *ci; ++ci, ++colIdx)
    {
        /* The calendar columns need special treatment. We just ignore them
         * for now. */
        if (specialColumns.find((*ci)->getName()) != specialColumns.end() ||
            (*ci)->getName() == "chart")
        {
            colIdx--;
            continue;
        }

        QString cellText;
        TjReportCell* cell = new TjReportCell(row, columns[colIdx]);
        const TableColumnFormat* tcf =
            reportElement->getColumnFormat((*ci)->getName());

        /* Determine whether the cell content should be indented. And if so,
         * then on what level. */
        if (tcf->getIndent())
        {
            int indentLevel = 0;

            if (reportElement->getTaskSorting(0) ==
                CoreAttributesList::TreeMode)
                indentLevel += task->treeLevel() -
                    reportElement->taskRootLevel();

            if (resource && reportElement->getResourceSorting(0) ==
                CoreAttributesList::TreeMode)
                indentLevel += maxDepthResourceList;

            cell->setIndentLevel(indentLevel);
        }

        if ((*ci)->getName() == "completed")
        {
            if (task->getCompletionDegree(scenario) ==
                task->getCalcedCompletionDegree(scenario))
            {
                cellText = QString("%1%")
                    .arg((int) task->getCompletionDegree(scenario));
            }
            else
            {
                cellText = QString("%1% (%2%)")
                    .arg((int) task->getCompletionDegree(scenario))
                    .arg((int) task->getCalcedCompletionDegree(scenario));
            }
        }
        else if ((*ci)->getName() == "cost")
        {
            double val = task->getCredits
                (scenario, Interval(reportElement->getStart(),
                                    reportElement->getEnd()), Cost, resource);
            cellText = tcf->realFormat.format(val, FALSE);
        }
        else if ((*ci)->getName() == "criticalness")
        {
            cellText = QString().sprintf("%f", task->getCriticalness(scenario));
        }
        else if ((*ci)->getName() == "depends")
        {
            for (TaskListIterator it(task->getPreviousIterator()); *it != 0;
                 ++it)
            {
                if (!cellText.isEmpty())
                    cellText += ", ";
                cellText += (*it)->getId();
            }
        }
        else if ((*ci)->getName() == "duration")
            cellText = reportElement->scaledLoad
                (task->getCalcDuration(scenario), tcf->realFormat);
        else if ((*ci)->getName() == "effort")
        {
            double val = task->getLoad(scenario,
                                       Interval(task->getStart(scenario),
                                                task->getEnd(scenario)),
                                       resource);
            cellText = reportElement->scaledLoad (val, tcf->realFormat);
        }
        else if ((*ci)->getName() == "end")
            cellText = time2user((task->isMilestone() ? 1 : 0) +
                                 task->getEnd(scenario),
                                 reportElement->getTimeFormat());
        else if ((*ci)->getName() == "endbuffer")
            cellText.sprintf("%3.0f", task->getEndBuffer(scenario));
        else if ((*ci)->getName() == "endbufferstart")
            cellText = time2user(task->getEndBufferStart(scenario),
                                 reportElement->getTimeFormat());
        else if ((*ci)->getName() == "follows")
        {
            for (TaskListIterator it(task->getFollowersIterator()); *it != 0;
                 ++it)
            {
                if (!cellText.isEmpty())
                    cellText += ", ";
                cellText += (*it)->getId();
            }
        }
        else if ((*ci)->getName() == "hierarchindex")
        {
            if (!resource)
                cellText = task->getHierarchIndex();
        }
        else if ((*ci)->getName() == "hierarchno")
        {
            if (!resource)
                cellText = task->getHierarchNo();
        }
        else if ((*ci)->getName() == "id")
            cellText = task->getId();
        else if ((*ci)->getName() == "index")
        {
            if (!resource)
                cellText.sprintf("%d.", task->getIndex());
        }
        else if ((*ci)->getName() == "maxend")
            cellText = time2user(task->getMaxEnd(scenario),
                                 reportElement->getTimeFormat());
        else if ((*ci)->getName() == "maxstart")
            cellText = time2user(task->getMaxStart(scenario),
                                 reportElement->getTimeFormat());
        else if ((*ci)->getName() == "minend")
            cellText = time2user(task->getMinEnd(scenario),
                                 reportElement->getTimeFormat());
        else if ((*ci)->getName() == "minstart")
            cellText = time2user(task->getMinStart(scenario),
                                 reportElement->getTimeFormat());
        else if ((*ci)->getName() == "name")
            cellText = task->getName();
        else if ((*ci)->getName() == "no")
            cellText.sprintf("%d", row->getIndex());
        else if ((*ci)->getName() == "note" && !task->getNote().isEmpty())
            cellText = task->getNote();
        else if ((*ci)->getName() == "pathcriticalness")
            cellText =
                QString().sprintf("%f", task->getPathCriticalness(scenario));
        else if ((*ci)->getName() == "priority")
            cellText = QString().sprintf("%d", task->getPriority());
        else if ((*ci)->getName() == "projectid")
            cellText = task->getProjectId() + " (" +
                reportElement->getReport()->getProject()->getIdIndex
                (task->getProjectId()) + ")";
        else if ((*ci)->getName() == "profit")
        {
            double val = task->getCredits
                (scenario, Interval(reportElement->getStart(),
                                    reportElement->getEnd()), Revenue,
                 resource) - task->getCredits
                (scenario, Interval(reportElement->getStart(),
                                    reportElement->getEnd()), Cost, resource);
            cellText = tcf->realFormat.format(val, FALSE);
        }
        else if ((*ci)->getName() == "resources")
        {
            for (ResourceListIterator rli
                 (task->getBookedResourcesIterator(scenario)); *rli != 0; ++rli)
            {
                if (!cellText.isEmpty())
                    cellText += ", ";

                cellText += (*rli)->getName();
            }
        }
        else if ((*ci)->getName() == "responsible")
        {
            if (task->getResponsible())
                cellText = task->getResponsible()->getName();
        }
        else if ((*ci)->getName() == "revenue")
        {
            double val = task->getCredits
                (scenario, Interval(reportElement->getStart(),
                                    reportElement->getEnd()), Revenue,
                 resource);
            cellText = tcf->realFormat.format(val, FALSE);
        }
        else if ((*ci)->getName() == "seqno")
        {
            if (!resource)
                cellText.sprintf("%d.", task->getSequenceNo());
        }
        else if ((*ci)->getName() == "start")
            cellText = time2user(task->getStart(scenario),
                                 reportElement->getTimeFormat());
        else if ((*ci)->getName() == "startbuffer")
            cellText.sprintf("%3.0f", task->getStartBuffer(scenario));
        else if ((*ci)->getName() == "startbufferend")
            cellText = time2user(task->getStartBufferEnd(scenario),
                                 reportElement->getTimeFormat());
        else if ((*ci)->getName() == "status")
            cellText = task->getStatusText(scenario);
        else if ((*ci)->getName() == "statusnote")
            cellText = task->getStatusNote(scenario);
        else
            generateCustomAttribute(task, (*ci)->getName(), cellText);

        cell->setText(cellText);
        row->insertCell(cell, colIdx);
    }
}

void
TjPrintReport::generateResourceListRow(TjReportRow* row,
                                       const Resource* resource,
                                       const Task* task)
{
    int colIdx= 0;
    for (QPtrListIterator<TableColumnInfo>
         ci = reportElement->getColumnsIterator(); *ci; ++ci, ++colIdx)
    {
        /* The calendar columns need special treatment. We just ignore them
         * for now. */
        if (specialColumns.find((*ci)->getName()) != specialColumns.end() ||
            (*ci)->getName() == "chart")
        {
            colIdx--;
            continue;
        }

        QString cellText;
        TjReportCell* cell = new TjReportCell(row, columns[colIdx]);
        const TableColumnFormat* tcf =
            reportElement->getColumnFormat((*ci)->getName());

        /* Determine whether the cell content should be indented. And if so,
         * then on what level. */
        if (tcf->getIndent())
        {
            int indentLevel = 0;

            if (reportElement->getResourceSorting(0) ==
                CoreAttributesList::TreeMode)
                indentLevel += resource->treeLevel();

            if (task != 0 && reportElement->getTaskSorting(0) ==
                CoreAttributesList::TreeMode)
                indentLevel += maxDepthTaskList;

            cell->setIndentLevel(indentLevel);
        }

        if ((*ci)->getName() == "cost")
        {
            double val = resource->getCredits
                (scenario, Interval(reportElement->getStart(),
                                    reportElement->getEnd()), Cost, task);
            cellText = tcf->realFormat.format(val, FALSE);
        }
        else if ((*ci)->getName() == "efficiency")
        {
            cellText = QString().sprintf("%.1lf", resource->getEfficiency());
        }
        else if ((*ci)->getName() == "effort")
        {
            double val = 0.0;
            if (task)
                val = resource->getLoad(scenario,
                                        Interval(task->getStart(scenario),
                                                 task->getEnd(scenario)),
                                        AllAccounts, task);
            else
                val = resource->getLoad(scenario,
                                        Interval(reportElement->getStart(),
                                                 reportElement->getEnd()));
            cellText = reportElement->scaledLoad(val, tcf->realFormat);
        }
        else if ((*ci)->getName() == "freeload")
        {
            if (!task)
            {
                double val = 0.0;
                val = resource->getAvailableWorkLoad
                    (scenario, Interval(reportElement->getStart(),
                                        reportElement->getEnd()));
                cellText = reportElement->scaledLoad(val, tcf->realFormat);
            }
        }
        else if ((*ci)->getName() == "hierarchindex")
        {
            if (!task)
                cellText = task->getHierarchIndex();
        }
        else if ((*ci)->getName() == "hierarchno")
        {
            if (!task)
                cellText = task->getHierarchNo();
        }
        else if ((*ci)->getName() == "id")
            cellText = resource->getFullId();
        else if ((*ci)->getName() == "index")
        {
            if (!task)
                cellText.sprintf("%d.", task->getIndex());
        }
        else if ((*ci)->getName() == "maxeffort")
        {
            const UsageLimits* limits = resource->getLimits();
            if (limits == 0)
                cellText = i18n("no Limits");
            else
            {
                int sg = reportDef->getProject()->getScheduleGranularity();
                if (limits->getDailyMax() > 0)
                    cellText = i18n("D: %1h").arg(limits->getDailyMax() *
                                               sg / (60 * 60));
                if (limits->getWeeklyMax() > 0)
                {
                    if (!cellText.isEmpty())
                        cellText += ", ";
                    cellText += i18n("W: %1h").arg(limits->getWeeklyMax() *
                                                sg / (60 * 60));
                }
                if (limits->getMonthlyMax() > 0)
                {
                    if (!cellText.isEmpty())
                        cellText += ", ";
                    cellText += i18n("M: %1d").arg(limits->getMonthlyMax() *
                                                      sg / (60 * 60 * 24));
                }
            }
        }
        else if ((*ci)->getName() == "name")
            cellText = resource->getName();
        else if ((*ci)->getName() == "no")
            cellText.sprintf("%d", row->getIndex());
        else if ((*ci)->getName() == "projectids")
            cellText = resource->getProjectIDs
                (scenario, Interval(reportElement->getStart(),
                                    reportElement->getEnd()));
        else if ((*ci)->getName() == "rate")
        {
            cellText = tcf->realFormat.format(resource->getRate(), FALSE);
        }
        else if ((*ci)->getName() == "revenue")
        {
            double val = resource->getCredits
                (scenario, Interval(reportElement->getStart(),
                                    reportElement->getEnd()), Revenue, task);
            cellText = tcf->realFormat.format(val, FALSE);
        }
        else if ((*ci)->getName() == "seqno")
        {
            if (!task)
                cellText.sprintf("%d.", task->getSequenceNo());
        }
        else if ((*ci)->getName() == "utilization")
        {
            if (!task)
            {
                double load = resource->getLoad
                    (scenario, Interval(reportElement->getStart(),
                                        reportElement->getEnd()));
                double freeLoad = resource->getAvailableWorkLoad
                    (scenario, Interval(reportElement->getStart(),
                                        reportElement->getEnd()));
                double val = 100.0 / (1.0 + (freeLoad / load));
                cellText = QString().sprintf("%.1f%%", val);
            }
        }
        else
            generateCustomAttribute(resource, (*ci)->getName(), cellText);

        cell->setText(cellText);
        row->insertCell(cell, colIdx);

    }
}

void
TjPrintReport::generateCustomAttribute(const CoreAttributes* ca,
                                       const QString name,
                                       QString& cellText) const
{
    // Handle custom attributes
    const CustomAttribute* custAttr =
        ca->getCustomAttribute(name);
    if (custAttr)
    {
        switch (custAttr->getType())
        {
            case CAT_Undefined:
                break;
            case CAT_Text:
                cellText =
                    dynamic_cast<const TextAttribute*>(custAttr)->getText();
                break;
            case CAT_Reference:
                cellText =
                    dynamic_cast<const
                    ReferenceAttribute*>(custAttr)->getLabel();
                break;
        }
    }
}

void
TjPrintReport::layoutPages()
{
    QPaintDeviceMetrics metrics(printer);

    if (showGantt)
    {
        objPosTable = new TjObjPosTable;

        ganttChart->setProjectAndReportData(reportElement);
        ganttChart->setDPI(metrics.logicalDpiX(), metrics.logicalDpiY());
    }
    // Set the left and top margin to 15mm
    leftMargin = mmToXPixels(15);
    topMargin = mmToYPixels(15);
    pageWidth = metrics.width() - 2 * leftMargin;
    pageHeight = metrics.height() - 2 * topMargin;
    cellMargin = mmToXPixels(1);

    // Determine height of headline
    if (!reportElement->getHeadline().isEmpty())
    {
        QFontMetrics fm(headlineFont);
        QRect br = fm.boundingRect(reportElement->getHeadline());
        int margin = mmToYPixels(2);
        headlineHeight = br.height() + 2 * margin + 1;
        headlineX = leftMargin + (pageWidth - br.width()) / 2;
        headlineBase = topMargin + headlineHeight - 1 - margin -
            fm.descent();
    }

    // Determine geometries for table header elements
    headerY = topMargin + headlineHeight;
    headerHeight = 0;
    for (std::vector<TjReportColumn*>::iterator cit = columns.begin();
         cit != columns.end(); ++cit)
    {
        if ((*cit)->getIsGantt())
        {
            headerHeight = ganttChart->calcHeaderHeight();
        }
        else
        {
            QFontMetrics fm(tableHeaderFont);
            QRect br = fm.boundingRect((*cit)->getTitle());
            br.setWidth(br.width() + 2 * cellMargin + 1);
            br.setHeight(br.height() + 2 * cellMargin + 1);
            if (br.height() > headerHeight)
                headerHeight = br.height();
            if ((*cit)->getWidth() < br.width())
                (*cit)->setWidth(br.width());
        }
    }

    // Determine height of bottom line
    QFontMetrics fm(standardFont);
    bottomlineHeight = fm.height() + 2 * cellMargin;
    bottomlineY = topMargin + pageHeight - bottomlineHeight;

    // Determine the geometry of the footer
    if (showGantt)
        footerHeight = ganttChart->calcLegendHeight(pageWidth - 2);
    else
        footerHeight = mmToYPixels(0); // Use a fixed footer for now
    footerY = bottomlineY - footerHeight;

    // And now the table layout
    tableRight = leftMargin + pageWidth;
    tableBottom = footerY - 1;

    /* We iterate over all the rows and determine their heights. This also
     * defines the top Y coordinate and the vertical page number. */
    int topOfTable = topMargin + headlineHeight + headerHeight;
    int topOfRow = topOfTable;
    int absTopOfRow = 0;
    int minRowHeight = 0;
    int yPage = 0;
    TjReportRow* prevRow = 0;
    for (std::vector<TjReportRow*>::iterator rit = rows.begin();
         rit != rows.end(); prevRow = *rit, ++rit)
    {
        /* For each row we iterate over the cells to detemine their minimum
         * bounding rect. For each cell use the minimum width to determine the
         * overall column width. */
        int maxHeight = fm.height() + 2 * cellMargin;
        for (int col = 0; col < getNumberOfColumns(); ++col)
        {
            if (columns[col]->getIsGantt())
                continue;

            TjReportCell* cell = (*rit)->getCell(col);
            assert(cell != 0);
            assert(cell->getRow() == *rit);
            assert(cell->getColumn() == columns[col]);
            if (cell->getText().isEmpty())
                continue;
            QRect br = fm.boundingRect(cell->getText());
            // Compute the indentation depth for the cell.
            int indentation = 0;
            if (columns[col]->getTableColumnFormat()->getHAlign() ==
                TableColumnFormat::left)
                indentation = cell->getIndentLevel() * indentSteps;
            else if (columns[col]->getTableColumnFormat()->getHAlign() ==
                     TableColumnFormat::right)
                indentation = (columns[col]->getMaxIndentLevel() - 1 -
                               cell->getIndentLevel()) * indentSteps;
            br.setWidth(br.width() + 2 * cellMargin + 1 + indentation);
            br.setHeight(br.height() + 2 * cellMargin + 1);
            if (br.height() > maxHeight)
                maxHeight = br.height();
            if (columns[col]->getWidth() < br.width())
                columns[col]->setWidth(br.width());

            if (minRowHeight == 0 || minRowHeight > br.height())
                minRowHeight = br.height();

            // This is a hack to handle oversize columns
            if (columns[col]->getWidth() > ((int) (2.0/3 * pageWidth)))
                columns[col]->setWidth(pageWidth / 2);
        }
        if (topOfRow + maxHeight > tableBottom)
        {
            topOfRow = topMargin + headlineHeight + headerHeight;
            yPage++;
            if (prevRow)
                prevRow->setLastOnPage(true);
        }

        if (showGantt)
            objPosTable->addEntry((*rit)->getCoreAttributes(),
                                  (*rit)->getSubCoreAttributes(), absTopOfRow,
                                  maxHeight);

        (*rit)->setTopY(topOfRow);
        topOfRow += maxHeight;
        absTopOfRow += maxHeight;
        (*rit)->setYPage(yPage);
        (*rit)->setHeight(maxHeight);
    }
    if (prevRow)
        prevRow->setLastOnPage(true);

    /* Now that we know all the column widths, we can determine their absolute
     * X coordinate and the X page. */
    int colX = leftMargin;
    int xPage = 0;
    int ganttChartWidth = 0;
    TjReportColumn* prevColumn = 0;
    for (std::vector<TjReportColumn*>::iterator cit = columns.begin();
         cit != columns.end(); prevColumn = *cit, ++cit)
    {
        if (!(*cit)->getIsGantt())
        {
            if (colX + (*cit)->getWidth() > leftMargin + pageWidth)
            {
                int remainder = leftMargin + pageWidth - colX;
                expandColumns(xPage, remainder, prevColumn);
                // The first column is repeated at the left of each page
                colX = leftMargin + columns[0]->getWidth();
                xPage++;

                if (prevColumn)
                    prevColumn->setLastOnPage(true);
            }
            (*cit)->setLeftX(colX);
            (*cit)->setXPage(xPage);
            colX += (*cit)->getWidth();
        }
        else
        {
            /* The Gantt chart should be at least 1/3 of the page width. If it
             * does not fit on this page anymore, we start a new page. */
            if (colX > leftMargin + (int) ((2.0 / 3) * pageWidth))
            {
                int remainder = leftMargin + pageWidth - colX;
                expandColumns(xPage, remainder, prevColumn);
                // The first column is repeated at the left of each page
                colX = leftMargin + columns[0]->getWidth();
                xPage++;

                if (prevColumn)
                    prevColumn->setLastOnPage(true);
            }
            (*cit)->setLeftX(colX);
            (*cit)->setXPage(xPage);
            ganttChartWidth = pageWidth - (colX - leftMargin);
            (*cit)->setWidth(ganttChartWidth);
            colX += (*cit)->getWidth();
        }
    }
    if (prevColumn)
    {
        if (!prevColumn->getIsGantt())
            expandColumns(xPage, leftMargin + pageWidth - colX, prevColumn);
        prevColumn->setLastOnPage(true);
    }

    if (showGantt)
    {
        int tableHeight = 0;
        for (std::vector<TjReportRow*>::iterator rit = rows.begin();
             rit != rows.end(); ++rit)
            tableHeight += (*rit)->getHeight();
        ganttChart->setSizes(objPosTable, headerHeight - 4, tableHeight - 4,
                             ganttChartWidth - 3, minRowHeight);
        ganttChart->generate(TjGanttChart::autoZoom);
        ganttChart->generateLegend(pageWidth - mmToXPixels(10),
                                   footerHeight - 2);
    }
}

void
TjPrintReport::expandColumns(int xPage, int remainder,
                             TjReportColumn* lastColumn)
{
    int expandableColumns = 0;
    for (std::vector<TjReportColumn*>::iterator cit = columns.begin();
         cit != columns.end(); ++cit)
        if ((*cit)->getXPage() == xPage &&
            (*cit)->getTableColumnFormat()->getExpandable())
            expandableColumns++;

    if (expandableColumns > 0)
    {
        int xCorr = 0;
        for (std::vector<TjReportColumn*>::iterator cit = columns.begin();
             cit != columns.end(); ++cit)
            if ((*cit)->getXPage() == xPage)
            {
                (*cit)->setLeftX((*cit)->getLeftX() + xCorr);
                if ((*cit)->getTableColumnFormat()->
                    getExpandable())
                {
                    (*cit)->setWidth((*cit)->getWidth() + remainder /
                                     expandableColumns);
                    xCorr += remainder / expandableColumns;
                }
            }
    }
    else
    {
        if (lastColumn)
            lastColumn->setWidth(lastColumn->getWidth() + remainder);
    }
}

bool
TjPrintReport::beginPrinting()
{
    return p.begin(printer);
}

void
TjPrintReport::endPrinting()
{
    p.end();
}

void
TjPrintReport::printReportPage(int x, int y)
{
    QFont fnt;
    QFontMetrics fm(fnt);
    int descent = fm.descent();

    // Draw outer frame
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::white);
    p.drawRect(leftMargin, topMargin, pageWidth, pageHeight);

    // Draw headline if needed
    if (headlineHeight > 0)
    {
        p.setFont(headlineFont);
        p.drawText(headlineX, headlineBase, reportElement->getHeadline());
        p.setPen(QPen(Qt::black, 1));
        p.drawLine(leftMargin, topMargin + headlineHeight - 1,
                   leftMargin + pageWidth, topMargin + headlineHeight - 1);
    }

    // Draw the table header
    for (std::vector<TjReportColumn*>::iterator cit = columns.begin();
         cit != columns.end(); ++cit)
    {
        p.setPen(QPen(Qt::black, 1));
        p.setFont(tableHeaderFont);
        if ((*cit)->getIsGantt() && (*cit)->getXPage() == x)
        {
            ganttChart->paintHeader(QRect((*cit)->getLeftX() + 1, headerY + 2,
                                          (*cit)->getWidth() - 3,
                                          headerHeight - 4), &p);
        }
        else if ((*cit == columns.front() || (*cit)->getXPage() == x) &&
                 !(*cit)->getIsGantt())
        {
            p.drawText((*cit)->getLeftX() + cellMargin - 1,
                       headerY + headerHeight - cellMargin - descent - 1,
                       (*cit)->getTitle());
            // Draw right cell border
            if (!(*cit)->getLastOnPage())
                p.drawLine((*cit)->getLeftX() + (*cit)->getWidth() - 1,
                           headerY, (*cit)->getLeftX() + (*cit)->getWidth() - 1,
                           headerY + headerHeight - 1);
        }
    }
    // Draw lower border of header
    p.setPen(QPen(Qt::black, 1));
    p.drawLine(leftMargin, headerY + headerHeight - 1,
               leftMargin + pageWidth, headerY + headerHeight - 1);

    // Draw the table cells for this page
    bool ganttChartPainted = FALSE;
    for (std::vector<TjReportRow*>::iterator rit = rows.begin();
         rit != rows.end(); ++rit)
        if ((*rit)->getYPage() == y)
        {
            for (int col = 0; col < getNumberOfColumns(); ++col)
            {
                TjReportColumn* reportColumn = columns[col];

                /* The first column is repeated as left column on each page.
                 * On the first page we of course don't have to do this. */
                if ((reportColumn->getXPage() == x || col == 0) &&
                    !reportColumn->getIsGantt())
                {
                    printReportCell(*rit, col);

                    // Draw lower border line for row
                    if (col == 0 || (*rit)->getLastOnPage())
                        p.setPen(QPen(Qt::black, 1));
                    else
                        p.setPen(QPen(Qt::gray, 1));

                    p.drawLine(reportColumn->getLeftX(),
                               (*rit)->getTopY() + (*rit)->getHeight() - 1,
                               reportColumn->getLeftX() +
                               reportColumn->getWidth() - 1,
                               (*rit)->getTopY() + (*rit)->getHeight() - 1);

                    // Draw right cell border
                    if (!columns[col]->getLastOnPage())
                    {
                        p.setPen(QPen(col == 0 ? Qt::black : Qt::gray, 1));
                        p.drawLine(columns[col]->getLeftX() +
                                   columns[col]->getWidth() - 1,
                                   (*rit)->getTopY(),
                                   columns[col]->getLeftX() +
                                   columns[col]->getWidth() - 1,
                                   (*rit)->getTopY() + (*rit)->getHeight());
                    }
                }
                else if (reportColumn->getIsGantt() &&
                         reportColumn->getXPage() == x && !ganttChartPainted)
                {
                    ganttChartPainted = true;
                    /* Compute the height of the table on this page and the
                     * combined height of all previous tables. */
                    int tableHeight = 0;
                    int prevTablesHeight = 0;
                    for (std::vector<TjReportRow*>::iterator rit1 =
                         rows.begin(); rit1 != rows.end(); ++rit1)
                        if ((*rit1)->getYPage() < y)
                            prevTablesHeight += (*rit1)->getHeight();
                        else if ((*rit1)->getYPage() == y)
                            tableHeight += (*rit1)->getHeight();

                    ganttChart->paintChart
                        (0, prevTablesHeight,
                         QRect(columns[col]->getLeftX() + 1,
                               (*rit)->getTopY() + 1,
                               columns[col]->getWidth() - 3,
                               tableHeight - 4), &p);

                    // Draw lower border
                    p.setPen(QPen(Qt::black, 1));
                    p.drawLine(reportColumn->getLeftX(),
                               (*rit)->getTopY() + tableHeight - 1,
                               reportColumn->getLeftX() +
                               reportColumn->getWidth() - 1,
                               (*rit)->getTopY() + tableHeight - 1);
                }
            }
        }

    // Draw footer
    p.setPen(QPen(Qt::black, 1));
    p.drawLine(leftMargin, footerY, leftMargin + pageWidth - 1, footerY);
    ganttChart->paintLegend(QRect(leftMargin + mmToXPixels(5),
                                  footerY + 2, pageWidth - mmToXPixels(10),
                                  footerHeight - 1), &p);

    // Draw bottom line
    p.setPen(QPen(Qt::black, 1));
    p.drawLine(leftMargin, bottomlineY, leftMargin + pageWidth - 1,
               bottomlineY);
    // Page number in the center
    p.setFont(standardFont);
    QString pageMark = i18n("Page %1 of %2")
        .arg(y * (columns.back()->getXPage() + 1) + x + 1)
        .arg((columns.back()->getXPage() + 1) * (rows.back()->getYPage() + 1));
    QFontMetrics fm1(standardFont);
    QRect br = fm1.boundingRect(pageMark);
    p.drawText(leftMargin + (pageWidth - br.width()) / 2,
               bottomlineY + cellMargin + fm1.ascent(), pageMark);
    // Copyright message on the left
    if (!reportDef->getProject()->getCopyright().isEmpty())
        p.drawText(leftMargin + cellMargin,
                   bottomlineY + cellMargin + fm1.ascent(),
                   reportDef->getProject()->getCopyright());
    // Project scheduding reference date (now value) on the right
    QString now = time2user(reportDef->getProject()->getNow(),
                            reportElement->getTimeFormat());
    br = fm1.boundingRect(now);
    p.drawText(leftMargin + pageWidth - cellMargin - br.width(),
               bottomlineY + cellMargin + fm1.ascent(), now);

    // Print a small signature below the page frame
    if (reportDef->getTimeStamp())
    {
        p.setFont(signatureFont);
        QFontMetrics fm(signatureFont);
        // The signature is not marked for translation on purpose!
        QString signature = QString("Generated on %1 with TaskJuggler %2")
            .arg(time2user(time(0), reportElement->getTimeFormat()))
            .arg(VERSION);
        QRect br = fm.boundingRect(signature);
        p.drawText(leftMargin + pageWidth - br.width(),
                   topMargin + pageHeight + cellMargin + fm.height(),
                   signature);
    }
}

void
TjPrintReport::printReportCell(TjReportRow* row, int col)
{
    QFontMetrics fm(standardFont);
    int descent = fm.descent();

    TjReportCell* cell = row->getCell(col);
    TjReportColumn* column = cell->getColumn();
    p.setFont(standardFont);
    p.setPen(QPen(Qt::black, 1));
    int y = row->getTopY() + row->getHeight() - cellMargin - descent - 1;
    switch (column->getTableColumnFormat()->getHAlign())
    {
        case TableColumnFormat::left:
            p.drawText(column->getLeftX() + cellMargin - 1 +
                       cell->getIndentLevel() * indentSteps, y,
                       cell->getText());
            break;
        case TableColumnFormat::center:
            {
                QFontMetrics fm(standardFont);
                QRect br = fm.boundingRect(cell->getText());
                int x = (columns[col]->getWidth() -
                         (2 * cellMargin + br.width())) / 2;
                p.drawText(column->getLeftX() + cellMargin - 1 + x, y,
                           cell->getText());
                break;
            }
        case TableColumnFormat::right:
            {
                QFontMetrics fm(standardFont);
                QRect br = fm.boundingRect(cell->getText());
                int x = columns[col]->getWidth() -
                    (2 * cellMargin + br.width() +
                     ((column->getMaxIndentLevel() - 1 -
                       cell->getIndentLevel()) * indentSteps));
                p.drawText(column->getLeftX() + cellMargin - 1 + x, y,
                           cell->getText());
            }
    }
}

int
TjPrintReport::mmToXPixels(double mm)
{
    QPaintDeviceMetrics metrics(printer);
    return (int) ((mm / 25.4) * metrics.logicalDpiX());
}

int
TjPrintReport::mmToYPixels(double mm)
{
    QPaintDeviceMetrics metrics(printer);
    return (int) ((mm / 25.4) * metrics.logicalDpiY());
}

int
TjPrintReport::pointsToYPixels(double pts)
{
    QPaintDeviceMetrics metrics(printer);
    return (int) ((pts * (0.376 / 25.4)) * metrics.logicalDpiY());
}

