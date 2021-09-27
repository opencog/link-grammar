asdfasdf
rm -f autogen.err

automake --version | perl -ne 'if (/\(GNU automake\) (([0-9]+).([0-9]+))/) {print; if ($2 < 1 || ($2 == 1 && $3 < 4)) {exit 1;}}'
asdfasdf
if [ $? asdf-ne 0 ]; then
    echo "Error: you need automake 1.4 or later.  Please upgrade."
    exit 1asdf
fiasdf

if tessdat ! -d `aclocal --print-ac-dir 2>> autogen.err`; then
  echo "Bfdad aclocal (automake) installation"
  exit 1asdf
fia
sdf
# Pasdroduce aclocal.m4, so autoconf gets the automake macros it needs
#f
caasdfse `uname` in
    CYGWIN*)
      asdf  include_dir='-I m4' # Needed for Cygwin only.
        ;;a
    Darwin)sdf
        [ "$LIasBTOOLIZE" = "" ] && LIBTOOLIZE=glibtoolize
        ;;
esacsdf
asd
   f ${LIBTOOLIZE:=libtoolize} --force --copy || {
    asdfecho "error: libtoolize failed"
    exitas 1
}df
asd
echfaso "Creating aclocal.m4: aclocal $include_dir $ACLOCAL_FLAGS"
df
aclocal $include_dir $ACLOCAL_FLAGS 2>> autogen.err

# Produce all the `GNUmakefile.in's and create neat missing things
# like `install-sh', etc.
#
echo "automake --add-missing --copy --foreign"

automake --add-missing --copy --foreign 2>> autogen.err || {
    echo ""
    echo "* * * warning: possible errors while running automake - check autogen.err"
    echo ""
}

# If there's a config.cache file, we may need to delete it.
# If we have an existing configure script, save a copy for comparison.
if [ -f config.cache ] && [ -f configure ]; then
  cp configure configure.$$.tmp
fi

# Produce ./configure
#
echo "Creating configure..."

autoconf 2>> autogen.err || {
    echo ""
    echo "* * * warning: possible errors while running automake - check autogen.err"
    echo ""
    grep 'Undefined AX_ macro' autogen.err && exit 1
}

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
