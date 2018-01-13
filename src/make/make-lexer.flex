/* Copyright (C) 2017 xaizek <xaizek@posteo.net>
 *
 * This file is part of zograscope.
 *
 * zograscope is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * zograscope is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with zograscope.  If not, see <http://www.gnu.org/licenses/>.
 */

%option bison-bridge
%option bison-locations
%option reentrant
%option noyywrap
%option extra-type="struct MakeLexerData *"
%option prefix="make_"

%{

#include <iostream>
#include <locale>
#include <string>

#include "make/MakeLexerData.hpp"
#include "make/MakeSType.hpp"
#include "make/make-parser.hpp"
#include "TreeBuilder.hpp"

#define YY_INPUT(buf, result, maxSize) \
    do { (result) = yyextra->readInput((buf), (maxSize)); } while (false)

#define YY_USER_ACTION \
    { \
        yylval->text = { }; \
        yylval->text.from = yyextra->offset; \
        yylval->text.len = yyleng; \
        yylloc->first_line = yyextra->line; \
        yylloc->first_column = yyextra->col; \
        yylloc->last_line = yyextra->line; \
        yylloc->last_column = yyextra->col + yyleng; \
        yyextra->offset += yyleng; \
        yyextra->col += yyleng; \
    }

#define TOKEN(t) \
    do { \
        if ((t) != WS) { \
            yyextra->tb->markWithPostponed(yylval->text); \
        } \
        yyextra->contiguousChars = 0; \
        return (yylval->text.token = (t)); \
    } while (false)

#define CHAR_LIKE_TOKEN(t) \
    do { \
        if ((t) != WS) { \
            yyextra->tb->markWithPostponed(yylval->text); \
        } \
        ++yyextra->contiguousChars; \
        return (yylval->text.token = (t)); \
    } while (false)

#define KW(t) \
    do { \
        if (yyextra->nesting.empty()) { \
            TOKEN(t); \
        } else { \
            yyextra->offset -= yyleng; \
            yyextra->col -= yyleng; \
            REJECT; \
        } \
    } while (false)

#define ADVANCE_LINE() \
    do { \
        ++yyextra->line; \
        yyextra->col = 1U; \
        yyextra->lineoffset = yyextra->offset; \
    } while (false)

using namespace makestypes;

%}

%X slcomment

NL                      \n|\r|\r\n

%%

[ ]                     ;
{NL}\t|^\t {
    ADVANCE_LINE();
    yyextra->col = yyextra->tabWidth + 1;
    TOKEN(LEADING_TAB);
}
\t {
    yyextra->col += yyextra->tabWidth - (yyextra->col - 1)%yyextra->tabWidth;
}
{NL} {
    ADVANCE_LINE();
    TOKEN(NL);
}
\\{NL} {
    yylval->text.len = 1;
    yylloc->last_column = yylloc->first_column + 1;
    yyextra->tb->addPostponed(yylval->text, *yylloc, +MakeSType::LineGlue);
    ADVANCE_LINE();
}

# {
    yyextra->startTok = *yylval;
    yyextra->startLoc = *yylloc;
    BEGIN(slcomment);
}
<slcomment>\\{NL}              ADVANCE_LINE();
<slcomment>{NL} {
    yylval->text.from = yyextra->startTok.text.from;
    yylval->text.len = yyextra->offset - yyextra->startTok.text.from - 1;
    *yylloc = yyextra->startLoc;

    BEGIN(INITIAL);

    yyextra->offset -= yyleng;
    yyextra->col -= yyleng;
    yyless(0);

    TOKEN(COMMENT);
}
<slcomment>.            ;

"override"                     KW(OVERRIDE);
"export"                       KW(EXPORT);
"unexport"                     KW(UNEXPORT);

"ifdef"                        KW(IFDEF);
"ifndef"                       KW(IFNDEF);
"ifeq"                         KW(IFEQ);
"ifneq"                        KW(IFNEQ);
"else"                         KW(ELSE);
"endif"                        KW(ENDIF);

"define"                       KW(DEFINE);
"endef"                        KW(ENDEF);
"undefine"                     KW(UNDEFINE);

-?"include"                    KW(INCLUDE);

"="|"?="|":="|"::="|"+="|"!="  TOKEN(ASSIGN_OP);
"$("|"${" {
    if (yylval->text.from != yyextra->lastCharOffset &&
        yyextra->contiguousChars != 0) {
        yyextra->offset -= yyleng;
        yyextra->col -= yyleng;
        yyless(0);
        TOKEN(WS);
    }
    yyextra->lastCharOffset = yyextra->offset;
    yyextra->nesting.push_back(MakeLexerData::FunctionNesting);
    TOKEN(CALL_PREFIX);
}
$.                             TOKEN(VAR);
"(" {
    if (yylval->text.from != yyextra->lastCharOffset &&
        yyextra->contiguousChars != 0) {
        yyextra->offset -= yyleng;
        yyextra->col -= yyleng;
        yyless(0);
        TOKEN(WS);
    }
    yyextra->lastCharOffset = yyextra->offset;
    if (!yyextra->nesting.empty()) {
        yyextra->nesting.push_back(MakeLexerData::ArgumentNesting);
    }
    TOKEN('(');
}
")" {
    if (yyextra->nesting.empty() ||
        yyextra->nesting.back() == MakeLexerData::ArgumentNesting) {
        if (!yyextra->nesting.empty()) {
            yyextra->nesting.pop_back();
            CHAR_LIKE_TOKEN(')');
        } else {
            TOKEN(')');
        }
    }
    yyextra->nesting.pop_back();
    yyextra->lastCharOffset = yyextra->offset;
    CHAR_LIKE_TOKEN(CALL_SUFFIX);
}
"}" {
    if (yyextra->nesting.empty()) {
        yyextra->offset -= yyleng;
        yyextra->col -= yyleng;
        REJECT;
    }
    yyextra->nesting.pop_back();
    yyextra->lastCharOffset = yyextra->offset;
    CHAR_LIKE_TOKEN(CALL_SUFFIX);
}
","                            TOKEN(',');
":"                            TOKEN(':');
.|[-a-zA-Z0-9_/.]+ {
    if (yylval->text.from != yyextra->lastCharOffset &&
        yyextra->contiguousChars != 0) {
        yyextra->offset -= yyleng;
        yyextra->col -= yyleng;
        yyless(0);
        TOKEN(WS);
    }
    yyextra->lastCharOffset = yyextra->offset;
    CHAR_LIKE_TOKEN(CHARS);
}

%%

void
fakeYYunputUseMake()
{
    // This is needed to prevent compilation error on -Werror=unused.
    static_cast<void>(&yyunput);
}
