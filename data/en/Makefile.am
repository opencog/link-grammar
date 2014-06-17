SUBDIRS=words

DICTS=					         \
	4.0.affix                  \
	4.0.constituent-knowledge  \
	4.0.dict                   \
	4.0.knowledge              \
	4.0.regex

dictdir=$(pkgdatadir)/en
dict_DATA = $(DICTS)

# Do NOT install the batch files!
EXTRA_DIST = $(DICTS)         \
	4.0.dict.m4                \
	4.0.batch                  \
	4.0.biolg.batch            \
	4.0.fixes.batch            \
	4.0.fix-long.batch         \
	4.0.voa.batch              \
	tiny.dict
