/*
 * TaskJuggler Viewer
 *
 * Copyright (c) 2001, 2002 by Klaas Freitag <freitag@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */
#include <unistd.h>

#include "ktjview_part.h"
#include "ktjview.h"
#include <kshortcut.h>
#include <kiconloader.h>
#include <kaccel.h>
#include <kaction.h>
#include <kinstance.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kfiledialog.h>
#include <kparts/genericfactory.h>
#include <kdebug.h>
#include <qsplitter.h>
#include <kdatewidget.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qmultilineedit.h>
#include <kmessagebox.h>
#include "ktvtasktable.h"
#include "ktjgantt.h"
#include "ktvtaskcanvasview.h"

#include "TjMessageHandler.h"
#include <kshortcut.h>
#include "XMLFile.h"

/* Handler for TaskJuggler error messages. */
TjMessageHandler TJMH(TRUE);

typedef KParts::GenericFactory<KTjviewPart> KTjviewPartFactory;
K_EXPORT_COMPONENT_FACTORY( libktjviewpart, KTjviewPartFactory );

KTjviewPart::KTjviewPart( QWidget *parentWidget, const char *,
			  QObject *parent, const char *name,
			  const QStringList & /*args*/ )
    : KParts::ReadOnlyPart(parent, name),
      m_parentWidget( parentWidget )
    
{
    // we need an instance
    setInstance( KTjviewPartFactory::instance() );

    // this should be your custom internal widget
    m_gantt = new KTJGantt( parentWidget, "GANTT" );
    connect( m_gantt, SIGNAL( statusBarChange( const QString& )),
	     this, SLOT(  slChangeStatusBar( const QString& )));

    // notify the part that this is our internal widget
    setWidget(m_gantt);

    // create our actions
    setupActions();
    // set our XML-UI resource file
    setXMLFile("ktjview_part.rc");

}

void KTjviewPart::setupActions()
{
    KStdAction::open(this, SLOT(fileOpen()), actionCollection());
    (void) new KAction( i18n("&Reload"), "reload",
			KShortcut( "Ctrl+R" ), this,
			SLOT(slReload()), actionCollection(), "reload");

    (void) new KAction(i18n("Zoom In"), "viewmag+", CTRL+Key_I,
		       m_gantt, SLOT(slZoomIn()),
		       actionCollection(), "zoomIn" );

    (void) new KAction(i18n("Zoom Out"), "viewmag-", CTRL+Key_O,
		       m_gantt, SLOT(slZoomOut()),
		       actionCollection(), "zoomOut" );

    (void) new KAction(i18n("Zoom 1:1"), "viewmag1", CTRL+Key_S,
		       m_gantt, SLOT(slZoomOriginal()),
		       actionCollection(), "zoomOriginal" );

    (void) new KAction(i18n("Timeframe"), "history", CTRL+Key_T,
		       m_gantt, SLOT(slTimeFrame()),
		       actionCollection(), "timeFrame" );

    KToggleAction *act = new KToggleAction(i18n("Toggle Gantt View Visibility"),
					   "today", CTRL+Key_F9,
					   m_gantt, SLOT(slToggleGanttVisible()),
					   actionCollection(), "toggleGantt");
    act->setChecked(true);


}


KTjviewPart::~KTjviewPart()
{
}


KAboutData *KTjviewPart::createAboutData()
{
    // the non-i18n name here must be the same as the directory in
    // which the part's rc file is installed ('partrcdir' in the
    // Makefile)
    KAboutData *aboutData = new KAboutData("ktjviewpart", I18N_NOOP("KTjviewPart"),
					   KTJVIEW_VERSION /* from ktjview.h */ );
    aboutData->addAuthor("Klaas Freitag", 0, "freitag@kde.org");
    return aboutData;
}

bool KTjviewPart::openFile()
{
    // m_file is always local so we can use QFile on it
    QFile file(m_file);
    if (file.open(IO_ReadOnly) == false)
        return false;

    m_project = new Project();

    XMLFile* xf = new XMLFile(m_project);

    char cwd[1024];
    if (getcwd(cwd, 1023) == 0)
        qFatal("main(): getcwd() failed");

    if (!xf->readDOM(m_file, QString(cwd) + "/", "", TRUE))
	exit(EXIT_FAILURE);

    if( ! xf->parse() )
    {
	KMessageBox::error( m_parentWidget, i18n("The XML File to read is not valid for Taskjuggler.\n"
						 "KTJView requires XML following the schema of Taskjuggler V. 2.x!\n"
						 "To create a correct xml file for ktjview, add the following to your project:\n"
						 "   xmlreport \"foo.tjx\" {"
						 "       version 2"
						 "   }"),
			    i18n("XML Read Error") );
    }
    else
    {
	m_project->pass2(false);
	m_gantt->showProject(m_project);

	// just for fun, set the status bar
	emit setStatusBarText( m_url.prettyURL() );
    }

    delete xf;

    return true;
}

void KTjviewPart::slReload()
{
    slChangeStatusBar( i18n("Reverting file"));

    // FIXME: Delete the project. Project cores if deleted (08/2003) !
    m_gantt->clear();
    delete m_project;
    m_project = 0;
    openFile();

}


void KTjviewPart::slChangeStatusBar( const QString& str )
{
   emit setStatusBarText( str );
}


void KTjviewPart::fileOpen()
{
    // this slot is called whenever the File->Open menu is selected,
    // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
    // button is clicked
    QString file_name = KFileDialog::getOpenFileName(QString::null,
						     "*.xml *.tjx *.XML *.TJX|TaskJuggler XML output (v. 2.x)");

    if (file_name.isEmpty() == false)
        openURL(file_name);
}


#include "ktjview_part.moc"
