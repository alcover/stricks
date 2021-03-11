/*
    Example usage.

    We read rows from a db and append them formatted to the page.
    When the page can't accept the next row, we send it 
    and begin filling a new page.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../src/stx.h"

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
	FILE* db = fopen(DB, "r");

	if (!db) {
        fprintf(stderr, "db file %s not found\n", DB);
        exit(EXIT_FAILURE);
    }

	stx_t page = stx_new(PAGE_SZ);
	char row[100];

    puts ("Welcome to Stricky's forum !\n");
   
    while (fgets(row, sizeof row, db) != NULL) 
    {
    	int votes;
    	char user[20];
        char text[100];

        // parse row
        sscanf (row, "%d,%[^,],%[^\n]", &votes, user, text);

        // format and append row
        int appended = stx_catf (page, POST_FMT, user, votes, text);

        if (appended < 0) {
        	// post won't fit, so flush page 
        	send(page);
        	// and re-add post.
        	stx_reset(page);
        	stx_catf (page, POST_FMT, user, votes, text);
        }
    }

	// page not empty, send it.
	if (stx_len(page)) send(page);

    stx_free(page);
	
	return 0;
}