# Link Grammar Vala Bindings

If you run into an issue, please tag @kmwallio or [create an issue here](https://github.com/kmwallio/link-grammar-vapi) for direct support related to the Vala Bindings.

## Usage

Place link-grammar.vapi into your project's Vapi directory and add it to your build.

For meson, it would look like:

```meson
# Find link-grammar dependency
cc = meson.get_compiler('c')
linkgrammar = cc.find_library('link-grammar', required: true)

# Other build steps...

executable(
    meson.project_name(),
    '...source_files...',
    c_args: c_args,
     dependencies: [
        ...other dependencies...,
        linkgrammar
    ],
    vala_args: [
        ...other Vala args...,
        meson.source_root() + '/vapi/link-grammar.vapi'
    ],
    install : true/false
)
```

# Example Usage

This is based on the [Simple Example for C](https://www.abisource.com/projects/link-grammar/api/index.html#example1).

```vala
public static int main (string[] args) {
    print ("# Link Grammar Sample\n");
    string[] input_strings = {
        "He eats cake.",
        "He eat cake.",
        "The side affects were devastating.",
        "The side effects were devastating."
    };

    var opts = new ParseOptions ();
    var dict = new Dictionary ("en");

    // Allow for null links.
    opts.set_max_null_count (10);

    if (dict == null) {
        warning ("Could not load dictionary");
        return 1;
    }

    for (int i = 0; i < input_strings.length; i++) {
        var sentence = new Sentence (input_strings[i], dict);
        sentence.split (opts);
        var num_linkages = sentence.parse (opts);
        if (num_linkages > 0) {
            var linkage = new Linkage (0, sentence, opts);
            var diagram = linkage.print_diagram (true, 800);
            print ("%s\n", diagram);
        }
    }

    return 0;
}
```

## Sample Output

```
# Link Grammar Sample
link-grammar: Info: Dictionary found at /usr/share/link-grammar/en/4.0.dict

    +-----------Xp-----------+
    +---->WV--->+            |
    +->Wd--+-Ss-+---Ou--+    +--RW--+
    |      |    |       |    |      |
LEFT-WALL he eats.v cake.n-u . RIGHT-WALL



    +------------Xp-----------+
    +---->Wi-----+---Ou--+    +--RW--+
    |            |       |    |      |
LEFT-WALL [he] eat.v cake.n-u . RIGHT-WALL



    +-----------------------Xp----------------------+
    +-------->WV-------->+                          |
    +---->Wd-----+       |                          |
    |      +Ds**c+--Ss*s-+--------Os--------+       +--RW--+
    |      |     |       |                  |       |      |
LEFT-WALL the side.n affects.v [were] devastating.g . RIGHT-WALL



    +------------------------Xp-----------------------+
    +------------->WV------------->+                  |
    +-------->Wd---------+         |                  |
    |      +-----Dmc-----+         |                  |
    |      |     +---AN--+---Spx---+----Ost---+       +--RW--+
    |      |     |       |         |          |       |      |
LEFT-WALL the side.n effects.n were.v-d devastating.g . RIGHT-WALL
```
