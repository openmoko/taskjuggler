/*
 * The TaskJuggler Project Management Software
 *
 * Copyright (c) 2001, 2002, 2003, 2004, 2005 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _TjReportCell_h_
#define _TjReportCell_h_

#include <qstring.h>
#include <qnamespace.h>

class TjReportRow;
class TjReportColumn;

class TjReportCell
{
public:
    TjReportCell(TjReportRow* r, TjReportColumn* c);

    ~TjReportCell() { }

    void setText(const QString& t) { text = t; }
    const QString& getText() const { return text; }

    TjReportRow* getRow() const { return row; }
    TjReportColumn* getColumn() const { return column; }

    void setIndentLevel(int idl) { indentLevel = idl; }
    int getIndentLevel() const { return indentLevel; }

private:
    TjReportRow* row;
    TjReportColumn* column;

    QString text;
    int indentLevel;
} ;

#endif

