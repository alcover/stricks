#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "stx.h"
#include "log.h"
#include "util.c"

#define x4(s) s s s s

#define s8 "[8.....]"
#define s32 x4(s8)
#define s256 x4(s32) x4(s32)



int main() {

stx_t s = stx_new(8);
((char*)s)[0] = 'q';
stx_show(s);


stx_t f = stx_load("/tmp/toto");
stx_show(f);


#define PAGE_SZ 128
#define POST_FMT "■ %s (%d points) %s\n"

stx_t page = stx_new(PAGE_SZ);

int votes;
char user[20];
char text[100];
char row[100] = "1,Paul,First post!";
// parse row
int match = sscanf (row, "%d,%[^,],%[^\n]", &votes, user, text);
LOGVS(row);
LOGVI(votes);
LOGVS(user);
LOGVS(text);

// format and append row
int appended = stx_catf (page, POST_FMT, user, votes, text); 
LOGVI(appended);
stx_show(page);

// ■ Paul (1 points) First post!


	// stx_t s;

	// s = stx_from("abc", 0);
	// stx_show(s);

	// s = stx_from(s256, 0);
	// stx_show(s);

	// const char* text = rand_str(1<<8);
	// unsigned int l=0;
	// stx_t* list = stx_split(text, "a", &l);
	
	// LOG(text); puts("\n");
	// for (int i = 0; i < l; ++i)
	// 	printf("%s\n", list[i]);

    return 0;
}