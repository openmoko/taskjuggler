/*
 * ProjectFile.cpp - TaskJuggler
 *
 * Copyright (c) 2001, 2002 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#include <ctype.h>
#include <stdio.h>
#include <stream.h>
#include <unistd.h>

#include <qregexp.h>

#include "ProjectFile.h"
#include "Project.h"
#include "Token.h"
#include "ExpressionTree.h"
#include "kotrus.h"

extern Kotrus *kotrus;

#define READ_DATE(a, b) \
(token == a) \
{ \
	if ((tt = nextToken(token)) == DATE) \
		task->b(date2time(token)); \
	else \
	{ \
		fatalError("Date expected"); \
		return FALSE; \
	} \
}

FileInfo::FileInfo(ProjectFile* p, const QString& file_)
	: pf(p)
{
	tokenTypeBuf = INVALID;
	file = file_;
}

bool
FileInfo::open()
{
	if ((f = fopen(file, "r")) == 0)
		return FALSE;

	lineBuf = "";
	currLine = 1;
	return TRUE;
}

bool
FileInfo::close()
{
	if (fclose(f) == EOF)
		return FALSE;

	return TRUE;
}

int
FileInfo::getC(bool expandMacros)
{
 BEGIN:
	int c;
	if (ungetBuf.isEmpty())
	{
		c = getc(f);
	}
	else
	{
		c = ungetBuf.last();
		ungetBuf.remove(ungetBuf.fromLast());
		if (c == EOM)
		{
			macroStack.removeLast();
			pf->getMacros().popArguments();
			goto BEGIN;
		}
	}
	lineBuf += c;

	if (expandMacros)
	{
		if (c == '$')
		{
			int d;
			if ((d = getC()) == '{')
			{
				// remove $ from lineBuf;
				lineBuf = lineBuf.left(lineBuf.length() - 1);
				readMacroCall();
				goto BEGIN;
			}
			else
				ungetC(d);
		}
	}

	return c;
}

void
FileInfo::ungetC(int c)
{
	lineBuf = lineBuf.left(lineBuf.length() - 1);
	ungetBuf.append(c);
}

bool
FileInfo::getDateFragment(QString& token, int& c)
{
	token += c;
	c = getC();
	// c must be a digit
	if (!isdigit(c))
	{
		fatalError("Corrupted date");
		return FALSE;
	}
	token += c;
	// read other digits
	while ((c = getC()) != EOF && isdigit(c))
		token += c;

	return TRUE;
}

QString
FileInfo::getPath() const
{
	if (file.find('/'))
		return file.left(file.findRev('/') + 1);
	else
		return "";
}

TokenType
FileInfo::nextToken(QString& token)
{
	if (tokenTypeBuf != INVALID)
	{
		token = tokenBuf;
		TokenType tt = tokenTypeBuf;
		tokenTypeBuf = INVALID;
		return tt;
	}

	token = "";

	// skip blanks and comments
	for ( ; ; )
	{
		int c = getC();
		switch (c)
		{
		case EOF:
			return EndOfFile;
		case ' ':
		case '\t':
			break;
		case '#':	// Comments start with '#' and reach towards end of line
			while ((c = getC(FALSE)) != '\n' && c != EOF)
				;
			if (c == EOF)
				return EndOfFile;
			// break missing on purpose
		case '\n':
			// Increase line counter only when not replaying a macro.
			if (macroStack.isEmpty())
				currLine++;
			lineBuf = "";
			break;
		default:
			ungetC(c);
			goto BLANKS_DONE;
		}
	}
 BLANKS_DONE:

	// analyse non blank characters
	for ( ; ; )
	{
		int c = getC();
		if (c == EOF)
		{
			fatalError("Unexpected end of file");
			return EndOfFile;
		}
		else if (isalpha(c) || (c == '_') || (c == '!'))
		{
			token += c;
			while ((c = getC()) != EOF &&
				   (isalnum(c) || (c == '_') || (c == '.') || (c == '!')))
				token += c;
			ungetC(c);
			if (token.contains('!') || token.contains('.'))
				return RELATIVE_ID;
			else
				return ID;
		}
		else if (isdigit(c))
		{
			// read first number (maybe a year)
			token += c;
			while ((c = getC()) != EOF && isdigit(c))
				token += c;
			if (c == '-')
			{
				// this must be a ISO date yyyy-mm-dd[-hh:mm]
				getDateFragment(token, c);
				if (c != '-')
				{
					fatalError("Corrupted date");
					return EndOfFile;
				}
				getDateFragment(token, c);
				if (c == '-')
				{
					getDateFragment(token, c);
					if (c != ':')
					{
						fatalError("Corrupted date");
						return EndOfFile;
					}
					getDateFragment(token, c);
				}
				ungetC(c);
				return DATE;
			}
			else if (c == '.')
			{
				// must be a real number
				token += c;
				while ((c = getC()) != EOF && isdigit(c))
					token += c;
				ungetC(c);
				return REAL;
			}
			else if (c == ':')
			{
				// must be a time (HH:MM)
				token += c;
				for (int i = 0; i < 2; i++)
				{
					if ((c = getC()) != EOF && isdigit(c))
						token += c;
					else
					{
						fatalError("2 digits minutes expected");
						return EndOfFile;
					}
				}
				return HOUR;
			}
			else
			{
				ungetC(c);
				return INTEGER;
			}
		}
		else if (c == '"')
		{
			// quoted string
			while ((c = getC()) != EOF && c != '"')
			{
				if (c == '\n')
					currLine++;
				token += c;
			}
			if (c == EOF)
			{
				fatalError("Non terminated string");
				return EndOfFile;
			}
			return STRING;
		}
		else if (c == '[')
		{
			int nesting = 0;
			while ((c = getC(FALSE)) != EOF && (c != ']' || nesting > 0))
			{
				if (c == '[')
					nesting++;
				else if (c == ']')
					nesting--;
				if (c == '\n')
					currLine++;
				token += c;
			}
			if (c == EOF)
			{
				fatalError("Non terminated macro definition");
				return EndOfFile;
			}
			return MacroBody;
		}
		else
		{
			token += c;
			switch (c)
			{
			case '{':
				return LCBRACE;
			case '}':
				return RCBRACE;
			case '(':
				return LBRACE;
			case ')':
				return RBRACE;
			case ',':
				return COMMA;
			case '~':
				return TILDE;
			case '-':
				return MINUS;
			case '&':
				return AND;
			case '|':
				return OR;
			default:
				fatalError(QString("Illegal character '") + c + "'");
				return EndOfFile;
			}
		}
	}
}

bool
FileInfo::readMacroCall()
{
	QString id;
	TokenType tt;
	if ((tt = nextToken(id)) != ID && tt != INTEGER)
	{
		fatalError("Macro ID expected");
		return FALSE;
	}
	QString token;
	// Store all arguments in a newly created string list.
	QStringList* sl = new QStringList;
	while ((tt = nextToken(token)) == STRING)
		sl->append(token);
	if (tt != RCBRACE)
	{
		fatalError("'}' expected");
		return FALSE;
	}

	// push string list to global argument stack
	pf->getMacros().pushArguments(sl);

	// expand the macro
	QString macro = pf->getMacros().expand(id);
	if (macro.isNull())
	{
		fatalError(QString("Unknown macro ") + id);
		return FALSE;
	}

	// Push pointer to macro on stack. Needed for error handling.
	macroStack.append(pf->getMacros().getMacro(id));

	// mark end of macro
	ungetC(EOM);
	// push expanded macro reverse into ungetC buffer.
	for (int i = macro.length() - 1; i >= 0; --i)
		ungetC(macro[i].latin1());
	return TRUE;
}

void
FileInfo::returnToken(TokenType tt, const QString& buf)
{
	if (tokenTypeBuf != INVALID)
	{
		qFatal("Internal Error: Token buffer overflow!");
		return;
	}
	tokenTypeBuf = tt;
	tokenBuf = buf;
}

void
FileInfo::fatalError(const QString& msg)
{
	if (macroStack.isEmpty())
	{
		qWarning("%s:%d:%s", file.latin1(), currLine, msg.latin1());
		qWarning("%s", lineBuf.latin1());
	}
	else
	{
		qWarning("Error in expanded macro");
		qWarning("%s:%d: %s",
				 macroStack.last()->getFile().latin1(),
				 macroStack.last()->getLine(), msg.latin1());
		qWarning("%s", lineBuf.latin1());
	}
}

ProjectFile::ProjectFile(Project* p)
{
	proj = p;

	openFiles.setAutoDelete(TRUE);
}

bool
ProjectFile::open(const QString& file)
{
	QString absFileName = file;
	if (absFileName[0] != '/')
	{
		if (openFiles.isEmpty())
		{
			char buf[1024];
			if (getcwd(buf, 1023) != 0)
				absFileName = QString(buf) + "/" + absFileName;
		}
		else
			absFileName = openFiles.last()->getPath() + absFileName;
	}
	while (absFileName.find("/../") >= 0)
	{
		int end = absFileName.find("/../");
		int start = absFileName.findRev('/', end - 1);
		absFileName.replace(start, end + strlen("/../") - start, "/");
	}

	// Make sure that we include each file only once.
	if (includedFiles.findIndex(absFileName) != -1)
		return TRUE;
		
	FileInfo* fi = new FileInfo(this, absFileName);

	if (!fi->open())
	{
		qFatal("Cannot open '%s'", absFileName.latin1());
		return FALSE;
	}

	openFiles.append(fi);
	includedFiles.append(absFileName);
	return TRUE;
}

bool
ProjectFile::close()
{
	bool error = FALSE;

	FileInfo* fi = openFiles.getLast();

	if (!fi->close())
		error = TRUE;
	openFiles.removeLast();

	return error;
}

bool
ProjectFile::parse()
{
	TokenType tt;
	QString token;

	for ( ; ; )
	{
		switch (tt = nextToken(token))
		{
		case EndOfFile:
			return TRUE;
		case ID:
			if (token == "task")
			{
				if (!readTask(0))
					return FALSE;
				break;
			}
			if (token == "account")
			{
				if (!readAccount(0))
					return FALSE;
				break;
			}
			else if (token == "resource")
			{
				if (!readResource(0))
					return FALSE;
				break;
			}
			else if (token == "vacation")
			{
				time_t from, to;
				QString name;
				if (!readVacation(from, to, TRUE, &name))
					return FALSE;
				proj->addVacation(name, from, to);
				break;
			}
			else if (token == "priority")
			{
				int priority;
				if (!readPriority(priority))
					return FALSE;
				proj->setPriority(priority);
				break;
			}
			else if (token == "now")
			{
				if (nextToken(token) != DATE)
				{
					fatalError("Date expected");
					return FALSE;
				}
				proj->setNow(date2time(token));
				break;
			}
			else if (token == "mineffort")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value exptected");
					return FALSE;
				}
				proj->setMinEffort(token.toDouble());
				break;
			}
			else if (token == "maxeffort")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value exptected");
					return FALSE;
				}
				proj->setMaxEffort(token.toDouble());
				break;
			}
			else if (token == "rate")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value exptected");
					return FALSE;
				}
				proj->setRate(token.toDouble());
				break;
			}
			else if (token == "currency")
			{
				if (nextToken(token) != STRING)
				{
					fatalError("String expected");
					return FALSE;
				}
				proj->setCurrency(token);
				break;
			}
			else if (token == "currencydigits")
			{
				if (nextToken(token) != INTEGER)
				{
					fatalError("Integer value expected");
					return FALSE;
				}
				proj->setCurrencyDigits(token.toInt());
				break;
			}
			else if (token == "timingresolution")
			{
				ulong resolution;
				if (!readTimeValue(resolution))
					return FALSE;
				if (resolution < 60 * 5)
				{
					fatalError("scheduleGranularity must be at least 5 min");
					return FALSE;
				}
				proj->setScheduleGranularity(resolution);
				break;
			}
			else if (token == "copyright")
			{
				if (nextToken(token) != STRING)
				{
					fatalError("String expected");
					return FALSE;
				}
				proj->setCopyright(token);
				break;
			}
			else if (token == "include")
			{
				if (!readInclude())
					return FALSE;
				break;
			}
			else if (token == "macro")
			{
				QString id;
				if (nextToken(id) != ID)
				{
					fatalError("Macro ID expected");
					return FALSE;
				}
				QString file = openFiles.last()->getFile();
				uint line = openFiles.last()->getLine();
				if (nextToken(token) != MacroBody)
				{
					fatalError("Macro body expected");
					return FALSE;
				}
				Macro* macro = new Macro(id, token, file, line);
				if (!macros.addMacro(macro))
				{
					fatalError("Macro has been defined already");
					delete macro;
					return FALSE;
				}
				break;
			}
			else if (token == "flags")
			{
				for ( ; ; )
				{
					QString flag;
					if (nextToken(flag) != ID)
					{
						fatalError("flag ID expected");
						return FALSE;
					}

					/* Flags can be declared multiple times, but we
					 * register a flag only once. */
					if (!proj->isAllowedFlag(flag))
						proj->addAllowedFlag(flag);

					if ((tt = nextToken(token)) != COMMA)
					{
						openFiles.last()->returnToken(tt, token);
						break;
					}
				}
				break;
			}
			else if (token == "project")
			{
				if (nextToken(token) != ID)
				{
					fatalError("Project ID expected");
					return FALSE;
				}
				if (!proj->addId(token))
				{
					fatalError(QString().sprintf(
						"Project ID %s has already been registered",
						token.latin1()));
					return FALSE;
				}
				if (nextToken(token) != STRING)
				{
					fatalError("Project name expected");
					return FALSE;
				}
				proj->setName(token);
				if (nextToken(token) != STRING)
				{
					fatalError("Version string expected");
					return FALSE;
				}
				proj->setVersion(token);
				time_t start, end;
				if (nextToken(token) != DATE)
				{
					fatalError("Start date expected");
					return FALSE;
				}
				start = date2time(token);
				if (nextToken(token) != DATE)
				{
					fatalError("End date expected");
					return FALSE;
				}
				end = date2time(token);
				if (end <= start)
				{
					fatalError("End date must be larger then start date");
					return FALSE;
				}
				proj->setStart(start);
				proj->setEnd(end);
				break;
			}
			else if (token == "projectid")
			{
				for ( ; ; )
				{
					QString id;
					if (nextToken(id) != ID)
					{
						fatalError("Project ID expected");
						return FALSE;
					}

					if (!proj->addId(id))
					{
						fatalError(QString().sprintf(
							"Project ID %s has already been registered",
							id.latin1()));
						return FALSE;
					}

					if ((tt = nextToken(token)) != COMMA)
					{
						openFiles.last()->returnToken(tt, token);
						break;
					}
				}
				break;
			}
			else if (token == "xmltaskreport" )
			{
			   if( !readXMLTaskReport())
			      return FALSE;
			   break;
			}
			else if (token == "htmltaskreport" ||
					 token == "htmlresourcereport")
			{
				if (!readHTMLReport(token))
					return FALSE;
				break;
			}
			else if (token == "htmlaccountreport")
			{
				if (!readHTMLAccountReport())
					return FALSE;
				break;
			}
			else if( token == "kotrusmode" )
			{
			   if( kotrus )
			   {
			      if( nextToken(token) != STRING )
			      {
				 kotrus->setKotrusMode( "NoKotrus" );
			      }
			      else
			      {
				 if( token == "db" || token == "xml" || token == "nokotrus" )
				    kotrus->setKotrusMode( token );
				 else
				 {
				    fatalError( "Unknown kotrus-mode");
				    return( false );
				 }
			      }
			   }
			   break;
			}
			// break missing on purpose!
		default:
			fatalError(QString("Syntax Error at '") + token + "'!");
			return FALSE;
		}
	}

	return TRUE;
}

TokenType
ProjectFile::nextToken(QString& buf)
{
	TokenType tt;
	while ((tt = openFiles.last()->nextToken(buf)) == EndOfFile)
	{
		close();
		if (openFiles.isEmpty())
			return EndOfFile;
	}

	return tt;
}

void
ProjectFile::fatalError(const QString& msg)
{
	if (openFiles.isEmpty())
		qWarning("Unexpected end of file found. Probably a missing '}'.");
	else
		openFiles.last()->fatalError(msg);
}

bool
ProjectFile::readInclude()
{
	QString token;

	if (nextToken(token) != STRING)
	{
		fatalError("File name expected");
		return FALSE;
	}
	if (!open(token))
		return FALSE;

	return TRUE;
}

bool
ProjectFile::readTask(Task* parent)
{
	TokenType tt;
	QString token;

	QString id;
	if ((tt = nextToken(id)) != ID &&
		(tt != RELATIVE_ID))
	{
		fatalError("ID expected");
		return FALSE;
	}

	if (tt == RELATIVE_ID)
	{
		/* If a relative ID has been specified the task is declared out of
		 * it's actual scope. So we have to set 'parent' to point to the
		 * correct parent task. */
		do
		{
			if (id[0] == '!')
			{
				if (parent != 0)
					parent = parent->getParent();
				else
				{
					fatalError("Invalid relative task ID");
					return FALSE;
				}
				id = id.right(id.length() - 1);
			}
			else if (id.find('.') >= 0)
			{
				QString tn = (parent ? parent->getId() + "." : QString())
					+ id.left(id.find('.'));
				TaskList tl;
				if (parent)
					parent->getSubTaskList(tl);
				else
					tl = proj->getTaskList();
				bool found = FALSE;
				for (Task* t = tl.first(); t != 0; t = tl.next())
					if (t->getId() == tn)
					{
						parent = t;
						id = id.right(id.length() - id.find('.') - 1);
						found = TRUE;
						break;
					}
				if (!found)
				{
					fatalError(QString("Task ") + tn + " unknown");
					return FALSE;
				}
			}
		} while (id[0] == '!' || id.find('.') >= 0);
	}

	QString name;
	if ((tt = nextToken(name)) != STRING)
	{
		fatalError("String expected");
		return FALSE;
	}

	if ((tt = nextToken(token)) != LCBRACE)
	{
		fatalError("{ expected");
		return FALSE;
	}

	if (parent)
	{
		id = parent->getId() + "." + id;
		// We need to check that the task id has not been declared before.
		TaskList tl;
		parent->getSubTaskList(tl);
		for (Task* t = tl.first(); t != 0; t = tl.next())
			if (t->getId() == id)
			{
				fatalError(QString().sprintf(
					"Task %s has already been declared", id.latin1()));
				return FALSE;
			}
	}

	Task* task = new Task(proj, id, name, parent, getFile(), getLine());

	proj->addTask(task);
	if (parent)
		parent->addSub(task);

	for (bool done = false ; !done; )
	{
		switch (tt = nextToken(token))
		{
		case ID:
			if (token == "task")
			{
				if (!readTask(task))
					return FALSE;
			}
			else if (token == "note")
			{
				if ((tt = nextToken(token)) == STRING)
					task->setNote(token);
				else
				{
					fatalError("String expected");
					return FALSE;
				}
			}
			else if (token == "milestone")
			{
				task->setMilestone();
			}
			else if READ_DATE("start", setPlanStart)
			else if READ_DATE("end", setPlanEnd)
			else if READ_DATE("minstart", setMinStart)
			else if READ_DATE("maxstart", setMaxStart)
			else if READ_DATE("minend", setMinEnd)
			else if READ_DATE("maxend", setMaxEnd)
			else if READ_DATE("actualstart", setActualStart)
			else if READ_DATE("actualsnd", setActualEnd)
			else if (token == "length")
			{
				double d;
				if (!readPlanTimeFrame(task, d))
					return FALSE;
				task->setPlanLength(d);
			}
			else if (token == "effort")
			{
				double d;
				if (!readPlanTimeFrame(task, d))
					return FALSE;
				task->setPlanEffort(d);
			}
			else if (token == "duration")
			{
				double d;
				if (!readPlanTimeFrame(task, d))
					return FALSE;
				task->setPlanDuration(d);
			}
			else if (token == "actuallength")
			{
				double d;
				if (!readPlanTimeFrame(task, d))
					return FALSE;
				task->setActualLength(d);
			}
			else if (token == "actualeffort")
			{
				double d;
				if (!readPlanTimeFrame(task, d))
					return FALSE;
				task->setActualEffort(d);
			}
			else if (token == "actualduration")
			{
				double d;
				if (!readPlanTimeFrame(task, d))
					return FALSE;
				task->setActualDuration(d);
			}
			else if (token == "complete")
			{
				if (nextToken(token) != INTEGER)
				{
					fatalError("Integer value expected");
					return FALSE;
				}
				int complete = token.toInt();
				if (complete < 0 || complete > 100)
				{
					fatalError("Value of complete must be between 0 and 100");
					return FALSE;
				}
				task->setComplete(complete);
			}
			else if (token == "responsible")
			{
				Resource* r;
				if (nextToken(token) != ID ||
					(r = proj->getResource(token)) == 0)
				{
					fatalError("Resource ID expected");
					return FALSE;
				}
				task->setResponsible(r);
			}
			else if (token == "allocate")
			{
				if (!readAllocate(task))
					return FALSE;
			}
			else if (token == "depends")
			{
				for ( ; ; )
				{
					QString id;
					if ((tt = nextToken(id)) != ID &&
						tt != RELATIVE_ID)
					{
						fatalError("Task ID expected");
						return FALSE;
					}
					task->addDependency(id);
					task->setScheduling(Task::ASAP);
					if ((tt = nextToken(token)) != COMMA)
					{
						openFiles.last()->returnToken(tt, token);
						break;
					}
				}
			}
			else if (token == "preceeds")
			{
				for ( ; ; )
				{
					QString id;
					if ((tt = nextToken(id)) != ID &&
						tt != RELATIVE_ID)
					{
						fatalError("Task ID expected");
						return FALSE;
					}
					task->addPreceeds(id);
					task->setScheduling(Task::ALAP);
					if ((tt = nextToken(token)) != COMMA)
					{
						openFiles.last()->returnToken(tt, token);
						break;
					}
				}
			}
			else if (token == "scheduling")
			{
				nextToken(token);
				if (token == "asap")
					task->setScheduling(Task::ASAP);
				else if (token == "alap")
					task->setScheduling(Task::ALAP);
				else
				{
					fatalError("Unknown scheduling policy");
					return FALSE;
				}
			}
			else if (token == "flags")
			{
				for ( ; ; )
				{
					QString flag;
					if (nextToken(flag) != ID || !proj->isAllowedFlag(flag))
					{
						fatalError("Flag unknown");
						return FALSE;
					}
					task->addFlag(flag);
					if ((tt = nextToken(token)) != COMMA)
					{
						openFiles.last()->returnToken(tt, token);
						break;
					}
				}
			}
			else if (token == "priority")
			{
				int priority;
				if (!readPriority(priority))
					return FALSE;
				task->setPriority(priority);
				break;
			}
			else if (token == "account")
			{
				QString account;
				if (nextToken(account) != ID ||
					proj->getAccount(account) == 0)
				{
					fatalError("Account ID expected");
					return FALSE;
				}
				task->setAccount(proj->getAccount(account));
				break;
			}
			else if (token == "startcredit")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value expected");
					return FALSE;
				}
				task->setStartCredit(token.toDouble());
				break;
			}
			else if (token == "endcredit")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value expected");
					return FALSE;
				}
				task->setEndCredit(token.toDouble());
				break;
			}
			else if (token == "projectid")
			{
				if (nextToken(token) != ID ||
					!proj->isValidId(token))
				{
					fatalError("Project ID expected");
					return FALSE;
				}
				task->setProjectId(token);
				break;
			}
			else if (token == "include")
			{
				if (!readInclude())
					return FALSE;
				break;
			}
			else
			{
				fatalError(QString("Illegal task attribute '")
						   + token + "'");
				return FALSE;
			}
			break;
		case RCBRACE:
			done = true;
			break;
		default:
			qDebug("%s", token.latin1());
			fatalError(QString("Syntax Error at '") + token + "'");
			return FALSE;
		}
	}

	if (task->getName().isEmpty())
	{
		fatalError(QString("No name specified for task ") + id + "!");
		return FALSE;
	}

	return TRUE;
}

bool
ProjectFile::readVacation(time_t& from, time_t& to, bool readName, QString* n)
{
	TokenType tt;
	if (readName)
	{
		if ((tt = nextToken(*n)) != STRING)
		{
			fatalError("String expected");
			return FALSE;
		}
	}
	QString start;
	if ((tt = nextToken(start)) != DATE)
	{
		fatalError("Date expected");
		return FALSE;
	}
	QString token;
	if ((tt = nextToken(token)) != MINUS)
	{
		// vacation e. g. 2001-11-28
		openFiles.last()->returnToken(tt, token);
		from = date2time(start);
		to = sameTimeNextDay(date2time(start)) - 1;
	}
	else
	{
		// vacation e. g. 2001-11-28 - 2001-11-30
		QString end;
		if ((tt = nextToken(end)) != DATE)
		{
			fatalError("Date expected");
			return FALSE;
		}
		from = date2time(start);
		if (date2time(start) > date2time(end))
		{
			fatalError("Vacation must start before end");
			return FALSE;
		}
		to = date2time(end) - 1;
	}
	return TRUE;
}

bool
ProjectFile::readResource(Resource* parent)
{
	// Syntax: 'resource id "name" { ... }
	QString id;
	if (nextToken(id) != ID)
	{
		fatalError("ID expected");
		return FALSE;
	}
	QString name;
	if (nextToken(name) != STRING)
	{
		fatalError("String expected");
		return FALSE;
	}

	if (proj->getResource(id))
	{
		fatalError(QString().sprintf(
			"Resource %s has already been defined", id.latin1()));
		return FALSE;
	}

	Resource* r = new Resource(proj, id, name, parent);

	TokenType tt;
	QString token;
	if ((tt = nextToken(token)) == LCBRACE)
	{
		// read optional attributes
		while ((tt = nextToken(token)) != RCBRACE)
		{
			if (tt != ID)
			{
				fatalError(QString("Unknown attribute '") + token + "'");
				return FALSE;
			}
			if (token == "resource")
			{
				if (!readResource(r))
					return FALSE;
			}
			else if (token == "mineffort")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value exptected");
					return FALSE;
				}
				r->setMinEffort(token.toDouble());
			}
			else if (token == "maxeffort")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value exptected");
					return FALSE;
				}
				r->setMaxEffort(token.toDouble());
			}
			else if (token == "efficiency")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Read value expected");
					return FALSE;
				}
				r->setEfficiency(token.toDouble());
			}
			else if (token == "rate")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value exptected");
					return FALSE;
				}
				r->setRate(token.toDouble());
			}
			else if (token == "kotrusid")
			{
				if (nextToken(token) != STRING)
				{
					fatalError("String expected");
					return FALSE;
				}
				r->setKotrusId(token);
			}
			else if (token == "vacation")
			{
				time_t from, to;
				if (!readVacation(from, to))
					return FALSE;
				r->addVacation(new Interval(from, to));
			}
			else if (token == "workinghours")
			{
				int dow;
				QPtrList<Interval>* l = new QPtrList<Interval>();
				if (!readWorkingHours(dow, l))
					return FALSE;
				
				r->setWorkingHours(dow, l);
			}
			else if (token == "flags")
			{
				for ( ; ; )
				{
					QString flag;
					if (nextToken(flag) != ID || !proj->isAllowedFlag(flag))
					{
						fatalError("flag unknown");
						return FALSE;
					}
					r->addFlag(flag);
					if ((tt = nextToken(token)) != COMMA)
					{
						openFiles.last()->returnToken(tt, token);
						break;
					}
				}
			}
			else if (token == "include")
			{
				if (!readInclude())
					return FALSE;
				break;
			}
			else
			{
				fatalError(QString("Unknown attribute '") + token + "'");
				return FALSE;
			}
		}
	}
	else
		openFiles.last()->returnToken(tt, token);

	proj->addResource(r);

	return TRUE;
}

bool
ProjectFile::readAccount(Account* parent)
{
	// Syntax: 'account id "name" { ... }
	QString id;
	if (nextToken(id) != ID)
	{
		fatalError("ID expected");
		return FALSE;
	}

	if (proj->getAccount(id))
	{
		fatalError(QString().sprintf(
			"Account %s has already been defined", id.latin1()));
		return FALSE;
	}

	QString name;
	if (nextToken(name) != STRING)
	{
		fatalError("String expected");
		return FALSE;
	}
	Account::AccountType acctType;
	if (parent == 0)
	{
		/* Only accounts with no parent can have a type specifier. All
		 * sub accounts inherit the type of the parent. */
		QString at;
		if (nextToken(at) != ID && (at != "cost" || at != "revenue"))
		{
			fatalError("Account type 'cost' or 'revenue' expected");
			return FALSE;
		}
		acctType = at == "cost" ? Account::Cost : Account::Revenue;
	}
	else
		acctType = parent->getType();

	Account* a = new Account(proj, id, name, parent, acctType);
	if (parent)
		parent->addSub(a);

	TokenType tt;
	QString token;
	if ((tt = nextToken(token)) == LCBRACE)
	{
		bool hasSubAccounts = FALSE;
		bool cantBeParent = FALSE;
		// read optional attributes
		while ((tt = nextToken(token)) != RCBRACE)
		{
			if (tt != ID)
			{
				fatalError(QString("Unknown attribute '") + token + "'");
				return FALSE;
			}
			if (token == "account" && !cantBeParent)
			{
				if (!readAccount(a))
					return FALSE;
				hasSubAccounts = TRUE;
			}
			else if (token == "credit")
			{
				if (!readCredit(a))
					return FALSE;
			}
			else if (token == "kotrusid" && !hasSubAccounts)
			{
				if (nextToken(token) != STRING)
				{
					fatalError("String expected");
					return FALSE;
				}
				a->setKotrusId(token);
				cantBeParent = TRUE;
			}
			else if (token == "include")
			{
				if (!readInclude())
					return FALSE;
			}
			else
			{
				fatalError("Illegal attribute");
				return FALSE;
			}
		}
	}
	else
		openFiles.last()->returnToken(tt, token);

	proj->addAccount(a);

	return TRUE;
}

bool
ProjectFile::readCredit(Account* a)
{
	QString token;

	if (nextToken(token) != DATE)
	{
		fatalError("Date expected");
		return FALSE;
	}
	time_t date = date2time(token);

	QString description;
	if (nextToken(description) != STRING)
	{
		fatalError("String expected");
		return FALSE;
	}

	if (nextToken(token) != REAL)
	{
		fatalError("Real value expected");
		return FALSE;
	}
	Transaction* t = new Transaction(date, token.toDouble(), description);
	a->credit(t);

	return TRUE;
}

bool
ProjectFile::readAllocate(Task* t)
{
	QString id;
	Resource* r;
	if (nextToken(id) != ID || (r = proj->getResource(id)) == 0)
	{
		fatalError("Resource ID expected");
		return FALSE;
	}
	Allocation* a = new Allocation(r);
	QString token;
	TokenType tt;
	if ((tt = nextToken(token)) == LCBRACE)
	{
		while ((tt = nextToken(token)) != RCBRACE)
		{
			if (tt != ID)
			{
				fatalError(QString("Unknown attribute '") + token + "'");
				return FALSE;
			}
			if (token == "load")
			{
				if (nextToken(token) != REAL)
				{
					fatalError("Real value expected");
					return FALSE;
				}
				double load = token.toDouble();
				if (load < 0.01 || load > 1.0)
				{
					fatalError("Value must be in the range 0.01 - 1.0");
					return FALSE;
				}
				a->setLoad((int) (100 * load));
			}
			else if (token == "persistent")
			{
				a->setPersistent(TRUE);
			}
			else if (token == "alternative")
			{
				do
				{
					Resource* r;
					if ((tt = nextToken(token)) != ID ||
						(r = proj->getResource(token)) == 0)
					{
						fatalError("Resource ID expected");
						return FALSE;
					}
					a->addAlternative(r);
				} while ((tt = nextToken(token)) == COMMA);
				openFiles.last()->returnToken(tt, token);
			}
		}
	}
	else
		openFiles.last()->returnToken(tt, token);
	t->addAllocation(a);

	return TRUE;
}

bool
ProjectFile::readTimeValue(ulong& value)
{
	QString val;
	if (nextToken(val) != INTEGER)
	{
		fatalError("Integer value expected");
		return FALSE;
	}
	QString unit;
	if (nextToken(unit) != ID)
	{
		fatalError("Unit expected");
		return FALSE;
	}
	if (unit == "min")
		value = val.toULong() * 60;
	else if (unit == "h")
		value = val.toULong() * (60 * 60);
	else if (unit == "d")
		value = val.toULong() * (60 * 60 * 24);
	else if (unit == "w")
		value = val.toULong() * (60 * 60 * 24 * 7);
	else if (unit == "m")
		value = val.toULong() * (60 * 60 * 24 * 30);
	else if (unit == "y")
		value = val.toULong() * (60 * 60 * 24 * 356);
	else
	{
		fatalError("Unit expected");
		return FALSE;
	}
	return TRUE;
}

bool
ProjectFile::readPlanTimeFrame(Task* task, double& value)
{
	QString val;
	TokenType tt;
	if ((tt = nextToken(val)) != REAL && tt != INTEGER)
	{
		fatalError("Real value expected");
		return FALSE;
	}
	QString unit;
	if (nextToken(unit) != ID)
	{
		fatalError("Unit expected");
		return FALSE;
	}
	if (unit == "min")
		value = val.toDouble() / (8 * 60);
	else if (unit == "h")
		value = val.toDouble() / 8;
	else if (unit == "d")
		value = val.toDouble();
	else if (unit == "w")
		value = val.toDouble() * 5;
	else if (unit == "m")
		value = val.toDouble() * 20;
	else if (unit == "y")
		value = val.toDouble() * 240;
	else
	{
		fatalError("Unit expected");
		return FALSE;
	}

	return TRUE;
}

bool
ProjectFile::readWorkingHours(int& dayOfWeek, QPtrList<Interval>* l)
{
	l->setAutoDelete(TRUE);
	QString day;
	if (nextToken(day) != ID)
	{
		fatalError("Weekday expected");
		return FALSE;
	}
	const char* days[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
	for (dayOfWeek = 0; dayOfWeek < 7; dayOfWeek++)
		if (days[dayOfWeek] == day)
			break;
	if (dayOfWeek == 7)
	{
		fatalError("Weekday expected");
		return FALSE;
	}
	for ( ; ; )
	{
		QString start;
		if (nextToken(start) != HOUR)
		{
			fatalError("Start time as HH:MM expected");
			return FALSE;
		}
		QString token;
		if (nextToken(token) != MINUS)
		{
			fatalError("'-' expected");
			return FALSE;
		}
		QString end;
		if (nextToken(end) != HOUR)
		{
			fatalError("End time as HH:MM expected");
			return FALSE;
		}
		l->append(new Interval(hhmm2time(start), hhmm2time(end)));
		TokenType tt;
		if ((tt = nextToken(token)) != COMMA)
		{
			returnToken(tt, token);
			break;
		}
	}
	return TRUE;
}

bool
ProjectFile::readPriority(int& priority)
{
	QString token;

	if (nextToken(token) != INTEGER)
	{
		fatalError("Integer value expected");
		return FALSE;
	}
	priority = token.toInt();
	if (priority < 1 || priority > 1000)
	{
		fatalError("Priority value must be between 1 and 1000");
		return FALSE;
	}
	return TRUE;
}

bool
ProjectFile::readXMLTaskReport()
{
   QString token;
   if (nextToken(token) != STRING)
   {
      fatalError("File name expected");
      return FALSE;
   }
   ReportXML *rep = new ReportXML( proj, token, proj->getStart(), proj->getEnd());
   proj->addXMLReport( rep );

   return( true );
}


bool
ProjectFile::readHTMLReport(const QString& reportType)
{
	QString token;
	if (nextToken(token) != STRING)
	{
		fatalError("File name expected");
		return FALSE;
	}
	
	ReportHtml* report;
	if (reportType == "htmltaskreport")
		report = new HTMLTaskReport(proj, token, proj->getStart(),
									proj->getEnd());
	else if (reportType == "htmlresourcereport")
		report = new HTMLResourceReport(proj, token, proj->getStart(),
										proj->getEnd());
	else
	{
		qFatal("readHTMLReport: bad report type");
		return FALSE;	// Just to please the compiler.
	}
		
	TokenType tt;
	if ((tt = nextToken(token)) != LCBRACE)
	{
		openFiles.last()->returnToken(tt, token);
		return TRUE;
	}

	for ( ; ; )
	{
		if ((tt = nextToken(token)) == RCBRACE)
			break;
		else if (tt != ID)
		{
			fatalError("Attribute ID or '}' expected");
			return FALSE;
		}
		if (token == "columns")
		{
			report->clearColumns();
			for ( ; ; )
			{
				QString col;
				if ((tt = nextToken(col)) != ID)
				{
					fatalError("Column ID expected");
					return FALSE;
				}
				report->addColumn(col);
				if ((tt = nextToken(token)) != COMMA)
				{
					openFiles.last()->returnToken(tt, token);
					break;
				}
			}
		}
		else if (token == "start")
		{
			if (nextToken(token) != DATE)
			{
				fatalError("Date expected");
				return FALSE;
			}
			report->setStart(date2time(token));
		}
		else if (token == "end")
		{
			if (nextToken(token) != DATE)
			{
				fatalError("Date expected");
				return FALSE;
			}
			report->setEnd(date2time(token));
		}
		else if (token == "headline")
		{
			if (nextToken(token) != STRING)
			{
				fatalError("String exptected");
				return FALSE;
			}
			report->setHeadline(token);
		}
		else if (token == "caption")
		{
			if (nextToken(token) != STRING)
			{
				fatalError("String exptected");
				return FALSE;
			}
			report->setCaption(token);
		}
		else if (token == "showactual")
		{
			report->setShowActual(TRUE);
		}
		else if (token == "showprojectids")
		{
			report->setShowPIDs(TRUE);
		}
		else if (token == "hidetask")
		{
			Operation* op;
			if ((op = readLogicalExpression()) == 0)
				return FALSE;
			ExpressionTree* et = new ExpressionTree(op);
			report->setHideTask(et);
		}
		else if (token == "rolluptask")
		{
			Operation* op;
			if ((op = readLogicalExpression()) == 0)
				return FALSE;
			ExpressionTree* et = new ExpressionTree(op);
			report->setRollUpTask(et);
		}
		else if (token == "sorttasks")
		{
			if (!readSorting(report, 0))
				return FALSE;
		}
		else if (token == "hideresource")
		{
			Operation* op;
			if ((op = readLogicalExpression()) == 0)
				return FALSE;
			ExpressionTree* et = new ExpressionTree(op);
			report->setHideResource(et);
		}
		else if (token == "rollupresource")
		{
			Operation* op;
			if ((op = readLogicalExpression()) == 0)
				return FALSE;
			ExpressionTree* et = new ExpressionTree(op);
			report->setRollUpResource(et);
		}
		else if (token == "sortresources")
		{
			if (!readSorting(report, 1))
				return FALSE;
		}
		else
		{
			fatalError("Illegal attribute");
			return FALSE;
		}
	}

	if (reportType == "htmltaskreport")
		proj->addHTMLTaskReport((HTMLTaskReport*) report);
	else
		proj->addHTMLResourceReport((HTMLResourceReport*) report);

	return TRUE;
}

bool
ProjectFile::readHTMLAccountReport()
{
	QString token;
	if (nextToken(token) != STRING)
	{
		fatalError("File name expected");
		return FALSE;
	}
	
	HTMLAccountReport* report;
	report = new HTMLAccountReport(proj, token, proj->getStart(),
								   proj->getEnd());
		
	TokenType tt;
	if ((tt = nextToken(token)) != LCBRACE)
	{
		openFiles.last()->returnToken(tt, token);
		return TRUE;
	}

	for ( ; ; )
	{
		if ((tt = nextToken(token)) == RCBRACE)
			break;
		else if (tt != ID)
		{
			fatalError("Attribute ID or '}' expected");
			return FALSE;
		}
		if (token == "columns")
		{
			report->clearColumns();
			for ( ; ; )
			{
				QString col;
				if ((tt = nextToken(col)) != ID)
				{
					fatalError("Column ID expected");
					return FALSE;
				}
				report->addColumn(col);
				if ((tt = nextToken(token)) != COMMA)
				{
					openFiles.last()->returnToken(tt, token);
					break;
				}
			}
		}
		else if (token == "start")
		{
			if (nextToken(token) != DATE)
			{
				fatalError("Date expected");
				return FALSE;
			}
			report->setStart(date2time(token));
		}
		else if (token == "end")
		{
			if (nextToken(token) != DATE)
			{
				fatalError("Date expected");
				return FALSE;
			}
			report->setEnd(date2time(token));
		}
		else if (token == "headline")
		{
			if (nextToken(token) != STRING)
			{
				fatalError("String exptected");
				return FALSE;
			}
			report->setHeadline(token);
		}
		else if (token == "caption")
		{
			if (nextToken(token) != STRING)
			{
				fatalError("String exptected");
				return FALSE;
			}
			report->setCaption(token);
		}
		else if (token == "hideplan")
		{
			report->setHidePlan(TRUE);
		}
		else if (token == "showactual")
		{
			report->setShowActual(TRUE);
		}
		else if (token == "accumulate")
		{
			report->setAccumulate(TRUE);
		}
		else if (token == "hideaccount")
		{
			Operation* op;
			if ((op = readLogicalExpression()) == 0)
				return FALSE;
			ExpressionTree* et = new ExpressionTree(op);
			report->setHideAccount(et);
		}
		else if (token == "rollupaccount")
		{
			Operation* op;
			if ((op = readLogicalExpression()) == 0)
				return FALSE;
			ExpressionTree* et = new ExpressionTree(op);
			report->setRollUpAccount(et);
		}
		else if (token == "sortaccounts")
		{
			if (!readSorting(report, 2))
				return FALSE;
		}
		else
		{
			fatalError("Illegal attribute");
			return FALSE;
		}
	}

	proj->addHTMLAccountReport(report);

	return TRUE;
}

Operation*
ProjectFile::readLogicalExpression(int precedence)
{
	Operation* op;
	QString token;
	TokenType tt;

	qDebug("readLogicalExpression");
	if ((tt = nextToken(token)) == ID || tt == INTEGER)
	{
		if (tt == ID)
		{
			if (!proj->isAllowedFlag(token))
			{
				fatalError(QString("Flag ") + token + " is unknown.");
				qDebug("Done 0");
				return 0;
			}
			op = new Operation(token);
			qDebug(token);
		}
		else
			op = new Operation(token.toLong());
		if (precedence == 0)
		{
			if ((tt = nextToken(token)) != AND && tt != OR)
			{
				returnToken(tt, token);
			}
			else if (tt == AND)
			{
				qDebug("&");
				Operation* op2 = readLogicalExpression();
				op = new Operation(op, Operation::And, op2);
			}
			else if (tt == OR)
			{
				qDebug("|");
				Operation* op2 = readLogicalExpression();
				op = new Operation(op, Operation::Or, op2);
			}
		}
	}
	else if (tt == TILDE)
	{
		qDebug("~");
		if ((op = readLogicalExpression(1)) == 0)
		{
			qDebug("Done 0");
			return 0;
		}
		op = new Operation(op, Operation::Not);
	}
	else if (tt == LBRACE)
	{
		qDebug("(");
		if ((op = readLogicalExpression()) == 0)
		{
			qDebug("Done 0");
			return 0;
		}
		if ((tt = nextToken(token)) != RBRACE)
		{
			fatalError("')' expected");
			qDebug("Done 0");
			return 0;
		}
		qDebug(")");
	}
	else
	{
		fatalError("Logical expression expected");
		qDebug("Done 0");
		return 0;
	}
	
	qDebug("Done op");
	return op;
}

bool
ProjectFile::readSorting(Report* report, int which)
{
	QString token;

	nextToken(token);
	CoreAttributesList::SortCriteria sorting;
	if (token == "tree")
		sorting = CoreAttributesList::TreeMode;
	else if (token == "indexup")
		sorting = CoreAttributesList::IndexUp;
	else if (token == "indexdown")
		sorting = CoreAttributesList::IndexDown;
	else if (token == "idup")
		sorting = CoreAttributesList::IdUp;
	else if (token == "iddown")
		sorting = CoreAttributesList::IdDown;
	else if (token == "fullnameup")
		sorting = CoreAttributesList::FullNameUp;
	else if (token == "fullnamedown")
		sorting = CoreAttributesList::FullNameDown;
	else if (token == "nameup")
		sorting = CoreAttributesList::NameUp;
	else if (token == "namedown")
		sorting = CoreAttributesList::NameDown;
	else if (token == "startup")
		sorting = CoreAttributesList::StartUp;
	else if (token == "startdown")
		sorting = CoreAttributesList::StartDown;
	else if (token == "endup")
		sorting = CoreAttributesList::EndUp;
	else if (token == "enddown")
		sorting = CoreAttributesList::EndDown;
	else if (token == "priorityup")
		sorting = CoreAttributesList::PrioUp;
	else if (token == "prioritydown")
		sorting = CoreAttributesList::PrioDown;
	else if (token == "responsibleup")
		sorting = CoreAttributesList::ResponsibleUp;
	else if (token == "responsibledown")
		sorting = CoreAttributesList::ResponsibleDown;
	else if (token == "mineffortup")
		sorting = CoreAttributesList::MinEffortUp;
	else if (token == "mineffortdown")
		sorting = CoreAttributesList::MinEffortDown;
	else if (token == "maxeffortup")
		sorting = CoreAttributesList::MaxEffortUp;
	else if (token == "maxeffortdown")
		sorting = CoreAttributesList::MaxEffortDown;
	else if (token == "rateup")
		sorting = CoreAttributesList::RateUp;
	else if (token == "ratedown")
		sorting = CoreAttributesList::RateDown;
	else if (token == "kotrusidup")
		sorting = CoreAttributesList::KotrusIdUp;
	else if (token == "kotrusiddown")
		sorting = CoreAttributesList::KotrusIdDown;
	else
	{
		fatalError("Sorting criteria expected");
		return FALSE;
	}

	switch (which)
	{
	case 0:
		report->setTaskSorting(sorting);
		break;
	case 1:
		report->setResourceSorting(sorting);
		break;
	case 2:
		report->setAccountSorting(sorting);
		break;
	default:
		qFatal("readSorting: Unknown sorting attribute");
		return FALSE;
	}

	return TRUE;
}

time_t
ProjectFile::date2time(const QString& date)
{
	int y, m, d, hour, min;
	if (date.find(':') == -1)
	{
		sscanf(date, "%d-%d-%d", &y, &m, &d);
		hour = min = 0;
	}
	else
		sscanf(date, "%d-%d-%d-%d:%d", &y, &m, &d, &hour, &min);

	if (y < 1970)
	{
		fatalError("Year must be larger than 1969");
		y = 1970;
	}
	if (m < 1 || m > 12)
	{
		fatalError("Month must be between 1 and 12");
		m = 1;
	}
	if (d < 1 || d > 31)
	{
		fatalError("Day must be between 1 and 31");
		d = 1;
	}

	struct tm t = { 0, min, hour, d, m - 1, y - 1900, 0, 0, -1, 0, 0 };
	time_t localTime = mktime(&t);

	return localTime;
}

time_t
ProjectFile::hhmm2time(const QString& hhmm)
{
	int hour, min;
	sscanf(hhmm, "%d:%d", &hour, &min);
	return hour * 60 * 60 + min * 60;
}
