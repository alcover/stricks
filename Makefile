CC = gcc
STD = c11
OPTIM = -O2
WARN =  -Wall -Wextra -Wno-pedantic -Wno-unused-function -Wno-unused-variable
CP = $(CC) -std=$(STD) $(WARN) $(OPTIM) -g
COMP = $(CP) -c $< -o $@
LINK = $(CP) $^ -o $@

lib		= bin/stx
check 	= bin/check
bench 	= bin/bench
benchcpp 	= bin/benchcpp
sds 	= bin/sds
example	= bin/example
try		= bin/try

.PHONY: all check clean bench benchcpp

bin = $(lib) $(check) $(bench) $(sds) $(example) $(try)

all: $(bin)
	
$(lib): src/stx.c src/stx.h src/util.c
	@ echo $@
	@ $(COMP) #-D ENABLE_LOG -D STX_WARNINGS
# 	@ ./$(check)

$(check): src/check.c $(lib) src/util.c
	@ echo $@
	@ $(CP) $< $(lib) -o $@
# 	@ ./$(check)

$(sds): bench/sds/sds.c bench/sds/sds.h
	@ echo $@
	@ $(CC) -std=c99 -Wall $(OPTIM) -c $< -o $@

$(bench): bench/bench.c $(lib) $(sds)
	@ echo $@
	@ $(CC) -std=$(STD) -O0 $(WARN) $^ -o $@ -lm

$(benchcpp): bench/bench.cpp $(lib) $(sds)
	@ echo $@
	@ g++ -std=c++11 -O2 -fpermissive -lbenchmark -lpthread $^ -o $@

$(example): ex/example.c $(lib)
	@ echo $@
	@ $(LINK)

$(try): ex/try.c $(lib)
	@ echo $@
	@ $(LINK) 

check:
	@ ./$(check)

bench:
	@ ./$(bench)

# benchcpp:
# 	@ sudo cpupower frequency-set --governor performance
# 	@ ./$(benchcpp)
# 	@ sudo cpupower frequency-set --governor powersave

benchcpp: $(benchcpp)
	@ ./$(benchcpp)

clean:
	@ rm -f $(bin) $(benchcpp)