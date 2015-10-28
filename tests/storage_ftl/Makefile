APPLICATION = $(shell basename $(shell pwd))
include ../Makefile.tests_common

USEMODULE += embunit
USEMODULE += xtimer
USEMODULE += flash_sim
USEMODULE += ftl

USEPKG += libfixmath

INCLUDES += -I$(shell pwd)
CFLAGS += -DDEVELHELP -g

include $(RIOTBASE)/Makefile.include
