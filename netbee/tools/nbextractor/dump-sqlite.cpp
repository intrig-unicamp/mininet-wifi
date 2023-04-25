/*
* Copyright (c) 2002 - 2011
* NetGroup, Politecnico di Torino (Italy)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following condition
* is met:
*
* Neither the name of the Politecnico di Torino nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


#include "dump-sqlite.h"
#include "../utils/utils.h"


#if (!defined(_WIN32) && !defined(_WIN64))
#define _stricmp strcasecmp
#endif



int SQLGetColumnNamesCallbackFunc(void* strBuff /* used to store columnNames*/, int nColumns, char** columnValues, char** columnNames)
{
	for (int i= 0; i< nColumns ; i ++)
	{	
		if (!strcmp(columnNames[i], "name"))
		{
			sstrncat((char*) strBuff, columnValues[i], SQLCOMMAND_MAX_LEN);
			sstrncat((char*) strBuff, (char*) ",", SQLCOMMAND_MAX_LEN);
			break;
		}
	}

	return 0;
}

int SQLGetIndexNameCallbackFunc(void* strBuff /* used to store columnNames*/, int nColumns, char** columnValues, char** columnNames)
{
	for (int i= 0; i< nColumns ; i ++)
	{	
		if (!strcmp(columnNames[i], "name"))
		{
			sstrncat((char*) strBuff, columnValues[i], SQLCOMMAND_MAX_LEN);
			break;
		}
	}
	return 0;
}



int SQLGetRowsCountCallbackFunc(void* rowsCount, int nColumns, char** columnValues, char** columnNames)
{
	for (int i= 0; i< nColumns ; i ++)
	{
		if (!strcmp(columnNames[i], "ROWSCOUNT"))
		{
			sprintf((char*) rowsCount, "%s", columnValues[i]);
			break;
		}
	}

	// Return success
	return 0;
}

// Note: column names (but also indexes) are separated by the ',' delimiter
int CreateDBTable(sqlite3* pDB,	const char* TableName, const char* ColumnNames, const char* IndexNames)
{
	char SQLCmdBuffer[SQLCOMMAND_MAX_LEN] = "\0";
	int SQLCmdBufferOccupancy = 0;
	char CurrentColumnNames[SQLCOMMAND_MAX_LEN] = "\0";
	char* errMsg = NULL;
	int RetVal = 0;

	// Test if there exists table with same name
	sprintf(SQLCmdBuffer, "PRAGMA table_info(%s)", TableName);

#ifdef _DEBUG_LOUD
	fprintf(stderr, "executing sql cmd{%s}...\n", sqlCmdBuffer);
#endif

	// PRAGMA for performance
	RetVal= sqlite3_exec(pDB, "PRAGMA journal_mode = OFF;", NULL, NULL, &errMsg);
	if (RetVal)
	{
		fprintf(stderr, "PRAGMA journaling exec error: %s\n", errMsg);
		sqlite3_free(errMsg);
		return nbFAILURE;
	}

	RetVal= sqlite3_exec(pDB, "PRAGMA synchronous = OFF;", NULL, NULL, &errMsg);
	if (RetVal)
	{
		fprintf(stderr, "PRAGMA syncronous write exec error: %s\n", errMsg);
		sqlite3_free(errMsg);
		return nbFAILURE;
	}
      

	// Call the 'SQLGetColumnNamesCallbackFunc' callback function, which will return the column names defined for that table
	RetVal= sqlite3_exec(pDB, SQLCmdBuffer, SQLGetColumnNamesCallbackFunc, CurrentColumnNames, &errMsg);
	if (RetVal)
	{
		fprintf(stderr, "SQL command {%s} exec error: %s\n", SQLCmdBuffer, errMsg);
		sqlite3_free(errMsg);
		return nbFAILURE;
	}

	// A table with that name already exist
	// Column names are stored in 'CurrentColumnNames'
	if (CurrentColumnNames[0])
	{
		if (_stricmp(CurrentColumnNames, ColumnNames) == 0)
		{
			//a table with the same schema exists. Now we have to check the index
			char CurrentIndexName[SQLCOMMAND_MAX_LEN] = "\0";
			sprintf(SQLCmdBuffer, "PRAGMA index_list(%s)", TableName);
			RetVal= sqlite3_exec(pDB, SQLCmdBuffer, SQLGetIndexNameCallbackFunc, CurrentIndexName, &errMsg);
			if (RetVal)
			{
				fprintf(stderr, "SQL command {%s} exec error: %s\n", SQLCmdBuffer, errMsg);
				sqlite3_free(errMsg);
				return nbFAILURE;
			}	
		
			if(strcmp(CurrentIndexName,"anindex") != 0)
			{
				fprintf(stderr, "Table %s already exists, with the same schema but a different index. Cannot append new data to that table. Aborting\n", TableName);
				return nbFAILURE;	
			}
			
			char CurrentIndexColumnNames[SQLCOMMAND_MAX_LEN] = "\0";
			sprintf(SQLCmdBuffer, "PRAGMA index_info(%s)", CurrentIndexName);
			RetVal= sqlite3_exec(pDB, SQLCmdBuffer, SQLGetColumnNamesCallbackFunc, CurrentIndexColumnNames, &errMsg);
			if (RetVal)
			{
				fprintf(stderr, "SQL command {%s} exec error: %s\n", SQLCmdBuffer, errMsg);
				sqlite3_free(errMsg);
				return nbFAILURE;
			}
		
			if (_stricmp(CurrentIndexColumnNames, IndexNames) != 0)
			{
				fprintf(stderr, "Table %s already exists, with the same schema but a different index. Cannot append new data to that table. Aborting\n", TableName);
				return nbFAILURE;	
			}
		
			fprintf(stderr, "Table %s already exists, with the same schema and index. Appending new data to that table.\n", TableName);
			// We do not return any error code here; table creating has failed, but table already
			// exist, so there's no reason to return an error
			return nbSUCCESS;
		}
		else
		{
			fprintf(stderr, "Table %s already exists, with a different schema. Cannot append new data to that table. Aborting.\n", TableName);
			// We do not return any error code here; table creating has failed, but table already
			// exist, so there's no reason to return an error
			return nbFAILURE;
		}
	}
	else
	{
		char* pColName;

		// Table does not exist; let's create a new one
		sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, "CREATE TABLE ");
		sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, TableName);
		sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, " (");

		// Copy column names into a private variable, since strtok() rewrites the original buffer
		strcpy(CurrentColumnNames, ColumnNames);

		pColName= strtok(CurrentColumnNames, ",");
		while (pColName)
		{
			sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, pColName);

			pColName = strtok(NULL, ",");
			if (pColName)
				sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, ",");
			else
				sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, ")");
		}

#ifdef _DEBUG_LOUD
		fprintf(stderr, "executing sql cmd{%s}...\n", sqlCmdBuffer);
#endif

		RetVal= sqlite3_exec(pDB, SQLCmdBuffer, NULL, NULL, &errMsg);
		if (RetVal)
		{
			fprintf(stderr, "SQL command {%s} exec err: %s\n", SQLCmdBuffer, errMsg);
			sqlite3_free(errMsg);
			return nbFAILURE;
		}
		
		//lets create the indexes
		if(IndexNames != NULL)
		{
			SQLCmdBufferOccupancy = 0;
			sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, "CREATE INDEX anindex ON ");
			sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, TableName);
			sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, " (");
			pColName= strtok((char*)IndexNames, ",");
			while (pColName)
			{
				sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, pColName);

				pColName = strtok(NULL, ",");
				if (pColName)
					sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, ",");
				else
					sstrncat_ex(SQLCmdBuffer, sizeof(SQLCmdBuffer), &SQLCmdBufferOccupancy, ")");
			}
	
		#ifdef _DEBUG_LOUD
			fprintf(stderr, "executing sql cmd{%s}...\n", SQLCmdBuffer);
		#endif

			RetVal= sqlite3_exec(pDB, SQLCmdBuffer, NULL, NULL, &errMsg);
			if (RetVal)
			{
				fprintf(stderr, "SQL command {%s} exec err: %s\n", SQLCmdBuffer, errMsg);
				sqlite3_free(errMsg);
				return nbFAILURE;
			}
		}

	}

	return nbSUCCESS;
}



// Update two extra tables in the database: tables are created if they do not exist.
// Schemas of the extra tables are hardcoded and currently keep just some simple info
void UpdateExtraDBTable(sqlite3* pDB, int nRowsAdded, ConfigParams_t ConfigParams, char* db_current_filename, char* run_identifier)
{
	char SQLCommandBuffer[SQLCOMMAND_MAX_LEN];
	int SQLCommandBufferOccupancy;
	char SQLRowsCountCommand[1024];
	char ValueTempBuffer[MAX_FIELD_SIZE_EXTRA_TABLE];
	time_t      CurrentTime;                //variables used to update time
	struct tm * TimeInfo;
	FILE*       pFile;
	unsigned long       FileSize;

	// If the tables exists, CreateDBTable will still return SUCCESS as the table schema is currently hard-coded
	
	/* FIRST TABLE */
	{
		if (CreateDBTable(pDB, "DatabaseFileInfo", "DatabaseFileName,FileSize,LastUpdateTime,") == nbFAILURE)
		{
			fprintf(stderr, "Error creating 'DatabaseFileInfo' table.\n");
			return;
		}

		SQLCommandBufferOccupancy= 0;
		sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferOccupancy, "INSERT OR REPLACE INTO DatabaseFileInfo (");
		sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferOccupancy, "DatabaseFileName,FileSize,LastUpdateTime");
		sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferOccupancy, ") VALUES (?,?,?);");

		const char *tail;
		sqlite3_stmt *stmt;
		sqlite3_prepare_v2(pDB,SQLCommandBuffer,strlen(SQLCommandBuffer+1),&stmt, &tail);	

		sqlite3_bind_text(stmt,1, db_current_filename, strlen(db_current_filename),SQLITE_TRANSIENT);

		// Get databaseFileSize
		pFile = fopen(db_current_filename, "rb");
		fseek(pFile, 0, SEEK_END);
		FileSize = ftell(pFile);
		fclose(pFile);
		sprintf(ValueTempBuffer, "%ld", FileSize);

		sqlite3_bind_text(stmt,2, ValueTempBuffer, strlen(ValueTempBuffer),SQLITE_TRANSIENT);

		// Get current time
		time(&CurrentTime);
		TimeInfo = localtime(&CurrentTime);

		sqlite3_bind_text(stmt,3, asctime(TimeInfo), strlen(asctime(TimeInfo)),SQLITE_TRANSIENT);

		if (AddNewDataRecord(pDB,stmt)  == nbFAILURE)
		{
			fprintf(stderr, "Error updating 'DatabaseFileInfo' table.\n");
			return;
		}
	}

	/* SECOND TABLE */
	{
		if (CreateDBTable(pDB, "TableSummary", "TableName,TotalNumOfRows,NumOfRowsAdded,TimeWhenRowsAdded,RunIdentifier,") == nbFAILURE)
		{
			fprintf(stderr, "Error creating 'TableSummary' table.\n");
			return;
		}

		SQLCommandBufferOccupancy= 0;
		sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferOccupancy, "INSERT INTO TableSummary (");
		sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferOccupancy, "TableName,TotalNumOfRows,NumOfRowsAdded,TimeWhenRowsAdded,RunIdentifier");
		sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferOccupancy, ") VALUES (?,?,?,?,?);");

		const char *tail;
		sqlite3_stmt *stmt;
		sqlite3_prepare_v2(pDB,SQLCommandBuffer,strlen(SQLCommandBuffer+1),&stmt, &tail);	
		
		sqlite3_bind_text(stmt,1, ConfigParams.SQLTableName, strlen(ConfigParams.SQLTableName),SQLITE_TRANSIENT);

		// Get number of rows in current database
		ssnprintf(SQLRowsCountCommand, sizeof(SQLRowsCountCommand), "SELECT count(*) as ROWSCOUNT FROM %s", ConfigParams.SQLTableName);
		sqlite3_exec(pDB, SQLRowsCountCommand, SQLGetRowsCountCallbackFunc, ValueTempBuffer, NULL);
		
		sqlite3_bind_text(stmt,2, ValueTempBuffer, strlen(ValueTempBuffer),SQLITE_TRANSIENT);

		sprintf(ValueTempBuffer, "%d", nRowsAdded);
		
		sqlite3_bind_text(stmt,3, ValueTempBuffer, strlen(ValueTempBuffer),SQLITE_TRANSIENT);
		
		sqlite3_bind_text(stmt,4, asctime(TimeInfo), strlen(asctime(TimeInfo)),SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt,5, run_identifier, strlen(run_identifier),SQLITE_TRANSIENT);

		if (AddNewDataRecord(pDB, stmt)  == nbFAILURE)
		{
			fprintf(stderr, "Error updating 'TableSummary' table.\n");
			return;
		}
	}
}


// Prepare the string that is needed to insert data into th SQLite3 database; the first portion of that string is always the same, so we can prepare it in advance
// The string is in this form: INSERT INTO table (tstamp,field1,field2,...) VALUES (?,?,?,...);
// Return failure in case the buffer is not large enough; in this case, let's trying increase the buffer
int PrepareAddNewDataRecordSQLCommand(sqlite3* pDB, const char* SQLTableName, _nbExtractFieldInfo *DescriptorVector, int NumEntries, char* SQLCommandBuffer, int SQLCommandBufferSize, int& SQLCommandBufferOccupancy)
{
int i;

	sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, "INSERT INTO ");
	sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, SQLTableName);
	sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, " (\"tstamp\", \"packetlength\",");

    for (i= 0; i < NumEntries; i++)
    {
		sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, "\"");
		sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, DescriptorVector[i].Proto);
		sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, "_");
		sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, DescriptorVector[i].Name);
		sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, "\"");

		if (i < (DescriptorVector->NumEntries - 1))
			sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, ",");
    }

	sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, ") VALUES (");
	
	for (i = 0; i < NumEntries+2/*+2 is because, for each packet, also its timestamp and its length are extracted*/; i++)
	{
		sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, "?");
		if (i <= (NumEntries))
			sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, ",");
		else
			sstrncat_ex(SQLCommandBuffer, SQLCommandBufferSize, &SQLCommandBufferOccupancy, ");");
	}

	if (SQLCommandBufferOccupancy == SQLCommandBufferSize)
		return nbFAILURE;
	else
		return nbSUCCESS;
}

// Inserts a new data record into the database (the table name is contained in the SQLCommandBuffer)
int AddNewDataRecord(sqlite3* pDB, sqlite3_stmt *stmt)
{
	int RetVal;

	RetVal = sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	if(RetVal !=SQLITE_DONE )
	{
		fprintf(stderr, "SQLLite error while adding a new record in the database: %s\n",sqlite3_errmsg(pDB));
		return nbFAILURE;
	}

	return nbSUCCESS;
}


int SQL_open(char *db_filename, ConfigParams_t *cfg, sqlite3** db, _nbExtractFieldInfo * DescriptorVector, int NumEntries)
{
  char SQLColumnNames[2048]= "\0";
  int SQLColumnNamesOccupancy= 0;
  char SQLIndexNames[2048]= "\0";
  int SQLIndexNamesOccupancy= 0;
  
  int RetVal= sqlite3_open(db_filename, db);
  if (RetVal)
	{
      fprintf(stderr, "Failed to open Database: %s\n", sqlite3_errmsg(*db));
      return nbFAILURE;
    }

  // Let's create the list of columns (i.e. the list of fields) we want to store in the database
  // First column is the timestamp, then the other fields will follow
  // This field is called "tstamp" because "timestamp" is a SQLite reserved word
  sstrncat_ex(SQLColumnNames, sizeof(SQLColumnNames), &SQLColumnNamesOccupancy, "tstamp,");
  
  // For each packet, we extract also its length (including padding)
  sstrncat_ex(SQLColumnNames, sizeof(SQLColumnNames), &SQLColumnNamesOccupancy, "packetlength,");

  for (int i= 0; i < NumEntries; i++)
  {
	sstrncat_ex(SQLColumnNames, sizeof(SQLColumnNames), &SQLColumnNamesOccupancy, DescriptorVector[i].Proto);
	// We have to use '_', since the '.' is a reserved character in SQLite
	sstrncat_ex(SQLColumnNames, sizeof(SQLColumnNames), &SQLColumnNamesOccupancy, "_");
	sstrncat_ex(SQLColumnNames, sizeof(SQLColumnNames), &SQLColumnNamesOccupancy, DescriptorVector[i].Name);
	// Let's put the separator even if we're the last field name (the following code will take care of this)
	sstrncat_ex(SQLColumnNames, sizeof(SQLColumnNames), &SQLColumnNamesOccupancy, ",");
  }
  
  for (int i = 0; i < cfg->IndexNo; i++)
  {
	char delims[] = ".";
	char *proto = strtok(cfg->SQLIndexFields[i],delims);
	char *field = strtok( NULL, delims );
	
	if(proto==NULL || field==NULL)
	{
      	fprintf(stderr, "A field specified as index is malformed: %s\n", cfg->SQLIndexFields[i]);
      	return nbFAILURE;
    }
	
	sstrncat_ex(SQLIndexNames, sizeof(SQLIndexNames), &SQLIndexNamesOccupancy, proto);
	// We have to use '_', since the '.' is a reserved character in SQLite
	sstrncat_ex(SQLIndexNames, sizeof(SQLIndexNames), &SQLIndexNamesOccupancy, "_");
	sstrncat_ex(SQLIndexNames, sizeof(SQLIndexNames), &SQLIndexNamesOccupancy, field);
	// Let's put the separator even if we're the last field name (the following code will take care of this)
	sstrncat_ex(SQLIndexNames, sizeof(SQLIndexNames), &SQLIndexNamesOccupancy, ",");
  }

  RetVal= CreateDBTable(*db, cfg->SQLTableName, SQLColumnNames, (SQLIndexNames[0]!='\0') ? SQLIndexNames : NULL);

  return RetVal;
}
