#include "ktjTaskReport.h"

// TJ includes
#include "Interval.h"
#include "Utility.h"

KTJTaskReport::KTJTaskReport( Project * proj )
    : KTJReport( proj )
{

}

QString KTJTaskReport::name() const
{
    return i18n( "Task Report" );
}

QString KTJTaskReport::description() const
{
    return i18n( "Task report combined with allocated resources." );
}

QicsDataModel KTJTaskReport::generate() const
{
    int columns = generateHeader(); // generate the column header

    TaskListIterator tasks = m_proj->getTaskListIterator();
    Task * task = 0;

    while ( ( task = static_cast<Task *>( tasks.current() ) ) != 0 )
    {
        ++it;

        if ( task->isContainer() ) // skip groups
            continue;

        generateRow( task );    // generate main rows
    }
}

int KTJTaskReport::generateHeader()
{
    QString format;             // corresponds with strftime(3)

    switch ( m_scale )
    {
    case SC_DAY:
        format = "%d/%m";
        break;
    case SC_WEEK:
        format = "%V/%y";
        break;
    case SC_MONTH:
        format = "%b/%y";
        break;
    case SC_QUARTER:
        format = "Q%q/%y";
        break;
    default:
        kdWarning() << "Invalid scale in KTJTaskReport::getColumnLabels" << endl;
        break;
    }

    time_t delta = intervalForCol( 0 ).getDuration();
    QicsDataModelRow result;
    time_t tmp = m_start.toTime_t();
    time_t tmpEnd = m_end.toTime_t();

    //kdDebug() << "generateHeader: m_scale: " << m_scale << " , delta: " << delta <<
    //" , start: " << tmp << ", end: " << tmpEnd << endl;

    while ( tmp <= tmpEnd )
    {
        result.append( formatDate( tmp, format ) );
        tmp += delta;
    }

    m_model->addRows( 1 );
    m_model->setRowItems( 0, result );

    return result.count();
}

void KTJTaskReport::generateRow( const Task * task )
{
    // generate the sum row

    // generate the resource subrows
    for ( ResourceListIterator tli( task->getBookedResourcesIterator( 0 ) ); *tli != 0; ++tli )
    {
        generateRow( task, ( *tli ) );
    }
}

void KTJTaskReport::generateRow( const Task * task, const Resource * res )
{

}
