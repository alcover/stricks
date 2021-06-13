#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../src/stx.h"
#include "../src/log.h"
#include "../src/util.c"
 
#define DB_PATH "assets/posts.csv"
#define PAGE_SZ 128
#define POST_FMT "%s \t (+%d) %s\n"

int pagen = 1;

void send(stx_t page) 
{ 
	printf ("--- PAGE %d (%zu/%d bytes) ---\n\n%s\n", pagen++, stx_len(page), PAGE_SZ, page);
}

int main()
{
	size_t filelen;
    char* db_str = load(DB_PATH, &filelen);
    stx_t db = stx_from(db_str);
	stx_t page = stx_new(PAGE_SZ);

	if (!db) {
        ERR ("Failed to load db file %s\n", DB_PATH);
        exit(EXIT_FAILURE);
    }
    
    const size_t db_len = stx_len(db);
    size_t nrows;
    stx_t *rows = stx_split(db, "\n", &nrows);
    
    LOG ("Welcome to Stricky's forum !");
    LOG ("db : %zu bytes, %d rows\n", db_len, nrows-1);

    for (size_t i = 0; i < nrows; ++i)
    {
        stx_t row = rows[i];

        if (!stx_len(row)) break;
        
        size_t ncols; 
        stx_t *columns = stx_split(row, ",", &ncols);
        
        int votes = atoi(columns[0]);
        stx_t user = columns[1];
        stx_t text = columns[2];

        int appended = stx_append_fmt_strict(page, POST_FMT, user, votes, text); 
        
       	// post too long, so flush page, reset and re-add post 
        if (appended <= 0) {
        	send(page);
        	stx_reset(page);
        	stx_append_fmt_strict (page, POST_FMT, user, votes, text);
        }
        
        stx_list_free(columns);
    }

	// page not empty, send it.
	if (stx_len(page)) send(page);

    stx_list_free(rows);
    stx_free(page);
    stx_free(db);
    free(db_str);
	
	return 0;
} 