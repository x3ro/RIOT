APPLICATION = $(shell basename $(shell pwd))
include ../Makefile.tests_common

USEPKG += coffee
USEMODULE += embunit
USEMODULE += xtimer

INCLUDES += -I$(shell pwd)
CFLAGS += -DDEVELHELP -g

include $(RIOTBASE)/Makefile.include
