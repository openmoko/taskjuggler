/**************************************************************************
**
** Copyright (C) 2002-2003 Integrated Computer Solutions, Inc.
** All rights reserved.
**
** This file is part of the QicsTable Product.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.ics.com/qt/licenses/gpl/ for GPL licensing information.
**
** If you would like to use this software under a commericial license
** for the development of proprietary software, send email to sales@ics.com
** for details.
**
** Contact info@ics.com if any conditions of this licensing are
** not clear to you.
**
**************************************************************************/


#ifndef _QICSUTIL_H
#define _QICSUTIL_H

#include <qframe.h>
#include <qfont.h>

class QicsGrid;
class QicsGridInfo;
class QicsStyleManager;

#ifndef QICS_MIN
# define QICS_MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef QICS_MAX
# define QICS_MAX(a,b) ((a) > (b) ? (a) : (b))
#endif




extern const char *Qics_arrow_xpm[];



int qicsHeightOfFont(const QFont &fnt);

int qicsWidthOfFont(const QFont &fnt);

#endif /* _QICSUTIL_H */
