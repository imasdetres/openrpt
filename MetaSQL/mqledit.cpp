/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2008 by OpenMFG, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */

#include "mqledit.h"

#include <QSqlDatabase>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QSqlRecord>
#include <QSqlDriver>
#include <QSqlError>
#include <QTextStream>

#include <parameter.h>
#include <xsqlquery.h>
#include <login.h>

#include "data.h"
#include "metasql.h"
#include "parameteredit.h"
#include "logoutput.h"
#include "resultsoutput.h"


MQLEdit::MQLEdit(QWidget* parent, Qt::WindowFlags fl)
    : QMainWindow(parent, fl)
{
    setupUi(this);

    (void)statusBar();

    // signals and slots connections
    connect(fileNewAction, SIGNAL(activated()), this, SLOT(fileNew()));
    connect(fileOpenAction, SIGNAL(activated()), this, SLOT(fileOpen()));
    connect(fileSaveAction, SIGNAL(activated()), this, SLOT(fileSave()));
    connect(fileSaveAsAction, SIGNAL(activated()), this, SLOT(fileSaveAs()));
    connect(filePrintAction, SIGNAL(activated()), this, SLOT(filePrint()));
    connect(fileExitAction, SIGNAL(activated()), this, SLOT(fileExit()));
    connect(editFindAction, SIGNAL(activated()), this, SLOT(editFind()));
    connect(helpIndexAction, SIGNAL(activated()), this, SLOT(helpIndex()));
    connect(helpContentsAction, SIGNAL(activated()), this, SLOT(helpContents()));
    connect(helpAboutAction, SIGNAL(activated()), this, SLOT(helpAbout()));
    connect(fileDatabaseConnectAction, SIGNAL(activated()), this, SLOT(fileDatabaseConnect()));
    connect(fileDatabaseDisconnectAction, SIGNAL(activated()), this, SLOT(fileDatabaseDisconnect()));
    connect(viewParameter_ListAction, SIGNAL(activated()), this, SLOT(showParamList()));
    connect(toolsParse_QueryAction, SIGNAL(activated()), this, SLOT(parseQuery()));
    connect(toolsExecute_QueryAction, SIGNAL(activated()), this, SLOT(execQuery()));
    connect(viewLog_OutputAction, SIGNAL(activated()), this, SLOT(showLog()));
    connect(viewResultsAction, SIGNAL(activated()), this, SLOT(showResults()));
    connect(viewExecuted_SQLAction, SIGNAL(activated()), this, SLOT(showExecutedSQL()));

    QSqlDatabase db = QSqlDatabase();
    if(_loggedIn && db.isValid() && db.isOpen()) {
	_loggedIn = true;
    } else {
	_loggedIn = false;
    }
    
    fileDatabaseConnectAction->setEnabled(!_loggedIn);
    fileDatabaseDisconnectAction->setEnabled(_loggedIn);
    
    _pEdit = new ParameterEdit(this);
    _log = new LogOutput(this);
    _sql = new LogOutput(this);
    _results = new ResultsOutput(this);
}

MQLEdit::~MQLEdit()
{
    // no need to delete child widgets, Qt does it all for us
}

void MQLEdit::languageChange()
{
    retranslateUi(this);
}

void MQLEdit::fileNew()
{
    if(askSaveIfModified()) {
	// now that we have that out of the way we are free to go ahead and do our stuff
	_text->clear();
	_text->document()->setModified(false);
	_fileName = QString::null;
    }
}


void MQLEdit::fileOpen()
{
    if(askSaveIfModified()) {
	QString fileName = QFileDialog::getOpenFileName(this);
	if(!fileName.isEmpty()) {
	    QFile file(fileName);
	    if(file.open(QIODevice::ReadOnly)) {
		QTextStream stream(&file);
		_text->setText(stream.readAll());
		_text->document()->setModified(false);
		_fileName = fileName;
	    }
	}
    }
}


void MQLEdit::fileSave()
{
    if(_fileName.isEmpty()) {
	saveAs();
    } else {
	save();
    }
}


void MQLEdit::fileSaveAs()
{
    saveAs();
}


void MQLEdit::filePrint()
{
    QMessageBox::information(this, tr("Not Yet Implemented"),
               tr("This function has not been implemented."));
}


void MQLEdit::fileExit()
{
    if(askSaveIfModified()) {
	QApplication::exit();
    }
}


void MQLEdit::editFind()
{
    QMessageBox::information(this, tr("Not Yet Implemented"),
               tr("This function has not been implemented."));
}


void MQLEdit::helpIndex()
{
    QMessageBox::information(this, tr("Not Yet Implemented"),
               tr("This function has not been implemented."));
}


void MQLEdit::helpContents()
{
    QMessageBox::information(this, tr("Not Yet Implemented"),
               tr("This function has not been implemented."));
}


void MQLEdit::helpAbout()
{
    QMessageBox::about(this, tr("About MetaSQL Editor"),
               tr("About MetaSQL Editor version 3.1.0"));
}

//
// Checks to see if the document has been modified and asks the
// user if they would like to save the document before they continue
// with the operation they are trying to perform.
//
// Returns TRUE if the operation can continue otherwise returns FALSE
// and the calling process should not perform any actions.
//
bool MQLEdit::askSaveIfModified()
{
    if(_text->document()->isModified()) {
	int ret = QMessageBox::question(this, tr("Document Modified!"),
					tr("Would you like to save your changes before continuing?"),
			      QMessageBox::Yes | QMessageBox::Default,
			      QMessageBox::No,
			      QMessageBox::Cancel | QMessageBox::Escape);
	switch(ret) {
	case QMessageBox::Yes:
	    if(saveAs() == false) return false; // the save was canceled so take no action
	    break;
	case QMessageBox::No: break;
	case QMessageBox::Cancel: return false;
	default:
	    QMessageBox::warning(this, tr("Warning"),
                 tr("Encountered an unknown response. No action will be taken."));
	    return false;
	
	};
    }
    return true;
}

//
// This functions saves the document returning TRUE if the save was completed successfully
// or FALSE is the save was canceled or encountered some other kind of error.
//
bool MQLEdit::save()
{
   if(_fileName.isEmpty()) {
	QMessageBox::warning(this, tr("No file Specified"),
                   tr("No file was specified to save to."));
	return false;
    }
    
    QFile file(_fileName);
    if (file.open(QIODevice::WriteOnly)) {
	QTextStream stream(&file);
	stream << _text->toPlainText();
	_text->document()->setModified(false);
    } else {
	QMessageBox::warning(this, tr("Error Saving file"),
          tr("There was an error while trying to save the file."));
	return false;
    }
    return true;
}

//
// This function asks the user what name they would like to save the document as then saves
// the document to the disk with the given name. This function will return TRUE if the save
// was successfull otherwise it will return FALSE if the user canceled the save or some other
// error was encountered.
//
bool MQLEdit::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, QString::null, _fileName);
    if(fileName.isEmpty()) {
	return false;
    }
    
    _fileName = fileName;
    return save();
}


void MQLEdit::fileDatabaseConnect()
{
  if (!_loggedIn) {
    ParameterList params;
    params.append("name", MQLEdit::name());
    params.append("copyright", _copyright);
    params.append("version", _version);

    if(!_databaseURL.isEmpty())
      params.append("databaseURL", _databaseURL);

    login newdlg(0, "", TRUE);
    newdlg.set(params, 0);

    if (newdlg.exec() != QDialog::Rejected)
    {
      _databaseURL = newdlg._databaseURL;
      _loggedIn = true;
    }
  }
  fileDatabaseConnectAction->setEnabled(!_loggedIn);
  fileDatabaseDisconnectAction->setEnabled(_loggedIn);
}


void MQLEdit::fileDatabaseDisconnect()
{
    QSqlDatabase db = QSqlDatabase::database();
    if(db.isValid()) {
	db.close();
    }
    _loggedIn = false;
    fileDatabaseConnectAction->setEnabled(!_loggedIn);
    fileDatabaseDisconnectAction->setEnabled(_loggedIn);
}


void MQLEdit::showParamList()
{
    _pEdit->show();
}


void MQLEdit::parseQuery()
{
    _sql->_log->clear();
    _log->_log->clear();
    _log->_log->append(tr("---- Parsing Query ----\n"));
    MetaSQLQuery mql(_text->toPlainText());
    _log->_log->append(mql.parseLog());
    if(mql.isValid()) {
        _log->_log->append(tr("Query parsed."));
    } else {
        _log->_log->append(tr("ERROR: Invalid query!"));
    }
    showLog();
}


void MQLEdit::execQuery()
{
    if(!_loggedIn) {
	QMessageBox::warning(this, tr("Not Connected"),
          tr("You must be connected to a database in order to execute a query."));
	return;
    }

    _results->_table->setRowCount(0);
    _results->_table->setColumnCount(0);
    
    _sql->_log->clear();
    _log->_log->clear();
    _log->_log->append(tr("---- Parsing Query ----\n"));
    MetaSQLQuery mql(_text->toPlainText());
    _log->_log->append(mql.parseLog());
    if(mql.isValid()) {
	_log->_log->append(tr("Query parsed."));
	_log->_log->append(tr("---- Executing Query ----"));
	ParameterList plist = _pEdit->getParameterList();
	XSqlQuery qry = mql.toQuery(plist);
        _sql->_log->append(qry.executedQuery());
	if(qry.isActive()) {
            QSqlRecord rec = qry.record();
	    int ncols = rec.count();
            _results->_table->setColumnCount(ncols);
            int c;
            for(c = 0; c < ncols; c++) {
                _results->_table->setHorizontalHeaderItem(c, new QTableWidgetItem(rec.fieldName(c)));
            }
            int nrows = 0;
            while(qry.next()) {
                _results->_table->setRowCount(nrows + 1);
                for(c = 0; c < ncols; c++) {
                    _results->_table->setItem(nrows, c, new QTableWidgetItem(qry.value(c).toString()));
                }
                nrows++;
            }
            showResults();
	} else {
	    _log->_log->append(tr("Failed to execute query."));
	    QSqlError err = qry.lastError();
	    _log->_log->append(err.text());
	}
    } else {
	_log->_log->append(tr("ERROR: Invalid query!"));
	showLog();
    }   
}


void MQLEdit::showLog()
{
    _log->show();
}


void MQLEdit::showResults()
{
    _results->show();
}


void MQLEdit::showExecutedSQL()
{
    _sql->show();
}

QString MQLEdit::name()
{
#if defined Q_WS_WIN
  return QObject::tr("MQLEdit for Windows");
#elif defined Q_WS_X11
  return QObject::tr("MQLEdit for Linux");
#elif defined Q_WS_MAC
  return QObject::tr("MQLEdit for OS X");
#else
  return QObject::tr("MQLEdit");
#endif
}

void MQLEdit::sTick()
{
  if(_loggedIn)
  {
    XSqlQuery tickle;
    // TODO make this work on other databases.
    tickle.exec("SELECT CURRENT_DATE AS dbdate;");
    // TODO: check for errors. Do we even care?
  }
  _tick.singleShot(60000, this, SLOT(sTick()));
}
