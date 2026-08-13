#include "balance.h"
#include <stdio.h>

#include <sqlite3.h>

extern sqlite3 *db;

int init_balances(void){
	const char *sql = "CREATE TABLE IF NOT EXISTS balances ("
		" address TEXT PRIMARY KEY,"
		" balance INTEGER NOT NULL"
		");";

	char *err = NULL;
	sqlite3_exec(db, sql, NULL, NULL, &err);

	if(err){
		fprintf(stderr, "[init_balances] Erro: %s\n", err);
		sqlite3_free(err);
		return 0;
	}

	return 1;
}

uint64_t get_balance(const char *address){
	if(db == NULL) return 0;
	const char *sql = "SELECT balance FROM balances WHERE address = ?;";

	sqlite3_stmt *stmt = NULL;
	uint64_t balance = 0;

	if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;

	sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC);

	if(sqlite3_step(stmt) == SQLITE_ROW){
		balance = (uint64_t)sqlite3_column_int64(stmt, 0);
	}

	sqlite3_finalize(stmt);
	return balance;
}


int update_balance(const char *address, int64_t delta){
	if(db == NULL) return 0;
	uint64_t current = get_balance(address);

	if(delta < 0 && current < (uint64_t)(-delta)){
		return 0;
	}

	uint64_t new_balance = current + delta;

	const char *sql = "INSERT OR REPLACE INTO balances (address, balance) VALUES (?, ?);";

	sqlite3_stmt *stmt = NULL;
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_text(stmt, 1, address, -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, (sqlite3_int64)new_balance);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 1;

}

int transfer(const char *from, const char *to, uint64_t amount){

	if(db == NULL) return 0;

	if(get_balance(from) < amount){
		return 0;
	}

	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

	if(!update_balance(from, -(int64_t)amount)){
		sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
		return 0;
	}

	if(!update_balance(to, (int64_t)amount)){
		sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
		return 0;
	}


	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
	return 1;
}


void close_balances(void){

};
