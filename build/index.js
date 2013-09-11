(function () {
  var ffi, ref, Struct, pointerType, string, int, ParseOptions, Dictionary, Sentence, Linkage, CNode, CNodePtr, apiTemplate, libPath, lib, merge, getNodePtrFromPtr, defaultConfig;

  /* # Link Grammar Parser 
      
     The parser is built using the awesome ffi library which allows us to communicate with a native library under the covers. 
     Along with ffi, we also use ref library to reference the native objects used by the library. 
      */
  ffi = require("ffi");
  ref = require("ref");
  Struct = require("ref-struct");
  /*  
     Easier references to base types of data. 
      */
  pointerType = "pointer";
  string = ref.types.CString;
  int = ref.types.int;
  /*  
     These are just references to different native structs, which are all just pointers because we never use their actual referenced object. 
      */
  ParseOptions = pointerType;
  Dictionary = pointerType;
  Sentence = pointerType;
  Linkage = pointerType;
  /*  */
  CNode = Struct({label: ref.types.CString, child: pointerType, next: pointerType, start: ref.types.int, end: ref.types.int});
  /*  */
  CNodePtr = ref.refType(CNode);
  /*  
     Here are the templates for the native functions we will use. 
      */
  apiTemplate = {parse_options_create: [ParseOptions, []], dictionary_create: [Dictionary, [string, string, string, string]], sentence_create: [Sentence, [string, Dictionary]], sentence_parse: [int, [Sentence, ParseOptions]], linkage_create: [Linkage, [int, Sentence, ParseOptions]], linkage_print_diagram: [string, [Linkage]], linkage_constituent_tree: [CNodePtr, [Linkage]], linkage_print_constituent_tree: [string, [Linkage, int]], linkage_get_num_links: [int, [Linkage]], linkage_get_link_label: [string, [Linkage, int]], linkage_get_link_llabel: [string, [Linkage, int]], linkage_get_link_rlabel: [string, [Linkage, int]], linkage_get_word: [string, [Linkage, int]], linkage_get_link_lword: [int, [Linkage, int]], linkage_get_link_rword: [int, [Linkage, int]]};
  /*  
     Load the library. 
      */
  libPath = "./lib/libparser";
  lib = ffi.Library(libPath, apiTemplate);
  /*  
     Utility functions... 
      */
  merge = function (a, b) {
    var c, key, ki$1, kobj$1, ki$2, kobj$2;
    c = {};
    kobj$1 = a;
    for (key in kobj$1) {
      if (!kobj$1.hasOwnProperty(key)) {continue;}
      c[key] = a[key];
    }
    kobj$2 = b;
    for (key in kobj$2) {
      if (!kobj$2.hasOwnProperty(key)) {continue;}
      c[key] = b[key];
    }
    return c;
  };

  /*  */
  String.prototype.contains = function (n) {
    return this.indexOf(n) !== -1;
  };

  /*  */
  getNodePtrFromPtr = function (ptr) {
    var tempPtr;
    tempPtr = ref.alloc(CNodePtr);
    ref.writePointer(tempPtr, 0, ptr);
    return tempPtr.deref();
  };

  /*  
     Default configuration for data paths. 
      */
  defaultConfig = {dictPath: "./data/4.0.dict", ppPath: "./data/4.0.knowledge", consPath: "./data/4.0.constituent-knowledge", affixPath: "./data/4.0.affix"};

  /*  
     Main parser class which interfaces the native library to make it very simple to get link grammar data from an input string. 
      */
  function Parser(config) {
    this._config = merge(defaultConfig, config);
    this._options = lib.parse_options_create();
    this._dictionary = lib.dictionary_create(this._config.dictPath, this._config.ppPath, this._config.consPath, this._config.affixPath);
  }

  /* A few utility methods for the parser. 
      */

  /*  */
  Parser.prototype.buildNode = function (node, linkage) {
    var n;
    n = {label: node.label};
    if (!(ref.isNull(node.child))) {
      n.child = this.buildNode(getNodePtrFromPtr(node.child).deref(), linkage);
    }
    if (!(ref.isNull(node.next))) {
      n.next = this.buildNode(getNodePtrFromPtr(node.next).deref(), linkage);
    }
    return n;
  }

  /*  */
  Parser.prototype.getLinkageLinks = function (linkage) {
    var links, index, numLinks, link, temp;
    links = [];
    index = 0;
    numLinks = lib.linkage_get_num_links(linkage);
    while (index < numLinks) {
      link = {label: lib.linkage_get_link_label(linkage, index), left: {label: lib.linkage_get_link_llabel(linkage, index), word: lib.linkage_get_word(linkage, lib.linkage_get_link_lword(linkage, index))}, right: {label: lib.linkage_get_link_rlabel(linkage, index), word: lib.linkage_get_word(linkage, lib.linkage_get_link_rword(linkage, index))}};
      if (link.left.word.contains('.')) {
        temp = link.left.word.split('.');
        link.left.word = temp[0];
        link.left.type = temp[1];
      }
      if (link.right.word.contains('.')) {
        temp = link.right.word.split('.');
        link.right.word = temp[0];
        link.right.type = temp[1];
      }
      links.push(link);
      index += 1;
    }
    return links;
  }

  /*  
     Get a tree of grammar nodes which map out how the input sentence is structered. 
      */
  Parser.prototype.getTree = function (input) {
    var sentence, numLinkages, linkage, rootPtr;
    sentence = lib.sentence_create(input, this._dictionary);
    numLinkages = lib.sentence_parse(sentence, this._options);
    linkage = lib.linkage_create(0, sentence, this._options);
    rootPtr = lib.linkage_constituent_tree(linkage);
    return this.buildNode(rootPtr.deref(), linkage);
  }

  /*  
     Get an array of linkages, each containing their grammar links. */
  Parser.prototype.getLinks = function (input) {
    var sentence, numLinkages, links, index, linkage;
    sentence = lib.sentence_create(input, this._dictionary);
    numLinkages = lib.sentence_parse(sentence, this._options);
    links = [];
    index = 0;
    while (index < numLinkages) {
      linkage = lib.linkage_create(index, sentence, this._options);
      links.push(this.getLinkageLinks(linkage));
      index += 1;
    }
    return links;
  }

  /*  
     Export the module, so others can use it. 
      */
  module.exports = Parser;
})()