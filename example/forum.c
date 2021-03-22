#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../src/stx.h"
#include "../src/log.h"

#define DB_PATH "posts.csv"
#define PAGE_SZ 128
#define POST_FMT "%s (+%d) %s\n"

int pagen = 1;

void send(stx_t page) 
{ 
	printf ("--- PAGE %d (%zu/%zu bytes) ---\n\n%s\n", pagen++, stx_len(page), PAGE_SZ, page); 
}

int main()
{
	stx_t page = stx_new(PAGE_SZ);
	stx_t db_text = stx_load(DB_PATH);
    // LOG(db_text);

	if (!db_text) {
        ERR ("Failed to load db file %s\n", DB_PATH);
        exit(EXIT_FAILURE);
    }
    
    const size_t db_len = stx_len(db_text);
    int nrows;
    stx_t *rows = stx_split(db_text, db_len, "\n", &nrows);
    
    LOG ("Welcome to Stricky's forum !");
    LOG ("db : %zu bytes, rows : %d\n", db_len, nrows);

    // puts("");

    stx_t row;

    while ((row = *rows++)) {

        if (!stx_len(row)) break;
        // LOGVS(row);
        int ncols; 
        //free?
        stx_t *cols = stx_split(row, stx_len(row), ",", &ncols);
        
        int votes = atoi(cols[0]);
        stx_t user = cols[1];
        stx_t text = cols[2];
        
        // LOGVI(votes);
        // LOGVS(user);
        // LOGVS(text);

        // format and append row
        int appended = stx_catf (page, POST_FMT, user, votes, text); 
        // LOGVI(appended);
        // stx_show(page);

        if (appended <= 0) {
            // LOG("cut");
        	// post won't fit, so flush page 
        	send(page);
        	// and re-add post.
        	stx_reset(page);
            // stx_show(page);
        	stx_catf (page, POST_FMT, user, votes, text);
        }
        // puts("");
    }

	// page not empty, send it.
	if (stx_len(page)) send(page);

    stx_free(page);
	
	return 0;
} 