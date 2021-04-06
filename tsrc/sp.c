/*
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains the implementation for SPs
*/
#include "sqliteInt.h"
#include "vdbeInt.h"

#ifndef SQLITE_OMIT_SP
/*
** Delete a linked list of SPStep structures.
*/
void sqlite3DeleteSPStep(sqlite3 *db, SPStep *pSPStep) {
	while (pSPStep) {
		SPStep * pTmp = pSPStep;
		pSPStep = pSPStep->pNext;

		sqlite3ExprDelete(db, pTmp->pWhere);
		sqlite3ExprListDelete(db, pTmp->pExprList);
		sqlite3SelectDelete(db, pTmp->pSelect);
		sqlite3IdListDelete(db, pTmp->pIdList);

		sqlite3DbFree(db, pTmp);
	}
}

/*
** Given table pTab, return a list of all the SPs attached to
** the table. The list is connected by SP.pNext pointers.
**
** All of the SPs on pTab that are in the same database as pTab
** are already attached to pTab->pSP.  But there might be additional
** SPs on pTab in the TEMP schema.  This routine prepends all
** TEMP SPs on pTab to the beginning of the pTab->pSP list
** and returns the combined list.
**
** To state it another way:  This routine returns a list of all SPs
** that fire off of pTab.  The list will include any TEMP SPs on
** pTab as well as the SPs lised in pTab->pSP. 

SP *sqlite3SPList(Parse *pParse, Table *pTab) {
	Schema * const pTmpSchema = pParse->db->aDb[1].pSchema;
	SP *pList = 0;                 // List of SPs to return 

	if (pParse->disableTriggers) {
		return 0;
	}
	
	if (pTmpSchema != pTab->pSchema) {
		HashElem *p;
		assert(sqlite3SchemaMutexHeld(pParse->db, 0, pTmpSchema));
		for (p = sqliteHashFirst(&pTmpSchema->trigHash); p; p = sqliteHashNext(p)) {
			SP *pSP = (SP *)sqliteHashData(p);
			if (pSP->pTabSchema == pTab->pSchema
				&& 0 == sqlite3StrICmp(pSP->table, pTab->zName)
				) {
				pSP->pNext = (pList ? pList : pTab->pSP);
				pList = pSP;
			}
		}
	}

	return (pList ? pList : pTab->pSP);
}*/

/*
** This is called by the parser when it sees a CREATE SP statement
** up to the point of the BEGIN before the SP actions.  A SP
** structure is generated based on the information available and stored
** in pParse->pNewSP.  After the SP actions have been parsed, the
** sqlite3FinishSP() function is called to complete the SP
** construction process.
*/
void sqlite3BeginSP(
	Parse *pParse,      /* The parse context of the CREATE SP statement */
	Token *pName1,      /* The name of the SP */
	Token *pName2,      /* The name of the SP */
	int isTemp,         /* True if the TEMPORARY keyword is present */
	int noErr           /* Suppress errors if the SP already exists */
) {
	SP *pSP = 0;  /* The new SP */
	char *zName = 0;        /* Name of the SP */
	sqlite3 *db = pParse->db;  /* The database connection */
	int iDb;                /* The database to store the SP in */
	Token *pName;           /* The unqualified db name */
	DbFixer sFix;           /* State vector for the DB fixer */

	assert(pName1 != 0);   /* pName1->z might be NULL, but not pName1 itself */
	assert(pName2 != 0);
	if (isTemp) {
		/* If TEMP was specified, then the SP name may not be qualified. */
		if (pName2->n>0) {
			sqlite3ErrorMsg(pParse, "temporary SP may not have qualified name");
			goto SP_cleanup;
		}
		iDb = 1;
		pName = pName1;
	}
	else {
		/* Figure out the db that the SP will be created in */
		iDb = sqlite3TwoPartName(pParse, pName1, pName2, &pName);
		if (iDb<0) {
			goto SP_cleanup;
		}
	}
	
	/* Ensure the table name matches database name and that the table exists */
	if (db->mallocFailed) goto SP_cleanup;
	//assert(pTableName->nSrc == 1);
	sqlite3FixInit(&sFix, pParse, iDb, "SP", pName);
	
	/* Check that the SP name is not reserved and that no SP of the
	** specified name exists */
	zName = sqlite3NameFromToken(db, pName);
	if (!zName || SQLITE_OK != sqlite3CheckObjectName(pParse, zName)) {
		goto SP_cleanup;
	}
	assert(sqlite3SchemaMutexHeld(db, iDb, 0));
	if (sqlite3HashFind(&(db->aDb[iDb].pSchema->spHash), zName)) {
		if (!noErr) {
			sqlite3ErrorMsg(pParse, "SP %T already exists", pName);
		}
		else {
			assert(!db->init.busy);
			sqlite3CodeVerifySchema(pParse, iDb);
		}
		goto SP_cleanup;
	}

#ifndef SQLITE_OMIT_AUTHORIZATION
	{
		//int iTabDb = sqlite3SchemaToIndex(db, pSP->pSchema);
		int code = SQLITE_CREATE_SP;
		const char *zDb = db->aDb[iDb].zDbSName;
		const char *zDbSp = isTemp ? db->aDb[1].zDbSName : zDb;
		if (iDb == 1 || isTemp) code = SQLITE_CREATE_TEMP_SP;
		if (sqlite3AuthCheck(pParse, code, zName, 0, zDbSp)) {
			goto SP_cleanup;
		}
		if (sqlite3AuthCheck(pParse, SQLITE_INSERT, SCHEMA_TABLE(iDb), 0, zDb)) {
			goto SP_cleanup;
		}
	}
#endif

	

	/* Build the SP object */
	pSP = (SP*)sqlite3DbMallocZero(db, sizeof(SP));
	if (pSP == 0) goto SP_cleanup;
	pSP->zName = zName;
	zName = 0;
	//pSP->table = sqlite3DbStrDup(db, pTableName->a[0].zName);
	pSP->pSchema = db->aDb[iDb].pSchema;
	
	assert(pParse->pNewSP == 0);
	pParse->pNewSP = pSP;

SP_cleanup:
	sqlite3DbFree(db, zName);
	if (!pParse->pNewSP) {
		sqlite3DeleteSP(db, pSP);
	}
	else {
		assert(pParse->pNewSP == pSP);
	}
}

/*
** This routine is called after all of the SP actions have been parsed
** in order to complete the process of building the SP.
*/
void sqlite3FinishSP(
	Parse *pParse,          /* Parser context */
	SPStep *pStepList, /* The SPed program */
	Token *pAll             /* Token that describes the complete CREATE SP */
) {
	SP *pSP = pParse->pNewSP;   /* SP being finished */
	char *zName;                            /* Name of SP */
	sqlite3 *db = pParse->db;               /* The database */
	DbFixer sFix;                           /* Fixer object */
	int iDb;                                /* Database containing the SP */
	Token nameToken;                        /* SP name for error reporting */

	pParse->pNewSP = 0;
	if (NEVER(pParse->nErr) || !pSP) goto SPfinish_cleanup;
	zName = pSP->zName;
	iDb = sqlite3SchemaToIndex(pParse->db, pSP->pSchema);
	pSP->step_list = pStepList;
	while (pStepList) {
		pStepList->pSP = pSP;
		pStepList = pStepList->pNext;
	}
	sqlite3TokenInit(&nameToken, pSP->zName);
	sqlite3FixInit(&sFix, pParse, iDb, "SP", &nameToken);
	/*if (sqlite3FixSPStep(&sFix, pSP->step_list) || sqlite3FixExpr(&sFix, pTrig->pWhen)) {
		goto SPfinish_cleanup;
	}*/

	/* if we are not initializing,
	** build the sqlite_master entry
	*/
	if (!db->init.busy) {
		Vdbe *v;
		char *z;

		/* Make an entry in the sqlite_master table */
		v = sqlite3GetVdbe(pParse);
		if (v == 0) goto SPfinish_cleanup;
		sqlite3BeginWriteOperation(pParse, 0, iDb);
		z = sqlite3DbStrNDup(db, (char*)pAll->z, pAll->n);
		sqlite3NestedParse(pParse,
			"INSERT INTO %Q.%s VALUES('SP',%Q,%Q,0,'CREATE WEBPROC %q')",
			db->aDb[iDb].zDbSName, MASTER_NAME, zName,	zName, z);
		sqlite3DbFree(db, z);
		sqlite3ChangeCookie(pParse, iDb);
		sqlite3VdbeAddParseSchemaOp(v, iDb,	sqlite3MPrintf(db, "type='SP' AND name='%q'", zName));
	}

	if (db->init.busy) {
		SP *pLink = pSP;
		Hash *pHash = &db->aDb[iDb].pSchema->spHash;
		assert(sqlite3SchemaMutexHeld(db, iDb, 0));
		pSP = sqlite3HashInsert(pHash, zName, pSP);
		if (pSP) {
			sqlite3OomFault(db);
		}
		/*else if (pLink->pSchema == pLink->pTabSchema) {
			Table *pTab;
			pTab = sqlite3HashFind(&pLink->pTabSchema->tblHash, pLink->table);
			assert(pTab != 0);
			pLink->pNext = pTab->pSP;
			pTab->pSP = pLink;
		}*/
	}

SPfinish_cleanup:
	sqlite3DeleteSP(db, pSP);
	assert(!pParse->pNewSP);
	sqlite3DeleteSPStep(db, pStepList);
}

/*
** Turn a SELECT statement (that the pSelect parameter points to) into
** a SP step.  Return a pointer to a SPStep structure.
**
** The parser calls this routine when it finds a SELECT statement in
** body of a SP.
*/
SPStep *sqlite3SPSelectStep(sqlite3 *db, Select *pSelect) {
	SPStep *pSPStep = sqlite3DbMallocZero(db, sizeof(SPStep));
	if (pSPStep == 0) {
		sqlite3SelectDelete(db, pSelect);
		return 0;
	}
	pSPStep->op = TK_SELECT;
	pSPStep->pSelect = pSelect;
	pSPStep->orconf = OE_Default;
	return pSPStep;
}

SPStep *sqlite3SPForEachStep(sqlite3 *db, Select *pSelect) {
	SPStep *pSPStep = sqlite3DbMallocZero(db, sizeof(SPStep));
	if (pSPStep == 0) {
		sqlite3SelectDelete(db, pSelect);
		return 0;
	}
	pSPStep->op = TK_FOREACH;
	pSPStep->pSelect = pSelect;
	pSPStep->orconf = OE_Default;
	return pSPStep;
}

SPStep *sqlite3SPEndForEachStep(sqlite3 *db) {
	SPStep *pSPStep = sqlite3DbMallocZero(db, sizeof(SPStep));
	pSPStep->op = TK_END;
	return pSPStep;
}

SPStep *sqlite3SPDefineStep(sqlite3 *db, Expr* pLeft,i16 nLeft,Expr* pValue) {
	SPStep *pSPStep = sqlite3DbMallocZero(db, sizeof(SPStep));
	if (pSPStep == 0) {
		sqlite3ExprDelete(db, pValue);
		return 0;
	}
	pSPStep->op = TK_DEFINE;
	pSPStep->zTarget = pLeft->u.zToken;
	pSPStep->nzTarget = nLeft;
	pSPStep->pWhere= pValue;
	pSPStep->orconf = OE_Default;
	return pSPStep;
}

SPStep *sqlite3SPIfStep(sqlite3 *db, Expr *pIf,u8 op, u8 ifIndex) {
	SPStep *pSPStep = sqlite3DbMallocZero(db, sizeof(SPStep));
	if (pSPStep == 0) {
		sqlite3ExprListDelete(db, pIf);
		return 0;
	}
	pSPStep->op = op;
	pSPStep->pIf = pIf;
	pSPStep->iIf = ifIndex;
	pSPStep->orconf = OE_Default;
}


/*
** Allocate space to hold a new SP step.  The allocated space
** holds both the SPStep object and the SPStep.target.z string.
**
** If an OOM error occurs, NULL is returned and db->mallocFailed is set.
*/
static SPStep *SPStepAllocate(
	sqlite3 *db,                /* Database connection */
	u8 op,                      /* SP opcode */
	Token *pName                /* The target name */
) {
	SPStep *pSPStep;

	pSPStep = sqlite3DbMallocZero(db, sizeof(SPStep) + pName->n + 1);
	if (pSPStep) {
		char *z = (char*)&pSPStep[1];
		memcpy(z, pName->z, pName->n);
		sqlite3Dequote(z);
		pSPStep->zTarget = z;
		pSPStep->op = op;
	}
	return pSPStep;
}

/*
** Build a SP step out of an INSERT statement.  Return a pointer
** to the new SP step.
**
** The parser calls this routine when it sees an INSERT inside the
** body of a SP.
*/
SPStep *sqlite3SPInsertStep(
	sqlite3 *db,        /* The database connection */
	Token *pTableName,  /* Name of the table into which we insert */
	IdList *pColumn,    /* List of columns in pTableName to insert into */
	Select *pSelect,    /* A SELECT statement that supplies values */
	u8 orconf           /* The conflict algorithm (OE_Abort, OE_Replace, etc.) */
) {
	SPStep *pSPStep;

	assert(pSelect != 0 || db->mallocFailed);

	pSPStep = SPStepAllocate(db, TK_INSERT, pTableName);
	if (pSPStep) {
		
		pSPStep->pSelect = sqlite3SelectDup(db, pSelect, EXPRDUP_REDUCE);
		
		pSPStep->pIdList = pColumn;
		pSPStep->orconf = orconf;
	}
	else {
		sqlite3IdListDelete(db, pColumn);
	}
	sqlite3SelectDelete(db, pSelect);

	return pSPStep;
}

/*
** Construct a SP step that implements an UPDATE statement and return
** a pointer to that SP step.  The parser calls this routine when itpSPStep->pSP
** sees an UPDATE statement inside the body of a CREATE SP.
*/
SPStep *sqlite3SPUpdateStep(
	sqlite3 *db,         /* The database connection */
	Token *pTableName,   /* Name of the table to be updated */
	ExprList *pEList,    /* The SET clause: list of column and new values */
	Expr *pWhere,        /* The WHERE clause */
	u8 orconf            /* The conflict algorithm. (OE_Abort, OE_Ignore, etc) */
) {
	SPStep *pSPStep;

	pSPStep = SPStepAllocate(db, TK_UPDATE, pTableName);
	if (pSPStep) {
		pSPStep->pExprList = sqlite3ExprListDup(db, pEList, EXPRDUP_REDUCE);
		pSPStep->pWhere = sqlite3ExprDup(db, pWhere, EXPRDUP_REDUCE);
		pSPStep->orconf = orconf;
	}
	sqlite3ExprListDelete(db, pEList);
	sqlite3ExprDelete(db, pWhere);
	return pSPStep;
}

/*
** Construct a SP step that implements a DELETE statement and return
** a pointer to that SP step.  The parser calls this routine when it
** sees a DELETE statement inside the body of a CREATE SP.
*/
SPStep *sqlite3SPDeleteStep(
	sqlite3 *db,            /* Database connection */
	Token *pTableName,      /* The table from which rows are deleted */
	Expr *pWhere            /* The WHERE clause */
) {
	SPStep *pSPStep;

	pSPStep = SPStepAllocate(db, TK_DELETE, pTableName);
	if (pSPStep) {
		pSPStep->pWhere = sqlite3ExprDup(db, pWhere, EXPRDUP_REDUCE);
		pSPStep->orconf = OE_Default;
	}
	sqlite3ExprDelete(db, pWhere);
	return pSPStep;
}

/*
** Recursively delete a SP structure
*/
void sqlite3DeleteSP(sqlite3 *db, SP *pSP) {
	if (pSP == 0) return;
	sqlite3DeleteSPStep(db, pSP->step_list);
	sqlite3DbFree(db, pSP->zName);
	//sqlite3DbFree(db, pSP->table);
	//sqlite3ExprDelete(db, pSP->pWhen);
	//sqlite3IdListDelete(db, pSP->pColumns);
	sqlite3DbFree(db, pSP);
}

/*
** This function is called to drop a SP from the database schema.
**
** This may be called directly from the parser and therefore identifies
** the SP by name.  The sqlite3DropSPPtr() routine does the
** same job as this routine except it takes a pointer to the SP
** instead of the SP name.
**/
void sqlite3DropSP(Parse *pParse, SrcList *pName, int noErr) {
	SP *pSP = 0;
	int i;
	const char *zDb;
	const char *zName;
	sqlite3 *db = pParse->db;

	if (db->mallocFailed) goto drop_SP_cleanup;
	/*if (SQLITE_OK != sqlite3ReadSchema(pParse)) {
		goto drop_SP_cleanup;
	}*/

	assert(pName->nSrc == 1);
	zDb = pName->a[0].zDatabase;
	zName = pName->a[0].zName;
	assert(zDb != 0 || sqlite3BtreeHoldsAllMutexes(db));
	for (i = OMIT_TEMPDB; i<db->nDb; i++) {
		int j = (i<2) ? i ^ 1 : i;  /* Search TEMP before MAIN */
		if (zDb && sqlite3StrICmp(db->aDb[j].zDbSName, zDb)) continue;
		assert(sqlite3SchemaMutexHeld(db, j, 0));
		pSP = sqlite3HashFind(&(db->aDb[j].pSchema->spHash), zName);
		if (pSP) break;
	}
	if (!pSP) {
		if (!noErr) {
			sqlite3ErrorMsg(pParse, "no such SP: %S", pName, 0);
		}
		else {
			sqlite3CodeVerifyNamedSchema(pParse, zDb);
		}
		pParse->checkSchema = 1;
		goto drop_SP_cleanup;
	}
	sqlite3DropSPPtr(pParse, pSP);

drop_SP_cleanup:
	sqlite3SrcListDelete(db, pName);
}

void sqlite3ExecSP(Parse *pParse, SrcList *pName, ExprList *pParam,int onconf) {
	SP *pSP = 0;
	int i;
	const char *zDb;
	const char *zName;
	sqlite3 *db = pParse->db;

	if (db->mallocFailed) goto drop_SP_cleanup;
	if (SQLITE_OK != sqlite3ReadSchema(pParse)) {
		goto drop_SP_cleanup;
	}

	assert(pName->nSrc == 1);
	zDb = pName->a[0].zDatabase;
	zName = pName->a[0].zName;
	assert(zDb != 0 || sqlite3BtreeHoldsAllMutexes(db));
	for (i = OMIT_TEMPDB; i<db->nDb; i++) {
		int j = (i<2) ? i ^ 1 : i;  /* Search TEMP before MAIN */
		if (zDb && sqlite3StrICmp(db->aDb[j].zDbSName, zDb)) continue;
		assert(sqlite3SchemaMutexHeld(db, j, 0));
		pSP = sqlite3HashFind(&(db->aDb[j].pSchema->spHash), zName);
		if (pSP) break;
	}
	if (!pSP) {
		sqlite3ErrorMsg(pParse, "no such SP: %S", pName, 0);
		//if (!noErr) {}else {sqlite3CodeVerifyNamedSchema(pParse, zDb);}
		pParse->checkSchema = 1;
		goto drop_SP_cleanup;
	}



	sqlite3CodeRowSP(pParse, pSP,pParam, onconf ,0);

drop_SP_cleanup:
	sqlite3SrcListDelete(db, pName);
}


/*
** Return a pointer to the Table structure for the table that a SP
** is set on.
*/
/*
static Table *tableOfSP(SP *pSP) {
	return sqlite3HashFind(&pSP->pTabSchema->tblHash, pSP->table);
}*/


/*
** Drop a SP given a pointer to that SP.
*/
void sqlite3DropSPPtr(Parse *pParse, SP *pSP) {
	Vdbe *v;
	sqlite3 *db = pParse->db;
	int iDb;

	iDb = sqlite3SchemaToIndex(pParse->db, pSP->pSchema);
	assert(iDb >= 0 && iDb<db->nDb);
	//assert( iDb == 1); //pTable->pSchema == pSP->pSchema ||
#ifndef SQLITE_OMIT_AUTHORIZATION
	{
		int code = SQLITE_DROP_SP;
		const char *zDb = db->aDb[iDb].zDbSName;
		const char *zTab = SCHEMA_TABLE(iDb);
		if (iDb == 1) code = SQLITE_DROP_TEMP_SP;
		if (sqlite3AuthCheck(pParse, code, pSP->zName, 0, zDb) ||sqlite3AuthCheck(pParse, SQLITE_DELETE, zTab, 0, zDb)) {
			return;
		}
	}
#endif
	/*
	Generate code to destroy the database record of the SP.
	*/
	
	if ((v = sqlite3GetVdbe(pParse)) != 0) {
		sqlite3NestedParse(pParse,
			"DELETE FROM %Q.%s WHERE name=%Q AND type='SP'",
			db->aDb[iDb].zDbSName, MASTER_NAME, pSP->zName
		);
		sqlite3ChangeCookie(pParse, iDb);
		sqlite3VdbeAddOp4(v, OP_DropSP, iDb, 0, 0, pSP->zName, 0);
	}
}

/*
** Remove a SP from the hash tables of the sqlite* pointer.
*/

void sqlite3UnlinkAndDeleteSP(sqlite3 *db, int iDb, const char *zName) {
	SP *pSP;
	Hash *pHash;

	assert(sqlite3SchemaMutexHeld(db, iDb, 0));
	pHash = &(db->aDb[iDb].pSchema->spHash);
	pSP = sqlite3HashInsert(pHash, zName, 0);
	if (ALWAYS(pSP)) {
		//if (pSP->pSchema == pSP->pTabSchema) {Table *pTab = tableOfSP(pSP);SP **pp;for (pp = &pTab->pSP; *pp != pSP; pp = &((*pp)->pNext));*pp = (*pp)->pNext;}
		sqlite3DeleteSP(db, pSP);
		db->flags |= SQLITE_InternChanges;
	}
}

/*
** pEList is the SET clause of an UPDATE statement.  Each entry
** in pEList is of the format <id>=<expr>.  If any of the entries
** in pEList have an <id> which matches an identifier in pIdList,
** then return TRUE.  If pIdList==NULL, then it is considered a
** wildcard that matches anything.  Likewise if pEList==NULL then
** it matches anything so always return true.  Return false only
** if there is no match.

static int checkColumnOverlap(IdList *pIdList, ExprList *pEList) {
	int e;
	if (pIdList == 0 || NEVER(pEList == 0)) return 1;
	for (e = 0; e<pEList->nExpr; e++) {
		if (sqlite3IdListIndex(pIdList, pEList->a[e].zName) >= 0) return 1;
	}
	return 0;
}
*/

/*
** Convert the pStep->zTarget string into a SrcList and return a pointer
** to that SrcList.
**
** This routine adds a specific database name, if needed, to the target when
** forming the SrcList.  This prevents a SP in one database from
** referring to a target in another database.  An exception is when the
** SP is in TEMP in which case it can refer to any other database it
** wants.
*/
static SrcList *targetSrcSPList(
	Parse *pParse,       /* The parsing context */
	SPStep *pStep   /* The SP containing the target token */
) {
	sqlite3 *db = pParse->db;
	int iDb;             /* Index of the database to use */
	SrcList *pSrc;       /* SrcList to be returned */

	pSrc = sqlite3SrcListAppend(db, 0, 0, 0);
	if (pSrc) {
		assert(pSrc->nSrc > 0);
		pSrc->a[pSrc->nSrc - 1].zName = sqlite3DbStrDup(db, pStep->zTarget);
		iDb = sqlite3SchemaToIndex(db, pStep->pSP->pSchema);
		if (iDb == 0 || iDb >= 2) {
			const char *zDb;
			assert(iDb < db->nDb);
			zDb = db->aDb[iDb].zDbSName;
			pSrc->a[pSrc->nSrc - 1].zDatabase = sqlite3DbStrDup(db, zDb);
		}
	}
	return pSrc;
}

/*
** Generate VDBE code for the statements inside the body of a single
** SP.
*/
static int codeSPProgram(
	Parse *pParse,            /* The parser context */
	SP *pSP,   /* List of statements inside the SP body */
	int orconf                /* Conflict algorithm. (OE_Abort, etc) */
) {
	SPStep *pStep;
	SPStep *pStepList = pSP->step_list;
	SelectDest sDest;
	Vdbe *v = pParse->pVdbe;
	sqlite3 *db = pParse->db;
	int iCurrLoop = -1,iCurrIf = -1;;
	i16 addrStart, addrGoto,*addrEndLoop = sqlite3Malloc(7 * sizeof(i16)),*addrEndIf = sqlite3Malloc(7 * sizeof(i16));
	int *regYieldLoop = sqlite3Malloc(7 * sizeof(int));
	assert(pParse->pToplevel); //pParse->pSPTab &&
	assert(pStepList);
	assert(v != 0);
	
	pSP->ppIf = sqlite3Malloc(7 * sizeof(SPStep*));
	pSP->ppLoop = sqlite3Malloc(7 * sizeof(SPStep*));
	for (pStep = pStepList; pStep; pStep = pStep->pNext) {
		/* Figure out the ON CONFLICT policy that will be used for this step
		** of the SP program. If the statement that caused this SP
		** to fire had an explicit ON CONFLICT, then use it. Otherwise, use
		** the ON CONFLICT policy that was specified as part of the SP
		** step statement. Example:
		**
		**   CREATE SP AFTER INSERT ON t1 BEGIN;
		**     INSERT OR REPLACE INTO t2 VALUES(new.a, new.b);
		**   END;
		**
		**   INSERT INTO t1 ... ;            -- insert into t2 uses REPLACE policy
		**   INSERT OR IGNORE INTO t1 ... ;  -- insert into t2 uses IGNORE policy
		*/
		pParse->eOrconf = (orconf == OE_Default) ? pStep->orconf : (u8)orconf;
		assert(pParse->okConstFactor == 0);

		switch (pStep->op) {
		case TK_UPDATE: {
			sqlite3Update(pParse,
				targetSrcSPList(pParse, pStep),
				sqlite3ExprListDup(db, pStep->pExprList, 0),
				sqlite3ExprDup(db, pStep->pWhere, 0),
				pParse->eOrconf
			);
			break;
		}
		case TK_INSERT: {
			sqlite3Insert(pParse,
				targetSrcSPList(pParse, pStep),
				sqlite3SelectDup(db, pStep->pSelect, 0),
				sqlite3IdListDup(db, pStep->pIdList),
				pParse->eOrconf
			);
			break;
		}
		case TK_DELETE: {
			sqlite3DeleteFrom(pParse,
				targetSrcSPList(pParse, pStep),
				sqlite3ExprDup(db, pStep->pWhere, 0)
			);
			break;
		}
		case TK_DEFINE: {
			sqlite3Define(pParse, pStep->zTarget,pStep->nzTarget, sqlite3ExprDup(db, pStep->pWhere, 0));
			break;
		}

		case TK_IF: {
			pSP->ppIf[++iCurrIf] = pStep;
			addrEndIf[iCurrIf] = sqlite3VdbeMakeLabel(v);
			pStep->addrNext = sqlite3VdbeMakeLabel(v);
			sqlite3ExprIfFalse(pParse, pStep->pIf, pStep->addrNext, SQLITE_JUMPIFNULL);
			break;
		}

		case TK_ELSE: {
			if (pStep->iIf == iCurrIf) {
				sqlite3VdbeResolveLabel(v, pSP->ppIf[pStep->iIf]->addrNext);
				sqlite3ExprIfTrue(pParse, pSP->ppIf[pStep->iIf]->pIf, addrEndIf[pStep->iIf], SQLITE_JUMPIFNULL);
				if (pStep->pIf != 0) {
					pStep->addrNext = sqlite3VdbeMakeLabel(v);
					pSP->ppIf[pStep->iIf] = pStep;
					sqlite3ExprIfFalse(pParse, pStep->pIf, pStep->addrNext, SQLITE_JUMPIFNULL);
				}
			}
			break;
		}
		case TK_END: {
			if (pStep->iIf == iCurrIf) {
				iCurrIf--;
				sqlite3VdbeResolveLabel(v, addrEndIf[pStep->iIf]);

			}
			if (pStep->iLoop == iCurrLoop) {
				sqlite3VdbeEndCoroutine(v, sDest.iSdst);
				sqlite3VdbeChangeP2(v, addrGoto, sqlite3VdbeCurrentAddr(v));
				/* Clear the temporary register cache, thereby ensuring that each
				** co-routine has its own independent set of registers, because co-routines
				** might expect their registers to be preserved across an OP_Yield, and
				** that could cause problems if two or more co-routines are using the same
				** temporary register.
				*/
				pParse->nTempReg = 0;
				pParse->nRangeReg = 0;
				
				iCurrLoop--;
			}
			break;
		}

		case TK_FOREACH: {
			pSP->ppLoop[++iCurrLoop] = pStep;
			
			//regYieldLoop[iCurrLoop] = ++pParse->nMem;
			

			Select *pSelect = sqlite3SelectDup(db, pStep->pSelect, 0);
			sqlite3SelectDestInit(&sDest, SRT_EachInto, 0);
			sqlite3Select(pParse, pSelect, &sDest);
			sqlite3SelectDelete(db, pSelect);

			addrGoto  = sqlite3VdbeAddOp0(v, OP_Goto);
			sqlite3VdbeChangeP3(v, sDest.iSDParm, sqlite3VdbeCurrentAddr(v));
		
			break;
		}
		
		
		default: assert(pStep->op == TK_SELECT); {
			SelectDest selDest;
			Select *pSelect = sqlite3SelectDup(db, pStep->pSelect, 0);
			if ((pStep->pSelect->selFlags & 0xf0000) != SF_Into) {
			  sqlite3SelectDestInit(&selDest, SRT_Discard, 0);
			}else{
			  sqlite3SelectDestInit(&selDest, SRT_Into, 0);
			}
			sqlite3Select(pParse, pSelect, &selDest);
			sqlite3SelectDelete(db, pSelect);
			break;
		}
		}
		if (pStep->op != TK_SELECT
			&& pStep->op != TK_DEFINE
			&& pStep->op != TK_IF
			&& pStep->op != TK_ELSE
			&& pStep->op != TK_END
			&& pStep->op != TK_FOREACH) {
			sqlite3VdbeAddOp0(v, OP_ResetCount);
		}
	}
	if (iCurrIf > -1) {
		sqlite3ErrorMsg(pParse, "Syntax error: IF clause not properly ended.");
	}
	else if (iCurrLoop > -1) {
		sqlite3ErrorMsg(pParse, "Syntax error: LOOP clause not properly ended.");
	}
	pSP->ppIf = 0;
	pSP->ppLoop = 0;
	return 0;
}

#ifdef SQLITE_ENABLE_EXPLAIN_COMMENTS
/*
** This function is used to add VdbeComment() annotations to a VDBE
** program. It is not used in production code, only for debugging.
*/

static const char *onSPErrorText(int onError) {
	switch (onError) {
	case OE_Abort:    return "abort";
	case OE_Rollback: return "rollback";
	case OE_Fail:     return "fail";
	case OE_Replace:  return "replace";
	case OE_Ignore:   return "ignore";
	case OE_Default:  return "default";
	}
	return "n/a";
}


#endif

/*
** Parse context structure pFrom has just been used to create a sub-vdbe
** (SP program). If an error has occurred, transfer error information
** from pFrom to pTo.
*/

static void transferSPParseError(Parse *pTo, Parse *pFrom) {
	assert(pFrom->zErrMsg == 0 || pFrom->nErr);
	assert(pTo->zErrMsg == 0 || pTo->nErr);
	if (pTo->nErr == 0) {
		pTo->zErrMsg = pFrom->zErrMsg;
		pTo->nErr = pFrom->nErr;
		pTo->rc = pFrom->rc;
	}
	else {
		sqlite3DbFree(pFrom->db, pFrom->zErrMsg);
	}
}


/*
** Create and populate a new SPPrg object with a sub-program
** implementing SP pSP with ON CONFLICT policy orconf.
*/
static SPPrg *codeRowSP(
	Parse *pParse,       /* Current parse context */
	SP *pSP,   /* SP to code */
	int onconf           /* ON CONFLICT policy to code SP program with */
) {
	Parse *pTop = sqlite3ParseToplevel(pParse);
	sqlite3 *db = pParse->db;   /* Database handle */
	SPPrg *pPrg;           /* Value to return */
	Expr *pWhen = 0;            /* Duplicate of SP WHEN expression */
	Vdbe *v;                    /* Temporary VM */
	NameContext sNC;            /* Name context for sub-vdbe */
	SubProgram *pProgram = 0;   /* Sub-vdbe for SP program */
	Parse *pSubParse;           /* Parse context for sub-vdbe */
	int iEndSP = 0;        /* Label to jump to if WHEN is false */

	//assert(pSP->zName == 0 ); //|| pTab == tableOfSP(pSP)
	assert(pTop->pVdbe);

	/* Allocate the SPPrg and SubProgram objects. To ensure that they
	** are freed if an error occurs, link them into the Parse.pSPPrg
	** list of the top-level Parse object sooner rather than later.  */
	pPrg = sqlite3DbMallocZero(db, sizeof(SPPrg));
	if (!pPrg) return 0;
	pPrg->pNext = pTop->pSPPrg;
	pTop->pSPPrg = pPrg;
	pPrg->pProgram = pProgram = sqlite3DbMallocZero(db, sizeof(SubProgram));
	if (!pProgram) return 0;
	sqlite3VdbeLinkSubProgram(pTop->pVdbe, pProgram);
	pPrg->pSP = pSP;
	pPrg->onconf = onconf;
	//pPrg->aColmask[0] = 0xffffffff;
	//pPrg->aColmask[1] = 0xffffffff;

	/* Allocate and populate a new Parse context to use for coding the
	** SP sub-program.  */
	pSubParse = sqlite3StackAllocZero(db, sizeof(Parse));
	if (!pSubParse) return 0;
	memset(&sNC, 0, sizeof(sNC));
	sNC.pParse = pSubParse;
	pSubParse->db = db;
	//pSubParse->pSPTab = pTab;
	pSubParse->pToplevel = pTop;
	pSubParse->zAuthContext = pSP->zName;
	//pSubParse->eSPOp = pSP->op;
	pSubParse->nQueryLoop = pParse->nQueryLoop;

	v = sqlite3GetVdbe(pSubParse);
	if (v) {
		VdbeComment((v, "Start: %s.%s ", pSP->zName, onSPErrorText(onconf)));
#ifndef SQLITE_OMIT_TRACE
		sqlite3VdbeChangeP4(v, -1,sqlite3MPrintf(db, "-- SP %s", pSP->zName), P4_DYNAMIC	);
#endif
		/* copy variable name and count to the subparse */
		if (pParse->pVList != 0) {
			memcpy(&pSubParse->pVList, &pParse->pVList, pParse->pVList[0]);
			pSubParse->nVar = pParse->nVar;
		}
		/* Code the SP program into the sub-vdbe. */
		codeSPProgram(pSubParse, pSP, onconf);

		/* Insert an OP_Halt at the end of the sub-program. */
		if (iEndSP) {
			sqlite3VdbeResolveLabel(v, iEndSP);
		}
		sqlite3VdbeAddOp0(v, OP_Halt);
		VdbeComment((v, "End: %s.%s", pSP->zName, onSPErrorText(onconf)));

		
		transferSPParseError(pParse, pSubParse);
		if (db->mallocFailed == 0) {
			pProgram->aOp = sqlite3VdbeTakeOpArray(v, &pProgram->nOp, &pTop->nMaxArg);
		}
		pProgram->nMem = pSubParse->nMem;
		pProgram->nCsr = pSubParse->nTab;
		pProgram->token = (void *)pSP;
		//pPrg->aColmask[0] = pSubParse->oldmask;
		//pPrg->aColmask[1] = pSubParse->newmask;
		sqlite3VdbeDelete(v);
	}

	assert(!pSubParse->pAinc && !pSubParse->pZombieTab);
	//assert(!pSubParse->pSPPrg && !pSubParse->nMaxArg);
	sqlite3ParserReset(pSubParse);
	sqlite3StackFree(db, pSubParse);

	return pPrg;
}

/*
** Return a pointer to a SPPrg object containing the sub-program for
** SP pSP with default ON CONFLICT algorithm orconf. If no such
** SPPrg object exists, a new object is allocated and populated before
** being returned.
*/
static SPPrg *getRowSP(
	Parse *pParse,       /* Current parse context */
	SP *pSP,   /* SP to code */
	int onconf           /* ON CONFLICT algorithm. */
) {
	Parse *pRoot = sqlite3ParseToplevel(pParse);
	SPPrg *pPrg;

	//assert(pSP->zName == 0);

	/* It may be that this SP has already been coded (or is in the
	** process of being coded). If this is the case, then an entry with
	** a matching SPPrg.pSP field will be present somewhere
	** in the Parse.pSPPrg list. Search for such an entry.  */
	for (pPrg = pRoot->pSPPrg;
		pPrg && (pPrg->pSP != pSP || pPrg->onconf != onconf);
		pPrg = pPrg->pNext);

	/* If an existing SPPrg could not be located, create a new one. */
	if (!pPrg) {
		pPrg = codeRowSP(pParse, pSP, onconf);
	}

	return pPrg;
}

/*
** Generate code for the SP program associated with SP p on
** table pTab. The reg, orconf and ignoreJump parameters passed to this
** function are the same as those described in the header function for
** sqlite3CodeRowSP()
*/

void sqlite3CodeRowSPDirect(
	Parse *pParse,       /* Parse context */
	SP *p,          /* SP to code */
	ExprList *pParam,
	int onconf,          /* ON CONFLICT policy */
	int ignoreJump       /* Instruction to jump to for RAISE(IGNORE) */
) {
	Vdbe *v = sqlite3GetVdbe(pParse); /* Main VM */
	SPPrg *pPrg;

	if (pParam != 0 && pParam->nExpr > 0) { /** Parameters processing **/
		if (p->nPrm != pParam->nExpr) {
			sqlite3ErrorMsg(pParse, "Required %d parameters, obtained %d parameters.", p->nPrm, pParam->nExpr);
			goto coderow_out;
		}

		for (int i = 0; i < p->nPrm; i++) {

			int tmp = sqlite3GetTempReg(pParse);
			sqlite3ExprCode(pParse, pParam->a[i].pExpr, tmp);
			sqlite3VdbeAddOp2(v, OP_Define, tmp, ++pParse->nVar);
			sqlite3VdbeAppendP4(v, p->aPrm[i].zName, P4_TRANSIENT);
			pParse->pVList = sqlite3VListAdd(pParse->db, pParse->pVList, p->aPrm[i].zName, p->aPrm[i].nName, pParse->nVar - 1);
			VdbeComment((v, "%s", pParam->a[i].zName));
		}
	}

	pPrg = getRowSP(pParse, p, onconf);
	assert(pPrg || pParse->nErr || pParse->db->mallocFailed);

	/* Code the OP_Program opcode in the parent VDBE. P4 of the OP_Program
	** is a pointer to the sub-vdbe containing the SP program.  */
	if (pPrg) {
		
		int bRecursive = 0; // (p->zName && 0 == (pParse->db->flags&SQLITE_RecSPs));

		sqlite3VdbeAddOp4(v, OP_Program, 0, ignoreJump, ++pParse->nMem,(const char *)pPrg->pProgram, P4_SUBPROGRAM);
		VdbeComment((v, "Call: %s.%s", (p->zName ? p->zName : "fkey"), onSPErrorText(onconf)));

		/* Set the P5 operand of the OP_Program instruction to non-zero if
		** recursive invocation of this SP program is disallowed. Recursive
		** invocation is disallowed if (a) the sub-program is really a SP,
		** not a foreign key action, and (b) the flag to enable recursive SPs
		** is clear.  */
		sqlite3VdbeChangeP5(v, (u8)bRecursive);
	}
coderow_out:
	return;
}

/*
** This is called to code the required FOR EACH ROW SPs for an operation
** on table pTab. The operation to code SPs for (INSERT, UPDATE or DELETE)
** is given by the op parameter. The tr_tm parameter determines whether the
** BEFORE or AFTER SPs are coded. If the operation is an UPDATE, then
** parameter pChanges is passed the list of columns being modified.
**
** If there are no SPs that fire at the specified time for the specified
** operation on pTab, this function is a no-op.
**
** The reg argument is the address of the first in an array of registers
** that contain the values substituted for the new.* and old.* references
** in the SP program. If N is the number of columns in table pTab
** (a copy of pTab->nCol), then registers are populated as follows:
**
**   Register       Contains
**   ------------------------------------------------------
**   reg+0          OLD.rowid
**   reg+1          OLD.* value of left-most column of pTab
**   ...            ...
**   reg+N          OLD.* value of right-most column of pTab
**   reg+N+1        NEW.rowid
**   reg+N+2        OLD.* value of left-most column of pTab
**   ...            ...
**   reg+N+N+1      NEW.* value of right-most column of pTab
**
** For ON DELETE SPs, the registers containing the NEW.* values will
** never be accessed by the SP program, so they are not allocated or
** populated by the caller (there is no data to populate them with anyway).
** Similarly, for ON INSERT SPs the values stored in the OLD.* registers
** are never accessed, and so are not allocated by the caller. So, for an
** ON INSERT SP, the value passed to this function as parameter reg
** is not a readable register, although registers (reg+N) through
** (reg+N+N+1) are.
**
** Parameter orconf is the default conflict resolution algorithm for the
** SP program to use (REPLACE, IGNORE etc.). Parameter ignoreJump
** is the instruction that control should jump to if a SP program
** raises an IGNORE exception.
*/
void sqlite3CodeRowSP(
	Parse *pParse,       /* Parse context */
	SP *pSP,   /* List of SPs on table pTab */
	ExprList* pParam,
	int onconf,          /* ON CONFLICT policy */
	int ignoreJump       /* Instruction to jump to for RAISE(IGNORE) */
) {
	SP *p;          /* Used to iterate through pSP list */
	for (p = pSP; p; p = p->pNext) {
		/* Sanity checking:  The schema for the SP and for the table are
		** always defined.  The SP must be in the same schema as the table
		** or else it must be a TEMP SP. */
		assert(p->pSchema != 0);
		//assert(p->pSchema == pParse->db->aDb[1].pSchema);
		
		/* Determine whether we should code this SP */
		sqlite3CodeRowSPDirect(pParse, p, pParam, onconf, ignoreJump); //reg,
		
	}
}

/*
** SPs may access values stored in the old.* or new.* pseudo-table.
** This function returns a 32-bit bitmask indicating which columns of the
** old.* or new.* tables actually are used by SPs. This information
** may be used by the caller, for example, to avoid having to load the entire
** old.* record into memory when executing an UPDATE or DELETE command.
**
** Bit 0 of the returned mask is set if the left-most column of the
** table may be accessed using an [old|new].<col> reference. Bit 1 is set if
** the second leftmost column value is required, and so on. If there
** are more than 32 columns in the table, and at least one of the columns
** with an index greater than 32 may be accessed, 0xffffffff is returned.
**
** It is not possible to determine if the old.rowid or new.rowid column is
** accessed by SPs. The caller must always assume that it is.
**
** Parameter isNew must be either 1 or 0. If it is 0, then the mask returned
** applies to the old.* table. If 1, the new.* table.
**
** Parameter tr_tm must be a mask with one or both of the SP_BEFORE
** and SP_AFTER bits set. Values accessed by BEFORE SPs are only
** included in the returned mask if the SP_BEFORE bit is set in the
** tr_tm parameter. Similarly, values accessed by AFTER SPs are only
** included in the returned mask if the SP_AFTER bit is set in tr_tm.

u32 sqlite3SPColmask(
	Parse *pParse,       // Parse context 
	SP *pSP,   // List of SPs on table pTab 
	ExprList *pChanges,  // Changes list for any UPDATE OF SPs 
	int isNew,           // 1 for new.* ref mask, 0 for old.* ref mask 
		int tr_tm,           // Mask of SP_BEFORE|SP_AFTER 
	Table *pTab,         // The table to code SPs from 
	int orconf           // Default ON CONFLICT policy for SP steps 
) {
	const int op = pChanges ? TK_UPDATE : TK_DELETE;
	u32 mask = 0;
	SP *p;

	assert(isNew == 1 || isNew == 0);
	for (p = pSP; p; p = p->pNext) {
		if (p->op == op && (tr_tm&p->tr_tm)
			&& checkColumnOverlap(p->pColumns, pChanges)
			) {
			SPPrg *pPrg;
			pPrg = getRowSP(pParse, p, pTab, orconf);
			if (pPrg) {
				mask |= pPrg->aColmask[isNew];
			}
		}
	}

	return mask;
}
*/



/*
** Add a new column to the table currently being constructed.
**
** The parser calls this routine once for each column declaration
** in a CREATE TABLE statement.  sqlite3StartTable() gets called
** first to get things going.  Then this routine is called for each
** column.
*/
void sqlite3AddParam(Parse *pParse, Token *pName, Token *pType) {
	SP *p;
	int i;
	char *z;
	char *zType;
	Param *pPrm;
	sqlite3 *db = pParse->db;
	if ((p = pParse->pNewSP) == 0) return;
#if SQLITE_MAX_COLUMN
	if (p->nPrm + 1>db->aLimit[SQLITE_LIMIT_COLUMN]) {
		sqlite3ErrorMsg(pParse, "too many params on %s", p->zName);
		return;
	}
#endif
	z = sqlite3DbMallocRaw(db, pName->n + pType->n + 2);
	if (z == 0) return;
	memcpy(z, pName->z, pName->n);
	z[pName->n] = 0;
	sqlite3Dequote(z);
	for (i = 0; i<p->nPrm; i++) {
		if (sqlite3_stricmp(z, p->aPrm[i].zName) == 0) {
			sqlite3ErrorMsg(pParse, "duplicate parameter name: %s", z);
			sqlite3DbFree(db, z);
			return;
		}
	}
	if ((p->nPrm & 0x7) == 0) {
		Param *aNew;
		aNew = sqlite3DbRealloc(db, p->aPrm, (p->nPrm + 8) * sizeof(p->aPrm[0]));
		if (aNew == 0) {
			sqlite3DbFree(db, z);
			return;
		}
		p->aPrm = aNew;
	}
	pPrm = &p->aPrm[p->nPrm];
	memset(pPrm, 0, sizeof(p->aPrm[0]));
	pPrm->zName = z;
	pPrm->nName = pName->n;
	//sqlite3ColumnPropertiesFromName(p, pCol);

	if (pType->n == 0) {
		/* If there is no type specified, columns have the default affinity
		** 'BLOB'. */
		pPrm->affinity = SQLITE_AFF_BLOB;
		pPrm->szEst = 1;
	}
	else {
		zType = z + sqlite3Strlen30(z) + 1;
		memcpy(zType, pType->z, pType->n);
		zType[pType->n] = 0;
		sqlite3Dequote(zType);
		pPrm->affinity = sqlite3AffinityType(zType, &pPrm->szEst);
		pPrm->paramFlags |= PRMFLAG_HASTYPE;
	}
	p->nPrm++;
	
}

void sqlite3AddParamDflt(Parse *pParse, ExprSpan *pSpan) {
	SP *p;
	Param *pPrm;
	sqlite3 *db = pParse->db;
	p = pParse->pNewSP;
	if (p != 0) {
		pPrm = &(p->aPrm[p->nPrm - 1]);
		if (!sqlite3ExprIsConstantOrFunction(pSpan->pExpr, db->init.busy)) {
			sqlite3ErrorMsg(pParse, "default value of parameter [%s] is not constant",pPrm->zName);
		}
		else {
			/* A copy of pExpr is used instead of the original, as pExpr contains
			** tokens that point to volatile memory. The 'span' of the expression
			** is required by pragma table_info.
			*/
			Expr x;
			sqlite3ExprDelete(db, pPrm->pDflt);
			memset(&x, 0, sizeof(x));
			x.op = TK_SPAN;
			x.u.zToken = sqlite3DbStrNDup(db, (char*)pSpan->zStart,	(int)(pSpan->zEnd - pSpan->zStart));
			x.pLeft = pSpan->pExpr;
			x.flags = EP_Skip;
			pPrm->pDflt = sqlite3ExprDup(db, &x, EXPRDUP_REDUCE);
			sqlite3DbFree(db, x.u.zToken);
		}
	}
	sqlite3ExprDelete(db, pSpan->pExpr);
}

void sqlite3DeleteParam(sqlite3 *db, Param *pParam) {
	Param *pTmp = pParam;
	while (pTmp) {
		sqlite3DbFree(db, pTmp);
		*pTmp++;
	}
}

void sqlite3DeleteRole(sqlite3 *db, Role *pRole) {
	Role *pTmp = pRole;
	while (pTmp) {
		sqlite3DbFree(db, pTmp);
		*pTmp++;
	}
}

Table* sqlite3FindRoleTable(sqlite3* db) {
	
	HashElem *elem;                /* Used to loop thru the element list */
	int count;                     /* Number of elements left to test */
	unsigned int h;                /* The computed hash */
	Hash* ht = &db->aDb[0].pSchema->tblHash;
	struct _ht *pEntry;
	pEntry = &ht[0];
	elem = pEntry->chain;
	count = pEntry->count;
	while (count--) {
		assert(elem != 0);
		if ((((Table*)elem)->tabFlags & TF_LuciaGID) == 0) {
			return elem;
		}
		elem = elem->next;
	}
	return 0;
}

void sqlite3SPAddRole(Parse *pParse, Token *pName) {
	Role *pRole;
	SP *p;
	Table *pGrpTable;
	if ((p = pParse->pNewSP) == 0) return;
	assert(sqlite3Strlen30(pName->z) == pName->n);
	pGrpTable = sqlite3FindRoleTable(pParse->db);
	if (pRole->iTable == 0) {
		sqlite3ErrorMsg(pParse, "Lucia Error: Unable to find the table containing the WEB GROUP column");
	}
	p->aRole = sqlite3DbRealloc(pParse->db, p->aRole, (p->nRole + 1) * sizeof(p->aRole[0]));

	pRole = &p->aRole[p->nRole];
	memset(pRole, 0, sizeof(p->aRole[0]));
	pRole->zName = pName->z;
	pRole->nName = pName->n;
	pRole->iTable = pGrpTable;
	p->nRole++;
}


//void sqlite3If(Parse *pParse, ExprList *pIf) {Vdbe* v = sqlite3GetVdbe(pParse);}

#endif /* !defined(SQLITE_OMIT_SP) */

#ifndef SQLITE_OMIT_BIND
void sqlite3Define(Parse* pParse, char* zVar, i16 nzVar, Expr* pVal) {
	Vdbe* v = sqlite3GetVdbe(pParse);
	int i;
	int tmp = sqlite3GetTempReg(pParse);
	assert(sqlite3Strlen30(zVar) == nzVar);
	if ((i = sqlite3VListNameToNum(pParse->pVList,zVar,nzVar)) == SQLITE_IOERR_NOMEM) { /* if new variable */
		i = pParse->nVar;
		pParse->pVList = sqlite3VListAdd(pParse->db, pParse->pVList, zVar, nzVar, i);
		pParse->nVar++;
	}
	
	sqlite3ExprCode(pParse, pVal, tmp);
	sqlite3VdbeAddOp2(v, OP_Define, tmp, i+1);
	sqlite3VdbeAppendP4(v,zVar, P4_TRANSIENT);
	
	VdbeComment((v, "%s", zVar));
}

void sqlite3DefineVariable(Parse* pParse, Token *pVar, Expr *pVal) {
	sqlite3Define(pParse, sqlite3NameFromToken(pParse->db, pVar), pVar->n,pVal);
}

int sqlite3ShowVariable(
	Parse *pParse,
	Token *pId1        /* First part of [schema.]id field */
) {
	int rc, zIndex;
	sqlite3* db = pParse->db;
	sqlite3_value *t = db->aVar;
	char* zLeft = sqlite3NameFromToken(db, pId1);
	Vdbe* v = sqlite3GetVdbe(pParse);
	sqlite3VdbeRunOnlyOnce(v);
	if (v == 0) return SQLITE_ERROR;
	assert(sqlite3_mutex_held(db->mutex));
	pParse->nMem++;
	zIndex = sqlite3VListNameToNum(db->zVar, zLeft, pId1->n);
	if (zIndex == SQLITE_IOERR_NOMEM) {
		sqlite3ErrorMsg(pParse,"Bind variable \"%s\" not found.", zLeft);
		goto bind_out;
	}
	sqlite3VdbeSetNumCols(v, 1);
	sqlite3VdbeSetColName(v, 0, COLNAME_NAME, zLeft, SQLITE_TRANSIENT);
	
	switch (sqlite3_value_type((int)t+zIndex)) {
	case SQLITE_TEXT:
		sqlite3VdbeLoadString(v, 1, (const char*)sqlite3_value_text((int)t + zIndex));
		sqlite3VdbeAddOp2(v, OP_ResultRow, 1, 1);
		break;
	case SQLITE_INTEGER:
		sqlite3VdbeAddOp4Dup8(v, OP_Int64, 0, 1, 0, sqlite3_value_int64((int)t + zIndex), P4_INT64);
		sqlite3VdbeAddOp2(v, OP_ResultRow, 1, 1);
		break;
	/*case MEM_Real:
		returnSingleInt(v, pVal->u.r); //returnSingleReal(v, pVal->u.)
		break; */
	}
bind_out:
	sqlite3_mutex_leave(db->mutex);
}
#endif

