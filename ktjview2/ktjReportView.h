#ifndef _KTJREPORTVIEW_H_
#define _KTJREPORTVIEW_H_

#include <klistview.h>

class KtjReportView: public KListView
{
    Q_OBJECT
public:
    /**
     * Constructor
     */
    KtjReportView( QWidget *parent=0, const char *name=0 );

    /**
     * @override
     * remove the columns before clearing the data
     */
    virtual void clear();

protected:
    virtual QString tooltip( QListViewItem *item, int column ) const;
    virtual bool showTooltip( QListViewItem *, const QPoint &, int ) const;

private:
    /**
     * Remove all the columns from the view
     */
    void removeColumns();
};

#endif
