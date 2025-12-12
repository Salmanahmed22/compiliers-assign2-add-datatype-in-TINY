#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
using namespace std;

////////////////////////////////////////////////////////////////////////////////////
// Strings /////////////////////////////////////////////////////////////////////////

bool Equals(const char* a, const char* b)
{
    return strcmp(a, b)==0;
}

bool StartsWith(const char* a, const char* b)
{
    int nb=strlen(b);
    return strncmp(a, b, nb)==0;
}

void Copy(char* a, const char* b, int n=0)
{
    if(n>0) {strncpy(a, b, n); a[n]=0;}
    else strcpy(a, b);
}

void AllocateAndCopy(char** a, const char* b)
{
    if(b==0) {*a=0; return;}
    int n=strlen(b);
    *a=new char[n+1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile
{
    FILE* file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char* str) {file=0; if(str) file=fopen(str, "r"); cur_line_size=0; cur_ind=0; cur_line_num=0;}
    ~InFile(){if(file) fclose(file);}

    void SkipSpaces()
    {
        while(cur_ind<cur_line_size)
        {
            char ch=line_buf[cur_ind];
            if(ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n') break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char* str)
    {
        while(true)
        {
            SkipSpaces();
            while(cur_ind>=cur_line_size) {if(!GetNewLine()) return false; SkipSpaces();}

            if(StartsWith(&line_buf[cur_ind], str))
            {
                cur_ind+=strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine()
    {
        cur_ind=0; line_buf[0]=0;
        if(!fgets(line_buf, MAX_LINE_LENGTH, file)) return false;
        cur_line_size=strlen(line_buf);
        if(cur_line_size==0) return false; // End of file
        cur_line_num++;
        return true;
    }

    char* GetNextTokenStr()
    {
        SkipSpaces();
        while(cur_ind>=cur_line_size) {if(!GetNewLine()) return 0; SkipSpaces();}
        return &line_buf[cur_ind];
    }

    void Advance(int num)
    {
        cur_ind+=num;
    }
};

struct OutFile
{
    FILE* file;
    OutFile(const char* str) {file=0; if(str) file=fopen(str, "w");}
    ~OutFile(){if(file) fclose(file);}

    void Out(const char* s)
    {
        fprintf(file, "%s\n", s); fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo
{
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char* in_str, const char* out_str, const char* debug_str)
                : in_file(in_str), out_file(out_str), debug_file(debug_str)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

// Define maximum token length for identifiers and numbers
#define MAX_TOKEN_LEN 40

// Enumeration of all token types recognized by the scanner
// Extended to include type keywords (INT_TYPE, REAL_TYPE, BOOL_TYPE) for type system
enum TokenType{
                IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
                ASSIGN, EQUAL, LESS_THAN, GREATER_THAN, GREATER_EQUAL, LESS_EQUAL,
                PLUS, MINUS, TIMES, DIVIDE, POWER,
                SEMI_COLON,
                LEFT_PAREN, RIGHT_PAREN,
                LEFT_BRACE, RIGHT_BRACE,
                ID, NUM,
                ENDFILE, ERROR, AND_OP,  // AND_OP: & operator for logical and
                INT_TYPE, REAL_TYPE, BOOL_TYPE  // Type keywords: int, real, bool
              };

// String representations of token types used for debugging output
// Helps in tracing scanner behavior and understanding token stream
// Must be kept in sync with TokenType enum above
const char* TokenTypeStr[]=
            {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan", "GreaterThan", "GreaterEqual", "LessEqual",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error", "And",     // Added And for debugging & operator
                "IntType", "RealType", "BoolType"  // Type keywords for debugging
            };

// Note: keep this array in sync with enum TokenType

struct Token
{
    TokenType type;
    char str[MAX_TOKEN_LEN+1];

    Token(){str[0]=0; type=ERROR;}
    Token(TokenType _type, const char* _str) {type=_type; Copy(str, _str);}
};

// Table of reserved keywords recognized by the scanner
// These are special tokens that cannot be used as identifiers
// Extended to include type keywords: int, real, bool for type system
const Token reserved_words[]=
{
    Token(IF, "if"),
    Token(THEN, "then"),
    Token(ELSE, "else"),
    Token(END, "end"),
    Token(REPEAT, "repeat"),
    Token(UNTIL, "until"),
    Token(READ, "read"),
    Token(WRITE, "write"),
    Token(INT_TYPE, "int"),     // Type keyword for integer type
    Token(REAL_TYPE, "real"),   // Type keyword for real (double) type
    Token(BOOL_TYPE, "bool")    // Type keyword for boolean type
};
// Count of reserved words in the table
const int num_reserved_words=sizeof(reserved_words)/sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[]=
{
    Token(ASSIGN, ":="),
    Token(EQUAL, "="),
    Token(GREATER_EQUAL, ">="),  // Must come before > to avoid conflict
    Token(LESS_EQUAL, "<="),     // Must come before < to avoid conflict
    Token(GREATER_THAN, ">"),
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
const int num_symbolic_tokens=sizeof(symbolic_tokens)/sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch){return (ch>='0' && ch<='9');}
inline bool IsLetter(char ch){return ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'));}
inline bool IsLetterOrUnderscore(char ch){return (IsLetter(ch) || ch=='_');}

void GetNextToken(CompilerInfo* pci, Token* ptoken)
{
    ptoken->type=ERROR;
    ptoken->str[0]=0;

    int i;
    char* s=pci->in_file.GetNextTokenStr();
    if(!s)
    {
        ptoken->type=ENDFILE;
        ptoken->str[0]=0;
        return;
    }

    for(i=0;i<num_symbolic_tokens;i++)
    {
        if(StartsWith(s, symbolic_tokens[i].str))
            break;
    }

    if(i<num_symbolic_tokens)
    {
        if(symbolic_tokens[i].type==LEFT_BRACE)
        {
            pci->in_file.Advance(strlen(symbolic_tokens[i].str));
            if(!pci->in_file.SkipUpto(symbolic_tokens[i+1].str)) return;
            return GetNextToken(pci, ptoken);
        }
        ptoken->type=symbolic_tokens[i].type;
        Copy(ptoken->str, symbolic_tokens[i].str);
    }
    else if(IsDigit(s[0]))
    {
        // Parse numeric literal - support both integers and real numbers
        int j=1;
        int has_decimal = 0;
        
        // Parse digits and optional decimal point for real numbers
        while(IsDigit(s[j]) || (s[j]=='.' && !has_decimal))
        {
            if(s[j]=='.') has_decimal=1;
            j++;
        }

        ptoken->type=NUM;
        Copy(ptoken->str, s, j);
    }
    else if(IsLetterOrUnderscore(s[0]))
    {
        int j=1;
        while(IsLetterOrUnderscore(s[j])) j++;

        ptoken->type=ID;
        Copy(ptoken->str, s, j);

        for(i=0;i<num_reserved_words;i++)
        {
            if(Equals(ptoken->str, reserved_words[i].str))
            {
                ptoken->type=reserved_words[i].type;
                break;
            }
        }
    }

    int len=strlen(ptoken->str);
    if(len>0) pci->in_file.Advance(len);
}

////////////////////////////////////////////////////////////////////////////////////
// Parser //////////////////////////////////////////////////////////////////////////

// program -> stmtseq
// stmtseq -> stmt { ; stmt }
// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
// ifstmt -> if exp then stmtseq [ else stmtseq ] end
// repeatstmt -> repeat stmtseq until expr
// assignstmt -> identifier := expr
// declstmt -> type identifier := expr       (explicit type declaration with assignment)
// readstmt -> read identifier
// writestmt -> write expr
// expr -> mathexpr [ (<|=) mathexpr ]
// mathexpr -> term { (+|-) term }    left associative
// term -> factor { (*|/) factor }    left associative
// factor -> newexpr { ^ newexpr }    right associative
// newexpr -> ( mathexpr ) | number | identifier

enum NodeKind{
                IF_NODE, REPEAT_NODE, ASSIGN_NODE, READ_NODE, WRITE_NODE,
                OPER_NODE, NUM_NODE, ID_NODE, DECL_NODE  // DECL_NODE for type declarations
             };

// Used for debugging only /////////////////////////////////////////////////////////
const char* NodeKindStr[]=
            {
                "If", "Repeat", "Assign", "Read", "Write",
                "Oper", "Num", "ID", "Decl"  // Added Decl for declarations
            };

// Enumeration of expression data types for the TINY language type system
// VOID: expressions without type (used for statements)
// INTEGER: int type - supports arithmetic operations
// REAL: double type - supports arithmetic operations, can mix with INTEGER
// BOOLEAN: bool type - result of comparison operators, used only in conditions
enum ExprDataType {VOID, INTEGER, REAL, BOOLEAN};

// String representations of expression data types used for debugging
// Helps in tracing type checking and understanding type system behavior
const char* ExprDataTypeStr[]=
            {
                "Void", "Integer", "Real", "Boolean"  // Added Real and Boolean types
            };

#define MAX_CHILDREN 3

// Tree node structure for representing the abstract syntax tree (AST)
// Each node represents a statement or expression in the TINY program
// Extended to support real (double) values in addition to integer values
struct TreeNode
{
    TreeNode* child[MAX_CHILDREN];      // Child nodes (up to 3 children max)
    TreeNode* sibling;                  // Sibling nodes for sequences of statements

    NodeKind node_kind;                 // Type of node (IF, ASSIGN, OPER, etc.)

    // Union for node-specific data
    // oper: operator type for OPER_NODE expressions
    // num: integer value for NUM_NODE with integer literals
    // real_num: real (double) value for NUM_NODE with real literals
    // id: identifier name for ID_NODE variables
    union{TokenType oper; int num; double real_num; char* id;};
    
    ExprDataType expr_data_type;        // Data type of expression result
    ExprDataType var_type;              // Variable type (only for ID_NODE): INTEGER, REAL, or BOOLEAN

    int line_num;                       // Source line number for error reporting

    // Default constructor: initialize all fields
    TreeNode() {
        int i;
        for(i=0;i<MAX_CHILDREN;i++) child[i]=0;
        sibling=0;
        expr_data_type=VOID;
        var_type=VOID;          // Add var_type initialization
        real_num=0.0;           // Initialize real_num to 0.0
    }
};

struct ParseInfo
{
    Token next_token;
};

void Match(CompilerInfo* pci, ParseInfo* ppi, TokenType expected_token_type)
{
    pci->debug_file.Out("Start Match");
    if(ppi->next_token.type!=expected_token_type) {
        throw 0;
    }
    GetNextToken(pci, &ppi->next_token);

    fprintf(pci->debug_file.file, "[%d] %s (%s)\n", pci->in_file.cur_line_num, ppi->next_token.str, TokenTypeStr[ppi->next_token.type]); fflush(pci->debug_file.file);
}

TreeNode* MathExpr(CompilerInfo*, ParseInfo*);

// newexpr -> ( mathexpr ) | number | identifier
TreeNode* NewExpr(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start NewExpr");

    // Compare the next token with the First() of possible statements
    // handle unary minus: produce (0 - newexpr)
    if(ppi->next_token.type==MINUS)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=MINUS;
        new_tree->line_num=pci->in_file.cur_line_num;

        // left child is numeric zero
        TreeNode* zero=new TreeNode;
        zero->node_kind=NUM_NODE;
        zero->num=0;

        Match(pci, ppi, MINUS);
        new_tree->child[0]=zero;
        new_tree->child[1]=NewExpr(pci, ppi);

        pci->debug_file.Out("End NewExpr");
        return new_tree;
    }
    if(ppi->next_token.type==NUM)
    {
        TreeNode* tree=new TreeNode;
        tree->node_kind=NUM_NODE;
        char* num_str=ppi->next_token.str;
        
        // Check if this is a real number (contains decimal point) or integer
        int has_decimal = 0;
        char* temp_str = num_str;
        while(*temp_str)
        {
            if(*temp_str == '.') { has_decimal = 1; break; }
            temp_str++;
        }
        
        if(has_decimal)
        {
            // Parse as real number (double)
            tree->real_num = 0.0;
            double multiplier = 1.0;
            int before_decimal = 1;
            
            while(*num_str)
            {
                if(*num_str == '.')
                {
                    before_decimal = 0;
                    multiplier = 0.1;
                }
                else
                {
                    if(before_decimal)
                        tree->real_num = tree->real_num * 10.0 + (double)((*num_str) - '0');
                    else
                    {
                        tree->real_num = tree->real_num + multiplier * (double)((*num_str) - '0');
                        multiplier *= 0.1;
                    }
                }
                num_str++;
            }
            tree->expr_data_type = REAL;
        }
        else
        {
            // Parse as integer
            tree->num = 0;
            while(*num_str)
                tree->num = tree->num * 10 + ((*num_str++) - '0');
            tree->expr_data_type = INTEGER;
        }
        
        tree->line_num=pci->in_file.cur_line_num;
        Match(pci, ppi, ppi->next_token.type);

        pci->debug_file.Out("End NewExpr");
        return tree;
    }

    if(ppi->next_token.type==ID)
    {
        TreeNode* tree=new TreeNode;
        tree->node_kind=ID_NODE;
        AllocateAndCopy(&tree->id, ppi->next_token.str);
        tree->line_num=pci->in_file.cur_line_num;
        Match(pci, ppi, ppi->next_token.type);

        pci->debug_file.Out("End NewExpr");
        return tree;
    }

    if(ppi->next_token.type==LEFT_PAREN)
    {
        Match(pci, ppi, LEFT_PAREN);
        TreeNode* tree=MathExpr(pci, ppi);
        Match(pci, ppi, RIGHT_PAREN);

        pci->debug_file.Out("End NewExpr");
        return tree;
    }

    throw 0;
    return 0;
}

// factor -> newexpr { ^ newexpr }    right associative
TreeNode* Factor(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Factor");

    TreeNode* tree=NewExpr(pci, ppi);

    if(ppi->next_token.type==POWER)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=Factor(pci, ppi);

        pci->debug_file.Out("End Factor");
        return new_tree;
    }
    pci->debug_file.Out("End Factor");
    return tree;
}

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
        new_tree->child[1] = AndExpr(pci, ppi);

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

// mathexpr -> term { (+|-) term }    left associative
TreeNode* MathExpr(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start MathExpr");

    TreeNode* tree=Term(pci, ppi);

    while(ppi->next_token.type==PLUS || ppi->next_token.type==MINUS)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=Term(pci, ppi);

        tree=new_tree;
    }
    pci->debug_file.Out("End MathExpr");
    return tree;
}

// expr -> mathexpr [ (<|=) mathexpr ]
TreeNode* Expr(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Expr");

    TreeNode* tree=MathExpr(pci, ppi);

    if(ppi->next_token.type==EQUAL || ppi->next_token.type==LESS_THAN ||
       ppi->next_token.type==GREATER_THAN || ppi->next_token.type==GREATER_EQUAL ||
       ppi->next_token.type==LESS_EQUAL)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=MathExpr(pci, ppi);

        pci->debug_file.Out("End Expr");
        return new_tree;
    }
    pci->debug_file.Out("End Expr");
    return tree;
}

// writestmt -> write expr
TreeNode* WriteStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start WriteStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=WRITE_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    Match(pci, ppi, WRITE);
    tree->child[0]=Expr(pci, ppi);

    pci->debug_file.Out("End WriteStmt");
    return tree;
}

// readstmt -> read identifier
TreeNode* ReadStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start ReadStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=READ_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    Match(pci, ppi, READ);
    if(ppi->next_token.type==ID) AllocateAndCopy(&tree->id, ppi->next_token.str);
    Match(pci, ppi, ID);

    pci->debug_file.Out("End ReadStmt");
    return tree;
}

// assignstmt -> identifier := expr
TreeNode* AssignStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start AssignStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=ASSIGN_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    if(ppi->next_token.type==ID) AllocateAndCopy(&tree->id, ppi->next_token.str);
    Match(pci, ppi, ID);
    Match(pci, ppi, ASSIGN); tree->child[0]=Expr(pci, ppi);

    pci->debug_file.Out("End AssignStmt");
    return tree;
}

TreeNode* StmtSeq(CompilerInfo*, ParseInfo*);

// repeatstmt -> repeat stmtseq until expr
TreeNode* RepeatStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start RepeatStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=REPEAT_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    Match(pci, ppi, REPEAT); tree->child[0]=StmtSeq(pci, ppi);
    Match(pci, ppi, UNTIL); tree->child[1]=Expr(pci, ppi);

    pci->debug_file.Out("End RepeatStmt");
    return tree;
}

// ifstmt -> if exp then stmtseq [ else stmtseq ] end
TreeNode* IfStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start IfStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=IF_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    Match(pci, ppi, IF); tree->child[0]=Expr(pci, ppi);
    Match(pci, ppi, THEN); tree->child[1]=StmtSeq(pci, ppi);
    if(ppi->next_token.type==ELSE) {Match(pci, ppi, ELSE); tree->child[2]=StmtSeq(pci, ppi);}
    Match(pci, ppi, END);

    pci->debug_file.Out("End IfStmt");
    return tree;
}

// declstmt -> type identifier := expr
// Explicit type declaration with initialization
// Examples: int x := 5; real y := 3.14; bool flag := true;
TreeNode* DeclStmt(CompilerInfo* pci, ParseInfo* ppi, ExprDataType decl_type)
{
    pci->debug_file.Out("Start DeclStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=DECL_NODE;
    tree->line_num=pci->in_file.cur_line_num;
    tree->var_type=decl_type;  // Store the declared type

    // Parse: identifier := expr
    if(ppi->next_token.type==ID) AllocateAndCopy(&tree->id, ppi->next_token.str);
    Match(pci, ppi, ID);
    Match(pci, ppi, ASSIGN);
    tree->child[0]=Expr(pci, ppi);  // Parse the initializing expression

    pci->debug_file.Out("End DeclStmt");
    return tree;
}

// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt | declstmt
TreeNode* Stmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Stmt");

    // Compare the next token with the First() of possible statements
    TreeNode* tree=0;
    
    // Check for explicit type declarations: int id := expr; real id := expr; bool id := expr;
    if(ppi->next_token.type==INT_TYPE)
    {
        Match(pci, ppi, INT_TYPE);
        tree=DeclStmt(pci, ppi, INTEGER);
    }
    else if(ppi->next_token.type==REAL_TYPE)
    {
        Match(pci, ppi, REAL_TYPE);
        tree=DeclStmt(pci, ppi, REAL);
    }
    else if(ppi->next_token.type==BOOL_TYPE)
    {
        Match(pci, ppi, BOOL_TYPE);
        tree=DeclStmt(pci, ppi, BOOLEAN);
    }
    else if(ppi->next_token.type==IF) tree=IfStmt(pci, ppi);
    else if(ppi->next_token.type==REPEAT) tree=RepeatStmt(pci, ppi);
    else if(ppi->next_token.type==ID) tree=AssignStmt(pci, ppi);
    else if(ppi->next_token.type==READ) tree=ReadStmt(pci, ppi);
    else if(ppi->next_token.type==WRITE) tree=WriteStmt(pci, ppi);
    else {
        throw 0;
    }

    pci->debug_file.Out("End Stmt");
    return tree;
}

// stmtseq -> stmt { ; stmt }
TreeNode* StmtSeq(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start StmtSeq");

    TreeNode* first_tree=Stmt(pci, ppi);
    TreeNode* last_tree=first_tree;

    // If we did not reach one of the Follow() of StmtSeq(), we are not done yet
    while(ppi->next_token.type!=ENDFILE && ppi->next_token.type!=END &&
          ppi->next_token.type!=ELSE && ppi->next_token.type!=UNTIL)
    {
        Match(pci, ppi, SEMI_COLON);
        // after consuming a semicolon, if the next token is in the Follow() of StmtSeq,
        // then we had a trailing semicolon — don't try to parse another statement
        if(ppi->next_token.type==ENDFILE || ppi->next_token.type==END ||
           ppi->next_token.type==ELSE || ppi->next_token.type==UNTIL)
        {
            break;
        }
        TreeNode* next_tree=Stmt(pci, ppi);
        last_tree->sibling=next_tree;
        last_tree=next_tree;
    }

    pci->debug_file.Out("End StmtSeq");
    return first_tree;
}

// program -> stmtseq
TreeNode* Parse(CompilerInfo* pci)
{
    ParseInfo parse_info;
    GetNextToken(pci, &parse_info.next_token);

    TreeNode* syntax_tree=StmtSeq(pci, &parse_info);

    if(parse_info.next_token.type!=ENDFILE)
        pci->debug_file.Out("Error code ends before file ends");

    return syntax_tree;
}

void PrintTree(TreeNode* node, int sh=0)
{
    int i, NSH=3;
    for(i=0;i<sh;i++) printf(" ");

    printf("[%s]", NodeKindStr[node->node_kind]);

    if(node->node_kind==OPER_NODE) printf("[%s]", TokenTypeStr[node->oper]);
    else if(node->node_kind==NUM_NODE)
    {
        // Print numeric value based on type
        if(node->expr_data_type == REAL)
            printf("[%lf]", node->real_num);
        else
            printf("[%d]", node->num);
    }
    else if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || node->node_kind==ASSIGN_NODE || node->node_kind==DECL_NODE)
        printf("[%s]", node->id);

    // Print variable type for declarations and variable references
    if(node->node_kind==DECL_NODE)
        printf("[%s]", ExprDataTypeStr[node->var_type]);
    else if(node->expr_data_type!=VOID) printf("[%s]", ExprDataTypeStr[node->expr_data_type]);

    printf("\n");

    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) PrintTree(node->child[i], sh+NSH);
    if(node->sibling) PrintTree(node->sibling, sh);
}

void DestroyTree(TreeNode* node)
{
    int i;

    if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || node->node_kind==ASSIGN_NODE || node->node_kind==DECL_NODE)
        if(node->id) delete[] node->id;

    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) DestroyTree(node->child[i]);
    if(node->sibling) DestroyTree(node->sibling);

    delete node;
}

////////////////////////////////////////////////////////////////////////////////////
// Analyzer ////////////////////////////////////////////////////////////////////////

const int SYMBOL_HASH_SIZE=10007;

struct LineLocation
{
    int line_num;
    LineLocation* next;
};

// Structure representing a variable in the symbol table
// Extended with type information to support the type system
struct VariableInfo
{
    char* name;                             // Variable name/identifier
    int memloc;                             // Memory location index for runtime storage
    ExprDataType var_type;                  // Type of this variable: INTEGER, REAL, or BOOLEAN
    LineLocation* head_line;                // Head of linked list of source line locations
    LineLocation* tail_line;                // Tail of linked list of source line locations
    VariableInfo* next_var;                 // Next variable in hash bucket chain
};

struct SymbolTable
{
    int num_vars;
    VariableInfo* var_info[SYMBOL_HASH_SIZE];

    SymbolTable() {num_vars=0; int i; for(i=0;i<SYMBOL_HASH_SIZE;i++) var_info[i]=0;}

    int Hash(const char* name)
    {
        int i, len=strlen(name);
        int hash_val=11;
        for(i=0;i<len;i++) hash_val=(hash_val*17+(int)name[i])%SYMBOL_HASH_SIZE;
        return hash_val;
    }

    VariableInfo* Find(const char* name)
    {
        int h=Hash(name);
        VariableInfo* cur=var_info[h];
        while(cur)
        {
            if(Equals(name, cur->name)) return cur;
            cur=cur->next_var;
        }
        return 0;
    }

    void Insert(const char* name, int line_num, ExprDataType type)
    {
        // Create a new line location entry with the source line number
        LineLocation* lineloc=new LineLocation;
        lineloc->line_num=line_num;
        lineloc->next=0;

        // Calculate hash index for this variable name
        int h=Hash(name);
        VariableInfo* prev=0;
        VariableInfo* cur=var_info[h];

        // Check if variable already exists in the symbol table
        while(cur)
        {
            if(Equals(name, cur->name))
            {
                // Variable already exists - check type consistency
                if(cur->var_type != type)
                {
                    // Type mismatch: variable already declared with different type
                    printf("ERROR Type mismatch: variable '%s' already declared with type '%s', attempted redeclaration with type '%s'\n", 
                           name, ExprDataTypeStr[cur->var_type], ExprDataTypeStr[type]);
                    throw 0;  // Throw exception for type conflict
                }
                // Add this line location to the list of line locations
                cur->tail_line->next=lineloc;
                cur->tail_line=lineloc;
                return;
            }
            prev=cur;
            cur=cur->next_var;
        }

        // Create new variable entry with type information
        VariableInfo* vi=new VariableInfo;
        vi->head_line=vi->tail_line=lineloc;
        vi->next_var=0;
        vi->memloc=num_vars++;
        vi->var_type=type;                      // Store the variable's data type
        AllocateAndCopy(&vi->name, name);

        // Insert into hash table
        if(!prev) var_info[h]=vi;
        else prev->next_var=vi;
    }

    void Print()
    {
        int i;
        for(i=0;i<SYMBOL_HASH_SIZE;i++)
        {
            VariableInfo* curv=var_info[i];
            while(curv)
            {
                printf("[Var=%s][Mem=%d]", curv->name, curv->memloc);
                LineLocation* curl=curv->head_line;
                while(curl)
                {
                    printf("[Line=%d]", curl->line_num);
                    curl=curl->next;
                }
                printf("\n");
                curv=curv->next_var;
            }
        }
    }

    void Destroy()
    {
        int i;
        for(i=0;i<SYMBOL_HASH_SIZE;i++)
        {
            VariableInfo* curv=var_info[i];
            while(curv)
            {
                LineLocation* curl=curv->head_line;
                while(curl)
                {
                    LineLocation* pl=curl;
                    curl=curl->next;
                    delete pl;
                }
                VariableInfo* p=curv;
                curv=curv->next_var;
                delete p;
            }
            var_info[i]=0;
        }
    }
};

// Enhanced semantic analysis with full type checking for the type system
// Two-pass approach:
// 1. First pass: analyze children recursively
// 2. Second pass: determine and validate types based on context
void Analyze(TreeNode* node, SymbolTable* symbol_table)
{
    int i;

    // PASS 1: Recursively analyze all child nodes first
    // This ensures that expressions are fully analyzed before we check the parent node
    for(i=0;i<MAX_CHILDREN;i++) 
        if(node->child[i]) 
            Analyze(node->child[i], symbol_table);

    // PASS 2: Determine and validate types after children are analyzed
    
    // Handle explicit type declarations: int x := expr; real y := expr; bool z := expr;
    if(node->node_kind==DECL_NODE)
    {
        // Declared type is stored in node->var_type
        // RHS expression type is in node->child[0]->expr_data_type
        ExprDataType rhs_type = node->child[0]->expr_data_type;
        ExprDataType decl_type = node->var_type;
        
        // Check type compatibility
        if(decl_type != rhs_type)
        {
            // Type mismatch: declared type != RHS expression type
            printf("ERROR Line %d: Declaration type mismatch: cannot assign %s to %s variable '%s'\n",
                   node->line_num,
                   ExprDataTypeStr[rhs_type],
                   ExprDataTypeStr[decl_type],
                   node->id);
            throw 0;
        }
        
        // Register the variable with its declared type
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var)
        {
            symbol_table->Insert(node->id, node->line_num, decl_type);
        }
    }
    
    // Determine expression type based on operator and operands
    if(node->node_kind==OPER_NODE)
    {
        ExprDataType left_type = node->child[0]->expr_data_type;
        ExprDataType right_type = node->child[1]->expr_data_type;
        
        // Comparison operators (< = > >= <=) return BOOLEAN type
        if(node->oper==EQUAL || node->oper==LESS_THAN || node->oper==GREATER_THAN ||
           node->oper==GREATER_EQUAL || node->oper==LESS_EQUAL)
        {
            node->expr_data_type=BOOLEAN;
            
            // Type checking for comparisons
            if((left_type == BOOLEAN || right_type == BOOLEAN))
            {
                printf("ERROR Line %d: Cannot compare BOOLEAN values with comparison operators\n", node->line_num);
                throw 0;
            }
            if((left_type == VOID || right_type == VOID))
            {
                printf("ERROR Line %d: Invalid operand type in comparison\n", node->line_num);
                throw 0;
            }
        }
        // Arithmetic operators: + - * / ^ &
        else
        {
            // Check that neither operand is BOOLEAN
            if(left_type == BOOLEAN || right_type == BOOLEAN)
            {
                printf("ERROR Line %d: Arithmetic operator applied to BOOLEAN type\n", node->line_num);
                throw 0;
            }
            
            // Determine result type based on operands
            // If either is REAL, result is REAL
            // Otherwise result is INTEGER
            if(left_type == REAL || right_type == REAL)
                node->expr_data_type = REAL;
            else if(left_type == INTEGER && right_type == INTEGER)
                node->expr_data_type = INTEGER;
            else
            {
                printf("ERROR Line %d: Invalid operand types for arithmetic: %s and %s\n",
                       node->line_num, ExprDataTypeStr[left_type], ExprDataTypeStr[right_type]);
                throw 0;
            }
        }
    }
    // Set type for NUM_NODE (numeric literal)
    else if(node->node_kind==NUM_NODE)
    {
        // Type was already set during parsing:
        // REAL if has decimal point, INTEGER otherwise
        // Don't override it here
        if(node->expr_data_type == VOID)
            node->expr_data_type = INTEGER;  // Fallback only if not set
    }
    // Set type for ID_NODE (variable reference)
    else if(node->node_kind==ID_NODE)
    {
        // Type comes from the variable's declaration or first use
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var)
        {
            // Variable not yet found - will be created on first assignment
            // Mark as INTEGER for now
            node->var_type = INTEGER;
            node->expr_data_type = INTEGER;
        }
        else
        {
            // Type comes from the variable's declaration
            if(var->var_type != VOID)
                node->expr_data_type = var->var_type;
            else
                node->expr_data_type = INTEGER;  // Default fallback
            node->var_type = var->var_type;
        }
    }

    // Handle ASSIGN_NODE: variable := expression
    if(node->node_kind==ASSIGN_NODE)
    {
        // Now we know the RHS expression type from analysis above
        ExprDataType rhs_type = node->child[0]->expr_data_type;
        VariableInfo* var = symbol_table->Find(node->id);
        
        if(!var)
        {
            // Auto-declare variable with the type of the RHS expression
            symbol_table->Insert(node->id, node->line_num, rhs_type);
            var = symbol_table->Find(node->id);
            node->var_type = rhs_type;
        }
        else if(var->var_type == VOID)
        {
            // Variable was auto-declared but type not yet set
            var->var_type = rhs_type;
            node->var_type = rhs_type;
        }
        else if(var->var_type != rhs_type)
        {
            // Type mismatch: variable type != RHS expression type
            printf("ERROR Line %d: Assignment type mismatch: cannot assign %s to %s variable '%s'\n",
                   node->line_num,
                   ExprDataTypeStr[rhs_type],
                   ExprDataTypeStr[var->var_type],
                   node->id);
            throw 0;
        }
        else
        {
            node->var_type = var->var_type;
        }
    }
    
    // Type validation for specific statement types
    if(node->node_kind==IF_NODE)
    {
        // IF condition must be BOOLEAN
        if(node->child[0]->expr_data_type != BOOLEAN)
        {
            printf("ERROR Line %d: IF condition must evaluate to BOOLEAN, not %s\n",
                   node->line_num, ExprDataTypeStr[node->child[0]->expr_data_type]);
            throw 0;
        }
    }
    
    if(node->node_kind==REPEAT_NODE)
    {
        // REPEAT until condition must be BOOLEAN
        if(node->child[1]->expr_data_type != BOOLEAN)
        {
            printf("ERROR Line %d: REPEAT until condition must evaluate to BOOLEAN, not %s\n",
                   node->line_num, ExprDataTypeStr[node->child[1]->expr_data_type]);
            throw 0;
        }
    }
    
    if(node->node_kind==WRITE_NODE)
    {
        // WRITE can output any type: INTEGER, REAL, or BOOLEAN
        if(node->child[0]->expr_data_type == VOID)
        {
            printf("ERROR Line %d: WRITE expression has no type\n", node->line_num);
            throw 0;
        }
    }

    // Recursively analyze sibling nodes
    if(node->sibling) 
        Analyze(node->sibling, symbol_table);
}

////////////////////////////////////////////////////////////////////////////////////
// Code Generator //////////////////////////////////////////////////////////////////

// Structure to represent a typed value during evaluation
// Holds either an integer, real, or boolean value with its type
struct TypedValue
{
    ExprDataType type;      // Type of the value: INTEGER, REAL, or BOOLEAN
    int int_val;            // Integer value
    double real_val;        // Real (double) value
    int bool_val;           // Boolean value (0=false, 1=true)
    
    // Default constructor
    TypedValue() : type(VOID), int_val(0), real_val(0.0), bool_val(0) {}
    
    // Constructor for INTEGER
    TypedValue(int val) : type(INTEGER), int_val(val), real_val((double)val), bool_val(0) {}
    
    // Constructor for REAL
    TypedValue(double val) : type(REAL), int_val(0), real_val(val), bool_val(0) {}
    
    // Constructor for BOOLEAN
    TypedValue(int val, bool) : type(BOOLEAN), int_val(val), real_val(0.0), bool_val(val) {}
};

// Helper function to raise an integer to a power
// Used for the power operator (^)
// Parameters: base (a) and exponent (b)
// Returns: a raised to the power b
int Power(int a, int b)
{
    // Special cases for power operation
    if(a==0) return 0;           // 0^n = 0
    if(b==0) return 1;           // n^0 = 1
    if(b>=1) return a*Power(a, b-1);  // Recursive calculation
    return 0;
}

// Helper function for real power (using double)
double RealPower(double a, int b)
{
    if(a==0.0) return 0.0;
    if(b==0) return 1.0;
    if(b>0)
    {
        double result = 1.0;
        int i;
        for(i=0;i<b;i++) result = result * a;
        return result;
    }
    return 0.0;
}

// Enhanced evaluate function that handles all three types: int, real, and boolean
// Returns a TypedValue containing the result of evaluating the expression
// Supports type mixing with automatic conversion where appropriate
TypedValue Evaluate(TreeNode* node, SymbolTable* symbol_table, TypedValue* variables)
{
    TypedValue result;
    
    // NUM_NODE: numeric literal (int or real)
    if(node->node_kind==NUM_NODE)
    {
        // Check if this is a real literal or integer literal
        if(node->expr_data_type == REAL)
        {
            // Parse as real number
            result = TypedValue(node->real_num);
            result.type = REAL;
        }
        else
        {
            // Parse as integer
            result = TypedValue(node->num);
            result.type = INTEGER;
        }
        return result;
    }
    
    // ID_NODE: variable reference
    if(node->node_kind==ID_NODE)
    {
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var)
        {
            printf("ERROR Undefined variable '%s'\n", node->id);
            throw 0;
        }
        return variables[var->memloc];
    }

    // OPER_NODE: binary operations
    TypedValue a = Evaluate(node->child[0], symbol_table, variables);
    TypedValue b = Evaluate(node->child[1], symbol_table, variables);

    // Comparison operators: < = > >= <=
    if(node->oper==EQUAL)
    {
        // Equality comparison
        if(a.type == REAL || b.type == REAL)
        {
            // Real comparison
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            result = TypedValue((a_val == b_val) ? 1 : 0, true);
        }
        else
        {
            // Integer comparison
            result = TypedValue((a.int_val == b.int_val) ? 1 : 0, true);
        }
        result.type = BOOLEAN;
        return result;
    }
    
    if(node->oper==LESS_THAN)
    {
        // Less than comparison
        if(a.type == REAL || b.type == REAL)
        {
            // Real comparison
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            result = TypedValue((a_val < b_val) ? 1 : 0, true);
        }
        else
        {
            // Integer comparison
            result = TypedValue((a.int_val < b.int_val) ? 1 : 0, true);
        }
        result.type = BOOLEAN;
        return result;
    }

    if(node->oper==GREATER_THAN)
    {
        // Greater than comparison
        if(a.type == REAL || b.type == REAL)
        {
            // Real comparison
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            result = TypedValue((a_val > b_val) ? 1 : 0, true);
        }
        else
        {
            // Integer comparison
            result = TypedValue((a.int_val > b.int_val) ? 1 : 0, true);
        }
        result.type = BOOLEAN;
        return result;
    }

    if(node->oper==GREATER_EQUAL)
    {
        // Greater than or equal comparison
        if(a.type == REAL || b.type == REAL)
        {
            // Real comparison
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            result = TypedValue((a_val >= b_val) ? 1 : 0, true);
        }
        else
        {
            // Integer comparison
            result = TypedValue((a.int_val >= b.int_val) ? 1 : 0, true);
        }
        result.type = BOOLEAN;
        return result;
    }

    if(node->oper==LESS_EQUAL)
    {
        // Less than or equal comparison
        if(a.type == REAL || b.type == REAL)
        {
            // Real comparison
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            result = TypedValue((a_val <= b_val) ? 1 : 0, true);
        }
        else
        {
            // Integer comparison
            result = TypedValue((a.int_val <= b.int_val) ? 1 : 0, true);
        }
        result.type = BOOLEAN;
        return result;
    }
    
    // Arithmetic operators: + - * / ^ &
    if(node->oper==AND_OP)
    {
        // Arithmetic & operation: a^2 - b^2
        if(a.type == REAL || b.type == REAL)
        {
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            double result_val = a_val * a_val - b_val * b_val;
            result = TypedValue(result_val);
            result.type = REAL;
        }
        else
        {
            int result_val = a.int_val * a.int_val - b.int_val * b.int_val;
            result = TypedValue(result_val);
            result.type = INTEGER;
        }
        return result;
    }

    // Arithmetic operators: + - * / ^
    if(node->oper==PLUS)
    {
        if(a.type == REAL || b.type == REAL)
        {
            // Real addition
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            result.type = REAL;
            result.real_val = a_val + b_val;
        }
        else
        {
            // Integer addition
            result.type = INTEGER;
            result.int_val = a.int_val + b.int_val;
        }
        return result;
    }
    
    if(node->oper==MINUS)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            result.type = REAL;
            result.real_val = a_val - b_val;
        }
        else
        {
            result.type = INTEGER;
            result.int_val = a.int_val - b.int_val;
        }
        return result;
    }
    
    if(node->oper==TIMES)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            result.type = REAL;
            result.real_val = a_val * b_val;
        }
        else
        {
            result.type = INTEGER;
            result.int_val = a.int_val * b.int_val;
        }
        return result;
    }
    
    if(node->oper==DIVIDE)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            double b_val = (b.type == REAL) ? b.real_val : (double)b.int_val;
            if(b_val == 0.0)
            {
                printf("ERROR Division by zero\n");
                throw 0;
            }
            result.type = REAL;
            result.real_val = a_val / b_val;
        }
        else
        {
            if(b.int_val == 0)
            {
                printf("ERROR Division by zero\n");
                throw 0;
            }
            result.type = INTEGER;
            result.int_val = a.int_val / b.int_val;
        }
        return result;
    }
    
    if(node->oper==POWER)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double a_val = (a.type == REAL) ? a.real_val : (double)a.int_val;
            int b_val = (b.type == REAL) ? (int)b.real_val : b.int_val;
            result.type = REAL;
            result.real_val = RealPower(a_val, b_val);
        }
        else
        {
            result.type = INTEGER;
            result.int_val = Power(a.int_val, b.int_val);
        }
        return result;
    }

    throw 0;
    return result;
}

// Enhanced runtime execution with full type support
// Executes the abstract syntax tree with proper handling of int, real, and bool types
// Variables are stored as TypedValue structures in the variables array
void RunProgram(TreeNode* node, SymbolTable* symbol_table, TypedValue* variables)
{
    // IF statement: if (condition) then ... [else ...] end
    if(node->node_kind==IF_NODE)
    {
        // Evaluate the condition expression (must be BOOLEAN)
        TypedValue cond = Evaluate(node->child[0], symbol_table, variables);
        if(cond.bool_val)
        {
            // Condition is true: execute then branch
            RunProgram(node->child[1], symbol_table, variables);
        }
        else if(node->child[2])
        {
            // Condition is false and else branch exists: execute else branch
            RunProgram(node->child[2], symbol_table, variables);
        }
    }
    
    // DECL statement: type identifier := expression (explicit declaration with initialization)
    if(node->node_kind==DECL_NODE)
    {
        // Evaluate the initializing expression
        TypedValue v = Evaluate(node->child[0], symbol_table, variables);
        
        // Store the result in the variable
        VariableInfo* var = symbol_table->Find(node->id);
        if(var)
        {
            variables[var->memloc] = v;
        }
    }
    
    // ASSIGN statement: variable := expression
    if(node->node_kind==ASSIGN_NODE)
    {
        // Evaluate the right-hand side expression
        TypedValue v = Evaluate(node->child[0], symbol_table, variables);
        
        // Store the result in the variable
        VariableInfo* var = symbol_table->Find(node->id);
        if(var)
        {
            variables[var->memloc] = v;
        }
    }
    
    // READ statement: read variable_name
    if(node->node_kind==READ_NODE)
    {
        // Read input value of appropriate type from user
        VariableInfo* var = symbol_table->Find(node->id);
        if(var)
        {
            printf("Enter %s (%s): ", node->id, ExprDataTypeStr[var->var_type]);
            
            // Read based on variable type
            if(var->var_type == REAL)
            {
                // Read as real/double
                double input_val;
                scanf("%lf", &input_val);
                variables[var->memloc] = TypedValue(input_val);
                variables[var->memloc].type = REAL;
            }
            else if(var->var_type == BOOLEAN)
            {
                // Read as integer (0=false, non-0=true)
                int input_val;
                scanf("%d", &input_val);
                variables[var->memloc] = TypedValue(input_val, true);
                variables[var->memloc].type = BOOLEAN;
            }
            else  // INTEGER
            {
                // Read as integer
                int input_val;
                scanf("%d", &input_val);
                variables[var->memloc] = TypedValue(input_val);
                variables[var->memloc].type = INTEGER;
            }
        }
    }
    
    // WRITE statement: write expression
    if(node->node_kind==WRITE_NODE)
    {
        // Evaluate and output the expression result
        TypedValue v = Evaluate(node->child[0], symbol_table, variables);
        
        // Output based on type
        if(v.type == REAL)
        {
            printf("Val: %lf\n", v.real_val);
        }
        else if(v.type == BOOLEAN)
        {
            printf("Val: %s\n", v.bool_val ? "true" : "false");
        }
        else  // INTEGER
        {
            printf("Val: %d\n", v.int_val);
        }
    }
    
    // REPEAT statement: repeat ... until (condition)
    if(node->node_kind==REPEAT_NODE)
    {
        // Execute loop body repeatedly until condition becomes true
        do
        {
            // Execute loop body (first child)
            RunProgram(node->child[0], symbol_table, variables);
            // Evaluate condition (second child)
        }
        while(!Evaluate(node->child[1], symbol_table, variables).bool_val);
    }
    
    // Process sibling nodes (for statement sequences)
    if(node->sibling)
        RunProgram(node->sibling, symbol_table, variables);
}

// Wrapper function for RunProgram that allocates variable storage
// Creates array of TypedValue structures for all variables in the symbol table
// All variables initialized to 0 (or 0.0 for real)
void RunProgram(TreeNode* syntax_tree, SymbolTable* symbol_table)
{
    int i;
    // Allocate storage for all variables
    TypedValue* variables = new TypedValue[symbol_table->num_vars];
    
    // Initialize all variables to default values
    for(i=0;i<symbol_table->num_vars;i++)
    {
        variables[i] = TypedValue(0);
        variables[i].type = VOID;  // Will be set by actual assignments
    }
    
    // Execute the program
    RunProgram(syntax_tree, symbol_table, variables);
    
    // Clean up allocated memory
    delete[] variables;
}

////////////////////////////////////////////////////////////////////////////////////
// Scanner and Compiler ////////////////////////////////////////////////////////////

void StartCompiler(CompilerInfo* pci)
{
    TreeNode* syntax_tree=Parse(pci);

    SymbolTable symbol_table;
    Analyze(syntax_tree, &symbol_table);

    printf("Symbol Table:\n");
    symbol_table.Print();
    printf("---------------------------------\n"); fflush(NULL);

    printf("Syntax Tree:\n");
    PrintTree(syntax_tree);
    printf("---------------------------------\n"); fflush(NULL);

    printf("Run Program:\n");
    RunProgram(syntax_tree, &symbol_table);
    printf("---------------------------------\n"); fflush(NULL);

    symbol_table.Destroy();
    DestroyTree(syntax_tree);
}

////////////////////////////////////////////////////////////////////////////////////
// Scanner only ////////////////////////////////////////////////////////////////////

void StartScanner(CompilerInfo* pci)
{
    Token token;

    while(true)
    {
        GetNextToken(pci, &token);
        printf("[%d] %s (%s)\n", pci->in_file.cur_line_num, token.str, TokenTypeStr[token.type]); fflush(NULL);
        if(token.type==ENDFILE || token.type==ERROR) break;
    }
}

////////////////////////////////////////////////////////////////////////////////////

int main()
{
    printf("Start main()\n"); fflush(NULL);

    CompilerInfo compiler_info("input.txt", "output.txt", "debug.txt");

    StartCompiler(&compiler_info);

    printf("End main()\n"); fflush(NULL);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
