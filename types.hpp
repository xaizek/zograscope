#ifndef TYPES_HPP__
#define TYPES_HPP__

enum class Type
{
    Virtual,

    // ID
    Identifiers,

    // BREAK
    // CONTINUE
    // GOTO
    Jumps,

    // _ALIGNAS
    // EXTERN
    // STATIC
    // _THREAD_LOCAL
    // _ATOMIC
    // AUTO
    // REGISTER
    // INLINE
    // _NORETURN
    // CONST
    // VOLATILE
    // RESTRICT
    Specifiers,

    // VOID
    // CHAR
    // SHORT
    // INT
    // LONG
    // FLOAT
    // DOUBLE
    // SIGNED
    // UNSIGNED
    // _BOOL
    // _COMPLEX
    Types,

    // '('
    // '{'
    // '['
    LeftBrackets,

    // ')'
    // '}'
    // ']'
    RightBrackets,

    // LTE_OP
    // GTE_OP
    // EQ_OP
    // NE_OP
    // '<'
    // '>'
    Comparisons,

    // INC_OP
    // DEC_OP
    // LSH_OP
    // RSH_OP
    // AND_OP
    // OR_OP
    // '&'
    // '|'
    // '^'
    // '*'
    // '/'
    // '%'
    // '+'
    // '-'
    // '~'
    // '!'
    Operators,

    // '='
    // TIMESEQ_OP
    // DIVEQ_OP
    // MODEQ_OP
    // PLUSEQ_OP
    // MINUSEQ_OP
    // LSHIFTEQ_OP
    // RSHIFTEQ_OP
    // ANDEQ_OP
    // XOREQ_OP
    // OREQ_OP
    Assignments,

    // DIRECTIVE
    Directives,

    // SLCOMMENT
    // MLCOMMENT
    Comments,

    // SLIT
    // ICONST
    // FCONST
    // CHCONST
    // DEFAULT
    // RETURN
    // SIZEOF
    // _ALIGNOF
    // _GENERIC
    // DOTS
    // _STATIC_ASSERT
    // IF
    // ELSE
    // SWITCH
    // WHILE
    // DO
    // FOR
    // CASE
    // TYPEDEF
    // STRUCT
    // UNION
    // ENUM
    // '?'
    // ':'
    // ';'
    // '.'
    // ','
    // ARR_OP
    NonInterchangeable,
};

Type tokenToType(int token);

#endif // TYPES_HPP__
