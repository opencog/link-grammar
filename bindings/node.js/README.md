# Node.js Bindings for Link Grammar

## Building the bindings.
Build Link Grammar first. Then, in this directory, say:
```
       npm install
       npm run make
```
## Using
The parser is built using the awesome ffi library which allows
us to communicate with a native library under the covers.
Along with ffi, we also use ref library to reference the native
objects used by the library.

    ffi = require 'ffi'
    ref = require 'ref'
    Struct = require 'ref-struct'
    _ = require 'underscore'

Easier references to base types of data.

    pointerType = 'pointer'
    string = ref.types.CString
    int = ref.types.int

These are just references to different native structs, which are
all just pointers because we never use their actual referenced object.

    ParseOptions = pointerType
    Dictionary = pointerType
    Sentence = pointerType
    Linkage = pointerType

Here are the templates for the native functions we will use.

    apiTemplate =
        parse_options_create: [ ParseOptions, [ ] ]
        parse_options_set_islands_ok: [ ref.types.void, [ ParseOptions, int ] ]
        parse_options_set_verbosity: [ ref.types.void, [ ParseOptions, int ] ]
        parse_options_set_min_null_count: [ ref.types.void, [ ParseOptions, int ] ]
        parse_options_set_max_null_count: [ ref.types.void, [ ParseOptions, int ] ]
        dictionary_create_lang: [ Dictionary, [ string ] ]
        sentence_create: [ Sentence, [ string, Dictionary ] ]
        sentence_parse: [ int, [ Sentence, ParseOptions ] ]
        sentence_length: [ int, [ Sentence ] ]
        linkage_create: [ Linkage, [ int, Sentence, ParseOptions ] ]
        linkage_print_diagram: [ string, [ Linkage ] ]
        linkage_get_num_links: [ int, [ Linkage ] ]
        linkage_get_link_label: [ string, [ Linkage, int ] ]
        linkage_get_link_llabel: [ string, [ Linkage, int ] ]
        linkage_get_link_rlabel: [ string, [ Linkage, int ] ]
        linkage_get_word: [ string, [ Linkage, int ] ]
        linkage_get_link_lword: [ int, [ Linkage, int ] ]
        linkage_get_link_rword: [ int, [ Linkage, int ] ]

Load the library.

    libPath = __dirname + '/../../../build/link-grammar/.libs/liblink-grammar.so'
    lib = ffi.Library libPath, apiTemplate
    defaultDataPath = __dirname + '/../../../data/en/'

Default configuration for data paths.

    defaultConfig =
        lang: 'en'
        verbose: false

Main parser class which interfaces the native library to make
it very simple to get Link Grammar data from an input string.

    class LinkGrammar

A few utility methods for the parser.

        constructor: (config) ->
            @config = _.extend config or {}, defaultConfig
            @options = lib.parse_options_create()
            lib.parse_options_set_verbosity @options, (if @config.verbose then 1  else 0)
            lib.parse_options_set_min_null_count @options, 0
            lib.parse_options_set_max_null_count @options, 3
            @dictionary = lib.dictionary_create_lang @config.lang

Parse input, and return linkage, if it exists.

        parse: (input, index) ->
            sentence = lib.sentence_create input, @dictionary
            numLinkages = lib.sentence_parse sentence, @options
            if numLinkages > 0
              new Linkage lib.linkage_create index or 0, sentence, @options
            else
              throw new Error('No linkages found')

Linkage class which allows for more specific parsing of grammar.

    class Linkage

        constructor: (@linkage) ->
            @links = @getLinks()
            @words = @getWords()

Get array of grammar links based on linkage.

        getLinks: ->
            _(lib.linkage_get_num_links @linkage).times ((index) ->
                leftIndex = lib.linkage_get_link_lword @linkage, index
                rightIndex = lib.linkage_get_link_rword @linkage, index
                link =
                    label: lib.linkage_get_link_label @linkage, index
                    left:
                        label: lib.linkage_get_link_llabel @linkage, index
                        word: lib.linkage_get_word @linkage, leftIndex
                        index: leftIndex
                    right:
                        label: lib.linkage_get_link_rlabel @linkage, index
                        word: lib.linkage_get_word @linkage, rightIndex
                        index: rightIndex
                if link.left.word.indexOf '.' isnt -1
                    temp = link.left.word.split '.'
                    link.left.word = temp[0]
                    link.left.type = temp[1]
                if link.right.word.indexOf '.' isnt -1
                    temp = link.right.word.split '.'
                    link.right.word = temp[0]
                    link.right.type = temp[1]
                link
            ), @

Get parsed words with their grammar type and index

        getWords: ->
            _.chain(@links)
                .map (link) -> [link.left, link.right]
                .flatten()
                .uniq (link) -> link.word
                .value()

Get list of links by specific label type

        linksByLabel: (labelPattern) ->
            labelPattern = new RegExp labelPattern if typeof labelPattern is 'string'
            @links.filter (link) ->
                labelPattern.test link.left.label or labelPattern.test link.right.label

Get list of connector links for a specific word

        getConnectorWords: (common) ->
            words = []
            @links.forEach (link) ->
                if link.left.word is common
                    words.push
                        source: link.left
                        target: link.right
                if link.right.word is common
                    words.push
                        source: link.right
                        target: link.left
            words

    module.exports = LinkGrammar
