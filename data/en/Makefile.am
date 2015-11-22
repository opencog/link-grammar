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
	corpus-dict.m4             \
	corpus-basic.batch         \
	corpus-biolg.batch         \
	corpus-fixes.batch         \
	corpus-fix-long.batch      \
	corpus-voa.batch           \
	tiny.dict
