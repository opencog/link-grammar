"""
Run Link Grammar Python scripts using the build target locations.

This program sets PYTHONPATH and PATH, and uses Python2 or Python3
to run the script, as needed.

This program is designed to run from this directory
It reads Local.props in order to find Python's EXE location.
It also generates relative PYTHONPATH and PATH.
In case it is desired to move it to another directory, there is a need to
change the related variables.

The default script to run is tests.py from binding\python-examples.
in order to run the file example.py there, the following can be used:
> .\make-check.py  x64\Debug\Python2 ..\bindings\python-examples\example.py
"""
from __future__ import print_function
import os
import sys
import re

local_prop_file = 'Local.props' # In this directory
pyscript = r'..\bindings\python-examples\tests.py'
os.environ["LINK_GRAMMAR_DATA"] = r'..' # "data" in the parent directory

def error(msg):
    if msg:
        print(msg)
    prog = os.path.basename(sys.argv[0])
    print("Usage: ", prog, '[python_args] PYTHON_OUTDIR [script.py] [script_args]')
    print('\tOUTDIR is in the format "x64/Debug/Python2"')
    sys.exit(1)

def get_prop(vsfile, prop):
    """
    Get a macro definition from a .prop file. 
    I'm too lazy to leave the file open in case more properties are needed,
    or to read a list of properties in one scan. However, efficiency is not
    needed here.
    """
    vs_f = open(vsfile, 'r')
    var_re = re.compile('<'+prop+'>([^<]*)<')
    for line in vs_f:
        m = re.search(var_re, line)
        if m != None:
            return m.group(1)
    return None

#---

local_prop_file = \
  os.path.dirname(os.path.realpath(sys.argv[0]))+'\\'+ local_prop_file

#print('Running by:', sys.executable)

if len(sys.argv) < 2:
    error('Missing argument')

pyargs = ''
if len(sys.argv[1]) > 0 and sys.argv[1][0] == '-':
    pyargs = sys.argv.pop(1)

if len(sys.argv) < 2:
    error('Missing argument')

outdir = sys.argv.pop(1)
if not os.path.isdir(outdir):
    error('Directory "{}" doesn\'t exist'.format(outdir))

m = re.search(r'(.*)\\(.*)$', outdir)
if not m or len(m.groups()) != 2:
    error('Invalid output directory "{}"'.format(outdir))
config = m.group(1)
pydir = m.group(2).upper()
pyloc = get_prop(local_prop_file, pydir)
if pyloc == None:
    error('Python definition "{}" not found in {}' . \
          format(pyloc, local_prop_file))
pyexe = get_prop(local_prop_file, pydir+'_EXE')
if pyexe == None:
    error('Python executable definition "{}" not found in {}' . \
          format(pydir+'_EXE', local_prop_file))
pyexe = str.replace(pyexe, '$('+pydir+')', pyloc)

if len(sys.argv) == 2:
    pyscript = sys.argv[1]

args = ''
if len(sys.argv) >= 3:
    args = ' '.join(sys.argv[2:])

os.environ["PATH"] = ('{};' + os.environ["PATH"]).format(config)

# For linkgrammar.py, clinkgrammar.py and _clinkgrammar.pyd
os.environ["PYTHONPATH"] = r'..\bindings\python;{}'.format(outdir)
print("PYTHONPATH=" + os.environ["PYTHONPATH"])
#print("Searching modules in:\n" + '\n'.join(sys.path))

cmd = ' '.join((pyexe, pyargs, pyscript, args))
print('Issuing command:', cmd)
os.system(cmd)
