/*
 * Allocation.cpp - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include "qdom.h"

#include "Resource.h"
#include "Allocation.h"
#include "ReportXML.h"

#define KW(a) a

/* -- DTD --
    <!ELEMENT Allocation (Load, Persistent)>
    <!ELEMENT Load       (#PCDATA)>
    <!ELEMENT Persistent (#PCDATA)>
    <!ATTLIST Allocation ResourceID CDATA #REQUIRED>
  /-- DTD --/
*/

Allocation::Allocation() :
    load(0), lockedResource(0)
{
    shifts.setAutoDelete(TRUE);
    selectionMode = order;
    persistent = mandatory = FALSE;
}

Allocation::Allocation(const Allocation& a)
{
    shifts.setAutoDelete(TRUE);

    load = a.load;
    persistent = a.persistent;
    mandatory = a.mandatory;
    lockedResource = a.lockedResource;
    selectionMode = a.selectionMode;
    
    for (QPtrListIterator<ShiftSelection> sli(a.shifts); *sli; ++sli)
        shifts.append(new ShiftSelection(**sli));

    candidates = a.candidates;    
}

/* Creation of the XML Reprsentation of the Allocation */
QDomElement Allocation::xmlElement( QDomDocument& doc )
{
   QDomElement elem = doc.createElement( "Allocation" );
   elem.appendChild(ReportXML::createXMLElem( doc, "Load", QString::number(getLoad()) ));
   elem.appendChild(ReportXML::createXMLElem( doc, "Persistent", isPersistent() ? "Yes":"No" ));
   elem.setAttribute( "ResourceID", candidates.getFirst()->getId());
   
   /* candidates are missing TODO */
   return elem;

}

bool
Allocation::setSelectionMode(const QString& smt)
{
    if (smt == KW("order"))
        selectionMode = order;
    else if (smt == KW("minloaded"))
        selectionMode = minLoaded;
    else if (smt == KW("maxloaded"))
        selectionMode = maxLoaded;
    else if (smt == KW("random"))
        selectionMode = random;
    else
        return FALSE;
    return TRUE;
}

