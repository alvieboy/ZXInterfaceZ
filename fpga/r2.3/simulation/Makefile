GHDLMAKEFLAGS=-m --ieee=synopsys -fexplicit --std=00 -frelaxed -Wno-binding

TESTS=t001 \
t002 \
t004 \
t005 \
t007 \
t008 \
t010 \
t012 \
t013 \
t014 \
t017 \
t018

LOCALDESIGNFILES= \
	zxinterfacepkg.vhd \
	interfacez_top.vhd

LOCALSIMFILES=tb_top.vhd


include ../../simulation/Makefile.inc

all: run

run:	$(TESTS)
	for test in $(TESTS); do  \
		echo $$test; \
		sh run.sh $$test 2>&1 | tee log.$$test | tail -n 1 ;  \
	done

clean:
	rm -f $(TESTS)

$(TESTS):
	ghdl $(GHDLMAKEFLAGS) $@
