%{

#include <iostream>
#include <string>
#define YY_DECL extern "C" int yylex()

#include "tc.tab.hpp"

std::size_t yyoffset;
std::size_t yyline;
std::size_t yycolumn;

#define YY_USER_ACTION \
    yylval.text = { yyoffset, yyleng }; \
    yylloc.first_line = yyline; \
    yylloc.first_column = yycolumn; \
    yylloc.last_line = yyline; \
    yylloc.last_column = yycolumn + yyleng; \
    yyoffset += yyleng; \
    yycolumn += yyleng;

void yyerror(const char s[]);

%}

%X slcomment mlcomment

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
SESC                    "\'"|"\""|"\?"|"\\"|"\a"|"\b"|"\f"|"\n"|"\r"|"\t"|"\v"
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
CCHAR                   [^'\\\n]|ESEQ
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
 /*     hexadecimal-prefix hexadecimal-fractional-constant */
 /*     binary-exponent-part floating-suffixopt */
 /*     hexadecimal-prefix hexadecimal-digit-sequence */
 /*     binary-exponent-part floating-suffixopt */
HFCONST                 {HPREFIX}{HFRAC}|{BEXP}{FSUFFIX}?|{HPREFIX}{HSEQ}|{BEXP}{FSUFFIX}?
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
 /* (6.4.5) s-char-sequence: */
 /*     s-char */
 /*     s-char-sequence s-char */
SCHARSEQ                {SCHAR}*


%%

[ \t]                   ;
\n                      { ++yyline; yycolumn = 1U; }
"case"                  { return CASE; }
"default"               { return DEFAULT; }
"sizeof"                { return SIZEOF; }
"return"                { return RETURN; }
"_Alignof"              { return _ALIGNOF; }
"_Generic"              { return _GENERIC; }
"typedef"               { return TYPEDEF; }
"extern"                { return EXTERN; }
"static"                { return STATIC; }
"_Thread_local"         { return _THREAD_LOCAL; }
"auto"                  { return AUTO; }
"register"              { return REGISTER; }
"void"                  { return VOID; }
"char"                  { return CHAR; }
"short"                 { return SHORT; }
"int"                   { return INT; }
"long"                  { return LONG; }
"float"                 { return FLOAT; }
"double"                { return DOUBLE; }
"signed"                { return SIGNED; }
"unsigned"              { return UNSIGNED; }
"_Bool"                 { return _BOOL; }
"_Complex"              { return _COMPLEX; }
"struct"                { return STRUCT; }
"union"                 { return UNION; }
"enum"                  { return ENUM; }
"_Atomic"               { return _ATOMIC; }
"const"                 { return CONST; }
"restrict"              { return RESTRICT; }
"volatile"              { return VOLATILE; }
"inline"                { return INLINE; }
"_Noreturn"             { return _NORETURN; }
"_Alignas"              { return _ALIGNAS; }
"_Static_assert"        { return _STATIC_ASSERT; }
"if"                    { return IF; }
"else"                  { return ELSE; }
"switch"                { return SWITCH; }
"while"                 { return WHILE; }
"do"                    { return DO; }
"for"                   { return FOR; }
"break"                 { return BREAK; }
"continue"              { return CONTINUE; }
"goto"                  { return GOTO; }

{ID}    { return ID; }


 /* A.1.5 Constants */

 /* (6.4.4.1) integer-constant: */
 /*     decimal-constant integer-suffixopt */
 /*     octal-constant integer-suffixopt */
 /*     hexadecimal-constant integer-suffixopt */
{DCONST}{ISUFFIX}?|{OCONST}{ISUFFIX}?|{HCONST}{ISUFFIX}? { return ICONST; }

 /* (6.4.4.2) floating-constant: */
 /*     decimal-floating-constant */
 /*     hexadecimal-floating-constant */
{DFCONST}|{HFCONST}     { return FCONST; }

 /* (6.4.4.4) character-constant: */
 /*     ' c-char-sequence ' */
 /*     L' c-char-sequence ' */
 /*     u' c-char-sequence ' */
 /*     U' c-char-sequence ' */
[LuU]?'{CCHARSEQ}'      { return CHCONST; }

 /* A.1.6 String literals */

 /* (6.4.5) string-literal: */
 /*     encoding-prefixopt " s-char-sequenceopt " */
{EPREFIX}?\"{SCHARSEQ}?\" { return SLIT; }

"->"                    { return ARR_OP; }
"++"                    { return INC_OP; }
"--"                    { return DEC_OP; }
"<<"                    { return LSH_OP; }
">>"                    { return RSH_OP; }
"<="                    { return LTE_OP; }
">="                    { return GTE_OP; }
"=="                    { return EQ_OP; }
"!="                    { return NE_OP; }
"&&"                    { return AND_OP; }
"||"                    { return OR_OP; }
"*="                    { return TIMESEQ_OP; }
"/="                    { return DIVEQ_OP; }
"%="                    { return MODEQ_OP; }
"+="                    { return PLUSEQ_OP; }
"-="                    { return MINUSEQ_OP; }
"<<="                   { return LSHIFTEQ_OP; }
">>="                   { return RSHIFTEQ_OP; }
"&="                    { return ANDEQ_OP; }
"^="                    { return XOREQ_OP; }
"|="                    { return OREQ_OP; }

"//"                    { BEGIN(slcomment); }
<slcomment>\n           { ++yyline; yycolumn = 1U; BEGIN(INITIAL); }
<slcomment>.            ;
"/*"                    { BEGIN(mlcomment); }
<mlcomment>"*/"         { BEGIN(INITIAL); }
<mlcomment>.            ;

"..."                   { return DOTS; }

"("|")"|";"|"{"|"}"|"["|"]"|"."|","|"?"|":"|"&"|"|"|"^"|"*"|"/"|"%"|"+"|"-"|"~"|"!"|"<"|">"|"=" {
    return yytext[0];
}

.                       { yyerror("Unknown token"); }
