TESTS=t001 \
t002 \
t004 \
t005 \
t006 \
t007 \



LOCALDESIGNFILES= \
	zxinterfacepkg.vhd \
	interfacez_top.vhd

LOCALSIMFILES=tb_top.vhd

include ../../simulation/Makefile.inc

run:
	for test in $(TESTS); do  \
		sh run.sh $test;    \
	done
	