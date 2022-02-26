SUBDIRS=words

DICTS=                         \
   4.0.affix                   \
   4.0.constituent-knowledge   \
   4.0.dialect                 \
   4.0.dict                    \
   4.0.knowledge               \
   4.0.regex

dictdir=$(pkgdatadir)/th
dict_DATA = $(DICTS)

# Do NOT install the batch files!
EXTRA_DIST = README.md $(DICTS)     \
   corpus-basic.batch LINKDOC.md \
   pos_hierarchy.txt

# The make uninstall target should remove directories we created.
uninstall-hook:
	-rmdir $(dictdir)/words
	-rmdir $(dictdir)
