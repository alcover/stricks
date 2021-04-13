#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../src/stx.h"
#include "../src/log.h"
#include "../src/util.c"

#define DB_PATH "posts.csv"
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
    unsigned int nrows;
    stx_t *rows = stx_split(db, db_len, "\n", &nrows);
    stx_t row;
    
    LOG ("Welcome to Stricky's forum !");
    LOG ("db : %zu bytes, %d rows\n", db_len, nrows-1);

    while ((row = *rows++)) {

        if (!stx_len(row)) break;
        
        unsigned int ncols; 
        //todo free
        stx_t *cols = stx_split(row, stx_len(row), ",", &ncols);
        
        int votes = atoi(cols[0]);
        stx_t user = cols[1];
        stx_t text = cols[2];
        
        int appended = stx_catf (page, POST_FMT, user, votes, text); 

       	// post too long, so flush page, reset and re-add post 
        if (appended <= 0) {
        	send(page);
        	stx_reset(page);
        	stx_catf (page, POST_FMT, user, votes, text);
        }
    }

	// page not empty, send it.
	if (stx_len(page)) send(page);

    stx_free(page);
	
	return 0;
} 