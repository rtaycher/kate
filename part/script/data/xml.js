/** kate-script
 * name: XML Style
 * license: LGPL
 * author: Milian Wolff <mail@milianw.de>
 * revision: 1
 * kate-version: 3.4
 * type: indentation
 *
 * This file is part of the Kate Project.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

var debugMode = true;

// specifies the characters which should trigger indent, beside the default '\n'
triggerCharacters = "/>";

function dbg(s) {
    if (debugMode)
        debug(s);
}

/**
 * Process a newline character.
 * This function is called whenever the user hits <return/enter>.
 *
 * It gets three arguments: line, indentwidth in spaces, typed character.
 */
function indent(line, indentWidth, char)
{
    var prefLineString = line ? document.line(line-1) : "";
    var prefIndent = document.firstVirtualColumn(line-1);
    var lineString = document.line(line);

    var alignOnly = (char == "");
    if (alignOnly) {
        // XML might be all in one line, in which case we
        // want to break that up.
        var tokens = lineString.split(/>\s*</);
        if (tokens.length > 1) {
            var oldLine = line;
            var oldPrefIndent = prefIndent;
            for (var l in tokens) {
                var newLine = tokens[l];
                if (l > 0) {
                    newLine = '<' + newLine;
                }
                if (l < tokens.length - 1) {
                    newLine += '>';
                }

                if (newLine.match(/^\s*<\//)) {
                    char = '/';
                } else if (newLine.match(/\>[^<>]*$/)) {
                    char = '>';
                } else {
                    char = '\n';
                }
                var indentation = processChar(line, newLine, prefLineString, prefIndent, char, indentWidth);
                prefIndent = indentation;
                while (indentation > 0) {
                    //TODO: what about tabs
                    newLine = " " + newLine;
                    --indentation;
                }
                ++line;
                prefLineString = newLine;
                tokens[l] = newLine;
            }
            dbg(tokens.join('\n'));
            document.editBegin();
            document.removeLine(oldLine);
            document.insertText(oldLine, 0, tokens.join('\n'));
            document.editEnd();
            return oldPrefIndent;
        } else {
            if (lineString.match(/^\s*<\//)) {
                char = '/';
            } else if (lineString.match(/\>[^<>]*$/)) {
                char = '>';
            }
        }
    }

    return processChar(line, lineString, prefLineString, prefIndent, char, indentWidth);
}

function processChar(line, lineString, prefLineString, prefIndent, char, indentWidth)
{
    if (char == '/') {
        if (lineString.match(/^\s*<\//) && !prefLineString.match(/<[^/>]+>[^<>]*$/)) {
            // decrease indent when we write </ and prior line did not start a tag
            return prefIndent - indentWidth;
        }
    } else if (char == '>') {
        // increase indent width when we write <...> or <.../> but not </...>
        // and the prior line didn't close a tag
        if (line == 0) {
            return 0;
        } else if (prefLineString.match(/^<\?xml/)) {
            return 0;
        } else if (lineString.match(/^\s*<\//)) {
            // closing tag, decrease indentation when previous didn't open tag
            if (prefLineString.match(/<[^\/>]+>[^<>]*$/)) {
                // keep indent when pref line closed a tag
                return prefIndent;
            } else {
                return prefIndent - indentWidth;
            }
        } else if (prefLineString.match(/<\/[^\/>]+>\s*$/)) {
            // keep indent when pref line closed a tag
            return prefIndent;
        }
        return prefIndent + indentWidth;
    } else if (char == '\n') {
        if (prefLineString.match(/<[^\/>]+>[^<>]*$/)) {
            // increase indent when pref line opened a tag
            return prefIndent + indentWidth;
        }
    }

    return prefIndent;
}

// kate: space-indent on; indent-width 4; replace-tabs on;