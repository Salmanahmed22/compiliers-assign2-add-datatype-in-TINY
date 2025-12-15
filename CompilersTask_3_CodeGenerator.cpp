#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;

/*
Modified TINY compiler with three data types: int, real, bool
- Variables must be declared at the beginning: int x; real y; bool flag;
- Bool can be used in if/repeat conditions, but not int/real
- No arithmetic on bool
- Arithmetic on int/real, can mix (int to real conversion)
- No assignment between different types
- Added >, >=, <= operators
- Declarations: int x; (no initialization)

30 Test Cases:
1. int x;
2. real y;
3. bool flag;
4. x := 5;
5. y := 3.5;
6. flag := true;
7. x := x + 1;
8. y := y * 2.0;
9. flag := x < 10;
10. flag := y >= 3.0;
11. x := 2 ^ 3;
12. y := 4.0 ^ 2.0;
13. flag := 5 > 3;
14. flag := 2 <= 2;
15. flag := 3.5 = 3.5;
16. if flag then x := 100 end;
17. repeat x := x + 1 until x = 105;
18. write x;
19. write y;
20. write flag;
21. read x;
22. read y;
23. read flag;
24. x := x ^ 2;
25. y := y / 2.0;
26. x := 10 - x;
27. y := y + 1.5;
28. flag := x >= 5;
29. write x + y;
30. flag := 1 > 0;
*/

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

#define MAX_TOKEN_LEN 40

enum TokenType{
                IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
                ASSIGN, EQUAL, LESS_THAN, GREATER_THAN, GREATER_EQUAL, LESS_EQUAL,
                PLUS, MINUS, TIMES, DIVIDE, POWER,
                SEMI_COLON,
                LEFT_PAREN, RIGHT_PAREN,
                LEFT_BRACE, RIGHT_BRACE,
                ID, NUM,
                TRUE, FALSE,
                ENDFILE, ERROR, INT_TYPE, REAL_TYPE, BOOL_TYPE
              };

// Used for debugging only /////////////////////////////////////////////////////////
const char* TokenTypeStr[]=
            {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan", "GreaterThan", "GreaterEqual", "LessEqual",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "True", "False",
                "EndFile", "Error", "IntType", "RealType", "BoolType"
            };

struct Token
{
    TokenType type;
    char str[MAX_TOKEN_LEN+1];

    Token(){str[0]=0; type=ERROR;}
    Token(TokenType _type, const char* _str) {type=_type; Copy(str, _str);}
};

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
    Token(INT_TYPE, "int"),
    Token(REAL_TYPE, "real"),
    Token(BOOL_TYPE, "bool"),
    Token(TRUE, "true"),
    Token(FALSE, "false")
};
const int num_reserved_words=sizeof(reserved_words)/sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[]=
{
    Token(ASSIGN, ":="),
    Token(EQUAL, "="),
    Token(GREATER_EQUAL, ">="),
    Token(LESS_EQUAL, "<="),
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
    Token(RIGHT_BRACE, "}")
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
    else if(IsDigit(s[0]) || (s[0]=='.' && IsDigit(s[1])))
    {
        int j=1;
        while(IsDigit(s[j]) || s[j]=='.') j++;

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
// readstmt -> read identifier
// writestmt -> write expr
// expr -> mathexpr [ (<|=) mathexpr ]
// mathexpr -> term { (+|-) term }    left associative
// term -> factor { (*|/) factor }    left associative
// factor -> newexpr { ^ newexpr }    right associative
// newexpr -> ( mathexpr ) | number | identifier

enum NodeKind{
                IF_NODE, REPEAT_NODE, ASSIGN_NODE, READ_NODE, WRITE_NODE,
                OPER_NODE, NUM_NODE, ID_NODE, DECL_NODE
             };

// Used for debugging only /////////////////////////////////////////////////////////
const char* NodeKindStr[]=
            {
                "If", "Repeat", "Assign", "Read", "Write",
                "Oper", "Num", "ID", "Decl"
            };

enum ExprDataType {VOID, INTEGER, REAL, BOOLEAN};

// Used for debugging only /////////////////////////////////////////////////////////
const char* ExprDataTypeStr[]=
            {
                "Void", "Integer", "Real", "Boolean"
            };

#define MAX_CHILDREN 3

struct TreeNode
{
    TreeNode* child[MAX_CHILDREN];
    TreeNode* sibling; // used for sibling statements only

    NodeKind node_kind;

    union{TokenType oper; int num; double real_num; char* id;}; // defined for expression/int/identifier only
    ExprDataType expr_data_type; // defined for expression/int/identifier only
    ExprDataType var_type; // Variable type for ID_NODE and DECL_NODE

    int line_num;

    TreeNode() {int i; for(i=0;i<MAX_CHILDREN;i++) child[i]=0; sibling=0; expr_data_type=VOID; var_type=VOID; real_num=0.0;}
};

struct ParseInfo
{
    Token next_token;
};

void Match(CompilerInfo* pci, ParseInfo* ppi, TokenType expected_token_type)
{
    pci->debug_file.Out("Start Match");

    if(ppi->next_token.type!=expected_token_type) throw 0;
    GetNextToken(pci, &ppi->next_token);

    fprintf(pci->debug_file.file, "[%d] %s (%s)\n", pci->in_file.cur_line_num, ppi->next_token.str, TokenTypeStr[ppi->next_token.type]); fflush(pci->debug_file.file);
}

TreeNode* MathExpr(CompilerInfo*, ParseInfo*);

// newexpr -> ( mathexpr ) | number | identifier
TreeNode* NewExpr(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start NewExpr");

    // Compare the next token with the First() of possible statements
    if(ppi->next_token.type==NUM)
    {
        TreeNode* tree=new TreeNode;
        tree->node_kind=NUM_NODE;
        char* num_str=ppi->next_token.str;
        // Check if real number (contains decimal point)
        bool is_real = false;
        char* temp = num_str;
        while(*temp) { if(*temp == '.') { is_real = true; break; } temp++; }
        if(is_real)
        {
            // Parse as real
            tree->real_num = atof(num_str);
            tree->expr_data_type = REAL;
        }
        else
        {
            // Parse as integer
            tree->num = atoi(num_str);
            tree->expr_data_type = INTEGER;
        }
        tree->line_num=pci->in_file.cur_line_num;
        Match(pci, ppi, ppi->next_token.type);

        pci->debug_file.Out("End NewExpr");
        return tree;
    }

    if(ppi->next_token.type==TRUE)
    {
        TreeNode* tree=new TreeNode;
        tree->node_kind=NUM_NODE;
        tree->num = 1;
        tree->expr_data_type = BOOLEAN;
        tree->line_num=pci->in_file.cur_line_num;
        Match(pci, ppi, TRUE);

        pci->debug_file.Out("End NewExpr");
        return tree;
    }

    if(ppi->next_token.type==FALSE)
    {
        TreeNode* tree=new TreeNode;
        tree->node_kind=NUM_NODE;
        tree->num = 0;
        tree->expr_data_type = BOOLEAN;
        tree->line_num=pci->in_file.cur_line_num;
        Match(pci, ppi, FALSE);

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

// term -> factor { (*|/) factor }    left associative
TreeNode* Term(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Term");

    TreeNode* tree=Factor(pci, ppi);

    while(ppi->next_token.type==TIMES || ppi->next_token.type==DIVIDE)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=Factor(pci, ppi);

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

// declstmt -> type identifier ;
TreeNode* DeclStmt(CompilerInfo* pci, ParseInfo* ppi, ExprDataType decl_type)
{
    pci->debug_file.Out("Start DeclStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=DECL_NODE;
    tree->line_num=pci->in_file.cur_line_num;
    tree->var_type=decl_type;

    if(ppi->next_token.type==ID) AllocateAndCopy(&tree->id, ppi->next_token.str);
    Match(pci, ppi, ID);

    pci->debug_file.Out("End DeclStmt");
    return tree;
}

// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt | declstmt
TreeNode* Stmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Stmt");

    // Compare the next token with the First() of possible statements
    TreeNode* tree=0;
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
    else throw 0;

    pci->debug_file.Out("End Stmt");
    return tree;
}

// stmtseq -> stmt { ; stmt }
TreeNode* StmtSeq(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start StmtSeq");

    TreeNode* first_tree=Stmt(pci, ppi);
    TreeNode* last_tree=first_tree;

    while(ppi->next_token.type == SEMI_COLON)
    {
        Match(pci, ppi, SEMI_COLON);
        if(ppi->next_token.type == ENDFILE) break;
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
        if(node->expr_data_type == REAL) printf("[%.2f]", node->real_num);
        else printf("[%d]", node->num);
    }
    else if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || node->node_kind==ASSIGN_NODE || node->node_kind==DECL_NODE) printf("[%s]", node->id);

    if(node->node_kind==DECL_NODE) printf("[%s]", ExprDataTypeStr[node->var_type]);
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

struct VariableInfo
{
    char* name;
    int memloc;
    ExprDataType var_type; // Type of the variable
    LineLocation* head_line; // the head of linked list of source line locations
    LineLocation* tail_line; // the tail of linked list of source line locations
    VariableInfo* next_var; // the next variable in the linked list in the same hash bucket of the symbol table
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
        LineLocation* lineloc=new LineLocation;
        lineloc->line_num=line_num;
        lineloc->next=0;

        int h=Hash(name);
        VariableInfo* prev=0;
        VariableInfo* cur=var_info[h];

        while(cur)
        {
            if(Equals(name, cur->name))
            {
                if(cur->var_type != type)
                {
                    printf("ERROR Type mismatch: variable '%s' already declared with type '%s', attempted redeclaration with type '%s'\n", 
                           name, ExprDataTypeStr[cur->var_type], ExprDataTypeStr[type]);
                    throw 0;
                }
                // just add this line location to the list of line locations of the existing var
                cur->tail_line->next=lineloc;
                cur->tail_line=lineloc;
                return;
            }
            prev=cur;
            cur=cur->next_var;
        }

        VariableInfo* vi=new VariableInfo;
        vi->head_line=vi->tail_line=lineloc;
        vi->next_var=0;
        vi->memloc=num_vars++;
        vi->var_type=type;
        AllocateAndCopy(&vi->name, name);

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

void Analyze(TreeNode* node, SymbolTable* symbol_table)
{
    int i;

    // PASS 1: Analyze children first
    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) Analyze(node->child[i], symbol_table);

    // Handle declarations
    if(node->node_kind==DECL_NODE)
    {
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var)
        {
            symbol_table->Insert(node->id, node->line_num, node->var_type);
        }
    }

    // Determine expression types
    if(node->node_kind==OPER_NODE)
    {
        ExprDataType left_type = node->child[0]->expr_data_type;
        ExprDataType right_type = node->child[1]->expr_data_type;

        // Comparison operators return BOOLEAN
        if(node->oper==EQUAL || node->oper==LESS_THAN || node->oper==GREATER_THAN ||
           node->oper==GREATER_EQUAL || node->oper==LESS_EQUAL)
        {
            node->expr_data_type = BOOLEAN;
            if(left_type == BOOLEAN || right_type == BOOLEAN)
            {
                printf("ERROR Line %d: Cannot compare BOOLEAN values\n", node->line_num);
                throw 0;
            }
        }
        // Arithmetic operators
        else
        {
            if(left_type == BOOLEAN || right_type == BOOLEAN)
            {
                printf("ERROR Line %d: Arithmetic operator applied to BOOLEAN\n", node->line_num);
                throw 0;
            }
            if(left_type == REAL || right_type == REAL)
                node->expr_data_type = REAL;
            else
                node->expr_data_type = INTEGER;
        }
    }
    else if(node->node_kind==ID_NODE)
    {
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var)
        {
            printf("ERROR Line %d: Undefined variable '%s'\n", node->line_num, node->id);
            throw 0;
        }
        node->expr_data_type = var->var_type;
        node->var_type = var->var_type;
    }
    // NUM_NODE type already set in parsing

    // Type checks for statements
    if(node->node_kind==ASSIGN_NODE)
    {
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var)
        {
            printf("ERROR Line %d: Undefined variable '%s'\n", node->line_num, node->id);
            throw 0;
        }
        if(var->var_type != node->child[0]->expr_data_type)
        {
            printf("ERROR Line %d: Type mismatch in assignment: %s = %s\n",
                   node->line_num, ExprDataTypeStr[var->var_type], ExprDataTypeStr[node->child[0]->expr_data_type]);
            throw 0;
        }
        node->var_type = var->var_type;
    }

    if(node->node_kind==IF_NODE)
    {
        if(node->child[0]->expr_data_type != BOOLEAN)
        {
            printf("ERROR Line %d: IF condition must be BOOLEAN\n", node->line_num);
            throw 0;
        }
    }

    if(node->node_kind==REPEAT_NODE)
    {
        if(node->child[1]->expr_data_type != BOOLEAN)
        {
            printf("ERROR Line %d: REPEAT condition must be BOOLEAN\n", node->line_num);
            throw 0;
        }
    }

    if(node->node_kind==WRITE_NODE)
    {
        if(node->child[0]->expr_data_type == VOID)
        {
            printf("ERROR Line %d: Cannot write VOID expression\n", node->line_num);
            throw 0;
        }
    }

    if(node->sibling) Analyze(node->sibling, symbol_table);
}

////////////////////////////////////////////////////////////////////////////////////
// Code Generator //////////////////////////////////////////////////////////////////

// Structure to represent typed values during evaluation
struct TypedValue
{
    ExprDataType type;
    int int_val;
    double real_val;
    int bool_val;

    TypedValue() : type(VOID), int_val(0), real_val(0.0), bool_val(0) {}
    TypedValue(int val) : type(INTEGER), int_val(val), real_val((double)val), bool_val(0) {}
    TypedValue(double val) : type(REAL), int_val(0), real_val(val), bool_val(0) {}
    TypedValue(int val, bool) : type(BOOLEAN), int_val(val), real_val(0.0), bool_val(val) {}
};

int Power(int a, int b)
{
    if(a==0) return 0;
    if(b==0) return 1;
    if(b>=1) return a*Power(a, b-1);
    return 0;
}

double RealPower(double a, int b)
{
    if(a==0.0) return 0.0;
    if(b==0) return 1.0;
    if(b>0)
    {
        double result = 1.0;
        for(int i=0;i<b;i++) result *= a;
        return result;
    }
    return 0.0;
}

TypedValue Evaluate(TreeNode* node, SymbolTable* symbol_table, TypedValue* variables)
{
    TypedValue result;

    if(node->node_kind==NUM_NODE)
    {
        if(node->expr_data_type == REAL)
            result = TypedValue(node->real_num);
        else
            result = TypedValue(node->num);
        return result;
    }

    if(node->node_kind==ID_NODE)
    {
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var) throw 0;
        return variables[var->memloc];
    }

    TypedValue a = Evaluate(node->child[0], symbol_table, variables);
    TypedValue b = Evaluate(node->child[1], symbol_table, variables);

    if(node->oper==EQUAL)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            result = TypedValue(av == bv ? 1 : 0, true);
        }
        else
            result = TypedValue(a.int_val == b.int_val ? 1 : 0, true);
    }
    else if(node->oper==LESS_THAN)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            result = TypedValue(av < bv ? 1 : 0, true);
        }
        else
            result = TypedValue(a.int_val < b.int_val ? 1 : 0, true);
    }
    else if(node->oper==GREATER_THAN)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            result = TypedValue(av > bv ? 1 : 0, true);
        }
        else
            result = TypedValue(a.int_val > b.int_val ? 1 : 0, true);
    }
    else if(node->oper==GREATER_EQUAL)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            result = TypedValue(av >= bv ? 1 : 0, true);
        }
        else
            result = TypedValue(a.int_val >= b.int_val ? 1 : 0, true);
    }
    else if(node->oper==LESS_EQUAL)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            result = TypedValue(av <= bv ? 1 : 0, true);
        }
        else
            result = TypedValue(a.int_val <= b.int_val ? 1 : 0, true);
    }
    else if(node->oper==PLUS)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            result = TypedValue(av + bv);
        }
        else
            result = TypedValue(a.int_val + b.int_val);
    }
    else if(node->oper==MINUS)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            result = TypedValue(av - bv);
        }
        else
            result = TypedValue(a.int_val - b.int_val);
    }
    else if(node->oper==TIMES)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            result = TypedValue(av * bv);
        }
        else
            result = TypedValue(a.int_val * b.int_val);
    }
    else if(node->oper==DIVIDE)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            double bv = b.type == REAL ? b.real_val : (double)b.int_val;
            if(bv == 0.0) { printf("ERROR Division by zero\n"); throw 0; }
            result = TypedValue(av / bv);
        }
        else
        {
            if(b.int_val == 0) { printf("ERROR Division by zero\n"); throw 0; }
            result = TypedValue(a.int_val / b.int_val);
        }
    }
    else if(node->oper==POWER)
    {
        if(a.type == REAL || b.type == REAL)
        {
            double av = a.type == REAL ? a.real_val : (double)a.int_val;
            int bv = b.type == REAL ? (int)b.real_val : b.int_val;
            result = TypedValue(RealPower(av, bv));
        }
        else
            result = TypedValue(Power(a.int_val, b.int_val));
    }

    return result;
}

void RunProgram(TreeNode* node, SymbolTable* symbol_table, TypedValue* variables)
{
    if(node->node_kind==DECL_NODE)
    {
        VariableInfo* var = symbol_table->Find(node->id);
        if(var)
        {
            // Set the type for the variable
            variables[var->memloc].type = var->var_type;
            // Default value is already 0/0.0/false
        }
    }
    else if(node->node_kind==IF_NODE)
    {
        TypedValue cond = Evaluate(node->child[0], symbol_table, variables);
        if(cond.bool_val) RunProgram(node->child[1], symbol_table, variables);
        else if(node->child[2]) RunProgram(node->child[2], symbol_table, variables);
    }
    else if(node->node_kind==ASSIGN_NODE)
    {
        TypedValue v = Evaluate(node->child[0], symbol_table, variables);
        VariableInfo* var = symbol_table->Find(node->id);
        if(var) variables[var->memloc] = v;
    }
    else if(node->node_kind==READ_NODE)
    {
        VariableInfo* var = symbol_table->Find(node->id);
        if(var)
        {
            printf("Enter %s (%s): ", node->id, ExprDataTypeStr[var->var_type]);
            if(var->var_type == REAL)
            {
                double input;
                scanf("%lf", &input);
                variables[var->memloc] = TypedValue(input);
            }
            else if(var->var_type == BOOLEAN)
            {
                int input;
                scanf("%d", &input);
                variables[var->memloc] = TypedValue(input, true);
            }
            else
            {
                int input;
                scanf("%d", &input);
                variables[var->memloc] = TypedValue(input);
            }
        }
    }
    else if(node->node_kind==WRITE_NODE)
    {
        TypedValue v = Evaluate(node->child[0], symbol_table, variables);
        if(v.type == REAL) printf("Val: %lf\n", v.real_val);
        else if(v.type == BOOLEAN) printf("Val: %s\n", v.bool_val ? "true" : "false");
        else printf("Val: %d\n", v.int_val);
    }
    else if(node->node_kind==REPEAT_NODE)
    {
        do
        {
           RunProgram(node->child[0], symbol_table, variables);
        }
        while(!Evaluate(node->child[1], symbol_table, variables).bool_val);
    }
    if(node->sibling) RunProgram(node->sibling, symbol_table, variables);
}

void RunProgram(TreeNode* syntax_tree, SymbolTable* symbol_table)
{
    int i;
    TypedValue* variables = new TypedValue[symbol_table->num_vars];
    for(i=0;i<symbol_table->num_vars;i++)
    {
        variables[i] = TypedValue(0); // Default to 0
        variables[i].type = VOID;
    }
    RunProgram(syntax_tree, symbol_table, variables);
    delete[] variables;
}

////////////////////////////////////////////////////////////////////////////////////
// Scanner and Compiler ////////////////////////////////////////////////////////////

void StartCompiler(CompilerInfo* pci)
{
    TreeNode* syntax_tree;
    try {
        syntax_tree = Parse(pci);
    } catch(int) {
        printf("Error in Parse\n");
        return;
    }

    SymbolTable symbol_table;
    try {
        Analyze(syntax_tree, &symbol_table);
    } catch(int) {
        printf("Error in Analyze\n");
        return;
    }

    printf("Symbol Table:\n");
    symbol_table.Print();
    printf("---------------------------------\n"); fflush(NULL);

    printf("Syntax Tree:\n");
    PrintTree(syntax_tree);
    printf("---------------------------------\n"); fflush(NULL);

    printf("Run Program:\n");
    try {
        RunProgram(syntax_tree, &symbol_table);
    } catch(int) {
        printf("Error in RunProgram\n");
        return;
    }
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

    StartScanner(&compiler_info);

    printf("End main()\n"); fflush(NULL);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
