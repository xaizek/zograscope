// Copyright (C) 2022 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "ts/bash/TSBashLanguage.hpp"

#include <cassert>

#include "ts/bash/TSBashSType.hpp"
#include "ts/TSTransformer.hpp"
#include "TreeBuilder.hpp"
#include "mtypes.hpp"
#include "tree.hpp"
#include "types.hpp"

using namespace tsbash;

extern "C" const TSLanguage *tree_sitter_bash(void);

TsBashLanguage::TsBashLanguage() : tsLanguage(*tree_sitter_bash())
{
    stypes = {
        { "separator", +TSBashSType::Separator },
        { "comment", +TSBashSType::Comment },

        { "program", +TSBashSType::Program },

        { "command", +TSBashSType::Command },
        { "declaration_command", +TSBashSType::DeclarationCommand },
        { "negated_command", +TSBashSType::NegatedCommand },
        { "test_command", +TSBashSType::TestCommand },
        { "unset_command", +TSBashSType::UnsetCommand },

        { "_statement", +TSBashSType::Statement },
        { "c_style_for_statement", +TSBashSType::CStyleForStatement },
        { "case_statement", +TSBashSType::CaseStatement },
        { "compound_statement", +TSBashSType::CompoundStatement },
        { "for_statement", +TSBashSType::ForStatement },
        { "if_statement", +TSBashSType::IfStatement },
        { "while_statement", +TSBashSType::WhileStatement },

        { "variable_assignment", +TSBashSType::VariableAssignment },

        { "_expression", +TSBashSType::Expression },
        { "_primary_expression", +TSBashSType::PrimaryExpression },
        { "array", +TSBashSType::Array },
        { "binary_expression", +TSBashSType::BinaryExpression },
        { "case_item", +TSBashSType::CaseItem },
        { "command_name", +TSBashSType::CommandName },
        { "command_substitution", +TSBashSType::CommandSubstitution },
        { "concatenation", +TSBashSType::Concatenation },
        { "do_group", +TSBashSType::DoGroup },
        { "elif_clause", +TSBashSType::ElifClause },
        { "else_clause", +TSBashSType::ElseClause },
        { "expansion", +TSBashSType::Expansion },
        { "expansion_flags", +TSBashSType::ExpansionFlags },
        { "file_redirect", +TSBashSType::FileRedirect },
        { "function_definition", +TSBashSType::FunctionDefinition },
        { "heredoc_body", +TSBashSType::HeredocBody },
        { "heredoc_redirect", +TSBashSType::HeredocRedirect },
        { "herestring_redirect", +TSBashSType::HerestringRedirect },
        { "list", +TSBashSType::List },
        { "line_continuation", +TSBashSType::LineContinuation },
        { "parenthesized_expression", +TSBashSType::ParenthesizedExpression },
        { "pipeline", +TSBashSType::Pipeline },
        { "postfix_expression", +TSBashSType::PostfixExpression },
        { "process_substitution", +TSBashSType::ProcessSubstitution },
        { "redirected_statement", +TSBashSType::RedirectedStatement },
        { "simple_expansion", +TSBashSType::SimpleExpansion },
        { "string", +TSBashSType::String },
        { "string_content", +TSBashSType::StringContent },
        { "string_expansion", +TSBashSType::StringExpansion },
        { "subscript", +TSBashSType::Subscript },
        { "subshell", +TSBashSType::Subshell },
        { "ternary_expression", +TSBashSType::TernaryExpression },
        { "unary_expression", +TSBashSType::UnaryExpression },
        { "word", +TSBashSType::Word },
        { "ansii_c_string", +TSBashSType::AnsiiCString },
        { "comment", +TSBashSType::Comment },
        { "file_descriptor", +TSBashSType::FileDescriptor },
        { "heredoc_start", +TSBashSType::HeredocStart },
        { "raw_string", +TSBashSType::RawString },
        { "regex", +TSBashSType::Regex },
        { "special_variable_name", +TSBashSType::SpecialVariableName },
        { "test_operator", +TSBashSType::TestOperator },
        { "variable_name", +TSBashSType::VariableName },
    };

    types = {
        { "comment", Type::Comments },
        { "command_name", Type::Directives },

        { "special_variable_name", Type::UserTypes },
        { "variable_name", Type::UserTypes },
        { "$", Type::UserTypes },

        { "ansii_c_string", Type::StrConstants },
        { "raw_string", Type::StrConstants },
        { "string_content", Type::StrConstants },
        { "\"", Type::StrConstants },

        { "test_operator", Type::Operators },

        { "case", Type::Keywords },
        { "declare", Type::Keywords },
        { "do", Type::Keywords },
        { "done", Type::Keywords },
        { "elif", Type::Keywords },
        { "else", Type::Keywords },
        { "esac", Type::Keywords },
        { "export", Type::Keywords },
        { "fi", Type::Keywords },
        { "for", Type::Keywords },
        { "function", Type::Keywords },
        { "if", Type::Keywords },
        { "in", Type::Keywords },
        { "local", Type::Keywords },
        { "readonly", Type::Keywords },
        { "select", Type::Keywords },
        { "then", Type::Keywords },
        { "typeset", Type::Keywords },
        { "unset", Type::Keywords },
        { "unsetenv", Type::Keywords },
        { "until", Type::Keywords },
        { "while", Type::Keywords },

        { "&>", Type::Specifiers },
        { "&>>", Type::Specifiers },
        { ">", Type::Specifiers },
        { ">&", Type::Specifiers },
        { ">(", Type::Specifiers },
        { ">>", Type::Specifiers },
        { ">|", Type::Specifiers },

        { "==", Type::Comparisons },
        { "!=", Type::Comparisons },
        { "=~", Type::Comparisons },
        { "<=", Type::Comparisons },
        { ">=", Type::Comparisons },

        { "|", Type::Operators },
        { "|&", Type::Operators },
        { "||", Type::Operators },
        { "&&", Type::Operators },
        { "+", Type::Operators },
        { "++", Type::Operators },
        { "-", Type::Operators },
        { "--", Type::Operators },

        { "[", Type::LeftBrackets },
        { "]", Type::RightBrackets },

        { "[[", Type::LeftBrackets },
        { "]]", Type::RightBrackets },

        { "<(", Type::LeftBrackets },
        { "$(", Type::LeftBrackets },
        { "${", Type::LeftBrackets },
        { "(", Type::LeftBrackets },
        { ")", Type::RightBrackets },

        { "((", Type::LeftBrackets },
        { "))", Type::RightBrackets },

        { "{", Type::LeftBrackets },
        { "}", Type::RightBrackets },

        { "=", Type::Assignments },
        { "+=", Type::Assignments },
        { "-=", Type::Assignments },
    };

    // Unused: "!", "#", "%", "&", "/", ":", ":-", ":?", ";", ";&", ";;", ";;&",
    //         "<", "<&", "<<", "<<-", "<<<", "?", "`"

    badNodes = { "\n" };
}

Type
TsBashLanguage::mapToken(int token) const
{ return static_cast<Type>(token); }

TreeBuilder
TsBashLanguage::parse(const std::string &contents,
                      const std::string &/*fileName*/, int tabWidth, bool debug,
                      cpp17::pmr::monolithic &mr) const
{
    TreeBuilder tb(mr);
    TSTransformer t(contents, tsLanguage, tb, stypes, types, badNodes, tabWidth,
                    debug);
    t.transform();

    return tb;
}

bool
TsBashLanguage::isTravellingNode(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::hasFixedStructure(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::canBeFlattened(const Node */*parent*/, const Node */*child*/,
                               int level) const
{ return (level >= 3); }

bool
TsBashLanguage::isUnmovable(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::isContainer(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::isDiffable(const Node *x) const
{ return Language::isDiffable(x); }

bool
TsBashLanguage::isStructural(const Node *x) const
{
    return Language::isStructural(x)
        || -x->stype == TSBashSType::LineContinuation;
}

bool
TsBashLanguage::isEolContinuation(const Node *x) const
{ return (-x->stype == TSBashSType::LineContinuation); }

bool
TsBashLanguage::alwaysMatches(const Node *x) const
{ return (-x->stype == TSBashSType::Program); }

bool
TsBashLanguage::isPseudoParameter(const Node */*x*/) const
{ return false; }

bool
TsBashLanguage::shouldSplice(SType parent, const Node *childNode) const
{
    return -parent == TSBashSType::FunctionDefinition
        && -childNode->stype == TSBashSType::CompoundStatement;
}

bool
TsBashLanguage::isValueNode(SType /*stype*/) const
{ return false; }

bool
TsBashLanguage::isLayerBreak(SType /*parent*/, SType stype) const
{
    return isValueNode(stype)
        || -stype == TSBashSType::Command
        || -stype == TSBashSType::DeclarationCommand
        || -stype == TSBashSType::NegatedCommand
        || -stype == TSBashSType::TestCommand
        || -stype == TSBashSType::UnsetCommand
        || -stype == TSBashSType::Statement
        || -stype == TSBashSType::CStyleForStatement
        || -stype == TSBashSType::CaseStatement
        || -stype == TSBashSType::CompoundStatement
        || -stype == TSBashSType::ForStatement
        || -stype == TSBashSType::IfStatement
        || -stype == TSBashSType::WhileStatement
        || -stype == TSBashSType::VariableAssignment
        || -stype == TSBashSType::FunctionDefinition;
}

bool
TsBashLanguage::shouldDropLeadingWS(SType /*stype*/) const
{ return false; }

bool
TsBashLanguage::isSatellite(SType stype) const
{ return (-stype == TSBashSType::Separator); }

MType
TsBashLanguage::classify(SType stype) const
{
    switch (-stype) {
        case TSBashSType::Comment:
            return MType::Comment;

        case TSBashSType::DeclarationCommand:
            return MType::Declaration;

        case TSBashSType::Statement:
        case TSBashSType::CStyleForStatement:
        case TSBashSType::CaseStatement:
        case TSBashSType::CompoundStatement:
        case TSBashSType::ForStatement:
        case TSBashSType::IfStatement:
        case TSBashSType::WhileStatement:
            return MType::Statement;

        case TSBashSType::FunctionDefinition:
            return MType::Function;

        default:
            return MType::Other;
    }
}

const char *
TsBashLanguage::toString(SType stype) const
{
#define CASE(item) case TSBashSType::item: return "TSBashSType::" #item

    switch (-stype) {
        CASE(None);

        CASE(Separator);
        CASE(Comment);

        CASE(Program);

        CASE(Command);
        CASE(DeclarationCommand);
        CASE(NegatedCommand);
        CASE(TestCommand);
        CASE(UnsetCommand);

        CASE(Statement);
        CASE(CStyleForStatement);
        CASE(CaseStatement);
        CASE(CompoundStatement);
        CASE(ForStatement);
        CASE(IfStatement);
        CASE(WhileStatement);

        CASE(VariableAssignment);

        CASE(Expression);
        CASE(PrimaryExpression);
        CASE(Array);
        CASE(BinaryExpression);
        CASE(CaseItem);
        CASE(CommandName);
        CASE(CommandSubstitution);
        CASE(Concatenation);
        CASE(DoGroup);
        CASE(ElifClause);
        CASE(ElseClause);
        CASE(Expansion);
        CASE(ExpansionFlags);
        CASE(FileRedirect);
        CASE(FunctionDefinition);
        CASE(HeredocBody);
        CASE(HeredocRedirect);
        CASE(HerestringRedirect);
        CASE(List);
        CASE(LineContinuation);
        CASE(ParenthesizedExpression);
        CASE(Pipeline);
        CASE(PostfixExpression);
        CASE(ProcessSubstitution);
        CASE(RedirectedStatement);
        CASE(SimpleExpansion);
        CASE(String);
        CASE(StringContent);
        CASE(StringExpansion);
        CASE(Subscript);
        CASE(Subshell);
        CASE(TernaryExpression);
        CASE(UnaryExpression);
        CASE(Word);
        CASE(AnsiiCString);
        CASE(FileDescriptor);
        CASE(HeredocStart);
        CASE(RawString);
        CASE(Regex);
        CASE(SpecialVariableName);
        CASE(TestOperator);
        CASE(VariableName);
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";

#undef CASE
}
