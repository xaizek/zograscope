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

// Custom input function.
#define YY_INPUT(buf, result, maxSize) \
    do { (result) = yyextra->readInput((buf), (maxSize)); } while (false)

// Piece of code to run at the start of every rule.
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

// Puts a fake token to the stream.
#define FAKE_TOKEN(t) \
    ({ \
        yyextra->offset -= yyleng; \
        yyextra->col -= yyleng; \
        yyless(0); \
        token(t, yylval, yyextra); \
    })

// Handles a keyword.
#define KW(t) \
    do { \
        if (yyextra->nesting.empty()) { \
            return token((t), yylval, yyextra); \
        } \
        yyextra->offset -= yyleng; \
        yyextra->col -= yyleng; \
        REJECT; \
    } while (false)

using namespace makestypes;

// Convenience definition for token() funciton's argument.
enum { NeedFakeWS = 1 };

// Performs additional operations on returning a token.
static inline int
token(int tokenId, YYSTYPE *lval, MakeLexerData *extra, bool needFakeWS = false)
{
    if (tokenId != WS) {
        extra->tb->markWithPostponed(lval->text);
    }
    extra->fakeWSIsNeeded = needFakeWS;
    extra->lastCharOffset = extra->offset;
    lval->text.token = tokenId;
    return tokenId;
}

// Checks whether fake WS token should be inserted into the token stream.
static inline bool
shouldInsertFakeWS(YYSTYPE *lval, MakeLexerData *extra)
{
    return lval->text.from != extra->lastCharOffset
        && extra->fakeWSIsNeeded;
}

// Advances line tracking to the next line.
static inline void
advanceLine(MakeLexerData *extra)
{
    ++extra->line;
    extra->col = 1U;
}

%}

%X slcomment dslit sslit achar

NL                      \n|\r|\r\n

%%

[ ]                     ;
{NL}\t|^\t {
    advanceLine(yyextra);
    yyextra->col = yyextra->tabWidth + 1;
    return token(LEADING_TAB, yylval, yyextra);
}
\t {
    yyextra->col += yyextra->tabWidth - (yyextra->col - 1)%yyextra->tabWidth;
}
{NL} {
    advanceLine(yyextra);
    return token(NL, yylval, yyextra);
}
\\{NL} {
    yylval->text.len = 1;
    yylloc->last_column = yylloc->first_column + 1;
    yyextra->tb->addPostponed(yylval->text, *yylloc, +MakeSType::LineGlue);
    advanceLine(yyextra);
}

# {
    yyextra->startTok = *yylval;
    yyextra->startLoc = *yylloc;
    BEGIN(slcomment);
}
<slcomment>\\{NL}              advanceLine(yyextra);
<slcomment>{NL} {
    yylval->text.from = yyextra->startTok.text.from;
    yylval->text.len = yyextra->offset - yyextra->startTok.text.from - 1;
    *yylloc = yyextra->startLoc;

    BEGIN(INITIAL);

    yyextra->offset -= yyleng;
    yyextra->col -= yyleng;
    yyless(0);

    return token(COMMENT, yylval, yyextra);
}
<slcomment>.            ;

\" {
    yyextra->startTok = *yylval;
    yyextra->startTok.text.token = SLIT;
    yyextra->startLoc = *yylloc;
    BEGIN(dslit);
}
<dslit>\" {
    yyextra->startTok.text.len = yyextra->offset - yyextra->startTok.text.from;

    *yylval = yyextra->startTok;
    *yylloc = yyextra->startLoc;

    BEGIN(INITIAL);
    return token(yylval->text.token, yylval, yyextra, NeedFakeWS);
}

' {
    yyextra->startTok = *yylval;
    yyextra->startTok.text.token = SLIT;
    yyextra->startLoc = *yylloc;
    BEGIN(sslit);
}
<sslit>' {
    yyextra->startTok.text.len = yyextra->offset - yyextra->startTok.text.from;

    *yylval = yyextra->startTok;
    *yylloc = yyextra->startLoc;

    BEGIN(INITIAL);
    return token(yylval->text.token, yylval, yyextra, NeedFakeWS);
}

<dslit,sslit>{NL} {
    const int length = yyextra->offset - yyextra->startTok.text.from;
    const char *const base = yyextra->contents + yyextra->startTok.text.from;
    for (int i = length - 1; i >= 0; --i) {
        unput(base[i]);
    }

    yyextra->offset = yyextra->startTok.text.from;
    yyextra->col = yyextra->startLoc.first_column;
    yyextra->line = yyextra->startLoc.first_line;

    BEGIN(achar);
}
<dslit,sslit>\\{NL}                  advanceLine(yyextra);
<dslit,sslit>.                       ;

<achar>. {
    BEGIN(INITIAL);
    return token(CHARS, yylval, yyextra, NeedFakeWS);
}

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

"="|"?="|":="|"::="|"+="|"!="  return token(ASSIGN_OP, yylval, yyextra);
"$("|"${" {
    if (shouldInsertFakeWS(yylval, yyextra)) {
        return FAKE_TOKEN(WS);
    }
    yyextra->nesting.push_back(MakeLexerData::FunctionNesting);
    return token(CALL_PREFIX, yylval, yyextra);
}
$.                             return token(VAR, yylval, yyextra);
"(" {
    if (shouldInsertFakeWS(yylval, yyextra)) {
        return FAKE_TOKEN(WS);
    }
    if (!yyextra->nesting.empty()) {
        yyextra->nesting.push_back(MakeLexerData::ArgumentNesting);
    }
    return token('(', yylval, yyextra);
}
")" {
    if (yyextra->nesting.empty()) {
        return token(')', yylval, yyextra);
    }
    if (yyextra->nesting.back() == MakeLexerData::ArgumentNesting) {
        yyextra->nesting.pop_back();
        return token(')', yylval, yyextra, NeedFakeWS);
    }
    yyextra->nesting.pop_back();
    return token(CALL_SUFFIX, yylval, yyextra, NeedFakeWS);
}
"}" {
    if (yyextra->nesting.empty()) {
        yyextra->offset -= yyleng;
        yyextra->col -= yyleng;
        REJECT;
    }
    yyextra->nesting.pop_back();
    return token(CALL_SUFFIX, yylval, yyextra, NeedFakeWS);
}
","                            return token(',', yylval, yyextra);
":"                            return token(':', yylval, yyextra);
.|[-a-zA-Z0-9_/.]+ {
    if (shouldInsertFakeWS(yylval, yyextra)) {
        return FAKE_TOKEN(WS);
    }
    return token(CHARS, yylval, yyextra, NeedFakeWS);
}

%%

void
fakeYYunputUseMake()
{
    // This is needed to prevent compilation error on -Werror=unused.
    static_cast<void>(&yyunput);
}
