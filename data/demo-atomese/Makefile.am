
DICTS=                           \
      cogserver.dict

dictdir=$(pkgdatadir)/demo-atomese
dict_DATA = $(DICTS)

EXTRA_DIST = $(DICTS)            \
      README.md

# The make uninstall target should remove directories we created.
uninstall-hook:
	-rmdir $(dictdir)
