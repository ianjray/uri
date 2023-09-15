.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o .unittest .profraw .profdata .coverage

VERSION    = 1.0.0

CC         = clang
LD         = ld
XCRUN      = xcrun
CPROF      = $(XCRUN) llvm-profdata
CCOV       = $(XCRUN) llvm-cov

PREFIX     = /opt/local
LIBDIR     = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

SANITIZE   = -fsanitize=address -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
COVERAGE   = -fprofile-instr-generate -fcoverage-mapping
OPTS       = -fvisibility=hidden -Werror -Weverything -Wno-padded -Wno-poison-system-directories

.PHONY: all
all: liburi.a example.unittest uri.coverage

liburi.a: internal/charset.o internal/percent.o internal/str.o uri.o
	$(LD) -r $^ -o $@

.c.o:
	$(CC) $(OPTS) -c $^ -o $@

liburi.pc:
	printf 'prefix=%s\n' "$(PREFIX)" > $@
	printf 'exec_prefix=$${prefix}\n' >> $@
	printf 'includedir=$${prefix}/include\n' >> $@
	printf 'libdir=$${prefix}/lib\n' >> $@
	printf 'Name: liburi\n' >> $@
	printf 'Version: %s\n' "$(VERSION)" >> $@
	printf 'Description: URI normalizer library\n' >> $@

example.c: README.md
	awk '/```c/{ C=1; next } /```/{ C=0 } C' $< > $@

example.unittest: uri.c internal/charset.c internal/percent.c internal/str.c

uri.unittest: test_uri.c internal/charset.c internal/percent.c internal/str.c
uri.profdata: example.profraw

.profdata.coverage:
	$(CCOV) show $*.unittest -instr-profile=$< $*.c > $@
	! grep " 0|" $@ |grep -ve "// UNREACHABLE$$"
	echo PASS $@

.profraw.profdata:
	$(CPROF) merge -sparse $^ -o $@

.unittest.profraw:
	LLVM_PROFILE_FILE=$@ ./$<

.c.unittest:
	$(CC) $(SANITIZE) $(COVERAGE) $(OPTS) $^ -o $@

.PHONY: install
install: uri.h liburi.a liburi.pc
	mkdir -p $(INCLUDEDIR)/liburi
	mkdir -p $(LIBDIR)/pkgconfig
	install -m644 uri.h $(INCLUDEDIR)/liburi/uri.h
	install -m644 liburi.a $(LIBDIR)/liburi.a
	install -m644 liburi.pc $(LIBDIR)/pkgconfig/liburi.pc

.PHONY: uninstall
uninstall:
	rm -f $(INCLUDEDIR)/liburi/uri.h
	rm -f $(LIBDIR)/liburi.a
	rm -f $(LIBDIR)/pkgconfig/liburi.pc

.PHONY: clean
clean:
	rm -rf liburi.a liburi.pc *.o internal/*.o example* *.coverage *.profdata *.profraw *.unittest*
