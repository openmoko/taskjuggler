/*
 * ExpressionParser.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _ExpressionParser_h_
#define _ExpressionParser_h_

#include "Tokenizer.h"

class Project;
class Operation;
class ExpressionTree;

/**
 * @short This class can be used to generate an @ref ExpressionTree from a
 * file stream or QString.
 * @author Chris Schlaeger <cs@kde.org>
 */
class ExpressionParser
{
public:
    ExpressionParser(const QString& text, const Project* proj) :
        tokenizer(text), project(proj)
    { }

    Operation* parse();

private:
    Operation* parseLogicalExpression(int precedence);
    Operation* parseFunctionCall(const QString& token);

    void errorMessage(const QString& msg);

    Tokenizer tokenizer;
    const Project* project;
};

#endif

