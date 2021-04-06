/*
** 2017 June 08
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains C code routines that are called by the SQLite parser
** when syntax rules are reduced.  The routines in this file handle the
** following kinds of SQL syntax:
*/

#include "sqlite3.h"
#include "sqliteInt.h"

/*
while (1) {
	for (i = OMIT_TEMPDB; i<db->nDb; i++) {
		int j = (i<2) ? i ^ 1 : i;   // Search TEMP before MAIN 
		if (zDatabase == 0 || sqlite3StrICmp(zDatabase, db->aDb[j].zDbSName) == 0) {
			assert(sqlite3SchemaMutexHeld(db, j, 0));
			p = sqlite3HashFind(&db->aDb[j].pSchema->tblHash, zName);
			if (p) return p;
		}
	}
	// Not found.  If the name we were looking for was temp.sqlite_master	** then change the name to sqlite_temp_master and try again. 
	if (sqlite3StrICmp(zName, MASTER_NAME) != 0) break;
	if (sqlite3_stricmp(zDatabase, db->aDb[1].zDbSName) != 0) break;
	zName = TEMP_MASTER_NAME;
}*/

