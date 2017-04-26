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

%}

%%

[ \t]                   ;
\n                      { ++yyline; yycolumn = 1U; }
[0-9]+                  { return NUM; }
return                  { return RETURN; }
[a-zA-Z][0-9a-zA-Z]*    { return ID; }

"("|")"|";"|"{"|"}" {
    return yytext[0];
}
