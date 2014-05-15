var LinkGrammar = require(__dirname + "/../build/index");

// Load module
var linkGrammar = new LinkGrammar();

// Parse text into linkage object
// Of which you can query the links, words and link tree 
var linkage = linkGrammar.parse('turn off the light');

// You can query links by label
var mvLinks = linkage.linksByLabel('MV');

// You can also get the word objects connected to a specific word in the text
// Each word object contains label and type informations about the connection
var connections = linkage.getConnectorWords('off');