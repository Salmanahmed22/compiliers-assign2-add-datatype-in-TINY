#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;
/*
20220195
20220120
20221072
*/
enum TokenType{
    IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
    ASSIGN, EQUAL, LESS_THAN,
    PLUS, MINUS, TIMES, DIVIDE, POWER,
    SEMI_COLON,
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    ID, NUM,
    ENDFILE, ERROR,AND_OP // added AND_OP for Scanner
};

// Used for debugging only /////////////////////////////////////////////////////////
const char* TokenTypeStr[]=
{
    "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
    "Assign", "Equal", "LessThan",
    "Plus", "Minus", "Times", "Divide", "Power",
    "SemiColon",
    "LeftParen", "RightParen",
    "LeftBrace", "RightBrace",
    "ID", "Num",
    "EndFile", "Error", "And" // Added And for debugging
};

const Token symbolic_tokens[]=
{
    Token(ASSIGN, ":="),
    Token(EQUAL, "="),
    Token(LESS_THAN, "<"),
    Token(PLUS, "+"),
    Token(MINUS, "-"),
    Token(TIMES, "*"),
    Token(DIVIDE, "/"),
    Token(POWER, "^"),
    Token(SEMI_COLON, ";"),
    Token(LEFT_PAREN, "("),
    Token(RIGHT_PAREN, ")"),
    Token(LEFT_BRACE, "{"),
    Token(RIGHT_BRACE, "}"),
    Token(AND_OP,"&") // Added & as symbolic token
};

// andexpr -> factor { & factor }    left associative
TreeNode* AndExpr(CompilerInfo* pci, ParseInfo* ppi)
{
    TreeNode* tree = Factor(pci, ppi);

    while (ppi->next_token.type == AND_OP)
    {
        TreeNode* new_tree = new TreeNode;
        new_tree->node_kind = OPER_NODE;
        new_tree->oper = ppi->next_token.type;
        new_tree->line_num = pci->in_file.cur_line_num;

        new_tree->child[0] = tree;
        Match(pci, ppi, AND_OP);
        new_tree->child[1] = Factor(pci, ppi);

        tree = new_tree;
    }
    return tree;
}

// old rule:
// term -> factor { (*|/) factor }    left associative
// new rule:
// term -> AndExpr { (*|/) AndExpr }
TreeNode* Term(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Term");
    TreeNode* tree=AndExpr(pci, ppi);

    while(ppi->next_token.type==TIMES || ppi->next_token.type==DIVIDE)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=AndExpr(pci, ppi);

        tree=new_tree;
    }
    pci->debug_file.Out("End Term");
    return tree;
}

int Evaluate(TreeNode* node, SymbolTable* symbol_table, int* variables)
{
    if(node->node_kind==NUM_NODE) return node->num;
    if(node->node_kind==ID_NODE) return variables[symbol_table->Find(node->id)->memloc];

    int a=Evaluate(node->child[0], symbol_table, variables);
    int b=Evaluate(node->child[1], symbol_table, variables);

    if(node->oper==EQUAL) return a==b;
    if(node->oper==LESS_THAN) return a<b;
    if(node->oper==PLUS) return a+b;
    if(node->oper==MINUS) return a-b;
    if(node->oper==TIMES) return a*b;
    if(node->oper==DIVIDE) return a/b;
    if(node->oper==POWER) return Power(a,b);
    if(node->oper==AND_OP) return (a*a)-(b*b); // add logic for & operator
    throw 0;
    return 0;
}