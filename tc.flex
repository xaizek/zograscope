%{

#include <iostream>
#include <string>
#define YY_DECL extern "C" int yylex()

#include "tc.tab.hpp"

%}

%%

[ \t\n]                 ;
[0-9]+                  { std::cout << "Found an integer: " << yytext << std::endl; return NUM; }
return                  { return RETURN; }
[a-zA-Z][0-9a-zA-Z]*    { std::cout << "Found an id: " << yytext << std::endl; return ID; }

"("|")"|";"|"{"|"}" {
    return yytext[0];
}
