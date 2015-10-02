APPLICATION = $(shell basename $(shell pwd))
include ../Makefile.tests_common

BOARD ?= msba2

USEPKG += coffee
USEMODULE += embunit
USEMODULE += xtimer
USEMODULE += coffee_mci
USEMODULE += mci
USEMODULE += fs_util

INCLUDES += -I$(shell pwd)
CFLAGS += -DDEVELHELP -g

include $(RIOTBASE)/Makefile.include
