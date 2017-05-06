%{

#include <iostream>
#include <string>
#define YY_DECL extern "C" int yylex()

#include "tc.tab.hpp"

enum { tabWidth = 4 };

std::size_t yyoffset;
std::size_t yyline;
std::size_t yycolumn;

static YYSTYPE startTok;
static YYLTYPE startLoc;

#define YY_USER_ACTION \
    yylval.text = { yyoffset, yyleng }; \
    yylloc.first_line = yyline; \
    yylloc.first_column = yycolumn; \
    yylloc.last_line = yyline; \
    yylloc.last_column = yycolumn + yyleng; \
    yyoffset += yyleng; \
    yycolumn += yyleng;

#define TOKEN(t) \
    tb->markWithPostponed(yylval.text); \
    return (yylval.text.token = (t))

void yyerror(const char s[]);

%}

%X directive slcomment mlcomment

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

[ ]                     ;
\t                      { yycolumn += tabWidth - (yycolumn - 1)%tabWidth; }
\n                      { ++yyline; yycolumn = 1U; }
"case"                  { TOKEN(CASE); }
"default"               { TOKEN(DEFAULT); }
"sizeof"                { TOKEN(SIZEOF); }
"return"                { TOKEN(RETURN); }
"_Alignof"              { TOKEN(_ALIGNOF); }
"_Generic"              { TOKEN(_GENERIC); }
"typedef"               { TOKEN(TYPEDEF); }
"extern"                { TOKEN(EXTERN); }
"static"                { TOKEN(STATIC); }
"_Thread_local"         { TOKEN(_THREAD_LOCAL); }
"auto"                  { TOKEN(AUTO); }
"register"              { TOKEN(REGISTER); }
"void"                  { TOKEN(VOID); }
"char"                  { TOKEN(CHAR); }
"short"                 { TOKEN(SHORT); }
"int"                   { TOKEN(INT); }
"long"                  { TOKEN(LONG); }
"float"                 { TOKEN(FLOAT); }
"double"                { TOKEN(DOUBLE); }
"signed"                { TOKEN(SIGNED); }
"unsigned"              { TOKEN(UNSIGNED); }
"_Bool"                 { TOKEN(_BOOL); }
"_Complex"              { TOKEN(_COMPLEX); }
"struct"                { TOKEN(STRUCT); }
"union"                 { TOKEN(UNION); }
"enum"                  { TOKEN(ENUM); }
"_Atomic"               { TOKEN(_ATOMIC); }
"const"                 { TOKEN(CONST); }
"restrict"              { TOKEN(RESTRICT); }
"volatile"              { TOKEN(VOLATILE); }
"inline"                { TOKEN(INLINE); }
"_Noreturn"             { TOKEN(_NORETURN); }
"_Alignas"              { TOKEN(_ALIGNAS); }
"_Static_assert"        { TOKEN(_STATIC_ASSERT); }
"if"                    { TOKEN(IF); }
"else"                  { TOKEN(ELSE); }
"switch"                { TOKEN(SWITCH); }
"while"                 { TOKEN(WHILE); }
"do"                    { TOKEN(DO); }
"for"                   { TOKEN(FOR); }
"break"                 { TOKEN(BREAK); }
"continue"              { TOKEN(CONTINUE); }
"goto"                  { TOKEN(GOTO); }

{ID}    { TOKEN(ID); }


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
{EPREFIX}?\"{SCHARSEQ}?\" { TOKEN(SLIT); }

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

^[[:space:]]*# {
    startTok = yylval;
    startTok.text.token = DIRECTIVE;
    startLoc = yylloc;
    BEGIN(directive);
}
<directive>\\\n         ;
<directive>\n {
    startTok.text.len = yyoffset - startTok.text.from - 1;
    startLoc.last_line = yylloc.last_line;
    startLoc.last_column = yylloc.last_column;
    tb->addPostponed(startTok.text, startLoc);

    ++yyline;
    yycolumn = 1U;
    BEGIN(INITIAL);
}
<directive>.            ;

"//" {
    startTok = yylval;
    startTok.text.token = SLCOMMENT;
    startLoc = yylloc;
    BEGIN(slcomment);
}
<slcomment>\n {
    startTok.text.len = yyoffset - startTok.text.from - 1;
    startLoc.last_line = yylloc.last_line;
    startLoc.last_column = yylloc.last_column;
    tb->addPostponed(startTok.text, startLoc);

    ++yyline;
    yycolumn = 1U;
    BEGIN(INITIAL);
}
<slcomment>.            ;

"/*" {
    startTok = yylval;
    startTok.text.token = MLCOMMENT;
    startLoc = yylloc;
    BEGIN(mlcomment);
}
<mlcomment>"*/" {
    startTok.text.len = yyoffset - startTok.text.from;
    startLoc.last_line = yylloc.last_line;
    startLoc.last_column = yylloc.last_column;
    tb->addPostponed(startTok.text, startLoc);

    BEGIN(INITIAL);
}
<mlcomment>.            ;

"..."                   { TOKEN(DOTS); }

"("|")"|";"|"{"|"}"|"["|"]"|"."|","|"?"|":"|"&"|"|"|"^"|"*"|"/"|"%"|"+"|"-"|"~"|"!"|"<"|">"|"=" {
    TOKEN(yytext[0]);
}

.                       { yyerror("Unknown token"); }
