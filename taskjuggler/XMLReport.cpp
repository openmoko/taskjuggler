/*
 * XMLReport.cpp - TaskJuggler
 *
 * Copyright (c) 2002 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include "XMLReport.h"

#include <zlib.h>

#include <qmap.h>

#include "tjlib-internal.h"
#include "Project.h"
#include "Scenario.h"
#include "Account.h"
#include "Resource.h"
#include "BookingList.h"
#include "Allocation.h"
#include "XMLReport.h"
#include "ExpressionTree.h"
#include "Operation.h"
#include "CustomAttributeDefinition.h"
#include "TextAttribute.h"
#include "ReferenceAttribute.h"

static QMap<QString, int> TaskAttributeDict;
typedef enum TADs {
    TA_UNDEFINED = 0,
    TA_COMPLETE,
    TA_DEPENDS,
    TA_DURATION,
    TA_EFFORT,
    TA_FLAGS,
    TA_LENGTH,
    TA_MAXEND,
    TA_MAXSTART,
    TA_MINEND,
    TA_MINSTART,
    TA_NOTE,
    TA_PRIORITY,
    TA_RESPONSIBLE,
    TA_STATUS,
    TA_STATUSNOTE,
    TA_ACCOUNT
};

XMLReport::XMLReport(Project* p, const QString& f,
                           const QString& df, int dl) :
    Report(p, f, df, dl),
    doc(0),
    accountAttributes(),
    taskAttributes()
{
    if (TaskAttributeDict.empty())
    {
        TaskAttributeDict[KW("complete")] = TA_COMPLETE;
        TaskAttributeDict[KW("depends")] = TA_DEPENDS;
        TaskAttributeDict[KW("duration")] = TA_DURATION;
        TaskAttributeDict[KW("effort")] = TA_EFFORT;
        TaskAttributeDict[KW("flags")] = TA_FLAGS;
        TaskAttributeDict[KW("length")] = TA_LENGTH;
        TaskAttributeDict[KW("maxend")] = TA_MAXEND;
        TaskAttributeDict[KW("maxstart")] = TA_MAXSTART;
        TaskAttributeDict[KW("minend")] = TA_MINEND;
        TaskAttributeDict[KW("minstart")] = TA_MINSTART;
        TaskAttributeDict[KW("note")] = TA_NOTE;
        TaskAttributeDict[KW("priority")] = TA_PRIORITY;
        TaskAttributeDict[KW("responsible")] = TA_RESPONSIBLE;
        TaskAttributeDict[KW("status")] = TA_STATUS;
        TaskAttributeDict[KW("statusnote")] = TA_STATUSNOTE;
        TaskAttributeDict[KW("account")] = TA_ACCOUNT;
    }
    // show all tasks
    hideTask = new ExpressionTree(new Operation(0));
    // show all resources
    hideResource = new ExpressionTree(new Operation(0));
    // show all accounts
    hideAccount = new ExpressionTree(new Operation(0));

    taskSortCriteria[0] = CoreAttributesList::TreeMode;
    taskSortCriteria[1] = CoreAttributesList::StartUp;
    taskSortCriteria[2] = CoreAttributesList::EndUp;
    resourceSortCriteria[0] = CoreAttributesList::TreeMode;
    resourceSortCriteria[1] = CoreAttributesList::IdUp;
    accountSortCriteria[0] = CoreAttributesList::TreeMode;
    accountSortCriteria[1] = CoreAttributesList::IdUp;

    // All XML reports default to just showing the first scenario.
    scenarios.append(0);
}

XMLReport::~XMLReport()
{
    delete doc;
}

bool
XMLReport::generate()
{
    if (!open())
        return false;

    doc = new QDomDocument
        ("taskjuggler PUBLIC "
         "\"-//The TaskJuggler Project//DTD TaskJuggler 2.0//EN\""
         " \"http://www.taskjuggler.org/dtds/TaskJuggler-2.0.dtd\"");

    doc->appendChild(doc->createProcessingInstruction
                     ("xml", "version=\"1.0\" encoding=\"UTF-8\" "
                      "standalone=\"no\""));

    QDomElement tjEl = doc->createElement("taskjuggler");
    doc->appendChild(tjEl);

    if (timeStamp)
    {
        doc->appendChild(doc->createComment(
            QString("This file has been generated by TaskJuggler ") +
            VERSION + " at " + time2ISO(time(0)) + "."));
        doc->appendChild(doc->createComment(
            QString("For details about TaskJuggler see ") + TJURL));
    }

    TaskList filteredTaskList;
    if (!filterTaskList(filteredTaskList, 0, hideTask, rollUpTask))
        return false;
    sortTaskList(filteredTaskList);

    ResourceList filteredResourceList;
    if (!filterResourceList(filteredResourceList, 0, hideResource,
                            rollUpResource))
        return false;
    sortResourceList(filteredResourceList);

    AccountList filteredAccountList;
    if (!filterAccountList(filteredAccountList, AllAccounts, hideAccount,
                           rollUpAccount))
        return false;
    sortAccountList(filteredAccountList);

    if (!generateProjectProperty(&tjEl))
        return false;
    if (!generateGlobalVacationList(&tjEl))
        return false;
    if (!generateShiftList(&tjEl))
        return false;
    if (!generateResourceList(&tjEl, filteredResourceList, filteredTaskList))
        return false;
    if (!generateAccountList(&tjEl, filteredAccountList, filteredTaskList))
        return false;
    if (!generateTaskList(&tjEl, filteredTaskList, filteredResourceList))
        return false;
    if (!generateBookingList(&tjEl, filteredTaskList, filteredResourceList))
       return false;

    gzFile zf = gzdopen(dup(f.handle()), "wb");
    if (!zf)
    {
        tjWarning(i18n("Cannot open compressed file %1 for writing.")
                 .arg(fileName));
        return false;
    }
    int bytes;
    if ((bytes = gzputs(zf, static_cast<const char*>(doc->toCString()))) == 0)
    {
        tjWarning(i18n("Compression of %1 failed").arg(fileName));
        return false;
    }
    int zError;
    if ((zError = gzclose(zf)) != 0)
    {
        tjWarning(i18n("Closing of file %1 failed: %2").arg(fileName)
                 .arg(gzerror(zf, &zError)));
        return false;
    }

    return close();
}

bool
XMLReport::generateProjectProperty(QDomElement* n)
{
    QDomElement el = doc->createElement("project");
    n->appendChild(el);

    genTextAttr(&el, "id", project->getId());
    genTextAttr(&el, "name", project->getName());
    genTextAttr(&el, "version", project->getVersion());
    genDateElement(&el, "start", getStart());
    genDateElement(&el, "end", getEnd() + 1);

    // Generate custom attribute definitions
    if (!generateCustomAttributeDeclaration
        (&el, "task", project->getTaskAttributeDict()))
        return false;
    if (!generateCustomAttributeDeclaration
        (&el, "resource", project->getResourceAttributeDict()))
        return false;
    if (!generateCustomAttributeDeclaration
        (&el, "account", project->getAccountAttributeDict()))
        return false;

    // Generate date/time related settings
    genLongAttr(&el, "weekStartMonday",
                   project->getWeekStartsMonday() ? 1 : 0);
    if (!project->getTimeZone().isEmpty())
        genTextAttr(&el, "timezone", project->getTimeZone());
    genDoubleAttr(&el, "dailyWorkingHours",
                     project->getDailyWorkingHours());
    genDoubleAttr(&el, "yearlyWorkingDays",
                     project->getYearlyWorkingDays());
    genLongAttr(&el, "timingResolution", project->getScheduleGranularity());
    genDateElement(&el, "now", project->getNow());
    genTextAttr(&el, "timeFormat", project->getTimeFormat());
    genTextAttr(&el, "shortTimeFormat", project->getShortTimeFormat());

    // Generate currency settings
    RealFormat rf = project->getCurrencyFormat();
    QDomElement cfEl = doc->createElement("currencyFormat");
    el.appendChild(cfEl);
    genTextAttr(&cfEl, "signPrefix", rf.getSignPrefix());
    genTextAttr(&cfEl, "signSuffix", rf.getSignSuffix());
    genTextAttr(&cfEl, "thousandSep", rf.getThousandSep());
    genTextAttr(&cfEl, "fractionSep", rf.getFractionSep());
    genLongAttr(&cfEl, "fracDigits", rf.getFracDigits());
    if (!project->getCurrency().isEmpty())
        genTextAttr(&el, "currency", project->getCurrency());

    generateWorkingHours(&el, project->getWorkingHours());

    generateScenario(&el, project->getScenario(0));

    return true;
}

bool
XMLReport::generateCustomAttributeDeclaration(QDomElement* parentEl,
    const QString& propertyName,
    QDictIterator<CustomAttributeDefinition> it)
{
    if (!it.current())
        return true;
    QDomElement el = doc->createElement("extend");
    parentEl->appendChild(el);
    genTextAttr(&el, "property", propertyName);

    for ( ; it.current(); ++it)
    {
        QString exElType;
        switch (it.current()->getType())
        {
            case CAT_Text:
                exElType = "text";
                break;
            case CAT_Reference:
                exElType = "reference";
                break;
            default:
                qFatal("XMLReport::generateCustomAttributeDeclaration: "
                       "Unknown CAT %d", it.current()->getType());
                return false;
        }
        QDomElement exEl = doc->createElement("extendAttributeDefinition");
        el.appendChild(exEl);
        genTextAttr(&exEl, "id", it.currentKey());
        genTextAttr(&exEl, "name", it.current()->getName());
        genTextAttr(&exEl, "type", exElType);
    }

    return true;
}

bool
XMLReport::generateScenario(QDomElement* parentEl, Scenario* scenario)
{
    QDomElement el = doc->createElement("scenario");
    parentEl->appendChild(el);

    genTextAttr(&el, "id", scenario->getId());
    genTextAttr(&el, "name", scenario->getName());
    genTextAttr(&el, "disabled", scenario->getEnabled() ? "0" : "1");
    genTextAttr(&el, "projectionMode",
                scenario->getProjectionMode() ? "1" : "0");

    for (ScenarioListIterator sci(scenario->getSubListIterator());
         *sci != 0; ++sci)
        generateScenario(&el, *sci);

    return true;
}

bool
XMLReport::generateGlobalVacationList(QDomElement* parentNode)
{
    VacationList::Iterator vli(project->getVacationListIterator());

    if (*vli != 0)
    {
        QDomElement el = doc->createElement("vacationList");
        parentNode->appendChild(el);

        for ( ; *vli != 0; ++vli)
        {
            QDomElement vEl = doc->createElement("vacation");
            el.appendChild(vEl);

            genDateElement(&vEl, "start", (*vli)->getStart());
            genDateElement(&vEl, "end", (*vli)->getEnd() + 1);
            genTextAttr(&vEl, "name", (*vli)->getName());
        }
    }

    return true;
}

bool
XMLReport::generateShiftList(QDomElement* parentNode)
{
    QDomElement el = doc->createElement("shiftList");
    parentNode->appendChild(el);

    for (ShiftListIterator sli(project->getShiftListIterator());
         *sli != 0; ++sli)
    {
        if ((*sli)->getParent() == 0)
            if (!generateShift(&el, *sli))
                return false;
    }

    return true;
}

bool
XMLReport::generateShift(QDomElement* parentEl, const Shift* shift)
{
    QDomElement el = doc->createElement("shift");
    parentEl->appendChild(el);

    genTextAttr(&el, "id", shift->getId());
    genTextAttr(&el, "name", shift->getName());
    generateWorkingHours(&el, shift->getWorkingHours());

    for (ShiftListIterator sli(shift->getSubListIterator()); *sli; ++sli)
        if (!generateShift(&el, *sli))
            return false;

    return true;
}

bool
XMLReport::generateWorkingHours(QDomElement* parentEl,
                                const QPtrList<Interval>* const* wh)
{
    QDomElement el = doc->createElement("workingHours");
    parentEl->appendChild(el);

    for (int i = 0; i < 7; ++i)
    {
        if (wh[i]->isEmpty())
            continue;

        QDomElement dayEl = doc->createElement("weekdayWorkingHours");
        genTextAttr(&dayEl, "weekday", QString().sprintf("%d", i));
        el.appendChild(dayEl);
        QPtrListIterator<Interval> it(*wh[i]);
        for ( ; *it; ++it)
        {
            QDomElement ivEl = doc->createElement("timeInterval");
            dayEl.appendChild(ivEl);
            genTimeElement(&ivEl, "start", (*it)->getStart());
            genTimeElement(&ivEl, "end", (*it)->getEnd() + 1);
        }
    }

    return true;
}

bool
XMLReport::generateResourceList(QDomElement* parentNode,
                                ResourceList& filteredResourceList,
                                TaskList& filteredTaskList)
{
    QDomElement el = doc->createElement("resourceList");
    parentNode->appendChild(el);

    for (ResourceListIterator rli(filteredResourceList); *rli != 0; ++rli)
        if ((*rli)->getParent() == 0)
            if (!generateResource(&el, filteredResourceList,
                                  filteredTaskList, *rli))
                return false;

    return true;
}

bool
XMLReport::generateResource(QDomElement* parentEl,
                            ResourceList& filteredResourceList,
                            TaskList& filteredTaskList,
                            const Resource* resource)
{
    QDomElement el = doc->createElement("resource");
    parentEl->appendChild(el);

    genTextAttr(&el, "id", resource->getId());
    genTextAttr(&el, "name", resource->getName());

    QStringList fl = resource->getFlagList();
    for (QStringList::Iterator jt = fl.begin(); jt != fl.end(); ++jt)
        genTextElement(&el, "flag", *jt);

    for (ResourceListIterator srli(resource->getSubListIterator());
         *srli != 0; ++srli)
    {
        if (filteredResourceList.findRef(*srli) >= 0)
        {
            if (!generateResource(&el, filteredResourceList, filteredTaskList,
                                  *srli))
                return false;
        }
    }

    QPtrListIterator<Interval> vli(resource->getVacationListIterator());
    if (*vli != 0)
    {
        QDomElement vlEl = doc->createElement("vacationList");
        el.appendChild(vlEl);
        for ( ; *vli != 0; ++vli)
        {
            QDomElement vEl = doc->createElement("vacation");
            vlEl.appendChild(vEl);
            genDateElement(&vEl, "start", (*vli)->getStart());
            genDateElement(&vEl, "end", (*vli)->getEnd() + 1);
        }
    }

    generateWorkingHours(&el, resource->getWorkingHours());
    for (ShiftSelectionList::Iterator sli(*resource->getShiftList()); *sli;
         ++sli)
    {
        QDomElement sSel = doc->createElement("shiftSelection");
        el.appendChild(sSel);

        genTextAttr(&sSel, "shiftId", (*sli)->getShift()->getId());
        genDateElement(&sSel, "start", (*sli)->getPeriod().getStart());
        genDateElement(&sSel, "end", (*sli)->getPeriod().getEnd() + 1);
    }

    return true;
}

bool
XMLReport::generateAccountList(QDomElement* parentNode,
                               AccountList& filteredAccountList,
                               TaskList& filteredTaskList)
{
    QDomElement el = doc->createElement("accountList");
    parentNode->appendChild(el);

    for (AccountListIterator ali(filteredAccountList); *ali != 0; ++ali)
    {
        if ((*ali)->getParent() == 0)
            if (!generateAccount(&el, filteredAccountList,
                                 filteredTaskList, *ali))
                return false;
    }
    return true;
}

bool
XMLReport::generateAccount(QDomElement* parentEl,
                           AccountList& filteredAccountList,
                           TaskList& filteredTaskList,
                           const Account* account)
{
    QDomElement el = doc->createElement("account");
    parentEl->appendChild(el);

    genTextAttr(&el, "id", account->getId());
    genTextAttr(&el, "name", account->getName());

    switch (account->getAcctType())
    {
    case Cost:
        genTextAttr(&el, "type", "cost");
        break;
    case Revenue:
        genTextAttr(&el, "type", "revenue");
        break;
    default:
        qFatal("XMLReport::generateAccount: Unknown AccountType %d",
               account->getAcctType());
        return false;
    }

    for (QStringList::Iterator it = accountAttributes.begin();
         it != accountAttributes.end(); ++it)
    {
        if (account->getCustomAttribute(*it))
            generateCustomAttributeValue(&el, *it, account);
    }

    for (AccountListIterator srli(account->getSubListIterator());
         *srli != 0; ++srli)
    {
        if (filteredAccountList.findRef(*srli) >= 0)
        {
            if (!generateAccount(&el, filteredAccountList, filteredTaskList,
                                 *srli))
                return false;
        }
    }
    return true;
}

bool
XMLReport::generateTaskList(QDomElement* parentNode, TaskList& filteredTaskList,
                            ResourceList&)
{
    QDomElement el = doc->createElement("taskList");
    parentNode->appendChild(el);

    for (TaskListIterator tli(filteredTaskList); *tli != 0; ++tli)
        if ((*tli)->getParent() == 0 ||
            (*tli)->getParent()->getId() + "." == taskRoot)
            if (!generateTask(&el, filteredTaskList, *tli))
                return false;

    return true;
}

bool
XMLReport::generateTask(QDomElement* parentEl, TaskList& filteredTaskList,
                        const Task* task)
{
    QDomElement el = doc->createElement("task");
    parentEl->appendChild(el);

    QString taskId = task->getId();
    if (!taskRoot.isEmpty())
        taskId = stripTaskRoot(taskId);
    genTextAttr(&el, "id", taskId);
    genTextAttr(&el, "name", task->getName());
    genTextAttr(&el, "projectId", task->getProjectId());

    for (QStringList::Iterator it = taskAttributes.begin();
         it != taskAttributes.end(); ++it)
    {
        if (!TaskAttributeDict.contains(*it))
        {
            if (task->getCustomAttribute(*it))
                generateCustomAttributeValue(&el, *it, task);
            continue;
        }
        switch (TaskAttributeDict[*it])
        {
            case TA_FLAGS:
                {
                    QStringList fl = task->getFlagList();
                    for (QStringList::Iterator jt = fl.begin();
                         jt != fl.end(); ++jt)
                    {
                        genTextElement(&el, "flag", *jt);
                    }
                    break;
                }
            case TA_NOTE:
                if (!task->getNote().isEmpty())
                    genTextElement(&el, "note", task->getNote());
                break;
            case TA_PRIORITY:
                genLongAttr(&el, "priority", task->getPriority());
                break;
            case TA_EFFORT:
            case TA_LENGTH:
            case TA_DURATION:
            case TA_MINSTART:
            case TA_MAXSTART:
            case TA_MINEND:
            case TA_MAXEND:
            case TA_COMPLETE:
            case TA_STATUS:
            case TA_STATUSNOTE:
                // handled further down as scenario specific value.
                break;
            case TA_RESPONSIBLE:
                if (task->getResponsible())
                    genTextAttr(&el, "responsible",
                                task->getResponsible()->getId());
                break;
            case TA_DEPENDS:
                generateDepList(&el, filteredTaskList, task,
                                task->getDependsIterator(), "depends");
                generateDepList(&el, filteredTaskList, task,
                                task->getPrecedesIterator(), "precedes");
                break;
            case TA_ACCOUNT:
                if (task->getAccount())
                    genTextAttr(&el, "account",
                                task->getAccount()->getId());
                break;
            default:
                qDebug("XMLReport::generateTask(): "
                       "Unknown task attribute %s", (*it).latin1());
                return false;
        }

    }

    /* If a container task has sub tasks that are exported as well, we do
     * not export start/end date for those container tasks. */
    bool taskHasNoSubTasks = true;
    for (TaskListIterator stli(task->getSubListIterator());
         *stli != 0; ++stli)
    {
        if (filteredTaskList.findRef(*stli) >= 0)
        {
            taskHasNoSubTasks = false;
            if (!generateTask(&el, filteredTaskList, *stli))
                return false;
        }
    }

    for (QValueListIterator<int> it = scenarios.begin();
         it != scenarios.end(); ++it)
    {
        QDomElement scEl = doc->createElement("taskScenario");
        el.appendChild(scEl);
        genTextAttr(&scEl, "scenarioId", project->getScenarioId(*it));

        if (task->getStart(*it))
            genDateElement(&scEl, "start", task->getStart(*it));
        if (task->getEnd(*it) && !task->isMilestone())
            genDateElement(&scEl, "end", task->getEnd(*it) + 1);
        genLongAttr(&scEl, "scheduled", task->getScheduled(*it) ? 1 : 0);

        for (QStringList::Iterator atIt = taskAttributes.begin();
             atIt != taskAttributes.end(); ++atIt)
        {
            if (!TaskAttributeDict.contains(*atIt))
                continue;
            switch (TaskAttributeDict[*atIt])
            {
                case TA_EFFORT:
                    if (task->getEffort(*it) != 0 &&
                        (task->getStart(*it) == 0 || task->getEnd(*it) == 0))
                        genDoubleAttr(&scEl, "effort", task->getEffort(*it));
                    break;
                case TA_LENGTH:
                    if (task->getLength(*it) != 0 &&
                        (task->getStart(*it) == 0 || task->getEnd(*it) == 0))
                        genDoubleAttr(&scEl, "length", task->getLength(*it));
                    break;
                case TA_DURATION:
                    if (task->getDuration(*it) != 0 &&
                        (task->getStart(*it) == 0 || task->getEnd(*it) == 0))
                        genDoubleAttr(&scEl, "duration",
                                    task->getDuration(*it));
                    break;
                case TA_MINSTART:
                    if (task->getMinStart(*it) != 0)
                        genDateElement(&scEl, "minStart",
                                       task->getMinStart(*it));
                    break;
                case TA_MAXSTART:
                    if (task->getMaxStart(*it) != 0)
                        genDateElement(&scEl, "maxStart",
                                       task->getMaxStart(*it));
                    break;
                case TA_MINEND:
                    if (task->getMinEnd(*it) != 0)
                        genDateElement(&scEl, "minEnd",
                                       task->getMinEnd(*it) + 1);
                    break;
                case TA_MAXEND:
                    if (task->getMaxEnd(*it) != 0)
                        genDateElement(&scEl, "maxEnd",
                                       task->getMaxEnd(*it) + 1);
                    break;
                case TA_COMPLETE:
                    genDoubleAttr(&scEl, "complete", task->getComplete(*it));
                    break;
                case TA_STATUS:
                    if (task->getStatus(*it) > 0)
                        genLongAttr(&scEl, "status", task->getStatus(*it));
                    break;
                case TA_STATUSNOTE:
                    if (!task->getStatusNote(*it).isEmpty())
                        genTextElement(&scEl, "statusNote",
                                       task->getStatusNote(*it));
                    break;
            }
        }
    }

    genLongAttr(&el, "milestone", task->isMilestone() ? 1 : 0);
    genLongAttr(&el, "asapScheduling",
                task->getScheduling() == Task::ASAP ? 1 : 0);

    generateAllocate(&el, task);

    return true;
}

bool
XMLReport::generateDepList(QDomElement* parentEl, TaskList& filteredTaskList,
                           const Task* task,
                           QPtrListIterator<TaskDependency> depIt,
                           const char* tag)
{
    bool prev = (tag == "depends");
    for ( ; *depIt != 0; ++depIt)
    {
        /* Save current list item since findRef() modifies
         * it. Remember, we are still iterating the list. */
        CoreAttributes* curr = filteredTaskList.current();
        if (filteredTaskList.findRef((*depIt)->getTaskRef()) > -1 &&
            !(task->getParent() != 0 &&
              (prev ? task->getParent()->hasPrevious((*depIt)->getTaskRef()) :
               task->getParent()->hasFollower((*depIt)->getTaskRef()))))
        {
            QDomElement te= doc->createElement(tag);
            /* Putting the task ID as PCDATA in the depends/precedes element
             * was a mistake. We now store this information as 'task'
             * attribute. The PCDATA is now deprecated and should no longer be
             * used. It will be removed at some point with future versions of
             * the software. */
            te.appendChild(doc->createTextNode
                           (stripTaskRoot(((*depIt)->getTaskRef())->getId())));
            parentEl->appendChild(te);

            genTextAttr(&te, "task",
                        stripTaskRoot((*depIt)->getTaskRef()->getId()));

            for (int sc = 0; sc < project->getMaxScenarios(); ++sc)
                if ((*depIt)->getGapDuration(sc) != 0 ||
                    (*depIt)->getGapLength(sc) != 0)
                {
                    QDomElement dgs = doc->createElement
                        ("dependencyGapScenario");
                    te.appendChild(dgs);
                    genTextAttr(&dgs, "scenarioId",
                                project->getScenarioId(sc));
                    if ((*depIt)->getGapDuration(sc) != 0)
                        genLongAttr(&dgs, "gapDuration",
                                    (*depIt)->getGapDuration(sc));
                    if ((*depIt)->getGapLength(sc) != 0)
                        genLongAttr(&dgs, "gapLength",
                                    (*depIt)->getGapLength(sc));
                }
        }
        /* Restore current list item to continue
         * iteration. */
        filteredTaskList.findRef(curr);
    }

    return true;
}

bool
XMLReport::generateCustomAttributeValue(QDomElement* parentEl,
                                        const QString& id,
                                        const CoreAttributes* property)
{
    QDomElement el = doc->createElement("customAttribute");
    parentEl->appendChild(el);

    genTextAttr(&el, "id", id);

    const CustomAttribute* ca = property->getCustomAttribute(id);
    switch (ca->getType())
    {
        case CAT_Text:
        {
            QDomElement cEl = doc->createElement("textAttribute");
            el.appendChild(cEl);

            genTextAttr(&cEl, "text", static_cast<const TextAttribute*>(ca)->getText());
            break;
        }
        case CAT_Reference:
        {
            QDomElement cEl = doc->createElement("referenceAttribute");
            el.appendChild(cEl);

            const ReferenceAttribute* a =
                static_cast<const ReferenceAttribute*>(ca);
            genTextAttr(&cEl, "url", a->getURL());
            genTextAttr(&cEl, "label", a->getLabel());
            break;
        }
        default:
            qFatal("XMLReport::"
                   "generateCustomAttributeValue: "
                   "Unknown CA Type %d",
                   ca->getType());
    }

    return true;
}

bool
XMLReport::generateAllocate(QDomElement* parentEl, const Task* t)
{
    for (QPtrListIterator<Allocation> ai = t->getAllocationIterator();
         *ai; ++ai)
    {
        QDomElement el = doc->createElement("allocate");
        parentEl->appendChild(el);

        for (QPtrListIterator<Resource> ri = (*ai)->getCandidatesIterator();
             ri != 0; ++ri)
        {
            QDomElement aEl = doc->createElement("candidate");
            el.appendChild(aEl);
            genTextAttr(&aEl, "resourceId", (*ri)->getId());
        }
    }

    return true;
}

bool
XMLReport::generateBookingList(QDomElement* parentEl,
                               TaskList& filteredTaskList,
                               ResourceList& filteredResourceList)
{
    QDomElement el = doc->createElement("bookingList");
    parentEl->appendChild(el);

    for (ResourceListIterator rli(filteredResourceList); *rli != 0; ++rli)
    {
        for (QValueListIterator<int> sit = scenarios.begin();
             sit != scenarios.end(); ++sit)
        {
            QDomElement scEl = doc->createElement("resourceBooking");
            el.appendChild(scEl);
            genTextAttr(&scEl, "resourceId", (*rli)->getId());
            genTextAttr(&scEl, "scenarioId", project->getScenarioId(*sit));

            BookingList bl = (*rli)->getJobs(*sit);
            bl.setAutoDelete(true);
            if (bl.isEmpty())
                continue;

            for (BookingList::Iterator bli(bl); *bli != 0; ++bli)
            {
                if (filteredTaskList.findRef((*bli)->getTask()) >= 0)
                {
                    QDomElement bEl = doc->createElement("booking");
                    scEl.appendChild(bEl);

                    genDateElement(&bEl, "start", (*bli)->getStart());
                    genDateElement(&bEl, "end", (*bli)->getEnd() + 1);
                    genTextAttr(&bEl, "taskId",
                                stripTaskRoot((*bli)->getTask()->getId()));
                }
            }
        }
    }
    return true;
}

bool
XMLReport::addAccountAttribute(const QString& aa)
{
    if (aa == KW("all"))
    {
        for (QDictIterator<CustomAttributeDefinition>
             it(project->getAccountAttributeDict()); *it; ++it)
        {
            accountAttributes.append(it.currentKey());
        }

        return true;
    }

    /* Make sure the 'ta' is a valid attribute name and that we don't
     * insert it twice into the list. Trying to insert it twice it not an
     * error though. */
    if (project->getAccountAttribute(aa) == 0)
        return false;

    if (accountAttributes.findIndex(aa) >= 0)
        return true;
    accountAttributes.append(aa);
    return true;
}

bool
XMLReport::addTaskAttribute(const QString& ta)
{
    if (ta == KW("all"))
    {
        QMap<QString, int>::ConstIterator it;
        for (it = TaskAttributeDict.begin(); it != TaskAttributeDict.end();
             ++it)
        {
            if (taskAttributes.findIndex(it.key()) < 0)
                taskAttributes.append(it.key());
        }
        for (QDictIterator<CustomAttributeDefinition>
             it(project->getTaskAttributeDict()); *it; ++it)
            taskAttributes.append(it.currentKey());

        return true;
    }

    /* Make sure the 'ta' is a valid attribute name and that we don't
     * insert it twice into the list. Trying to insert it twice it not an
     * error though. */
    if (TaskAttributeDict.find(ta) == TaskAttributeDict.end() &&
        project->getTaskAttribute(ta) == 0)
        return false;

    if (taskAttributes.findIndex(ta) >= 0)
        return true;
    taskAttributes.append(ta);
    return true;
}

void
XMLReport::genTextAttr(QDomElement* parentEl, const QString& name,
                       const QString& text)
{
   QDomAttr at = doc->createAttribute(name);
   at.setValue(text);
   parentEl->setAttributeNode(at);
}

void
XMLReport::genDoubleAttr(QDomElement* parentEl, const QString& name,
                         double val)
{
   QDomAttr at = doc->createAttribute(name);
   at.setValue(QString::number(val));
   parentEl->setAttributeNode(at);
}

void
XMLReport::genLongAttr(QDomElement* parentEl, const QString& name, long val)
{
   QDomAttr at = doc->createAttribute(name);
   at.setValue(QString::number(val));
   parentEl->setAttributeNode(at);
}

void
XMLReport::genTextElement(QDomElement* parentEl, const QString& name,
                          const QString& text)
{
    QDomElement el = doc->createElement(name);
    el.appendChild(doc->createTextNode(text));
    parentEl->appendChild(el);
}

void
XMLReport::genDateElement(QDomElement* parentEl, const QString& name,
                          time_t val)
{
   QDomElement el = doc->createElement(name);
   parentEl->appendChild(el);
   QDomText tEl = doc->createTextNode(QString::number(val));
   el.appendChild(tEl);

   QDomAttr at = doc->createAttribute("humanReadable");
   at.setValue(time2user(val, timeFormat));
   el.setAttributeNode(at);

   parentEl->appendChild(el);
}

void
XMLReport::genTimeElement(QDomElement* parentEl, const QString& name,
                          time_t val)
{
   QDomElement el = doc->createElement(name);
   parentEl->appendChild(el);
   QDomText tEl = doc->createTextNode(QString::number(val));
   el.appendChild(tEl);

   QDomAttr at = doc->createAttribute("humanReadable");
   at.setValue(time2user(val, shortTimeFormat, false));
   el.setAttributeNode(at);

   parentEl->appendChild(el);
}

