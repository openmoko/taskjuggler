/*
 * ReferenceAttribute.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */
#ifndef _ReferenceAttribute_h_
#define _ReferenceAttribute_h_

#include <qstring.h>

#include "CustomAttribute.h"

/*
 * @short User defined attribute that holds a text and a link.
 * @author Chris Schlaeger <cs@suse.de>
 */
class ReferenceAttribute : public CustomAttribute
{
public:
    ReferenceAttribute() { }
    ReferenceAttribute(const QString& u, const QString& l) :
        url(u), label(l) { }
    virtual ~ReferenceAttribute() { }

    CustomAttributeType getType() const { return CAT_Reference; }
    void setUrl(const QString& u) { url = u; }
    const QString& getUrl() const { return url; }

    void setLabel(const QString& l) { label = l; }
    const QString& getLabel() const { return label; }

private:
    QString url;
    QString label;
} ;

#endif


