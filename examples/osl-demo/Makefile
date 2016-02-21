# name of your application
APPLICATION = benchmark-osl

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

ifneq (,$(filter native,$(BOARD)))
	USEMODULE += flash_sim
endif

ifneq (,$(filter msba2,$(BOARD)))
	USEMODULE += mci
endif

USEMODULE += osl
USEMODULE += xtimer
USEMODULE += timex

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
