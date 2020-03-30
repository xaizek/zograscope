// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#include "srcml/cxx/SrcmlCxxLanguage.hpp"

#include <cassert>

#include "srcml/cxx/SrcmlCxxSType.hpp"
#include "srcml/SrcmlTransformer.hpp"
#include "TreeBuilder.hpp"
#include "mtypes.hpp"
#include "tree.hpp"
#include "types.hpp"

using namespace srcmlcxx;

static void postProcessTree(PNode *node, TreeBuilder &tb,
                            const std::string &contents);
static bool isConditional(SType stype);
static void postProcessIf(PNode *node, TreeBuilder &tb,
                          const std::string &contents);
static void postProcessIfStmt(PNode *node, TreeBuilder &tb,
                              const std::string &contents);
static void postProcessBlock(PNode *node, TreeBuilder &tb,
                             const std::string &contents);
static void postProcessEnum(PNode *node, TreeBuilder &tb,
                            const std::string &contents);
static void postProcessEnumClass(PNode *node, TreeBuilder &tb,
                                 const std::string &contents);
static void postProcessParameterList(PNode *node, TreeBuilder &tb,
                                     const std::string &contents);
static bool breakLeaf(PNode *node, TreeBuilder &tb,
                      const std::string &contents, char left, char right,
                      SrcmlCxxSType newChild);
static void takeWord(PNode *node, const PNode *of, int len);
static void skipWord(PNode *node, const PNode *of, int len,
                     const std::string &contents);
static void dropLeadingWS(PNode *node, const std::string &contents);
static void postProcessConditional(PNode *node, TreeBuilder &tb,
                                   const std::string &contents);

SrcmlCxxLanguage::SrcmlCxxLanguage()
{
    map["separator"] = +SrcmlCxxSType::Separator;

    map["argument"]      = +SrcmlCxxSType::Argument;
    map["comment"]       = +SrcmlCxxSType::Comment;
    map["cpp:endif"]     = +SrcmlCxxSType::CppEndif;
    map["cpp:literal"]   = +SrcmlCxxSType::CppLiteral;
    map["enum_decl"]     = +SrcmlCxxSType::EnumDecl;
    map["escape"]        = +SrcmlCxxSType::Escape;
    map["expr_stmt"]     = +SrcmlCxxSType::ExprStmt;
    map["incr"]          = +SrcmlCxxSType::Incr;
    map["macro"]         = +SrcmlCxxSType::Macro;
    map["macro-list"]    = +SrcmlCxxSType::MacroList;
    map["ref_qualifier"] = +SrcmlCxxSType::RefQualifier;
    map["unit"]          = +SrcmlCxxSType::Unit;

    map["cpp:define"]    = +SrcmlCxxSType::CppDefine;
    map["cpp:directive"] = +SrcmlCxxSType::CppDirective;
    map["cpp:elif"]      = +SrcmlCxxSType::CppElif;
    map["cpp:else"]      = +SrcmlCxxSType::CppElse;
    map["cpp:empty"]     = +SrcmlCxxSType::CppEmpty;
    map["cpp:error"]     = +SrcmlCxxSType::CppError;
    map["cpp:file"]      = +SrcmlCxxSType::CppFile;
    map["cpp:if"]        = +SrcmlCxxSType::CppIf;
    map["cpp:ifdef"]     = +SrcmlCxxSType::CppIfdef;
    map["cpp:ifndef"]    = +SrcmlCxxSType::CppIfndef;
    map["cpp:include"]   = +SrcmlCxxSType::CppInclude;
    map["cpp:line"]      = +SrcmlCxxSType::CppLine;
    map["cpp:macro"]     = +SrcmlCxxSType::CppMacro;
    map["cpp:number"]    = +SrcmlCxxSType::CppNumber;
    map["cpp:pragma"]    = +SrcmlCxxSType::CppPragma;
    map["cpp:undef"]     = +SrcmlCxxSType::CppUndef;
    map["cpp:value"]     = +SrcmlCxxSType::CppValue;
    map["cpp:warning"]   = +SrcmlCxxSType::CppWarning;

    map["alignas"]          = +SrcmlCxxSType::Alignas;
    map["alignof"]          = +SrcmlCxxSType::Alignof;
    map["argument_list"]    = +SrcmlCxxSType::ArgumentList;
    map["asm"]              = +SrcmlCxxSType::Asm;
    map["assert"]           = +SrcmlCxxSType::Assert;
    map["attribute"]        = +SrcmlCxxSType::Attribute;
    map["block"]            = +SrcmlCxxSType::Block;
    map["block_content"]    = +SrcmlCxxSType::BlockContent;
    map["break"]            = +SrcmlCxxSType::Break;
    map["call"]             = +SrcmlCxxSType::Call;
    map["capture"]          = +SrcmlCxxSType::Capture;
    map["case"]             = +SrcmlCxxSType::Case;
    map["cast"]             = +SrcmlCxxSType::Cast;
    map["catch"]            = +SrcmlCxxSType::Catch;
    map["class"]            = +SrcmlCxxSType::Class;
    map["class_decl"]       = +SrcmlCxxSType::ClassDecl;
    map["condition"]        = +SrcmlCxxSType::Condition;
    map["constructor"]      = +SrcmlCxxSType::Constructor;
    map["constructor_decl"] = +SrcmlCxxSType::ConstructorDecl;
    map["continue"]         = +SrcmlCxxSType::Continue;
    map["control"]          = +SrcmlCxxSType::Control;
    map["decl"]             = +SrcmlCxxSType::Decl;
    map["decl_stmt"]        = +SrcmlCxxSType::DeclStmt;
    map["decltype"]         = +SrcmlCxxSType::Decltype;
    map["default"]          = +SrcmlCxxSType::Default;
    map["destructor"]       = +SrcmlCxxSType::Destructor;
    map["destructor_decl"]  = +SrcmlCxxSType::DestructorDecl;
    map["do"]               = +SrcmlCxxSType::Do;
    map["else"]             = +SrcmlCxxSType::Else;
    map["elseif"]           = +SrcmlCxxSType::Elseif;
    map["empty_stmt"]       = +SrcmlCxxSType::EmptyStmt;
    map["enum"]             = +SrcmlCxxSType::Enum;
    map["expr"]             = +SrcmlCxxSType::Expr;
    map["extern"]           = +SrcmlCxxSType::Extern;
    map["for"]              = +SrcmlCxxSType::For;
    map["friend"]           = +SrcmlCxxSType::Friend;
    map["function"]         = +SrcmlCxxSType::Function;
    map["function_decl"]    = +SrcmlCxxSType::FunctionDecl;
    map["goto"]             = +SrcmlCxxSType::Goto;
    map["if"]               = +SrcmlCxxSType::If;
    map["if_stmt"]          = +SrcmlCxxSType::IfStmt;
    map["index"]            = +SrcmlCxxSType::Index;
    map["init"]             = +SrcmlCxxSType::Init;
    map["label"]            = +SrcmlCxxSType::Label;
    map["lambda"]           = +SrcmlCxxSType::Lambda;
    map["literal"]          = +SrcmlCxxSType::Literal;
    map["member_init_list"] = +SrcmlCxxSType::MemberInitList;
    map["member_list"]      = +SrcmlCxxSType::MemberList;
    map["modifier"]         = +SrcmlCxxSType::Modifier;
    map["name"]             = +SrcmlCxxSType::Name;
    map["namespace"]        = +SrcmlCxxSType::Namespace;
    map["noexcept"]         = +SrcmlCxxSType::Noexcept;
    map["operator"]         = +SrcmlCxxSType::Operator;
    map["param"]            = +SrcmlCxxSType::Param;
    map["parameter"]        = +SrcmlCxxSType::Parameter;
    map["parameter_list"]   = +SrcmlCxxSType::ParameterList;
    map["private"]          = +SrcmlCxxSType::Private;
    map["protected"]        = +SrcmlCxxSType::Protected;
    map["public"]           = +SrcmlCxxSType::Public;
    map["range"]            = +SrcmlCxxSType::Range;
    map["return"]           = +SrcmlCxxSType::Return;
    map["sizeof"]           = +SrcmlCxxSType::Sizeof;
    map["specifier"]        = +SrcmlCxxSType::Specifier;
    map["struct"]           = +SrcmlCxxSType::Struct;
    map["struct_decl"]      = +SrcmlCxxSType::StructDecl;
    map["super"]            = +SrcmlCxxSType::Super;
    map["super_list"]       = +SrcmlCxxSType::SuperList;
    map["switch"]           = +SrcmlCxxSType::Switch;
    map["template"]         = +SrcmlCxxSType::Template;
    map["ternary"]          = +SrcmlCxxSType::Ternary;
    map["then"]             = +SrcmlCxxSType::Then;
    map["throw"]            = +SrcmlCxxSType::Throw;
    map["try"]              = +SrcmlCxxSType::Try;
    map["type"]             = +SrcmlCxxSType::Type;
    map["typedef"]          = +SrcmlCxxSType::Typedef;
    map["typeid"]           = +SrcmlCxxSType::Typeid;
    map["typename"]         = +SrcmlCxxSType::Typename;
    map["union"]            = +SrcmlCxxSType::Union;
    map["union_decl"]       = +SrcmlCxxSType::UnionDecl;
    map["using"]            = +SrcmlCxxSType::Using;
    map["while"]            = +SrcmlCxxSType::While;

    keywords.insert({
        "alignas", "alignof", "asm", "auto", "bool", "break", "case", "catch",
        "char", "char16_t", "char32_t", "class", "const", "constexpr",
        "const_cast", "continue", "decltype", "default", "delete", "do",
        "double", "dynamic_cast", "else", "enum", "explicit", "export",
        "extern", "false", "float", "for", "friend", "goto", "if", "inline",
        "int", "long", "mutable", "namespace", "new", "noexcept", "nullptr",
        "operator", "private", "protected", "public", "register",
        "reinterpret_cast", "return", "short", "signed", "sizeof", "static",
        "static_assert", "static_cast", "struct", "switch", "template", "this",
        "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
        "while",
    });
}

Type
SrcmlCxxLanguage::mapToken(int token) const
{
    return static_cast<Type>(token);
}

TreeBuilder
SrcmlCxxLanguage::parse(const std::string &contents,
                        const std::string &fileName, bool /*debug*/,
                        cpp17::pmr::monolithic &mr) const
{
    TreeBuilder tb(mr);
    SrcmlTransformer(contents, fileName, tb, "C++", map, keywords).transform();

    postProcessTree(tb.getRoot(), tb, contents);

    return tb;
}

// Rewrites tree to be more diff-friendly.
static void
postProcessTree(PNode *node, TreeBuilder &tb, const std::string &contents)
{
    if (node->stype == +SrcmlCxxSType::If) {
        postProcessIf(node, tb, contents);
    }

    if (node->stype == +SrcmlCxxSType::IfStmt) {
        postProcessIfStmt(node, tb, contents);
    }

    if (node->stype == +SrcmlCxxSType::Block) {
        postProcessBlock(node, tb, contents);
    }

    if (node->stype == +SrcmlCxxSType::Enum) {
        postProcessEnum(node, tb, contents);
        postProcessEnumClass(node, tb, contents);
    }

    if (node->stype == +SrcmlCxxSType::ParameterList) {
        postProcessParameterList(node, tb, contents);
    }

    if (isConditional(node->stype)) {
        postProcessConditional(node, tb, contents);
    }

    for (PNode *child : node->children) {
        postProcessTree(child, tb, contents);
    }
}

// Checks whether node corresponds to one of control flow statements that
// contain conditions.
static bool
isConditional(SType stype)
{
    return stype == +SrcmlCxxSType::If
        || stype == +SrcmlCxxSType::Switch
        || stype == +SrcmlCxxSType::While
        || stype == +SrcmlCxxSType::Do;
}

// Rewrites if nodes to be more diff-friendly.
static void
postProcessIf(PNode *node, TreeBuilder &/*tb*/, const std::string &/*contents*/)
{
    // Move else-if node to respective if-statement.
    while (node->children.back()->stype == +SrcmlCxxSType::Elseif) {
        auto n = node->children.size();
        PNode *child = node->children[n - 1U];
        PNode *prev = node->children[n - 2U];

        if (prev->stype != +SrcmlCxxSType::Elseif) {
            break;
        }

        node->children.pop_back();
        prev->children[1]->children.push_back(child);
    }
}

// Rewrites if statement nodes to be more diff-friendly.
static void
postProcessIfStmt(PNode *node, TreeBuilder &tb, const std::string &contents)
{
    // Move else or else-if nodes to respective if-statement splitting "else-if"
    // into else and if parts.
    while (node->children.size() > 1) {
        auto n = node->children.size();
        PNode *prev = node->children[n - 2U];
        PNode *tailNode = node->children[n - 1U];
        node->children.pop_back();

        if (tailNode->stype == +SrcmlCxxSType::Else) {
            // Else nodes just need to be moved.
            prev->children.push_back(tailNode);
            continue;
        }

        PNode *elseIfKw = tailNode->children[0];

        PNode *elseKw = tb.addNode();
        elseKw->stype = +SrcmlCxxSType::Separator;
        elseKw->value.token = static_cast<int>(Type::Keywords);
        // Take "else" part.
        takeWord(elseKw, elseIfKw, 4);

        // Drop "else" prefix and whitespace that follows it.
        elseIfKw->value.token = static_cast<int>(Type::Keywords);
        skipWord(elseIfKw, elseIfKw, 4, contents);

        PNode *newNode = tb.addNode();
        newNode->stype = +SrcmlCxxSType::Elseif;
        newNode->children = { elseKw, tailNode };
        prev->children.push_back(newNode);
    }

    // Replace if-statement node with the if node.
    PNode *ifNode = node->children[0];
    node->children.erase(node->children.cbegin());
    node->children.insert(node->children.cbegin(),
                          ifNode->children.cbegin(), ifNode->children.cend());
    node->stype = ifNode->stype;
}

// Rewrites block nodes to be more diff-friendly.
static void
postProcessBlock(PNode *node, TreeBuilder &tb, const std::string &contents)
{
    // Children: `{` block-content `}`
    if (node->children.size() == 3 &&
        node->children[1]->stype == +SrcmlCxxSType::BlockContent) {
        PNode *content = node->children[1];

        // Empty block-content gets replaces with empty statements node.
        if (content->children.empty()) {
            node->children[1] = tb.addNode();
            node->children[1]->stype = +SrcmlCxxSType::Statements;
            return;
        }

        // Splice children of block-content in its place.
        node->children.erase(++node->children.cbegin());
        node->children.insert(++node->children.cbegin(),
                              content->children.cbegin(),
                              content->children.cend());
    }

    // Children: `{` statement+ `}`.
    if (node->children.size() > 2) {
        PNode *stmts = tb.addNode();
        stmts->stype = +SrcmlCxxSType::Statements;
        stmts->children.assign(++node->children.cbegin(),
                               --node->children.cend());

        node->children.erase(++node->children.cbegin(),
                             --node->children.cend());
        node->children.insert(++node->children.cbegin(), stmts);
        return;
    }

    // Children: `{}` (with any whitespace in between).
    if (breakLeaf(node, tb, contents, '{', '}', SrcmlCxxSType::Statements)) {
        return;
    }

    // Children: statement (possibly in multiple pieces).
    PNode *stmts = tb.addNode();
    stmts->stype = +SrcmlCxxSType::Statements;
    stmts->children = node->children;

    node->children.assign({ stmts });
}

// Rewrites enumeration nodes to be more diff-friendly.  This turns ",\s*}" into
// two separate tokens.
static void
postProcessEnum(PNode *node, TreeBuilder &tb, const std::string &contents)
{
    if (node->children.size() < 2) {
        return;
    }

    PNode *block = node->children[node->children.size() - 2];
    if (block->stype != +SrcmlCxxSType::Block) {
        return;
    }

    if (block->children.empty()) {
        return;
    }

    PNode *tail = block->children.back();
    if (contents[tail->value.from] != ',' ||
        contents[tail->value.from + tail->value.len - 1] != '}') {
        return;
    }

    PNode *comma = tb.addNode();
    comma->stype = +SrcmlCxxSType::Separator;

    // Take "," part.
    takeWord(comma, tail, 1);
    // Drop "," prefix and whitespace that follows it.
    skipWord(tail, tail, 1, contents);

    block->children.insert(block->children.cend() - 1, comma);

    comma->value.token = static_cast<int>(Type::Other);
    tail->value.token = static_cast<int>(Type::RightBrackets);
}

// Rewrites enumeration class nodes to be more diff-friendly.  This breaks
// "enum\s+class" into two separate keyword tokens.
static void
postProcessEnumClass(PNode *node, TreeBuilder &tb, const std::string &contents)
{
    PNode *originalKw = node->children.front();
    if (originalKw->value.len <= 4) {
        return;
    }

    PNode *enumKw = tb.addNode();
    enumKw->stype = +SrcmlCxxSType::Separator;

    // Take "enum" part.
    takeWord(enumKw, originalKw, 4);
    // Drop "enum" prefix and whitespace that follows it.
    skipWord(originalKw, originalKw, 4, contents);

    node->children.insert(node->children.cbegin(), enumKw);

    enumKw->value.token = static_cast<int>(Type::Keywords);
    originalKw->value.token = static_cast<int>(Type::Keywords);
}

// Rewrites parameter list nodes to be more diff-friendly.
static void
postProcessParameterList(PNode *node, TreeBuilder &tb,
                         const std::string &contents)
{
    // Children: `()` (with any whitespace in between).
    if (breakLeaf(node, tb, contents, '(', ')', SrcmlCxxSType::None)) {
        return;
    }
}

// Breaks pairs of glued single character tokens.  Returns `true` if node was
// rewritten.
static bool
breakLeaf(PNode *node, TreeBuilder &tb, const std::string &contents,
          char left, char right, SrcmlCxxSType newChild)
{
    // Children: `<left><right>` (with any whitespace in between).
    if (node->children.size() == 1 &&
        contents[node->children[0]->value.from] == left &&
        contents[node->children[0]->value.from +
                 node->children[0]->value.len - 1] == right) {
        PNode *child = node->children[0];

        PNode *left = tb.addNode();
        left->stype = +SrcmlCxxSType::Separator;
        left->value.token = static_cast<int>(Type::LeftBrackets);
        takeWord(left, child, 1);

        PNode *right = tb.addNode();
        right->stype = +SrcmlCxxSType::Separator;
        right->value.token = static_cast<int>(Type::RightBrackets);
        skipWord(right, child, 1, contents);

        if (newChild == SrcmlCxxSType::None) {
            node->children.assign({ left, right });
        } else {
            PNode *stmts = tb.addNode();
            stmts->stype = +newChild;

            node->children.assign({ left, stmts, right });
        }
        return true;
    }
    return false;
}

// Sets node label to prefix of different node's label.
static void
takeWord(PNode *node, const PNode *of, int len)
{
    assert(static_cast<int>(of->value.len) > len &&
           "Word length is too large.");

    node->value.from = of->value.from;
    node->value.len = len;
    node->line = of->line;
    node->col = of->col;
}

// Sets node label to label of a different node after dropping prefix from it.
static void
skipWord(PNode *node, const PNode *of, int len, const std::string &contents)
{
    assert(static_cast<int>(of->value.len) > len &&
           "Word length is too large.");

    node->value.from = of->value.from + len;
    node->value.len = of->value.len - len;
    node->line = of->line;
    node->col = of->col + len;

    dropLeadingWS(node, contents);
}

// Corrects node data to exclude leading whitespace.
static void
dropLeadingWS(PNode *node, const std::string &contents)
{
    const char *pos = &contents[node->value.from];
    while (node->value.len > 0 && (*pos == '\n' || *pos == ' ')) {
        if (*pos == '\n') {
            ++node->line;
            node->col = 1;
        } else {
            ++node->col;
        }
        ++pos;
        ++node->value.from;
        --node->value.len;
    }
}

// Rewrites control flow statements with conditions to move `(` and `)` one
// level up.
static void
postProcessConditional(PNode *node, TreeBuilder &/*tb*/,
                       const std::string &/*contents*/)
{
    auto pred = [](const PNode *n) {
        return n->stype == +SrcmlCxxSType::Condition;
    };

    auto it = std::find_if(node->children.begin(), node->children.end(), pred);
    if (it == node->children.end()) {
        // The node is incomplete.
        return;
    }

    auto pos = it - node->children.begin();
    PNode *cond = node->children[pos];

    node->children.insert(node->children.begin() + pos + 1,
                          cond->children.back());
    node->children.insert(node->children.begin() + pos,
                          cond->children.front());

    cond->children.erase(cond->children.begin());
    cond->children.erase(--cond->children.end());
}

bool
SrcmlCxxLanguage::isTravellingNode(const Node */*x*/) const
{
    return false;
}

bool
SrcmlCxxLanguage::hasFixedStructure(const Node */*x*/) const
{
    return false;
}

bool
SrcmlCxxLanguage::canBeFlattened(const Node */*parent*/, const Node *child,
                                 int level) const
{
    switch (level) {
        case 0:
        case 1:
        case 2:
            return false;
        default:
            return -child->stype != SrcmlCxxSType::Call
                && -child->stype != SrcmlCxxSType::FunctionDecl
                && -child->stype != SrcmlCxxSType::DeclStmt
                && -child->stype != SrcmlCxxSType::Parameter
                && -child->stype != SrcmlCxxSType::EnumDecl;
    }
}

bool
SrcmlCxxLanguage::isUnmovable(const Node *x) const
{
    return (-x->stype == SrcmlCxxSType::Statements);
}

bool
SrcmlCxxLanguage::isContainer(const Node *x) const
{
    return (-x->stype == SrcmlCxxSType::Statements);
}

bool
SrcmlCxxLanguage::isDiffable(const Node *x) const
{
    return Language::isDiffable(x);
}

bool
SrcmlCxxLanguage::isStructural(const Node *x) const
{
    return Language::isStructural(x)
        || x->label == ","
        || x->label == ";";
}

bool
SrcmlCxxLanguage::isEolContinuation(const Node *x) const
{
    return (x->label == "\\");
}

bool
SrcmlCxxLanguage::alwaysMatches(const Node *x) const
{
    return (-x->stype == SrcmlCxxSType::Unit);
}

bool
SrcmlCxxLanguage::isPseudoParameter(const Node *x) const
{
    return (x->label == "void");
}

bool
SrcmlCxxLanguage::shouldSplice(SType parent, const Node *childNode) const
{
    SrcmlCxxSType child = -childNode->stype;
    if (child == SrcmlCxxSType::Statements) {
        if (-parent == SrcmlCxxSType::Struct ||
            -parent == SrcmlCxxSType::Class ||
            -parent == SrcmlCxxSType::Enum) {
            return true;
        }
    }
    if (child == SrcmlCxxSType::Block) {
        if (-parent == SrcmlCxxSType::Function ||
            -parent == SrcmlCxxSType::Constructor ||
            -parent == SrcmlCxxSType::Destructor ||
            -parent == SrcmlCxxSType::Struct ||
            -parent == SrcmlCxxSType::Class ||
            -parent == SrcmlCxxSType::Union ||
            -parent == SrcmlCxxSType::Enum ||
            -parent == SrcmlCxxSType::If ||
            -parent == SrcmlCxxSType::Then ||
            -parent == SrcmlCxxSType::Else ||
            -parent == SrcmlCxxSType::For ||
            -parent == SrcmlCxxSType::While ||
            -parent == SrcmlCxxSType::Do ||
            -parent == SrcmlCxxSType::Switch) {
            return true;
        }
    }
    if (-parent == SrcmlCxxSType::If && child == SrcmlCxxSType::Then) {
        return true;
    }
    if (-parent == SrcmlCxxSType::Call &&
        child == SrcmlCxxSType::ArgumentList) {
        return true;
    }
    if (child == SrcmlCxxSType::ParameterList ||
        child == SrcmlCxxSType::BlockContent) {
        return true;
    }
    return false;
}

bool
SrcmlCxxLanguage::isValueNode(SType stype) const
{
    return (-stype == SrcmlCxxSType::Condition);
}

bool
SrcmlCxxLanguage::isLayerBreak(SType parent, SType stype) const
{
    return -stype == SrcmlCxxSType::Call
        || -stype == SrcmlCxxSType::Constructor
        || -stype == SrcmlCxxSType::Destructor
        || -stype == SrcmlCxxSType::Function
        || -stype == SrcmlCxxSType::FunctionDecl
        || -stype == SrcmlCxxSType::DeclStmt
        || -stype == SrcmlCxxSType::ExprStmt
        || -stype == SrcmlCxxSType::Parameter
        || -stype == SrcmlCxxSType::EnumDecl
        || -stype == SrcmlCxxSType::Return
        || -stype == SrcmlCxxSType::Name
        || -stype == SrcmlCxxSType::Lambda
        || -stype == SrcmlCxxSType::Case
        || -stype == SrcmlCxxSType::Class
        || -stype == SrcmlCxxSType::Struct
        || -stype == SrcmlCxxSType::Union
        || (-parent == SrcmlCxxSType::Expr && -stype == SrcmlCxxSType::Block)
        || isValueNode(stype);
}

bool
SrcmlCxxLanguage::shouldDropLeadingWS(SType /*stype*/) const
{
    return false;
}

bool
SrcmlCxxLanguage::isSatellite(SType stype) const
{
    return (-stype == SrcmlCxxSType::Separator);
}

MType
SrcmlCxxLanguage::classify(SType stype) const
{
    switch (-stype) {
        case SrcmlCxxSType::FunctionDecl:
        case SrcmlCxxSType::DeclStmt:
        case SrcmlCxxSType::EnumDecl:
            return MType::Declaration;

        case SrcmlCxxSType::ExprStmt:
            return MType::Statement;

        case SrcmlCxxSType::Function:
        case SrcmlCxxSType::Constructor:
        case SrcmlCxxSType::Destructor:
            return MType::Function;

        case SrcmlCxxSType::Call:
            return MType::Call;

        case SrcmlCxxSType::Parameter:
            return MType::Parameter;

        case SrcmlCxxSType::Comment:
            return MType::Comment;

        case SrcmlCxxSType::CppDefine:
        case SrcmlCxxSType::CppDirective:
        case SrcmlCxxSType::CppElif:
        case SrcmlCxxSType::CppElse:
        case SrcmlCxxSType::CppEmpty:
        case SrcmlCxxSType::CppError:
        case SrcmlCxxSType::CppFile:
        case SrcmlCxxSType::CppIf:
        case SrcmlCxxSType::CppIfdef:
        case SrcmlCxxSType::CppIfndef:
        case SrcmlCxxSType::CppInclude:
        case SrcmlCxxSType::CppLine:
        case SrcmlCxxSType::CppMacro:
        case SrcmlCxxSType::CppNumber:
        case SrcmlCxxSType::CppPragma:
        case SrcmlCxxSType::CppUndef:
        case SrcmlCxxSType::CppValue:
        case SrcmlCxxSType::CppWarning:
            return MType::Directive;

        case SrcmlCxxSType::Statements:
            return MType::Block;

        default:
            return MType::Other;
    }
}

const char *
SrcmlCxxLanguage::toString(SType stype) const
{
    switch (-stype) {
        case SrcmlCxxSType::None:       return "SrcmlCxxSType::None";
        case SrcmlCxxSType::Separator:  return "SrcmlCxxSType::Separator";
        case SrcmlCxxSType::Statements: return "SrcmlCxxSType::Statements";

        case SrcmlCxxSType::Argument:     return "SrcmlCxxSType::Argument";
        case SrcmlCxxSType::Comment:      return "SrcmlCxxSType::Comment";
        case SrcmlCxxSType::CppEndif:     return "SrcmlCxxSType::CppEndif";
        case SrcmlCxxSType::CppLiteral:   return "SrcmlCxxSType::CppLiteral";
        case SrcmlCxxSType::EnumDecl:     return "SrcmlCxxSType::EnumDecl";
        case SrcmlCxxSType::Escape:       return "SrcmlCxxSType::Escape";
        case SrcmlCxxSType::ExprStmt:     return "SrcmlCxxSType::ExprStmt";
        case SrcmlCxxSType::Incr:         return "SrcmlCxxSType::Incr";
        case SrcmlCxxSType::Macro:        return "SrcmlCxxSType::Macro";
        case SrcmlCxxSType::MacroList:    return "SrcmlCxxSType::MacroList";
        case SrcmlCxxSType::RefQualifier: return "SrcmlCxxSType::RefQualifier";
        case SrcmlCxxSType::Unit:         return "SrcmlCxxSType::Unit";

        case SrcmlCxxSType::CppDefine:    return "SrcmlCxxSType::CppDefine";
        case SrcmlCxxSType::CppDirective: return "SrcmlCxxSType::CppDirective";
        case SrcmlCxxSType::CppElif:      return "SrcmlCxxSType::CppElif";
        case SrcmlCxxSType::CppElse:      return "SrcmlCxxSType::CppElse";
        case SrcmlCxxSType::CppEmpty:     return "SrcmlCxxSType::CppEmpty";
        case SrcmlCxxSType::CppError:     return "SrcmlCxxSType::CppError";
        case SrcmlCxxSType::CppFile:      return "SrcmlCxxSType::CppFile";
        case SrcmlCxxSType::CppIf:        return "SrcmlCxxSType::CppIf";
        case SrcmlCxxSType::CppIfdef:     return "SrcmlCxxSType::CppIfdef";
        case SrcmlCxxSType::CppIfndef:    return "SrcmlCxxSType::CppIfndef";
        case SrcmlCxxSType::CppInclude:   return "SrcmlCxxSType::CppInclude";
        case SrcmlCxxSType::CppLine:      return "SrcmlCxxSType::CppLine";
        case SrcmlCxxSType::CppMacro:     return "SrcmlCxxSType::CppMacro";
        case SrcmlCxxSType::CppNumber:    return "SrcmlCxxSType::CppNumber";
        case SrcmlCxxSType::CppPragma:    return "SrcmlCxxSType::CppPragma";
        case SrcmlCxxSType::CppUndef:     return "SrcmlCxxSType::CppUndef";
        case SrcmlCxxSType::CppValue:     return "SrcmlCxxSType::CppValue";
        case SrcmlCxxSType::CppWarning:   return "SrcmlCxxSType::CppWarning";

        case SrcmlCxxSType::Alignas:         return "SrcmlCxxSType::Alignas";
        case SrcmlCxxSType::Alignof:         return "SrcmlCxxSType::Alignof";
        case SrcmlCxxSType::ArgumentList:    return "SrcmlCxxSType::ArgumentList";
        case SrcmlCxxSType::Asm:             return "SrcmlCxxSType::Asm";
        case SrcmlCxxSType::Assert:          return "SrcmlCxxSType::Assert";
        case SrcmlCxxSType::Attribute:       return "SrcmlCxxSType::Attribute";
        case SrcmlCxxSType::Block:           return "SrcmlCxxSType::Block";
        case SrcmlCxxSType::BlockContent:    return "SrcmlCxxSType::BlockContent";
        case SrcmlCxxSType::Break:           return "SrcmlCxxSType::Break";
        case SrcmlCxxSType::Call:            return "SrcmlCxxSType::Call";
        case SrcmlCxxSType::Capture:         return "SrcmlCxxSType::Capture";
        case SrcmlCxxSType::Case:            return "SrcmlCxxSType::Case";
        case SrcmlCxxSType::Cast:            return "SrcmlCxxSType::Cast";
        case SrcmlCxxSType::Catch:           return "SrcmlCxxSType::Catch";
        case SrcmlCxxSType::Class:           return "SrcmlCxxSType::Class";
        case SrcmlCxxSType::ClassDecl:       return "SrcmlCxxSType::ClassDecl";
        case SrcmlCxxSType::Condition:       return "SrcmlCxxSType::Condition";
        case SrcmlCxxSType::Constructor:     return "SrcmlCxxSType::Constructor";
        case SrcmlCxxSType::ConstructorDecl: return "SrcmlCxxSType::ConstructorDecl";
        case SrcmlCxxSType::Continue:        return "SrcmlCxxSType::Continue";
        case SrcmlCxxSType::Control:         return "SrcmlCxxSType::Control";
        case SrcmlCxxSType::Decl:            return "SrcmlCxxSType::Decl";
        case SrcmlCxxSType::DeclStmt:        return "SrcmlCxxSType::DeclStmt";
        case SrcmlCxxSType::Decltype:        return "SrcmlCxxSType::Decltype";
        case SrcmlCxxSType::Default:         return "SrcmlCxxSType::Default";
        case SrcmlCxxSType::Destructor:      return "SrcmlCxxSType::Destructor";
        case SrcmlCxxSType::DestructorDecl:  return "SrcmlCxxSType::DestructorDecl";
        case SrcmlCxxSType::Do:              return "SrcmlCxxSType::Do";
        case SrcmlCxxSType::Else:            return "SrcmlCxxSType::Else";
        case SrcmlCxxSType::Elseif:          return "SrcmlCxxSType::Elseif";
        case SrcmlCxxSType::EmptyStmt:       return "SrcmlCxxSType::EmptyStmt";
        case SrcmlCxxSType::Enum:            return "SrcmlCxxSType::Enum";
        case SrcmlCxxSType::Expr:            return "SrcmlCxxSType::Expr";
        case SrcmlCxxSType::Extern:          return "SrcmlCxxSType::Extern";
        case SrcmlCxxSType::For:             return "SrcmlCxxSType::For";
        case SrcmlCxxSType::Friend:          return "SrcmlCxxSType::Friend";
        case SrcmlCxxSType::Function:        return "SrcmlCxxSType::Function";
        case SrcmlCxxSType::FunctionDecl:    return "SrcmlCxxSType::FunctionDecl";
        case SrcmlCxxSType::Goto:            return "SrcmlCxxSType::Goto";
        case SrcmlCxxSType::If:              return "SrcmlCxxSType::If";
        case SrcmlCxxSType::IfStmt:          return "SrcmlCxxSType::IfStmt";
        case SrcmlCxxSType::Index:           return "SrcmlCxxSType::Index";
        case SrcmlCxxSType::Init:            return "SrcmlCxxSType::Init";
        case SrcmlCxxSType::Label:           return "SrcmlCxxSType::Label";
        case SrcmlCxxSType::Lambda:          return "SrcmlCxxSType::Lambda";
        case SrcmlCxxSType::Literal:         return "SrcmlCxxSType::Literal";
        case SrcmlCxxSType::MemberInitList:  return "SrcmlCxxSType::MemberInitList";
        case SrcmlCxxSType::MemberList:      return "SrcmlCxxSType::MemberList";
        case SrcmlCxxSType::Modifier:        return "SrcmlCxxSType::Modifier";
        case SrcmlCxxSType::Name:            return "SrcmlCxxSType::Name";
        case SrcmlCxxSType::Namespace:       return "SrcmlCxxSType::Namespace";
        case SrcmlCxxSType::Noexcept:        return "SrcmlCxxSType::Noexcept";
        case SrcmlCxxSType::Operator:        return "SrcmlCxxSType::Operator";
        case SrcmlCxxSType::Param:           return "SrcmlCxxSType::Param";
        case SrcmlCxxSType::Parameter:       return "SrcmlCxxSType::Parameter";
        case SrcmlCxxSType::ParameterList:   return "SrcmlCxxSType::ParameterList";
        case SrcmlCxxSType::Private:         return "SrcmlCxxSType::Private";
        case SrcmlCxxSType::Protected:       return "SrcmlCxxSType::Protected";
        case SrcmlCxxSType::Public:          return "SrcmlCxxSType::Public";
        case SrcmlCxxSType::Range:           return "SrcmlCxxSType::Range";
        case SrcmlCxxSType::Return:          return "SrcmlCxxSType::Return";
        case SrcmlCxxSType::Sizeof:          return "SrcmlCxxSType::Sizeof";
        case SrcmlCxxSType::Specifier:       return "SrcmlCxxSType::Specifier";
        case SrcmlCxxSType::Struct:          return "SrcmlCxxSType::Struct";
        case SrcmlCxxSType::StructDecl:      return "SrcmlCxxSType::StructDecl";
        case SrcmlCxxSType::Super:           return "SrcmlCxxSType::Super";
        case SrcmlCxxSType::SuperList:       return "SrcmlCxxSType::SuperList";
        case SrcmlCxxSType::Switch:          return "SrcmlCxxSType::Switch";
        case SrcmlCxxSType::Template:        return "SrcmlCxxSType::Template";
        case SrcmlCxxSType::Ternary:         return "SrcmlCxxSType::Ternary";
        case SrcmlCxxSType::Then:            return "SrcmlCxxSType::Then";
        case SrcmlCxxSType::Throw:           return "SrcmlCxxSType::Throw";
        case SrcmlCxxSType::Try:             return "SrcmlCxxSType::Try";
        case SrcmlCxxSType::Type:            return "SrcmlCxxSType::Type";
        case SrcmlCxxSType::Typedef:         return "SrcmlCxxSType::Typedef";
        case SrcmlCxxSType::Typeid:          return "SrcmlCxxSType::Typeid";
        case SrcmlCxxSType::Typename:        return "SrcmlCxxSType::Typename";
        case SrcmlCxxSType::Union:           return "SrcmlCxxSType::Union";
        case SrcmlCxxSType::UnionDecl:       return "SrcmlCxxSType::UnionDecl";
        case SrcmlCxxSType::Using:           return "SrcmlCxxSType::Using";
        case SrcmlCxxSType::While:           return "SrcmlCxxSType::While";
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";
}
