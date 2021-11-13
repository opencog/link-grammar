Installing an optional external regex package
=============================================
The default configuration uses the C++ regex library. An external regex
library can be used instead, but this option only exists for special needs and
you can ignore it (and the rest of this file).

Package implementation selection
--------------------------------
The type of regex implementation to be used in the build is defined in the
User Macro `DEFS` in the `LGlib-features` property:

 Implementation | Definition | Comment |
---|---|---|
C++             | `USE_CXXREGEX` | Default [*]|
PCRE2           | `HAVE_PCRE`    | Faster and more sophisticated [**] |
POSIX interface | `HAVE_REGEX_H` | Only desirable if significantly faster |

Exactly one of these definitions must be used.

[*]  The C++ Implementation is configured by default.<br>
[**] The `link-grammar` package don't use PCRE2-specific features.

Setup
-----
To locate the components of the optional external regex package,
the build files use directory locations as defined in the property
sheet "Local" (separated by `;` from other possible definitions there).

-  Add the library name to **Linker->Input->Additional Dependencies**.<br>
   Example: `pcre2-8.lib`. **It is not defined by default**.<br>
   If the regex library is not added, the rest of the definitions here are
   practically ignored.

-  The User Macro `GNUREGEX_DIR` defines the directory of the optional regex
   package.<br>
   Default: `%HOMEDRIVE%%HOMEPATH%\Libraries\gnuregex`.

-  Add the directory of `regex.h` to
   **C/C++->General->Additional Include Directories**.<br>
   Default: `$(GNUREGEX_DIR)\include`.

-  Add the library directory to
   **Linker->General->Additional Library Directories**.<br>
   Default: `$(GNUREGEX_DIR)\lib`.

-  Unless the directory of the regex DLL is in your `PATH`, put it in the
   User Macro `LG_DLLPATH`.<br>
   The default is `$(GNUREGEX_DIR)\lib`.
