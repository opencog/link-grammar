
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

static int callback(void *user_data, int argc, char **argv, char **colname)
{
	int i;
	for (i=0; i<argc; i++)
	{
		printf("%s = %s\n", colname[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int main(int argc, char **argv)
{
	int rc;
	sqlite3 *db;
	char *errmsg;
	char *qry;

	const char *dbname = "/home/linas/src/novamente/src/link-grammar/trunk/data/sql/disjuncts.db";

printf ("duuude dct is %s\n", DICTIONARY_DIR);
	rc = sqlite3_open(dbname, &db);
	if (rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	qry = "SELECT log_cond_probability FROM Disjuncts WHERE inflected_word = 'suffers.v' AND disjunct = 'Ss- Os+ MVp+ ';";

	rc = sqlite3_exec(db, qry, callback, 0, &errmsg);
	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
	}
	sqlite3_close(db);

}
