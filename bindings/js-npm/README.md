
# Link Grammar Parser

A node library which interfaces a well known link grammar native [library](<http://www.link.cs.cmu.edu/link/>)

The point of this project is to make the library easier to use, especially in node!

## Building Yourself

	npm install
	npm run make

Which will install, clean, compile, and test the project.

# Documentation

*Review list of Grammar Links [here](<http://www.link.cs.cmu.edu/link/dict/>)*.

# 

The parser is built using the awesome ffi library which allows us to communicate with a native library under the covers.
Along with ffi, we also use ref library to reference the native objects used by the library.

    ffi = require 'ffi'
    ref = require 'ref'
    Struct = require 'ref-struct'
    _ = require 'underscore'

Easier references to base types of data.

    pointerType = 'pointer'
    string = ref.types.CString
    int = ref.types.int

These are just references to different native structs, which are all just pointers because we never use their actual referenced object.

    ParseOptions = pointerType 
    Dictionary = pointerType
    Sentence = pointerType
    Linkage = pointerType

    CNode = Struct(
    	label: ref.types.CString
    	child: pointerType
    	next: pointerType
    	start: ref.types.int
    	end: ref.types.int
    )

    CNodePtr = ref.refType CNode

Here are the templates for the native functions we will use.

    apiTemplate =
        parse_options_create: [ ParseOptions, [ ] ]
        parse_options_set_verbosity: [ ref.types.void, [ ParseOptions, int ] ]
        dictionary_create: [ Dictionary, [ string, string, string, string ] ]
        sentence_create: [ Sentence, [ string, Dictionary ] ]
        sentence_parse: [ int, [ Sentence, ParseOptions ] ]
        sentence_length: [ int, [ Sentence ] ]
        sentence_get_word: [ string, [ Sentence, int ] ]
        linkage_create: [ Linkage, [ int, Sentence, ParseOptions ] ]
        linkage_print_diagram: [ string, [ Linkage ] ]
        linkage_constituent_tree: [ CNodePtr, [ Linkage ] ]
        linkage_print_constituent_tree: [ string, [ Linkage, int] ]
        linkage_get_num_links: [ int, [ Linkage ] ]
        linkage_get_link_label: [ string, [ Linkage, int ] ]
        linkage_get_link_llabel: [ string, [ Linkage, int ] ]
        linkage_get_link_rlabel: [ string, [ Linkage, int ] ]
        linkage_get_word: [ string, [ Linkage, int ] ]
        linkage_get_link_lword: [ int, [ Linkage, int ] ]
        linkage_get_link_rword: [ int, [ Linkage, int ] ]

Load the library.

    libPath = __dirname + '/../lib/libparser'
    lib = ffi.Library libPath, apiTemplate
    defaultDataPath = __dirname + '/../data/'

Utility functions...

    getNodePtrFromPtr = (ptr) ->
    	tempPtr = ref.alloc CNodePtr
    	ref.writePointer tempPtr, 0, ptr
    	tempPtr.deref()

Default configuration for data paths.

    defaultConfig =
        dictPath: defaultDataPath + '4.0.dict'
        ppPath: defaultDataPath + '4.0.knowledge'
        consPath: defaultDataPath + '4.0.constituent-knowledge'
        affixPath: defaultDataPath + '4.0.affix'
        verbose: false

Main parser class which interfaces the native library to make it very simple to get link grammar data from an input string.

    class LinkGrammar
    	
A few utility methods for the parser.

        constructor: (config) ->
            @config = _.extend config or {}, defaultConfig
            @options = lib.parse_options_create()
            lib.parse_options_set_verbosity @options, (if @config.verbose then 1  else 0)
            @dictionary = lib.dictionary_create @config.dictPath, @config.ppPath, @config.consPath, @config.affixPath

Parse input, and return linkage.

        parse: (input, index) ->
            sentence = lib.sentence_create input, @dictionary
            numLinkages = lib.sentence_parse sentence, @options 
            new Linkage lib.linkage_create index or 0, sentence, @options

Linkage class which allows for more specific parsing of grammar.

    class Linkage

        constructor: (@linkage) ->
            @links = @getLinks()
            @tree = @getTree()
            @words = @getWords()

Recursive tree builder method.

        buildNode: (node) ->
            n =
                label: node.label
            if not ref.isNull node.child
                n.child = @buildNode getNodePtrFromPtr(node.child).deref()
            if not ref.isNull node.next
                n.next = @buildNode getNodePtrFromPtr(node.next).deref()
            n

Get a tree of grammar nodes which map out how the input sentence is structered.

        getTree: ->
            rootPtr = lib.linkage_constituent_tree @linkage
            @buildNode rootPtr.deref(), @linkage

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