/*
 * CSVTaskReport.cpp - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include "CSVTaskReport.h"

bool
CSVTaskReport::generate()
{
    if (!open())
        return FALSE;

    generateHeader();
    tab->generate();
    generateFooter();

    f.close();
    return TRUE;
}
