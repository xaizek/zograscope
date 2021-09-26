/* Copyright (C) 2017 xaizek <xaizek@posteo.net>
 *
 * This file is part of zograscope.
 *
 * zograscope is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU Affero General Public License as
 * published by the Free Software Foundation.
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
%option extra-type="struct C11LexerData *"
%option prefix="c11_"

%{

#include <iostream>
#include <locale>
#include <string>

#include "c/C11LexerData.hpp"
#include "c/C11SType.hpp"
#include "c/c11-parser.gen.hpp"
#include "TreeBuilder.hpp"

#define YYSTYPE C11_STYPE
#define YYLTYPE C11_LTYPE

#define YY_INPUT(buf, result, maxSize) \
    do { (result) = yyextra->readInput((buf), (maxSize)); } while (false)

#define YY_USER_ACTION \
    yylval->text = { }; \
    yylval->text.from = yyextra->offset; \
    yylval->text.len = yyleng; \
    yylloc->first_line = yyextra->line; \
    yylloc->first_column = yyextra->col; \
    yylloc->last_line = yyextra->line; \
    yylloc->last_column = yyextra->col + yyleng; \
    yyextra->offset += yyleng; \
    yyextra->col += yyleng;

#define TOKEN(t) \
    yyextra->tb->markWithPostponed(yylval->text); \
    return (yylval->text.token = (t))

#define KW(t) \
    BEGIN(INITIAL); \
    TOKEN(t)

#define ADVANCE_LINE() \
    ++yyextra->line; \
    yyextra->col = 1U; \
    yyextra->lineoffset = yyextra->offset;

using namespace c11stypes;

static void reportError(C11_LTYPE *loc, const char text[], std::size_t len,
                        C11LexerData *data);

%}

%X directive dirmlcomment beforeparen slcomment mlcomment slit

 /* (6.4.3) hex-quad:
  *     hexadecimal-digit hexadecimal-digit hexadecimal-digit hexadecimal-digit
  */
HEXQUAD                 [[:xdigit:]]{4}
 /* (6.4.3) universal-character-name:
  *     \u hex-quad
  *     \U hex-quad hex-quad
  */
UCN                     \\u{HEXQUAD}|\\U{HEXQUAD}{2}
 /* (6.4.2.1) nondigit: one of
  * _ a b c d e f g h i j k l m
  *   n o p q r s t u v w x y z
  *   A B C D E F G H I J K L M
  *   N O P Q R S T U V W X Y Z
  */
NONDIGIT                [_a-zA-Z]
 /* (6.4.2.1) identifier-nondigit:
  *     nondigit
  *     universal-character-name
  *     other implementation-defined characters
  */
ID_NONDIGIT             {NONDIGIT}|{UCN}
 /* (6.4.2.1) identifier:
  *     identifier-nondigit
  *     identifier identifier-nondigit
  *     identifier digit
  */
ID                      {ID_NONDIGIT}({ID_NONDIGIT}|[[:digit:]])*
 /* (6.4.4.1) octal-digit: one of */
 /*     0 1 2 3 4 5 6 7 */
ODIGIT                  [0-7]
 /* (6.4.4.4) octal-escape-sequence: */
 /*     \ octal-digit */
 /*     \ octal-digit octal-digit */
 /*     \ octal-digit octal-digit octal-digit */
OESC                    \\{ODIGIT}{1,3}
 /* (6.4.4.4) hexadecimal-escape-sequence: */
 /*     \x hexadecimal-digit */
 /*     hexadecimal-escape-sequence hexadecimal-digit */
HESC                    \\x[[:xdigit:]]+
 /* (6.4.4.4) simple-escape-sequence: one of */
 /*     \' \" \? \\ */
 /*     \a \b \f \n \r \t \v */
SESC                    \\['"?\\abfnrtv]
 /* (6.4.4.4) escape-sequence: */
 /*     simple-escape-sequence */
 /*     octal-escape-sequence */
 /*     hexadecimal-escape-sequence */
 /*     universal-character-name */
ESEQ                    {SESC}|{OESC}|{HESC}|{UCN}
 /* (6.4.4.4) c-char: */
 /*     any member of the source character set except */
 /*         the single-quote ', backslash \, or new-line character */
 /*     escape-sequence */
CCHAR                   [^'\\\n]|{ESEQ}
 /* (6.4.4.4) c-char-sequence: */
 /*     c-char */
 /*     c-char-sequence c-char */
CCHARSEQ                {CCHAR}+
 /* (6.4.4.2) floating-suffix: one of */
 /*     f l F L */
FSUFFIX                 [flFL]
 /* (6.4.4.2) hexadecimal-digit-sequence: */
 /*     hexadecimal-digit */
 /*     hexadecimal-digit-sequence hexadecimal-digit */
HSEQ                    [[:xdigit:]]+
 /* (6.4.4.2) sign: one of */
 /*     + - */
SIGN                    [-+]
 /* (6.4.4.2) digit-sequence: */
 /*     digit */
 /*     digit-sequence digit */
DSEQ                    [[:digit:]]+
 /* (6.4.4.2) binary-exponent-part: */
 /*     p signopt digit-sequence */
 /*     P signopt digit-sequence */
BEXP                    [pP]{SIGN}?{DSEQ}
 /* (6.4.4.2) hexadecimal-fractional-constant: */
 /*     hexadecimal-digit-sequenceopt . */
 /*     hexadecimal-digit-sequence */
 /*     hexadecimal-digit-sequence . */
HFRAC                   {HSEQ}?\.|{HSEQ}\.?
 /* (6.4.4.2) exponent-part: */
 /*     e signopt digit-sequence */
 /*     E signopt digit-sequence */
EPART                   [eE]{SIGN}?{DSEQ}
 /* (6.4.4.2) fractional-constant: */
 /*     digit-sequenceopt . digit-sequence */
 /*     digit-sequence . */
FRACCONST               {DSEQ}?\.{DSEQ}|{DSEQ}\.
 /* (6.4.4.1) hexadecimal-prefix: one of */
 /*     0x 0X */
HPREFIX                 0[xX]
 /* (6.4.4.2) hexadecimal-floating-constant: */
 /*     hexadecimal-prefix hexadecimal-fractional-constant binary-exponent-part floating-suffixopt */
 /*     hexadecimal-prefix hexadecimal-digit-sequence binary-exponent-part floating-suffixopt */
HFCONST                 {HPREFIX}{HFRAC}{BEXP}{FSUFFIX}?|{HPREFIX}{HSEQ}{BEXP}{FSUFFIX}?
 /* (6.4.4.1) unsigned-suffix: one of */
 /*     u U */
USUFFIX                 [uU]
 /* (6.4.4.1) long-suffix: one of */
 /*     l L */
LSUFFIX                 [lL]
 /* (6.4.4.1) long-long-suffix: one of */
 /*     ll LL */
LLSUFFIX                ll|LL
 /* (6.4.4.1) nonzero-digit: one of */
 /*     1 2 3 4 5 6 7 8 9 */
NZDIGIT                 [1-9]
 /* (6.4.4.1) decimal-constant: */
 /*     nonzero-digit */
 /*     decimal-constant digit */
DCONST                  {NZDIGIT}[[:digit:]]*
 /* (6.4.4.1) octal-constant: */
 /*     0 */
 /*     octal-constant octal-digit */
OCONST                  0{ODIGIT}*
 /* (6.4.4.1) integer-suffix: */
 /*     unsigned-suffix long-suffixopt */
 /*     unsigned-suffix long-long-suffix */
 /*     long-suffix unsigned-suffixopt */
 /*     long-long-suffix unsigned-suffixopt */
ISUFFIX                 {USUFFIX}{LSUFFIX}?|{USUFFIX}{LLSUFFIX}|{LSUFFIX}{USUFFIX}?|{LLSUFFIX}{USUFFIX}?
 /* (6.4.4.2) decimal-floating-constant: */
 /*     fractional-constant exponent-partopt floating-suffixopt */
 /*     digit-sequence exponent-part floating-suffixopt */
DFCONST                 {FRACCONST}{EPART}?{FSUFFIX}?|{DSEQ}{EPART}{FSUFFIX}?
 /* (6.4.4.1) hexadecimal-constant: */
 /*     hexadecimal-prefix hexadecimal-digit */
 /*     hexadecimal-constant hexadecimal-digit */
HCONST                  {HPREFIX}[[:xdigit:]]+
 /* (6.4.5) encoding-prefix: */
 /*     u8 */
 /*     u */
 /*     U */
 /*     L */
EPREFIX                 u8|u|U|L
 /* (6.4.5) s-char: */
 /*     any member of the source character set except */
 /*         the double-quote ", backslash \, or new-line character */
 /*     escape-sequence */
SCHAR                   [^"\\\n]|{ESEQ}
 /* (6.4.7) header-name: */
 /*     < h-char-sequence > */
 /*     " q-char-sequence " */
HEADERNAME              <{HCHARSEQ}>|"{QCHARSEQ}"
 /* (6.4.7) h-char-sequence: */
 /*     h-char */
 /*     h-char-sequence h-char */
HCHARSEQ                {HCHAR}+
 /* (6.4.7) h-char: */
 /*     any member of the source character set except */
 /*         the new-line character and > */
HCHAR                   [^>\n]
 /* (6.4.7) q-char-sequence: */
 /*     q-char */
 /*     q-char-sequence q-char */
QCHARSEQ                {QCHAR}+
 /* (6.4.7) q-char: */
 /*     any member of the source character set except */
 /*         the new-line character and " */
QCHAR                   [^"\n]

NL                      \n|\r|\r\n

%%

[ ]                     ;
\t {
    yyextra->col += yyextra->tabWidth - (yyextra->col - 1)%yyextra->tabWidth;
}
{NL}                    { ADVANCE_LINE(); }
\\{NL} {
    yylval->text.len = 1;
    yylloc->last_column = yylloc->first_column + 1;
    yyextra->tb->addPostponed(yylval->text, *yylloc, +C11SType::LineGlue);
    ADVANCE_LINE();
}
<INITIAL,beforeparen>"case"                  { KW(CASE); }
<INITIAL,beforeparen>"default"               { KW(DEFAULT); }
<INITIAL,beforeparen>"sizeof"                { KW(SIZEOF); }
<INITIAL,beforeparen>"return"                { KW(RETURN); }
<INITIAL,beforeparen>"_Alignof"              { KW(_ALIGNOF); }
<INITIAL,beforeparen>"_Generic"              { KW(_GENERIC); }
<INITIAL,beforeparen>"typedef"               { KW(TYPEDEF); }
<INITIAL,beforeparen>"extern"                { KW(EXTERN); }
<INITIAL,beforeparen>"static"                { KW(STATIC); }
<INITIAL,beforeparen>"_Thread_local"         { KW(_THREAD_LOCAL); }
<INITIAL,beforeparen>"auto"                  { KW(AUTO); }
<INITIAL,beforeparen>"register"              { KW(REGISTER); }
<INITIAL,beforeparen>"void"                  { KW(VOID); }
<INITIAL,beforeparen>"char"                  { KW(CHAR); }
<INITIAL,beforeparen>"short"                 { KW(SHORT); }
<INITIAL,beforeparen>"int"                   { KW(INT); }
<INITIAL,beforeparen>"long"                  { KW(LONG); }
<INITIAL,beforeparen>"float"                 { KW(FLOAT); }
<INITIAL,beforeparen>"double"                { KW(DOUBLE); }
<INITIAL,beforeparen>"signed"                { KW(SIGNED); }
<INITIAL,beforeparen>"unsigned"              { KW(UNSIGNED); }
<INITIAL,beforeparen>"_Bool"                 { KW(_BOOL); }
<INITIAL,beforeparen>"_Complex"              { KW(_COMPLEX); }
<INITIAL,beforeparen>"struct"                { KW(STRUCT); }
<INITIAL,beforeparen>"union"                 { KW(UNION); }
<INITIAL,beforeparen>"enum"                  { KW(ENUM); }
<INITIAL,beforeparen>"_Atomic"               { KW(_ATOMIC); }
<INITIAL,beforeparen>"const"                 { KW(CONST); }
<INITIAL,beforeparen>"restrict"              { KW(RESTRICT); }
<INITIAL,beforeparen>"volatile"              { KW(VOLATILE); }
<INITIAL,beforeparen>"inline"                { KW(INLINE); }
<INITIAL,beforeparen>"_Noreturn"             { KW(_NORETURN); }
<INITIAL,beforeparen>"_Alignas"              { KW(_ALIGNAS); }
<INITIAL,beforeparen>"_Static_assert"        { KW(_STATIC_ASSERT); }
<INITIAL,beforeparen>"if"                    { KW(IF); }
<INITIAL,beforeparen>"else"                  { KW(ELSE); }
<INITIAL,beforeparen>"switch"                { KW(SWITCH); }
<INITIAL,beforeparen>"while"                 { KW(WHILE); }
<INITIAL,beforeparen>"do"                    { KW(DO); }
<INITIAL,beforeparen>"for"                   { KW(FOR); }
<INITIAL,beforeparen>"break"                 { KW(BREAK); }
<INITIAL,beforeparen>"continue"              { KW(CONTINUE); }
<INITIAL,beforeparen>"goto"                  { KW(GOTO); }
<INITIAL,beforeparen>"asm"                   { KW(ASM); }
<INITIAL,beforeparen>"__asm__"               { KW(ASM); }
<INITIAL,beforeparen>"__volatile__"          { KW(VOLATILE); }
<INITIAL,beforeparen>"__attribute__"         { KW(ATTRIBUTE); }
<INITIAL>{ID}                                { TOKEN(ID); }
<beforeparen>{ID} {
    BEGIN(INITIAL);
    yyextra->tb->markWithPostponed(yylval->text);
    yylval->text.token = FUNCTION;
    return ID;
}

{ID}[[:space:]]*"(" {
    BEGIN(beforeparen);
    yyextra->offset -= yyleng;
    yyextra->col -= yyleng;
    yyless(0);
}


 /* A.1.5 Constants */

 /* (6.4.4.1) integer-constant: */
 /*     decimal-constant integer-suffixopt */
 /*     octal-constant integer-suffixopt */
 /*     hexadecimal-constant integer-suffixopt */
{DCONST}{ISUFFIX}?|{OCONST}{ISUFFIX}?|{HCONST}{ISUFFIX}? { TOKEN(ICONST); }

 /* (6.4.4.2) floating-constant: */
 /*     decimal-floating-constant */
 /*     hexadecimal-floating-constant */
{DFCONST}|{HFCONST}     { TOKEN(FCONST); }

 /* (6.4.4.4) character-constant: */
 /*     ' c-char-sequence ' */
 /*     L' c-char-sequence ' */
 /*     u' c-char-sequence ' */
 /*     U' c-char-sequence ' */
[LuU]?'{CCHARSEQ}'      { TOKEN(CHCONST); }

 /* A.1.6 String literals */

 /* (6.4.5) string-literal: */
 /*     encoding-prefixopt " s-char-sequenceopt " */
 /* (6.4.5) s-char-sequence: */
 /*     s-char */
 /*     s-char-sequence s-char */
{EPREFIX}?\" {
    yyextra->startTok = *yylval;
    yyextra->startTok.text.token = SLIT;
    yyextra->startLoc = *yylloc;
    BEGIN(slit);
}

<slit>{SCHAR} ;

<slit>\" {
    yyextra->startTok.text.len = yyextra->offset - yyextra->startTok.text.from;
    yyextra->tb->markWithPostponed(yyextra->startTok.text);

    *yylval = yyextra->startTok;
    *yylloc = yyextra->startLoc;

    BEGIN(INITIAL);
    return SLIT;
}
<slit>\\?{NL}           { ADVANCE_LINE(); }
<slit>.                 { reportError(yylloc, yytext, yyleng, yyextra); }

"->"                    { TOKEN(ARR_OP); }
"++"                    { TOKEN(INC_OP); }
"--"                    { TOKEN(DEC_OP); }
"<<"                    { TOKEN(LSH_OP); }
">>"                    { TOKEN(RSH_OP); }
"<="                    { TOKEN(LTE_OP); }
">="                    { TOKEN(GTE_OP); }
"=="                    { TOKEN(EQ_OP); }
"!="                    { TOKEN(NE_OP); }
"&&"                    { TOKEN(AND_OP); }
"||"                    { TOKEN(OR_OP); }
"*="                    { TOKEN(TIMESEQ_OP); }
"/="                    { TOKEN(DIVEQ_OP); }
"%="                    { TOKEN(MODEQ_OP); }
"+="                    { TOKEN(PLUSEQ_OP); }
"-="                    { TOKEN(MINUSEQ_OP); }
"<<="                   { TOKEN(LSHIFTEQ_OP); }
">>="                   { TOKEN(RSHIFTEQ_OP); }
"&="                    { TOKEN(ANDEQ_OP); }
"^="                    { TOKEN(XOREQ_OP); }
"|="                    { TOKEN(OREQ_OP); }

^[[:space:]]{-}[\n\r]*# {
    yyextra->startTok = *yylval;
    yyextra->startTok.text.token = DIRECTIVE;
    yyextra->startLoc = *yylloc;
    BEGIN(directive);
}
<directive>\\{NL} {
    ADVANCE_LINE();
}
<directive>{NL} {
    yyextra->startTok.text.len = yyextra->offset
                               - yyextra->startTok.text.from - 1;
    yyextra->startLoc.last_line = yylloc->last_line;
    yyextra->startLoc.last_column = yylloc->last_column;
    yyextra->tb->addPostponed(yyextra->startTok.text, yyextra->startLoc,
                              +C11SType::Directive);

    ADVANCE_LINE();
    BEGIN(INITIAL);
}
<directive>"/*"         BEGIN(dirmlcomment);
<dirmlcomment>"*/"      BEGIN(directive);
<dirmlcomment>{NL}      ADVANCE_LINE();
<dirmlcomment>.         ;
<directive>HEADERNAME   ;
<directive>.            ;

"//" {
    yyextra->startTok = *yylval;
    yyextra->startTok.text.token = SLCOMMENT;
    yyextra->startLoc = *yylloc;
    BEGIN(slcomment);
}
<slcomment>{NL} {
    yyextra->startTok.text.len = yyextra->offset
                               - yyextra->startTok.text.from - 1;
    yyextra->startLoc.last_line = yylloc->last_line;
    yyextra->startLoc.last_column = yylloc->last_column;
    yyextra->tb->addPostponed(yyextra->startTok.text, yyextra->startLoc,
                              +C11SType::Comment);

    ADVANCE_LINE();
    BEGIN(INITIAL);
}
<slcomment>.            ;

"/*" {
    yyextra->startTok = *yylval;
    yyextra->startTok.text.token = MLCOMMENT;
    yyextra->startLoc = *yylloc;
    BEGIN(mlcomment);
}
<mlcomment>"*/" {
    yyextra->startTok.text.len = yyextra->offset - yyextra->startTok.text.from;
    yyextra->startLoc.last_line = yylloc->last_line;
    yyextra->startLoc.last_column = yylloc->last_column;
    yyextra->tb->addPostponed(yyextra->startTok.text, yyextra->startLoc,
                              +C11SType::Comment);

    BEGIN(INITIAL);
}
<mlcomment>{NL}         { ADVANCE_LINE(); }
<mlcomment>.            ;

"..."                   { TOKEN(DOTS); }

"("|")"|";"|"{"|"}"|"["|"]"|"."|","|"?"|":"|"&"|"|"|"^"|"*"|"/"|"%"|"+"|"-"|"~"|"!"|"<"|">"|"=" {
    TOKEN(yytext[0]);
}

. { reportError(yylloc, yytext, yyleng, yyextra); }

%%

static void
reportError(C11_LTYPE *loc, const char text[], std::size_t len,
            C11LexerData *data)
{
    std::string error;

    if (len > 1U) {
        error = std::string("Unknown token: ") + text;
    } else if (std::isprint(text[0], std::locale())) {
        error = std::string("Unknown token: ") + text[0];
    } else {
        error = std::string("Unknown token: <") + std::to_string(text[0]) + '>';
    }

    C11_LTYPE changedLoc = *loc;
    changedLoc.first_column = data->offset - data->lineoffset;
    c11_error(&changedLoc, nullptr, data->tb, data->pd, error.c_str());
}

void
fakeYYunputUseC11()
{
    // This is needed to prevent compilation error on -Werror=unused.
    static_cast<void>(&yyunput);
}
