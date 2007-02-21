/*
 * Report.cpp - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include "HTMLReport.h"

#include "Project.h"
#include "Utility.h"

HTMLReport::HTMLReport(Project* p, const QString& f, const QString& df,
                       int dl) :
    Report(p, f, df, dl),
    HTMLPrimitives(),
    rawStyleSheet(),
    rawHead(),
    rawTail()
{
    s.setEncoding(QTextStream::Latin1);
}

void
HTMLReport::generateHeader(const QString& title)
{
    s << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\""
        " \"http://www.w3.org/TR/REC-html40/loose.dtd\">"
      << endl;
    if (timeStamp)
        s << "<!-- Generated by TaskJuggler v"VERSION" -->" << endl;
    s << "<!-- For details about TaskJuggler see "
      << TJURL << " -->" << endl
      << "<html>" << endl
      << "<head>" << endl
      << "<title>" << (headline.isEmpty() ? title : headline) << "</title>"
      << endl
      << "<meta http-equiv=\"Content-Type\" content=\"text/html; "
      << "charset=utf-8\"/>" << endl;
#if 0
    s << "<script type=\"text/javascript\">" << endl
      << "  function tj_tooltip(el,flag)" << endl
      << "  {" << endl
      << "    elem = document.getElementById(el);" << endl
      << "    if (flag) {" << endl
      << "      elem.parentNode.parentNode.style.zIndex=1000;" << endl
      << "      elem.parentNode.parentNode.style.borderRight="
      << "'0px solid #000';" << endl
      << "      elem.style.visibility='visible';" << endl
      << "    }" << endl
      << "    else {" << endl
      << "      elem.parentNode.parentNode.style.zIndex=1;" << endl
      << "      elem.parentNode.parentNode.style.border='none';" << endl
      << "      elem.style.visibility='hidden' };" << endl
      << "  }" << endl
      << "</script>" << endl
      << "<style type=\"text/css\">" << endl
      << "<!--" << endl
      << ".tj_tooltip" << endl
      << "{" << endl
      << "    background-color:#FFFFE1;" << endl
      << "    border:1px solid black;" << endl
      << "    font-size:80%;" << endl
      << "    font-weight:normal;" << endl
      << "    line-height:normal;" << endl
      << "    text-align:left;" << endl
      << "    position:absolute;" << endl
      << "    z-index:100;" << endl
      << "    padding:0.5em;" << endl
      << "}" << endl
      << "-->" << endl
      << "</style>" << endl;
#endif
    if (!rawStyleSheet.isEmpty())
        s << rawStyleSheet << endl;
    s << "</head>" << endl
      << "<body>" << endl;

    if (!rawHead.isEmpty())
        s << rawHead << endl;
    if (!headline.isEmpty())
        s << "<h1>" << htmlFilter(headline) << "</h1>" << endl;
    if (!caption.isEmpty())
        s << "<p>" << htmlFilter(caption) << "</p>" << endl;
}

void
HTMLReport::generateFooter()
{
    if (!rawTail.isEmpty())
        s << rawTail << endl;

    if (timeStamp)
    {
        s << "<p><span style=\"font-size:0.7em\">";
        if (!project->getCopyright().isEmpty())
            s << htmlFilter(project->getCopyright()) << " - ";
        s << "Version " << htmlFilter(project->getVersion())
            << " - Created on " << time2user(time(0), timeFormat)
            << " with <a HREF=\"" << TJURL <<
            "\">TaskJuggler</a> <a HREF=\"" << TJURL << "/download.php\">v"
            << VERSION << "</a></span></p>" << endl << "</body></html>\n";
    }
}

