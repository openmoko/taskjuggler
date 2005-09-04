/*
 * The TaskJuggler Project Management Software
 *
 * Copyright (c) 2001, 2002, 2003, 2004, 2005 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include "TjTaskReport.h"

#include <qdict.h>
#include <qcanvas.h>

#include <klistview.h>
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kcursor.h>
#include <kmessagebox.h>

#include "Project.h"
#include "Task.h"
#include "Resource.h"
#include "ExpressionTree.h"
#include "QtTaskReport.h"
#include "QtTaskReportElement.h"
#include "ReportLayers.h"
#include "TjPrintTaskReport.h"
#include "TjGanttChart.h"

TjTaskReport::TjTaskReport(QWidget* p, Report* const rDef,
                           const QString& n) : TjReport(p, rDef, n)
{
}

TjTaskReport::~TjTaskReport()
{
}

TjPrintReport*
TjTaskReport::newPrintReport(QPaintDevice* pd)
{
    return new TjPrintTaskReport(reportDef, pd);
}

const QtReportElement*
TjTaskReport::getReportElement() const
{
    return reportElement;
}

bool
TjTaskReport::generateList()
{
    // Remove all items and columns from list view.
    listView->clear();
    ca2lviDict.clear();
    lvi2caDict.clear();
    lvi2ParentCaDict.clear();

    while (listView->columns())
        listView->removeColumn(0);
    maxDepth = 0;

    if (!reportDef)
        return FALSE;

    // We need those values frequently. So let's store them in a more
    // accessible place.
    reportElement =
        (dynamic_cast<QtTaskReport*>(reportDef))->getTable();
    scenario = reportElement->getScenario(0);
    taskList = reportDef->getProject()->getTaskList();

    // QListView can hide subtasks. So we feed the list with all tasks first
    // and then later close those items that we want to roll up. This
    // expression means "roll-up none".
    ExpressionTree* et = new ExpressionTree;
    et->setTree("0", reportDef->getProject());

    if (!reportElement->filterTaskList(taskList, 0,
                                       reportElement->getHideTask(), et))
        return FALSE;

    if (taskList.isEmpty())
        return TRUE;

    generateListHeader(i18n("Task"), reportElement);

    /* The task list need to be generated in two phases. First we insert all
     * tasks and the nested resources, and then we fill the rest of all the
     * lines. For some columns we need to know the maximum tree depth, so we
     * have to fill the table first with all entries before we can fill those
     * columns. */
    int i = 0;
    for (TaskListIterator tli(taskList); *tli; ++tli)
    {
        KListViewItem* newLvi;
        if ((*tli)->getParent() &&
            taskList.findRef((*tli)->getParent()) >= 0 &&
            ca2lviDict[QString("t:") + (*tli)->getParent()->getId()] &&
            (*tli)->getParent()->getId().length() >
            reportElement->getTaskRoot().length())
        {
            newLvi = new KListViewItem
                (ca2lviDict[QString("t:") + (*tli)->getParent()->getId()],
                 (*tli)->getName());
        }
        else
            newLvi = new KListViewItem(listView, (*tli)->getName());

        ca2lviDict.insert(QString("t:") + (*tli)->getId(), newLvi);
        lvi2caDict.insert(QString().sprintf("%p", newLvi), *tli);

        if (treeLevel(newLvi) > maxDepth)
            maxDepth = treeLevel(newLvi);

        if ((*tli)->isContainer())
        {
            newLvi->setPixmap(0, KGlobal::iconLoader()->
                              loadIcon("tj_task_group", KIcon::Small));
            if (reportElement->getRollUpTask())
            {
                if (!reportDef->isRolledUp(*tli,
                                           reportElement->getRollUpTask()))
                    newLvi->setOpen(TRUE);
                if (reportElement->getRollUpTask()->getErrorFlag())
                    return FALSE;
            }
            else
                newLvi->setOpen(TRUE);
        }
        else if ((*tli)->isMilestone())
        {
            newLvi->setPixmap(0, KGlobal::iconLoader()->loadIcon("tj_milestone",
                                                                 KIcon::Small));
        }
        else
        {
            newLvi->setPixmap(0, KGlobal::iconLoader()->loadIcon("tj_task",
                                                                 KIcon::Small));
            for (ResourceListIterator rli((*tli)->
                                          getBookedResourcesIterator(scenario));
                 *rli; ++rli)
            {
                QListViewItem* lvi =
                    new KListViewItem(newLvi, (*rli)->getName());
                lvi->setPixmap(0, KGlobal::iconLoader()->
                               loadIcon("tj_resource", KIcon::Small));
                ca2lviDict.insert(QString("r:") + (*tli)->getId() +
                                  ":" + (*rli)->getId(), lvi);
                lvi2caDict.insert(QString().sprintf("%p", lvi), *rli);
                lvi2ParentCaDict.insert(QString().sprintf("%p", lvi), *tli);
                if (treeLevel(lvi) > maxDepth)
                    maxDepth = treeLevel(lvi);
            }
        }
        newLvi->setText(1, QString().sprintf("%05d", i++));
    }

    // Now we know the maximum tree depth and can fill in the rest of the
    // columns.
    for (TaskListIterator tli(taskList); *tli; ++tli)
    {
        generateTaskListLine(reportElement, *tli,
                             ca2lviDict[QString("t:") + (*tli)->getId()]);
        if (!(*tli)->isContainer() && !(*tli)->isMilestone())
        {
            for (ResourceListIterator rli((*tli)->
                                          getBookedResourcesIterator(scenario));
                 *rli; ++rli)
            {
                generateResourceListLine(reportElement,
                                         *rli, ca2lviDict[QString("r:") +
                                         (*tli)->getId() + ":" +
                                         (*rli)->getId()], *tli);
            }
        }
    }

    return TRUE;
}

bool
TjTaskReport::generateChart(bool autoFit)
{
    setCursor(KCursor::waitCursor());

    prepareChart(autoFit, reportElement);

    ganttChart->getHeaderCanvas()->update();
    ganttChart->getChartCanvas()->update();

    setCursor(KCursor::arrowCursor());
    return TRUE;
}

QString
TjTaskReport::generateStatusBarText(const QPoint& pos,
                                    const CoreAttributes* ca,
                                    const CoreAttributes* parent) const
{
    QPoint chartPos = ganttChartView->viewportToContents(pos);
    time_t refTime = x2time(chartPos.x());
    Interval iv = stepInterval(refTime);
    QString ivName = stepIntervalName(refTime);

    QString text;
    if (ca->getType() == CA_Task)
    {
        const Task* t = dynamic_cast<const Task*>(ca);
        double load = t->getLoad(scenario, iv);
        text = i18n("%1(%2) - %3:  Load=%4")
            .arg(t->getName())
            .arg(t->getId())
            .arg(ivName)
            .arg(reportElement->scaledLoad
                 (load, reportDef->getNumberFormat()));
    }
    else
    {
        const Resource* r = dynamic_cast<const Resource*>(ca);
        const Task* t = dynamic_cast<const Task*>(parent);
        double load = r->getLoad(scenario, iv, AllAccounts, t);
        text = i18n("%1(%2) - %3:  Load=%4  %5(%6)")
            .arg(r->getName())
            .arg(r->getId())
            .arg(ivName)
            .arg(reportElement->scaledLoad
                 (load, reportDef->getNumberFormat()))
            .arg(t->getName())
            .arg(t->getId());
    }

    return text;
}

QListViewItem*
TjTaskReport::getTaskListEntry(const Task* t)
{
    /* Returns the QListViewItem pointer for the task if the task is shown in
     * the list view. Tasks that have closed parents are not considered to be
     * visible even though they are part of the list view. Offscreen tasks
     * are considered visible if they meet the above condition. */

    // Check that the task is in the list. Colum 1 contains the task ID.
    QListViewItem* lvi = ca2lviDict[QString("t:") + t->getId()];
    if (!lvi)
        return 0;

    // Now make sure that all parents are open.
    for (QListViewItem* i = lvi; i; i = i->parent())
        if (i->parent() && !i->parent()->isOpen())
            return 0;

    return lvi;
}
