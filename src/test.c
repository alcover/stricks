/*
	Testing snippets
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#define STX_SHORT_NAMES
#include "stx.h"

#define PAGE_SZ 128

int main() {

/**** README ******************************************************/
{	
    stx_t page = stx_new(PAGE_SZ);

    while(1) {

        char* user = "user";
        char* text = "text";

        if (0 >= stx_catf (page, "<div>%s<br>%s</div>\n", user, text))
            break;
    }
        
    printf("%s\n", page);
}

{
	stx_t s = stx_new(3);
	int rc = stx_cat(s, "foobar"); // -> -6
	if (rc<0) stx_resize(&s, -rc);
	stx_cat(s, "foobar");
	stx_show(s); 
}





    return 0;
}