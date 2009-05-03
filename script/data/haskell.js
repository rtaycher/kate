/** kate-script
 * name: Haskell
 * license: LGPL
 * author: Erlend Hamberg <ehamberg@gmail.com>
 * version: 1
 * kate-version: 3.0
 * type: indentation
 */

// based on Paul Giannaro's Python indenter

var debugMode = false;

String.prototype.startsWith = function(prefix) {
    return this.substring(0, prefix.length) == prefix;
}

String.prototype.endsWith = function(suffix) {
    var startPos = this.length - suffix.length;
    if (startPos < 0)
        return false;
    return (this.lastIndexOf(suffix, startPos) == startPos);
}

String.prototype.lastCharacter = function() {
    var l = this.length;
    if(l == 0)
        return '';
    else
        return this.charAt(l - 1);
}

String.prototype.sansWhiteSpace = function() {
    return this.replace(/[ \t\n\r]/g, '');
}
String.prototype.stripWhiteSpace = function() {
    return this.replace(/^[ \t\n\r]+/, '').replace(/[ \t\n\r]+$/, '');
}


function dbg(s) {
    // debug to the term in blue so that it's easier to make out amongst all
    // of Kate's other debug output.
    if (debugMode)
        debug("\u001B[34m" + s + "\u001B[0m");
}

var triggerCharacters = " \t";

// General notes:
// indent() returns the amount of characters (in spaces) to be indented.
// Special indent() return values:
//   -2 = no indent
//   -1 = keep last indent

function indent(line, indentWidth, character) {
    dbg(document.attribute.toString());
    dbg("indent character: '" + character + "'");
    dbg("line text: " + document.line(line));
    var currentLine = document.line(line);
    dbg("current line: " + currentLine);
    var lastLine = document.line(line - 1);
    var lastCharacter = lastLine.lastCharacter();
    //
    // we can't really indent line 0
    if(line == 0)
        return -2;
    //
    // make sure the last line is code
    if(!document.isCode(line - 1, document.lineLength(line - 1) - 1)
            && lastCharacter != "\"" && lastCharacter != "'") {
        dbg("attributes that we don't want! Returning");
        return -1;
    }
    // otherwise, check the line contents

    dbg('line without white space: ' + currentLine.sansWhiteSpace().length);

    // indent line after 'where' 6 characters for alignment:
    // ... where foo = 3
    //     >>>>>>bar = 4
    if(lastLine.stripWhiteSpace().startsWith('where')) {
        dbg('indenting line for where');
        return document.firstVirtualColumn(line - 1) + 6;
    }

    // indent line after 'let' 4 characters for alignment:
    // ... let foo = 3
    //     >>>>bar = 4
    if(lastLine.stripWhiteSpace().startsWith('let')) {
        dbg('indenting line for let');
        return document.firstVirtualColumn(line - 1) + 4;
    }

    // deindent line starting with 'in' to the level of its corresponding 'let':
    // ... let foo = 3
    //         bar = 4
    //     in foo+bar
    if(currentLine.search(/^\s*in/) != -1) {
        dbg('indenting line for in');
        var temp = line-1;
        var indent = -1;
        while (temp >= 0) {
            if (document.line(temp).sansWhiteSpace().startsWith('let') ) {
                indent = document.line(temp).search(/\S/);
                break;
            }
            temp = temp-1;
        }
        return indent;
    }

    // indent line after a line with just 'in' one indent width:
    // ... let foo = 3
    //         bar = 4
    //     in
    //     >>>>foo+bar
    if(lastLine.stripWhiteSpace() == 'in') {
        dbg('indenting line after in');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // indent line after 'case' 5 characters for alignment:
    // case xs of
    // >>>>>[] -> ...
    // >>>>>(y:ys) -> ...
    var caseCol = lastLine.search(/\bcase\b/);
    if(caseCol != -1) {
        dbg('indenting line for case');
        return document.firstVirtualColumn(line - 1) + 5 + caseCol;
    }

    // indent line after 'if/else' 3 characters for alignment:
    // if foo == bar
    // >>>then baz
    // >>>else vaff
    var ifCol = lastLine.search(/\bif\b/);
    if(ifCol != -1) {
        dbg('indenting line for if');
        return document.firstVirtualColumn(line - 1) + 3 + ifCol;
    }

    // indent lines following a line ending with '='
    if(lastLine.stripWhiteSpace().endsWith('=')) {
        dbg('indenting for =');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // indent lines following a line ending with 'do'
    if(lastLine.stripWhiteSpace().endsWith('do')) {
        dbg('indenting for do');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    // line ending with !#$%&*+./<=>?@\^|~-
    if (lastLine.search(/[!$#%&*+.\/<=>?@\\^|~-]$/) != -1) {
        dbg('indenting for operator');
        return document.firstVirtualColumn(line - 1) + indentWidth;
    }

    if (lastLine.search(/^\s*$/) != -1) {
        dbg('indenting for empty line');
        return 0;
    }


    dbg('continuing with regular indent');
    return -1;
}


// kate: space-indent on; indent-width 4; replace-tabs on;
