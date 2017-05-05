#include "types.hpp"

#include "tc.tab.hpp"

static Type *
tokenMap()
{
    static Type map[NTOKENS];
    map[ID] = Type::Identifiers;

    map[BREAK]    = Type::Jumps;
    map[CONTINUE] = Type::Jumps;
    map[GOTO]     = Type::Jumps;

    map[_ALIGNAS]      = Type::Specifiers;
    map[EXTERN]        = Type::Specifiers;
    map[STATIC]        = Type::Specifiers;
    map[_THREAD_LOCAL] = Type::Specifiers;
    map[_ATOMIC]       = Type::Specifiers;
    map[AUTO]          = Type::Specifiers;
    map[REGISTER]      = Type::Specifiers;
    map[INLINE]        = Type::Specifiers;
    map[_NORETURN]     = Type::Specifiers;
    map[CONST]         = Type::Specifiers;
    map[VOLATILE]      = Type::Specifiers;
    map[RESTRICT]      = Type::Specifiers;

    map[VOID]     = Type::Types;
    map[CHAR]     = Type::Types;
    map[SHORT]    = Type::Types;
    map[INT]      = Type::Types;
    map[LONG]     = Type::Types;
    map[FLOAT]    = Type::Types;
    map[DOUBLE]   = Type::Types;
    map[SIGNED]   = Type::Types;
    map[UNSIGNED] = Type::Types;
    map[_BOOL]    = Type::Types;
    map[_COMPLEX] = Type::Types;

    map['('] = Type::LeftBrackets;
    map['{'] = Type::LeftBrackets;
    map['['] = Type::LeftBrackets;

    map[')'] = Type::RightBrackets;
    map['}'] = Type::RightBrackets;
    map[']'] = Type::RightBrackets;

    map[LTE_OP] = Type::Comparisons;
    map[GTE_OP] = Type::Comparisons;
    map[EQ_OP]  = Type::Comparisons;
    map[NE_OP]  = Type::Comparisons;
    map['<']    = Type::Comparisons;
    map['>']    = Type::Comparisons;

    map[INC_OP] = Type::Operators;
    map[DEC_OP] = Type::Operators;
    map[LSH_OP] = Type::Operators;
    map[RSH_OP] = Type::Operators;
    map[AND_OP] = Type::Operators;
    map[OR_OP]  = Type::Operators;
    map['&']    = Type::Operators;
    map['|']    = Type::Operators;
    map['^']    = Type::Operators;
    map['*']    = Type::Operators;
    map['/']    = Type::Operators;
    map['%']    = Type::Operators;
    map['+']    = Type::Operators;
    map['-']    = Type::Operators;
    map['~']    = Type::Operators;
    map['!']    = Type::Operators;

    map['=']         = Type::Assignments;
    map[TIMESEQ_OP]  = Type::Assignments;
    map[DIVEQ_OP]    = Type::Assignments;
    map[MODEQ_OP]    = Type::Assignments;
    map[PLUSEQ_OP]   = Type::Assignments;
    map[MINUSEQ_OP]  = Type::Assignments;
    map[LSHIFTEQ_OP] = Type::Assignments;
    map[RSHIFTEQ_OP] = Type::Assignments;
    map[ANDEQ_OP]    = Type::Assignments;
    map[XOREQ_OP]    = Type::Assignments;
    map[OREQ_OP]     = Type::Assignments;

    map[DIRECTIVE] = Type::Directives;

    map[SLCOMMENT] = Type::Comments;
    map[MLCOMMENT] = Type::Comments;

    map[SLIT]           = Type::NonInterchangeable;
    map[ICONST]         = Type::NonInterchangeable;
    map[FCONST]         = Type::NonInterchangeable;
    map[CHCONST]        = Type::NonInterchangeable;
    map[DEFAULT]        = Type::NonInterchangeable;
    map[RETURN]         = Type::NonInterchangeable;
    map[SIZEOF]         = Type::NonInterchangeable;
    map[_ALIGNOF]       = Type::NonInterchangeable;
    map[_GENERIC]       = Type::NonInterchangeable;
    map[DOTS]           = Type::NonInterchangeable;
    map[_STATIC_ASSERT] = Type::NonInterchangeable;
    map[IF]             = Type::NonInterchangeable;
    map[ELSE]           = Type::NonInterchangeable;
    map[SWITCH]         = Type::NonInterchangeable;
    map[WHILE]          = Type::NonInterchangeable;
    map[DO]             = Type::NonInterchangeable;
    map[FOR]            = Type::NonInterchangeable;
    map[CASE]           = Type::NonInterchangeable;
    map[TYPEDEF]        = Type::NonInterchangeable;
    map[STRUCT]         = Type::NonInterchangeable;
    map[UNION]          = Type::NonInterchangeable;
    map[ENUM]           = Type::NonInterchangeable;
    map['?']            = Type::NonInterchangeable;
    map[':']            = Type::NonInterchangeable;
    map[';']            = Type::NonInterchangeable;
    map['.']            = Type::NonInterchangeable;
    map[',']            = Type::NonInterchangeable;
    map[ARR_OP]         = Type::NonInterchangeable;
    return map;
};

Type
tokenToType(int token)
{
    static Type *map = tokenMap();

    return map[token];
}
