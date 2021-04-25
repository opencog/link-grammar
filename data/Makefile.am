SUBDIRS=ady amy any gen ar de en fa he id kz lt ru tr vn demo-sql

dictdir=$(pkgdatadir)
dict_DATA=command-help-en.txt

# The make uninstall target should remove directories we created.
uninstall-hook:
	-rmdir $(pkgdatadir)

# Include the README in the tarball, but do not install it.
EXTRA_DIST= README $(dict_DATA)
