
BUILDTYPE ?= Debug
BUILDDIR ?= build
V ?=

DEPSDIR = deps
LIBUV = $(DEPSDIR)/libuv/uv.gyp
HTTP_PARSER = $(DEPSDIR)/http-parser/http_parser.gyp
TOOLSDIR = tools
GYP = $(TOOLSDIR)/gyp/gyp

SOURCES := src/*.c src/*.h

.PHONY: all clean

all: $(BUILDDIR)/$(BUILDTYPE)/uttp

clean:
	$(RM) -rf $(BUILDDIR)

$(GYP):
	git clone https://chromium.googlesource.com/external/gyp.git $(TOOLSDIR)/gyp

$(LIBUV):
	git clone -b v1.x https://github.com/libuv/libuv $(DEPSDIR)/libuv

$(HTTP_PARSER):
	git clone https://github.com/joyent/http-parser $(DEPSDIR)/http-parser

$(BUILDDIR)/$(BUILDTYPE)/uttp: $(BUILDDIR)/Makefile $(SOURCES)
	$(MAKE) -C $(BUILDDIR) V=$(V)

$(BUILDDIR)/Makefile: $(GYP) $(LIBUV) $(HTTP_PARSER) common.gypi uttp.gyp
	$(GYP)	\
		-Duv_library=static_library \
		-Goutput_dir=. \
		-Icommon.gypi \
		-f make \
		--depth=. \
		--generator-output=$(BUILDDIR) \
		uttp.gyp
