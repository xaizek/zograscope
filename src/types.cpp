#include "types.hpp"

#include <cassert>

#include <ostream>

#include "c11-parser.hpp"

std::ostream &
operator<<(std::ostream &os, Type type)
{
    switch (type) {
        case Type::Virtual:            return (os << "Virtual");
        case Type::Functions:          return (os << "Functions");
        case Type::UserTypes:          return (os << "UserTypes");
        case Type::Identifiers:        return (os << "Identifiers");
        case Type::Jumps:              return (os << "Jumps");
        case Type::Specifiers:         return (os << "Specifiers");
        case Type::Types:              return (os << "Types");
        case Type::LeftBrackets:       return (os << "LeftBrackets");
        case Type::RightBrackets:      return (os << "RightBrackets");
        case Type::Comparisons:        return (os << "Comparisons");
        case Type::Operators:          return (os << "Operators");
        case Type::Assignments:        return (os << "Assignments");
        case Type::Directives:         return (os << "Directives");
        case Type::Comments:           return (os << "Comments");
        case Type::NonInterchangeable: return (os << "NonInterchangeable");
        case Type::Constants:          return (os << "Constants");
        case Type::Keywords:           return (os << "Keywords");
        case Type::Other:              return (os << "Other");
    }

    assert("Unhandled enumeration item");
    return (os << "<UNKNOWN>");
}

static Type *
tokenMap()
{
    static Type map[NTOKENS];

    map[FUNCTION] = Type::Functions;

    map[TYPENAME] = Type::UserTypes;

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
    map[TYPEDEF]       = Type::Specifiers;

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

    map[SLIT]    = Type::Constants;
    map[ICONST]  = Type::Constants;
    map[FCONST]  = Type::Constants;
    map[CHCONST] = Type::Constants;

    map[DEFAULT]        = Type::Keywords;
    map[RETURN]         = Type::Keywords;
    map[SIZEOF]         = Type::Keywords;
    map[_ALIGNOF]       = Type::Keywords;
    map[_GENERIC]       = Type::Keywords;
    map[DOTS]           = Type::Keywords;
    map[_STATIC_ASSERT] = Type::Keywords;
    map[IF]             = Type::Keywords;
    map[ELSE]           = Type::Keywords;
    map[SWITCH]         = Type::Keywords;
    map[WHILE]          = Type::Keywords;
    map[DO]             = Type::Keywords;
    map[FOR]            = Type::Keywords;
    map[CASE]           = Type::Keywords;
    map[STRUCT]         = Type::Keywords;
    map[UNION]          = Type::Keywords;
    map[ENUM]           = Type::Keywords;
    map[__ASM__]        = Type::Keywords;
    map[__VOLATILE__]   = Type::Keywords;

    map['?']    = Type::Other;
    map[':']    = Type::Other;
    map[';']    = Type::Other;
    map['.']    = Type::Other;
    map[',']    = Type::Other;
    map[ARR_OP] = Type::Other;

    return map;
};

Type
tokenToType(int token)
{
    static Type *map = tokenMap();

    return map[token];
}
