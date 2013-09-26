
# Link Grammar Parser

A node library which interfaces a well known link grammar native [library](<http://www.link.cs.cmu.edu/link/>)

The point of this project is to make the library easier to use, especially in node!

## Building Yourself

You will need [mocha](https://npmjs.org/package/mocha) and [kal](https://npmjs.org/package/kal) installed **globally** to build this project yourself.

After those are installed, you can just run:

```
npm run-script make
```

Which will clean, compile, and test the project.

## Usage

```

/**
 * Load in the link grammar parser library
 */
Parser = require("link-grammar");

/* Create a parser */
parser = new Parser();

/**
 * Gets grammar links
 * @param {string} input - input sentence to parse
 * @returns {array} - array of linkages which each contain grammar links
 */
links = parser.getLinks("my name is sam");

/**
 * Gets constituent tree
 * @param {string} input - input sentence to parse
 * @returns {object} - root node of tree, each node contains a label and which nodes it links to
 */
root = parser.getTree("the dog ate my homework");

```

Link Grammar [Documentation](<http://www.link.cs.cmu.edu/link/dict/>)