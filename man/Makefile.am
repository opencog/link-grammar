## Process this file with automake to produce Makefile.in.

dist_man_MANS = link-parser.1

# Change the system dictionary location according the configuration options.
# This tracks --enable-binreloc, --bindir, and --datadir.
link-parser.1: link-parser.man1
	sed 's,%DATA_DIR%,$(datadir),;s,%BIN_DIR%,$(bindir),;s/%BINRELOC%/$(BINRELOC_CFLAGS)/' $< >$@
