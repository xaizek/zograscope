%{

#include <iostream>
#define YY_DECL extern "C" int yylex()

%}

%%

[ \t\n]                 ;
[0-9]+                  { std::cout << "Found an integer:" << yytext << std::endl; }
[a-zA-Z][0-9a-zA-Z]*    { std::cout << "Found a keyword: " << yytext << std::endl; }
\(                      { std::cout << "Found (: " << yytext << std::endl; }
\)                      { std::cout << "Found ): " << yytext << std::endl; }
;                       { std::cout << "Found ;: " << yytext << std::endl; }
\{                      { std::cout << "Found {: " << yytext << std::endl; }
\}                      { std::cout << "Found }: " << yytext << std::endl; }

%%

int
main()
{
    // lex through the input:
    yylex();
}
