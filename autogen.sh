#!/bin/sh
#
# Run this before configure
#

rm -f autogen.err

# For version x.y>1.4, x should not be 0, and y should be [4-9] or more than one digit.
automake --version >/dev/null &&
   automake --version | test "`sed -En '/^automake \(GNU automake\) [^0]\.([4-9]|[1-9][0-9])/p'`"
if [ $? -ne 0 ]; then
    echo "Error: you need automake 1.4 or later.  Please upgrade."
    exit 1
fi

# If there's a config.cache file, we may need to delete it.
# If we have an existing configure script, save a copy for comparison.
# (Based on Subversion's autogen.sh, see also the deletion code below.)
OLD_CONFIGURE="${TMPDIR:-/tmp}/configure.$$.tmp"
if [ -f config.cache ] && [ -f configure ]; then
  cp configure "$OLD_CONFIGURE"
fi

echo "Creating configure..."

# Regenerate configuration scripts with the latest autotools updates.
autoreconf -fvi 2>autogen.err
status=$?
if [ $status -ne 0 ]; then
    echo ""
    echo "* * * Warning: autoreconf returned bad status ($status) - check autogen.err"
    echo ""
    exit 1
fi

# If we have a config.cache file, toss it if the configure script has
# changed, or if we just built it for the first time.
if [ -f config.cache ]; then
  (
    [ -f "$OLD_CONFIGURE" ] && cmp configure "$OLD_CONFIGURE" > /dev/null 2>&1
  ) || (
    echo "Tossing config.cache, since configure has changed."
    rm config.cache
  )
  rm -f "$OLD_CONFIGURE"
fi

run_configure=true
for arg in $*; do
    case $arg in
        --no-configure)
            run_configure=false
            ;;
        *)
            ;;
    esac
done

if $run_configure; then
    mkdir -p build
    cd build
    echo
    echo "Running 'configure'..."
    ../configure --enable-maintainer-mode "$@"
    status=$?
    if [ $status -eq 0 ]; then
      echo
      echo "Now type 'make' to compile link-grammar (in the 'build' directory)."
    else
      echo
      echo "\"configure\" returned a bad status ($status)."
    fi
else
    echo
    echo "Now run 'configure' and 'make' to compile link-grammar."
fi
