/*
** -- Qt Includes --
*/

#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QXmlStreamReader>

/*
** -- Local Includes --
*/

#include "biblioteq.h"
#include "biblioteq_generic_thread.h"

extern biblioteq *qmain;

/*
** -- biblioteq_generic_thread() --
*/

biblioteq_generic_thread::biblioteq_generic_thread(QObject *parent):
  QThread(parent)
{
  m_type = -1;
  m_eType = "";
  m_errorStr = "";
  m_z3950Name = "";
  setTerminationEnabled(true);
}

/*
** -- ~biblioteq_generic_thread() --
*/

biblioteq_generic_thread::~biblioteq_generic_thread()
{
  m_list.clear();
  m_z3950Results.clear();
  m_outputListBool.clear();
}

/*
** -- run() --
*/

void biblioteq_generic_thread::run(void)
{
  /*
  ** Some of this could have been implemented with Qt signals.
  */

  switch(m_type)
    {
    case READ_GLOBAL_CONFIG_FILE:
      {
	QFile qf;
	QTextStream qts;
	biblioteq_myqstring str = "";

	qf.setFileName(m_filename);

	if(!qf.open(QIODevice::ReadOnly))
	  {
	    m_errorStr = tr("Unable to read ");
	    m_errorStr.append(m_filename);
	    m_errorStr.append(tr(". This file is required by BiblioteQ."));
	    return;
	  }

	qts.setDevice(&qf);

	while(!qts.atEnd())
	  {
	    str = str.prepConfigString(qts.readLine().trimmed(), true);
	    m_list.append(str);
	  }

	qf.close();
	break;
      }
    case Z3950_QUERY:
      {
	QHash<QString, QString> hash
	  (qmain->getZ3950Maps().value(m_z3950Name));
	QString recordSyntax(hash.value("RecordSyntax").trimmed());
	ZOOM_connection zoomConnection = 0;
	ZOOM_options options = ZOOM_options_create();
	ZOOM_resultset zoomResultSet = 0;
	const char *rec = 0;
	size_t i = 0;

	ZOOM_options_set
	  (options,
	   "databaseName",
	   hash.value("Database").toLatin1().constData());

	if(recordSyntax.isEmpty())
	  ZOOM_options_set(options, "preferredRecordSyntax", "MARC21");
	else
	  ZOOM_options_set(options, "preferredRecordSyntax", recordSyntax.
			   toLatin1().constData());

	if(!hash.value("proxy_host").isEmpty() &&
	   !hash.value("proxy_port").isEmpty())
	  {
	    QString value(QString("%1:%2").
			  arg(hash.value("proxy_host")).
			  arg(hash.value("proxy_port")));

	    ZOOM_options_set(options, "proxy", value.toLatin1().constData());
	  }

	if(!hash.value("Userid").isEmpty())
	  ZOOM_options_set
	    (options,
	     "user",
	     hash.value("Userid").toLatin1().constData());

	if(!hash.value("Password").isEmpty())
	  ZOOM_options_set
	    (options,
	     "password",
	     hash.value("Password").toLatin1().constData());

	zoomConnection = ZOOM_connection_create(options);
	ZOOM_connection_connect
	  (zoomConnection, (hash.value("Address") + ":" +
			    hash.value("Port")).toLatin1().constData(), 0);
 	zoomResultSet = ZOOM_connection_search_pqf
	  (zoomConnection,
	   m_z3950SearchStr.toLatin1().constData());

	QString format = hash.value("Format").trimmed().toLower();

	if(format.isEmpty())
	  format = "render";
	else
	  format.prepend("render; charset=");

	while((rec = ZOOM_record_get(ZOOM_resultset_record(zoomResultSet, i),
				     format.toLatin1().constData(), 0)) != 0)
	  {
	    i += 1;
	    m_z3950Results.append(QString::fromUtf8(rec));
	  }

	ZOOM_resultset_destroy(zoomResultSet);

	if(i == 0)
	  {
	    const char *addinfo;
	    const char *errmsg;

	    if(ZOOM_connection_error(zoomConnection, &errmsg, &addinfo) != 0)
	      {
		m_eType = errmsg;
		m_errorStr = addinfo;

		if(m_errorStr.isEmpty())
		  m_errorStr = m_eType;
	      }
	    else if(m_z3950Results.isEmpty())
	      {
		m_eType = tr("Z39.50 Empty Results Set");
		m_errorStr = tr("Z39.50 Empty Results Set");
	      }
	  }

	ZOOM_options_destroy(options);
	ZOOM_connection_destroy(zoomConnection);
	break;
      }
    default:
      break;
    }
}

/*
** -- getList() --
*/

QStringList biblioteq_generic_thread::getList(void) const
{
  return m_list;
}

/*
** -- getErrorStr() --
*/

QString biblioteq_generic_thread::getErrorStr(void) const
{
  return m_errorStr;
}

/*
** -- setType() --
*/

void biblioteq_generic_thread::setType(const int type)
{
  m_type = type;
}

/*
** -- setOutput() --
*/

void biblioteq_generic_thread::setOutputList(const QList<bool> &list)
{
  int i = 0;

  for(i = 0; i < list.size(); i++)
    m_outputListBool.append(list.at(i));
}

/*
** -- setFilename() --
*/

void biblioteq_generic_thread::setFilename(const QString &filename)
{
  m_filename = filename;
}

/*
** -- setZ3950SearchString() --
*/

void biblioteq_generic_thread::setZ3950SearchString(const QString &z3950SearchStr)
{
  m_z3950SearchStr = z3950SearchStr;
}

/*
** -- getZ3950Results() --
*/

QStringList biblioteq_generic_thread::getZ3950Results(void) const
{
  return m_z3950Results;
}

/*
** -- getEType() --
*/

QString biblioteq_generic_thread::getEType(void) const
{
  return m_eType;
}

/*
** -- msleep() --
*/

void biblioteq_generic_thread::msleep(const int msecs)
{
  QThread::msleep(msecs);
}

/*
** -- setZ3950Name() --
*/

void biblioteq_generic_thread::setZ3950Name(const QString &name)
{
  m_z3950Name = name;
}
