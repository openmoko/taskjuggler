/*
 * Account.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003 by Chris Schlaeger <cs@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _Account_h_
#define _Account_h_

#include <qstring.h>
#include <qlist.h>
#include <time.h>

#include "CoreAttributes.h"
#include "AccountList.h"
#include "TransactionList.h"

class Interval;

#include "taskjuggler.h"

/**
 * @short Stores all account related information.
 * @author Chris Schlaeger <cs@suse.de>
 */
class Account : public CoreAttributes
{
public:
    Account(Project* p, const QString& i, const QString& n, Account* pr,
            AccountType at);
    virtual ~Account();

    virtual CAType getType() const { return CA_Account; }

    Account* getParent() const { return (Account*) parent; }

    AccountListIterator getSubListIterator() const
    {
        return AccountListIterator(sub);
    }

    void inheritValues();

    void setKotrusId(const QString& k) { kotrusId = k; }
    const QString& getKotrusId() const { return kotrusId; }

    void setAcctType(AccountType at) { acctType = at; }
    AccountType getAcctType() const { return acctType; }

    void credit(Transaction* t);
    bool isGroup() const { return !sub.isEmpty(); }

    double getBalance(int sc, time_t d) const;
    double getVolume(int sc, const Interval& period) const;

private:
    Account() { };  // don't use this
    QString kotrusId;
    TransactionList transactions;
    AccountType acctType;
} ;

#endif
