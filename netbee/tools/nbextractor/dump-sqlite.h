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


#pragma once


#include <stdio.h>
#include <nbee.h>
#include <sqlite3.h>
#include "configparams.h"


#define SQLCOMMAND_MAX_LEN 1024
#define MAX_FIELD_SIZE_EXTRA_TABLE 100

int SQLGetColumnNamesCallbackFunc(void* strBuff, int nColumns, char** columnValues, char** columnNames);
int SQLGetRowsCountCallbackFunc(void* rowsCount, int nColumns, char** columnValues, char** columnNames);
int CreateDBTable(sqlite3* pDB, const char* TableName, const char* ColumnNames, const char* IndexName = NULL);
void UpdateExtraDBTable(sqlite3* pDB, int nRowsAdded, ConfigParams_t ConfigParams, char* db_current_filename, char* run_identifier);

int PrepareAddNewDataRecordSQLCommand(sqlite3* pDB, const char* SQLTableName, _nbExtractFieldInfo *DescriptorVector, int NumEntries, char* SQLCommandBuffer, int SQLCommandBufferSize, int& SQLCommandBufferOccupancy);
int AddNewDataRecord(sqlite3* pDB, sqlite3_stmt *stmt);

int SQL_open(char *db_filename, ConfigParams_t *cfg, sqlite3** db, _nbExtractFieldInfo* DescriptorVector, int NumEntries);
