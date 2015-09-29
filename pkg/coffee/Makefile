PKG_NAME=coffee
PKG_URL=https://github.com/x3ro/coffee-fs
PKG_VERSION=20253eb1214712a07ccc40d8bdc3ace094e8f9b0
PKG_DIR=$(CURDIR)/$(PKG_NAME)

ifneq ($(RIOTBOARD),)
include $(RIOTBOARD)/$(BOARD)/Makefile.include
endif

.PHONY: all clean

all: $(PKG_DIR)/.git/config
	"$(MAKE)" -C $(PKG_DIR)

$(PKG_DIR)/.git/config:
	test -d "$(PKG_DIR)" || git clone "$(PKG_URL)" "$(PKG_DIR)"; \
		cd "$(PKG_DIR)" && git checkout -f "$(PKG_VERSION)";

clean::
	@echo "Cleaning up $(PKG_NAME) package..."
	@cd "$(PKG_DIR)" 2> /dev/null > /dev/null && \
		git clean -x -f && \
		git am --abort && \
		git reset --hard "$(PKG_VERSION)" && \
		$(MAKE) patch || true

distclean::
	rm -rf "$(PKG_DIR)"

Makefile.include:
	@true
