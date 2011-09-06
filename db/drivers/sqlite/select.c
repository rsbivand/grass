
/**
 * \file select.c
 *
 * \brief Low level SQLite database functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author Support for multiple connections by Markus Metz
 *
 * \date 2005-2011
 */

#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include "globals.h"
#include "proto.h"


/**
 * \fn int db__driver_open_select_cursor (dbString *sel, dbCursor *dbc, int mode)
 *
 * \brief Open SQLite cursor for select.
 *
 * \param[in] sel
 * \param[in,out] dbc
 * \param[in] mode
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_open_select_cursor(dbString * sel, dbCursor * dbc, int mode)
{
    cursor *c;
    dbTable *table;
    char *str;
    const char *rest;
    int ret;

    init_error();

    /* allocate cursor */
    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    db_set_cursor_mode(dbc, mode);
    db_set_cursor_type_readonly(dbc);

    /* \ must be escaped, see explanation in db_driver_execute_immediate() */
    str = G_str_replace(db_get_string(sel), "\\", "\\\\");
    G_debug(3, "Escaped SQL: %s", str);

    ret = sqlite3_prepare(sqlite, str, -1, &(c->statement), &rest);
    while (ret == SQLITE_BUSY || ret == SQLITE_IOERR_BLOCKED) {
	ret = sqlite3_busy_handler(sqlite, sqlite_busy_callback, NULL);
    }

    if (str)
	G_free(str);

    if (ret != SQLITE_OK) {
	append_error("Error in sqlite3_prepare():");
	append_error(db_get_string(sel));
	append_error("\n");
	append_error((char *)sqlite3_errmsg(sqlite));
	report_error();
	return DB_FAILED;
    }

    if (describe_table(c->statement, &table, c) == DB_FAILED) {
	append_error("Cannot describe table\n");
	report_error();
	return DB_FAILED;
    }

    c->nrows = -1;
    c->row = -1;

    /* record table with dbCursor */
    db_set_cursor_table(dbc, table);

    /* set dbCursor's token for my cursor */
    db_set_cursor_token(dbc, c->token);

    return DB_OK;
}
