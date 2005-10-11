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

#include <assert.h>

#include <qwidgetstack.h>
#include <qstring.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

#include <kmainwindow.h>
#include <kstatusbar.h>
#include <klistview.h>
#include <klistviewsearchline.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kstdaction.h>
#include <kaction.h>
#include <klineedit.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editorchooser.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/editinterface.h>
#include <ktexteditor/selectioninterface.h>
#include <ktexteditor/clipboardinterface.h>
#include <ktexteditor/undointerface.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/printinterface.h>

#include <kdebug.h>

#include "CoreAttributes.h"
#include "Report.h"
#include "FileManager.h"
#include "ManagedFileInfo.h"
#include "FindDialog.h"

FileManager::FileManager(KMainWindow* m, QWidgetStack* v, KListView* b,
                         KListViewSearchLine* s) :
    mainWindow(m), viewStack(v), browser(b), searchLine(s)
{
    masterFile = 0;
    editorConfigured = FALSE;

    findDialog = 0;
    lastMatchLine = lastMatchCol = (uint) -1;

    // We don't want the URL column to be visible. This is internal data only.
    browser->setColumnWidthMode(1, QListView::Manual);
    browser->hideColumn(1);
}

FileManager::~FileManager()
{
    clear();
}

void
FileManager::updateFileList(const QStringList& fl, const KURL& url)
{
    if (fl.isEmpty())
        return;

    // First we mark all files as no longer part of the project.
    for (std::list<ManagedFileInfo*>::iterator fli = files.begin();
         fli != files.end(); ++fli)
        (*fli)->setPartOfProject(FALSE);

    /* Then we add the new ones and mark the existing ones as part of the
     * project again. */
    for (QStringList::ConstIterator sli = fl.begin(); sli != fl.end(); ++sli)
    {
        KURL url;
        url.setPath(*sli);

        bool newFile = TRUE;
        // Is this file being managed already?
        for (std::list<ManagedFileInfo*>::iterator mfi = files.begin();
             mfi != files.end(); ++mfi)
        {
            if ((*mfi)->getFileURL() == url)
            {
                (*mfi)->setPartOfProject(TRUE);
                newFile = FALSE;
                break;
            }
        }

        if (newFile)
        {
            // No, so let's add it to our internal list.
            ManagedFileInfo* mfi =
                new ManagedFileInfo(this, KURL::fromPathOrURL(*sli));
            mfi->setPartOfProject(TRUE);
            files.push_back(mfi);
        }
    }
    setMasterFile(url);

    updateFileBrowser();

    if (browser->currentItem())
        showInEditor(getCurrentFileURL());
    else
        showInEditor(files.front()->getFileURL());
}

void
FileManager::addFile(const KURL& url, const KURL& newURL)
{
    // Add new file to list of managed files.
    ManagedFileInfo* mfi = new ManagedFileInfo(this, url);
    files.push_back(mfi);
    // First show the file with the old name so it get's loaded.
    showInEditor(url);
    mfi->saveAs(newURL);

    // Insert the file into the browser and update the directory hierachy if
    // necessary.
    updateFileBrowser();
    QListViewItem* lvi = mfi->getBrowserEntry();
    lvi->setSelected(TRUE);
    browser->ensureItemVisible(lvi);

    // Open new file in editor.
    showInEditor(newURL);
}

QString
FileManager::findCommonPath()
{
    if (files.empty())
        return QString::null;

    int lastMatch = 0;
    int end;
    QString firstURL = files.front()->getFileURL().url();
    while ((end = firstURL.find("/", lastMatch)) >= 0)
    {
        for (std::list<ManagedFileInfo*>::iterator mfi = files.begin();
             mfi != files.end(); ++mfi)
        {
            QString url = (*mfi)->getFileURL().url();
            if (url.left(end) != firstURL.left(end))
                goto done;
        }
        lastMatch = end + 1;
    }
done:
    return firstURL.left(lastMatch);
}

void
FileManager::updateFileBrowser()
{
    QString commonPath = findCommonPath();

    QStringList openDirectories, closedDirectories;
    for (QListViewItemIterator lvi(browser); *lvi; ++lvi)
        if ((*lvi)->firstChild())
            if ((*lvi)->isOpen())
                openDirectories.append((*lvi)->text(1));
            else
                closedDirectories.append((*lvi)->text(1));
    QString currentFile;
    if (browser->currentItem())
        currentFile = browser->currentItem()->text(1);

    // Remove all entries from file browser
    browser->clear();

    for (std::list<ManagedFileInfo*>::iterator mfi = files.begin();
         mfi != files.end(); ++mfi)
    {
        /* Now we are inserting the file into the file browser again.
         * We don't care about the common path and create tree nodes for each
         * remaining directory. So we can browse the files in a directory like
         * tree. */
        QString url = (*mfi)->getFileURL().url();
        // Remove common path from URL.
        QString shortenedURL = url.right(url.length() -
                                         commonPath.length());
        KListViewItem* currentDir = 0;
        int start = 0;
        /* The remaining file name is traversed directory by directory. If
         * there is no node yet for this directory in the browser, we create a
         * directory node. */
        for (int dirNameEnd = -1;
             (dirNameEnd = shortenedURL.find("/", start)) > 0;
             start = dirNameEnd + 1)
        {
            KListViewItem* lvi;
            if ((lvi = static_cast<KListViewItem*>
                 (browser->findItem(shortenedURL.left(dirNameEnd), 1))) == 0)
            {
                if (!currentDir)
                    currentDir =
                        new KListViewItem(browser,
                                          shortenedURL.left(dirNameEnd),
                                          shortenedURL.left(dirNameEnd));
                else
                    currentDir =
                        new KListViewItem(currentDir,
                                          shortenedURL.mid(start, dirNameEnd -
                                                           start),
                                          shortenedURL.left(dirNameEnd));
                currentDir->setPixmap
                    (0, KGlobal::iconLoader()->loadIcon("folder",
                                                        KIcon::Small));
            }
            else
                currentDir = lvi;

            if (openDirectories.find(currentDir->text(1)) !=
                openDirectories.end())
                currentDir->setOpen(true);
            else if (closedDirectories.find(currentDir->text(1)) !=
                     closedDirectories.end())
                currentDir->setOpen(false);
            else
                currentDir->setOpen(true);
        }
        KListViewItem* newFile;
        if (currentDir)
            newFile =
                new KListViewItem(currentDir,
                                  shortenedURL.right(shortenedURL.length() -
                                                 start), url);
        else
            newFile =
                new KListViewItem(browser,
                                  shortenedURL.right(shortenedURL.length() -
                                                     start), url);

        if (newFile->text(1) == currentFile)
            browser->setCurrentItem(newFile);

        // Save the pointer to the browser entry.
        (*mfi)->setBrowserEntry(newFile);

        /* Decorate files with a file icon. The master file will be decorated
         * differently so it can be easyly identified. */
        if (*mfi == masterFile)
            newFile->setPixmap
                (0, KGlobal::iconLoader()->
                 loadIcon("tj_file_tjp", KIcon::Small));
        else
            newFile->setPixmap
                (0, KGlobal::iconLoader()->
                 loadIcon("tj_file_tji", KIcon::Small));

        /* The 3rd column shows whether the file is part of the current
         * project or not. So we need to set the proper icons. */
        if ((*mfi)->isPartOfProject())
            newFile->setPixmap
                (3, KGlobal::iconLoader()->loadIcon("tj_ok", KIcon::Small));
        else
            newFile->setPixmap
                (3, KGlobal::iconLoader()->loadIcon("tj_not_ok", KIcon::Small));
    }

    searchLine->updateSearch();
}

void
FileManager::setMasterFile(const KURL& url)
{
    for (std::list<ManagedFileInfo*>::iterator mfi = files.begin();
         mfi != files.end(); ++mfi)
        if ((*mfi)->getFileURL() == url)
        {
            masterFile = *mfi;
            return;
        }
    // Master file must be in list of managed files.
    assert(0);
}

const KURL&
FileManager::getMasterFileURL() const
{
    static KURL dummy;
    return masterFile ? masterFile->getFileURL() : dummy;
}

bool
FileManager::isProjectLoaded() const
{
    return masterFile != 0;
}

void
FileManager::showInEditor(const KURL& url)
{
    for (std::list<ManagedFileInfo*>::iterator mfi = files.begin();
         mfi != files.end(); ++mfi)
        if ((*mfi)->getFileURL() == url)
        {
            if (!(*mfi)->getEditor())
            {
                // The file has not yet been loaded, so we create an editor for
                // it.
                KTextEditor::Document* document;
                if (!(document = KTextEditor::EditorChooser::createDocument
                      (viewStack, "KTextEditor::Document", "Editor")))
                {
                    KMessageBox::error
                        (viewStack,
                         i18n("A KDE text-editor component could not "
                              "be found;\nplease check your KDE "
                              "installation."));
                }
                if (!editorConfigured)
                {
                    KTextEditor::configInterface(document)->readConfig(config);
                    editorConfigured = TRUE;
                }

                KTextEditor::View* editor =
                    document->createView(viewStack);
                viewStack->addWidget(editor);
                (*mfi)->setEditor(editor);
                editor->setMinimumSize(400, 200);
                editor->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
                                                  QSizePolicy::Maximum, 0, 85,
                                                  editor->sizePolicy()
                                                  .hasHeightForWidth()));
                document->openURL(url);
                document->setReadWrite(TRUE);
                document->setModified(FALSE);

                // Signal to update the file-modified status
                connect(document, SIGNAL(textChanged()),
                        *mfi, SLOT(setModified()));

                // Signal to en- or disable clipboard actions
                connect(document, SIGNAL(selectionChanged()),
                         this, SLOT(enableClipboardActions()));

                // Signal to en- or disable undo actions
                connect(document, SIGNAL(undoChanged()),
                         this, SLOT(enableUndoActions()));
            }
            viewStack->raiseWidget((*mfi)->getEditor());

            browser->setSelected((*mfi)->getBrowserEntry(), TRUE);

            break;
        }
}

void
FileManager::showInEditor(const KURL& url, int line, int col)
{
    showInEditor(url);
    setCursorPosition(line, col);
    setFocusToEditor();
}

void
FileManager::showInEditor(CoreAttributes* ca)
{
    KURL url = KURL(ca->getDefinitionFile());
    int line, col;
    line = ca->getDefinitionLine();
    col = 0;
    showInEditor(url, line, col);
}

void
FileManager::showInEditor(const Report* report)
{
    KURL url = KURL(report->getDefinitionFile());
    int line, col;
    line = report->getDefinitionLine();
    col = 0;
    showInEditor(url, line, col);
}

KURL
FileManager::getCurrentFileURL() const
{
    return files.empty() ? KURL() :
        KURL::fromPathOrURL(browser->currentItem()->text(1));
}

void
FileManager::readProperties(KConfig* cfg)
{
    // We can't do much here for now. As soon as we have the first editor
    // part, we use the config pointer to read the editor properties. So we
    // only store the pointer right now.
    config = cfg;
}

void
FileManager::writeProperties(KConfig* config)
{
    if (getCurrentFile())
        KTextEditor::configInterface(getCurrentFile()->getEditor()->
                                     document())->writeConfig(config);

    for (std::list<ManagedFileInfo*>::iterator mfi = files.begin();
         mfi != files.end(); ++mfi)
        (*mfi)->writeProperties(config);
}

ManagedFileInfo*
FileManager::getCurrentFile() const
{
    if (files.empty())
        return 0;

    KURL url = getCurrentFileURL();
    for (std::list<ManagedFileInfo*>::const_iterator mfi = files.begin();
         mfi != files.end(); ++mfi)
        if ((*mfi)->getFileURL() == url)
            return *mfi;

    return 0;
}

QString
FileManager::getWordUnderCursor() const
{
    static QString dummy;

    ManagedFileInfo* mfi = getCurrentFile();
    if (!mfi)
        return dummy;

    return mfi->getWordUnderCursor();
}

void
FileManager::saveCurrentFile()
{
    if (getCurrentFile())
        getCurrentFile()->save();
}

void
FileManager::saveCurrentFileAs(const KURL& url)
{
    if (getCurrentFile())
    {
        getCurrentFile()->saveAs(url);
        updateFileBrowser();
    }
}

void
FileManager::closeCurrentFile()
{
    ManagedFileInfo* mfi;
    if ((mfi = getCurrentFile()) != 0)
    {
        viewStack->removeWidget(mfi->getEditor());
        viewStack->raiseWidget(0);

        for (std::list<ManagedFileInfo*>::iterator mfit = files.begin();
             mfit != files.end(); ++mfit)
            if (*mfit == mfi)
            {
                if (masterFile == mfi)
                    masterFile = 0;

                delete mfi;
                mfi = 0;
                files.erase(mfit);
                break;
            }
        assert(mfi == 0);

        updateFileBrowser();
    }
}

void
FileManager::saveAllFiles()
{
    for (std::list<ManagedFileInfo*>::iterator mfi = files.begin();
         mfi != files.end(); ++mfi)
        (*mfi)->save();
}

void
FileManager::clear()
{
    for (std::list<ManagedFileInfo*>::iterator it = files.begin();
         it != files.end(); ++it)
        delete *it;
    files.clear();
    masterFile = 0;
}

void
FileManager::find()
{
    if (!findDialog)
    {
        findDialog = new FindDialog(mainWindow);
        findDialog->setCaption(QString("TaskJuggler"));
        connect(findDialog->findButton, SIGNAL(clicked()),
                this, SLOT(startSearch()));
    }

    findDialog->show();
}

void
FileManager::findNext()
{
    search();
}

void
FileManager::findPrevious()
{
    searchBackwards = !searchBackwards;
    search();
    searchBackwards = !searchBackwards;
}

void
FileManager::startSearch()
{
    if (!getCurrentFile())
        return;

    searchPattern = findDialog->pattern->text();
    searchCaseSensitive = findDialog->caseSensitiveCB->isChecked();
    searchFromCursor = findDialog->fromCursorCB->isChecked();
    searchBackwards = findDialog->backwardsCB->isChecked();

    if (searchPattern.isEmpty())
        return;

    KTextEditor::View* editor = getCurrentFile()->getEditor();

    if (searchFromCursor)
    {
        KTextEditor::viewCursorInterface(editor)->
            cursorPositionReal(&lastMatchLine, &lastMatchCol);
    }
    else
    {
        if (searchBackwards)
        {
            KTextEditor::Document* document = editor->document();
            lastMatchLine =
                KTextEditor::editInterface(document)->numLines() - 1;
            lastMatchCol = KTextEditor::editInterface(document)->
                lineLength(lastMatchLine) - 1;
            matchLen = 0;
        }
        else
            lastMatchLine = lastMatchCol = matchLen = 0;
    }
    search();
}

void
FileManager::search()
{
    if (!getCurrentFile() || searchPattern.isEmpty())
        return;

    KTextEditor::View* editor = getCurrentFile()->getEditor();
    KTextEditor::Document* document = editor->document();

    if (searchBackwards)
    {
        /* This is a rather ugly workaround for the strange searchText()
         * behaviour. In backwards mode it find the string right of the
         * position as well. So we need to move the last find position one
         * character to the left. */
        if (lastMatchCol > 0)
            lastMatchCol -= 1;
        else
        {
            lastMatchCol = KTextEditor::editInterface(document)->
                lineLength(--lastMatchLine);
        }
    }
    else
        lastMatchCol += matchLen;

    /* Now try to find the text pattern. */
    if (KTextEditor::searchInterface(document)->searchText
        (lastMatchLine, lastMatchCol, searchPattern,
         &lastMatchLine, &lastMatchCol, &matchLen,
         searchCaseSensitive, searchBackwards))
    {
        // Found it!
        KTextEditor::viewCursorInterface(editor)->
            setCursorPosition(lastMatchLine, lastMatchCol);
        mainWindow->statusBar()->message("");
    }
    else
    {
        // Nothing found. Prepare wrap-around.
        if (searchBackwards)
        {
            mainWindow->statusBar()->message
                (i18n("Nothing found. Press F3 again to start from bottom!"));
            lastMatchLine =
                KTextEditor::editInterface(document)->numLines() - 1;
            lastMatchCol = KTextEditor::editInterface(document)->
                lineLength(lastMatchLine) - 1;
        }
        else
        {
            mainWindow->statusBar()->message
                (i18n("Nothing found. Press F3 again to start from top!"));
            lastMatchLine = lastMatchCol = 0;
        }
    }
}

void
FileManager::undo()
{
    if (getCurrentFile())
        KTextEditor::undoInterface(getCurrentFile()->getEditor()->
                                   document())->undo();
}

void
FileManager::redo()
{
    if (getCurrentFile())
        KTextEditor::undoInterface(getCurrentFile()->getEditor()->
                                   document())->redo();
}

void
FileManager::cut()
{
    if (getCurrentFile())
        KTextEditor::clipboardInterface(getCurrentFile()->getEditor())->cut();
}

void
FileManager::copy()
{
    if (getCurrentFile())
        KTextEditor::clipboardInterface(getCurrentFile()->getEditor())->copy();
}

void
FileManager::paste()
{
    if (getCurrentFile())
        KTextEditor::clipboardInterface(getCurrentFile()->getEditor())->paste();
}

void
FileManager::selectAll()
{
    if (getCurrentFile())
        KTextEditor::selectionInterface(getCurrentFile()->getEditor()->
                                        document())->selectAll();
}

void
FileManager::print()
{
    if (getCurrentFile())
        KTextEditor::printInterface(getCurrentFile()->getEditor()->
                                    document())->print();
}

void
FileManager::configureEditor()
{
    if (getCurrentFile())
        KTextEditor::configInterface(getCurrentFile()->getEditor()->
                                     document())->configDialog();
}

void
FileManager::enableEditorActions(bool enable)
{
    mainWindow->action(KStdAction::name(KStdAction::Save))->setEnabled(enable);
    mainWindow->action(KStdAction::name(KStdAction::SelectAll))->
        setEnabled(enable);
    mainWindow->action("configure_editor")->setEnabled(enable);
    mainWindow->action(KStdAction::name(KStdAction::Find))->setEnabled(enable);
    mainWindow->action(KStdAction::name(KStdAction::FindNext))->
        setEnabled(enable);
    mainWindow->action(KStdAction::name(KStdAction::FindPrev))->setEnabled(enable);

    enableClipboardActions(enable);
    enableUndoActions(enable);
}

void
FileManager::enableClipboardActions(bool enable)
{
    bool hasSelection = FALSE;
    if (getCurrentFile())
        hasSelection = KTextEditor::selectionInterface
            (getCurrentFile()->getEditor()->document())->hasSelection();
    bool isClipEmpty = QApplication::clipboard()->
        text(QClipboard::Clipboard).isEmpty();

    mainWindow->action(KStdAction::name(KStdAction::Cut))->
        setEnabled(enable && hasSelection );
    mainWindow->action(KStdAction::name(KStdAction::Copy))->
        setEnabled(enable && hasSelection);
    mainWindow->action( KStdAction::name(KStdAction::Paste))->
        setEnabled( enable && !isClipEmpty);
}

void
FileManager::enableUndoActions(bool enable)
{
    bool undoEnable = FALSE;
    if (getCurrentFile())
        undoEnable = (KTextEditor::undoInterface
                      (getCurrentFile()->getEditor()->document())->
                      undoCount() > 0 );
    bool redoEnable = FALSE;
    if (getCurrentFile())
        redoEnable = (KTextEditor::undoInterface
                      (getCurrentFile()->getEditor()->document())->
                      redoCount() > 0 );

    mainWindow->action(KStdAction::name(KStdAction::Undo))->
        setEnabled(enable && undoEnable);
    mainWindow->action(KStdAction::name(KStdAction::Redo))->
        setEnabled(enable && redoEnable);
}

void
FileManager::setCursorPosition(int line, int col)
{
    if (getCurrentFile())
        KTextEditor::viewCursorInterface(getCurrentFile()->getEditor())->
            setCursorPosition(line, col);
}

void
FileManager::setFocusToEditor() const
{
    if (getCurrentFile())
        getCurrentFile()->getEditor()->setFocus();
}

#include "FileManager.moc"
