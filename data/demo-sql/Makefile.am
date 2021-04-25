
DICTS=                           \
      dict.db                    \
      4.0.affix                  \
      4.0.constituent-knowledge  \
      4.0.knowledge              \
      4.0.regex

dictdir=$(pkgdatadir)/demo-sql
dict_DATA = $(DICTS)

EXTRA_DIST = $(DICTS)            \
      corpus-basic.batch         \
      README.md

# The make uninstall target should remove directories we created.
uninstall-hook:
	-rmdir $(dictdir)
