/*
 * ktjview.h - TaskJuggler Viewer
 *
 * Copyright (c) 2001, 2002 by Klaas Freitag <freitag@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */
#ifndef KTJVIEW_H
#define KTJVIEW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <kapplication.h>
#include <kparts/mainwindow.h>

#define KTJVIEW_VERSION "0.2"

class KToggleAction;

/**
 * This is the application "Shell".  It has a menubar, toolbar, and
 * statusbar but relies on the "Part" to do all the real work.
 *
 * @short Application Shell
 * @author Klaas Freitag <freitag@kde.org>
 * @version 0.1
 */
class KTjview : public KParts::MainWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    KTjview();

    /**
     * Default Destructor
     */
    virtual ~KTjview();

    /**
     * Use this method to load whatever file/URL you have
     */
    void load(const KURL& url);

protected:
    /**
     * This method is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfig *);

    /**
     * This method is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(KConfig *);

private slots:
    void fileNew();
    void optionsShowToolbar();
    void optionsShowStatusbar();
    void optionsConfigureKeys();
    void optionsConfigureToolbars();

    void applyNewToolbarConfig();

private:
    void setupAccel();
    void setupActions();

private:
    KParts::ReadOnlyPart *m_part;

    KToggleAction *m_toolbarAction;
    KToggleAction *m_statusbarAction;
};

#endif // KTJVIEW_H
