# alco version

all: sds sds-test

sds: sds.c sds.h
	$(CC) -o $@ -c sds.c -Wall -std=c99 -pedantic -O2

sds-test: sds.c sds.h testhelp.h
	$(CC) -o sds-test sds.c -Wall -std=c99 -pedantic -O2 -DSDS_TEST_MAIN
	@echo ">>> Type ./sds-test to run the sds.c unit tests."

clean:
	rm -f sds sds-test
