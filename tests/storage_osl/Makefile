APPLICATION = $(shell basename $(shell pwd))
include ../Makefile.tests_common

USEMODULE += embunit
USEMODULE += xtimer
ifneq (,$(filter msba2,$(BOARD)))
	USEMODULE += mci
endif

ifneq (,$(filter native,$(BOARD)))
	USEMODULE += flash_sim
endif
USEMODULE += osl

USEPKG += libfixmath

INCLUDES += -I$(shell pwd)
CFLAGS += -DDEVELHELP -g

include $(RIOTBASE)/Makefile.include
