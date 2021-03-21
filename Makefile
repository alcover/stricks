CC = gcc
STD = c11
OPTIM = -O0
WARN = -Wno-pedantic # -Wall -Wno-unused-function -Wno-unused-variable
CP = $(CC) -std=$(STD) $(OPTIM) $(WARN) -g
COMP = $(CP) -c $< -o $@
LINK = $(CP) $^ -o $@

lib		= bin/libstx
unit 	= bin/unit
test 	= bin/test
bench 	= bin/bench
sds 	= bin/sds
example	= example/forum

.PHONY: all check clean bench

all: $(lib) $(unit) $(bench) $(example) $(test)
	

$(lib): src/stx.c src/stx.h src/util.c
	@ echo $@
	@ $(COMP)

$(unit): src/unit.c $(lib) src/util.c
	@ echo $@
	@ $(CP) $< $(lib) -o $@

$(bench): src/bench.c $(lib) $(sds)
	@ echo $@
	@ $(LINK) -lm

$(example): example/forum.c $(lib)
	@ echo $@
	@ $(LINK)

$(test): src/test.c $(lib)
	@ echo $@
	@ $(LINK)

$(sds): sds/sds.c sds/sds.h
	@ echo $@
	@ $(CC) -std=c99 -Wall $(OPTIM) -c $< -o $@

check:
	@ ./$(unit)

bench:
	@ ./$(bench)

clean:
	@ rm -f bin/* $(example)