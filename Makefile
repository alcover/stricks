CC = gcc
STD = c11
OPTIM = -O2
WARN =  -Wextra -Wno-pedantic -Wno-unused-function -Wno-unused-variable
CP = $(CC) -std=$(STD) $(WARN) $(OPTIM) -g
COMP = $(CP) -c $< -o $@
LINK = $(CP) $^ -o $@

lib		= bin/stx
unit 	= bin/unit
bench 	= bin/bench
example	= bin/example
try		= bin/try
sds 	= bin/sds

.PHONY: all check clean bench

all: $(lib) $(unit) $(bench) $(example) $(try)
	
$(lib): src/stx.c src/stx.h src/util.c
	@ echo $@
	@ $(COMP) #-D ENABLE_LOG -D STX_WARNINGS
# 	@ ./$(unit)

$(sds): sds/sds.c sds/sds.h
	@ echo $@
	@ $(CC) -std=c99 -Wall $(OPTIM) -c $< -o $@

$(unit): src/unit.c $(lib) src/util.c
	@ echo $@
	@ $(CP) $< $(lib) -o $@
# 	@ ./$(unit)

$(bench): src/bench.c $(lib) $(sds)
	@ echo $@
	@ $(CC) -std=$(STD) -O0 $(WARN) $^ -o $@ -lm

$(example): src/example.c $(lib)
	@ echo $@
	@ $(LINK)

$(try): src/try.c $(lib)
	@ echo $@
	@ $(LINK) 

check:
	@ ./$(unit)

bench:
	@ ./$(bench)

clean:
	@ rm -f bin/*