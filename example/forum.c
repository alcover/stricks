/*
    Example usage.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../src/stx.h"
#include "../src/log.h"

#define DB "posts.csv"
#define PAGE_SZ 128
#define POST_FMT "■ %s (%d points) %s\n"

int pagenum = 1;

void send(stx_t page) 
{ 
	printf ("---- PAGE %d [%zu/%d bytes] ----\n\n%s\n", 
        pagenum++, stx_len(page), PAGE_SZ, page); 
}

int main()
{
	stx_t content = stx_load(DB);

	if (!content) {
        ERR ("Failed to load db file %s\n", DB);
        exit(EXIT_FAILURE);
    }
    
    size_t content_len = stx_len(content);
    LOG ("content length : %zu", content_len);

    int nrows;
    stx_t *rows = stx_split(content, content_len, "\n", &nrows);
    LOG ("rows count : %d", nrows);
    puts("");

    stx_t row;

    while ((row = *rows++)) {

        if (!stx_len(row)) continue;
        LOGVS(row);
        int ncols;
        stx_t *cols = stx_split(row, stx_len(row), ",", &ncols);
        int votes = atoi(cols[0]);
        stx_t user = cols[1];
        stx_t text = cols[2];
        LOGVI(votes);
        LOGVS(user);
        LOGVS(text);
        puts("");
    }


	// stx_t page = stx_new(PAGE_SZ);
	// char row[100];

 //    puts ("Welcome to Stricky's forum !\n");
   
 //    while (fgets(row, sizeof row, db) != NULL) 
 //    {
 //    	int votes;
 //    	char user[20];
 //        char text[100];

 //        // parse row
 //        int match = sscanf (row, "%d,%[^,],%[^\n]", &votes, user, text);
 //        LOGVS(text);

 //        // format and append row
 //        int appended = stx_catf (page, POST_FMT, user, votes, text); 
 //        LOGVI(appended);
 //        stx_show(page);

 //        if (appended <= 0) {
 //        	// post won't fit, so flush page 
 //        	send(page);
 //        	// and re-add post.
 //        	stx_reset(page);
 //        	stx_catf (page, POST_FMT, user, votes, text);
 //        }
 //    }

	// // page not empty, send it.
	// if (stx_len(page)) send(page);

 //    stx_free(page);
	
	return 0;
} 