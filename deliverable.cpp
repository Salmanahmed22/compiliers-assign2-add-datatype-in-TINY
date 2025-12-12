#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
using namespace std;
/*
    20220195
    20220120
    20221072
*/
/*
int x := 5;
real y := 3.5;
bool flag := 5 < 10;

{ Test 1: Basic arithmetic operations }
x := x + 3;
x := x - 2;
x := x * 4;
x := x / 2;
x := 2 ^ 3;

{ Test 2: Real arithmetic operations }
y := y + 2.5;
y := y - 1.0;
y := y * 2.0;
y := y / 2.5;
*/
enum TokenType{
                IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
                ASSIGN, EQUAL, LESS_THAN, GREATER_THAN, GREATER_EQUAL, LESS_EQUAL, // Comparison operators
                PLUS, MINUS, TIMES, DIVIDE, POWER,
                SEMI_COLON,
                LEFT_PAREN, RIGHT_PAREN,
                LEFT_BRACE, RIGHT_BRACE,
                ID, NUM,
                ENDFILE, ERROR, AND_OP,  // added AND_OP for Scanner
                INT_TYPE, REAL_TYPE, BOOL_TYPE  // Type keywords: int, real, bool
};

const char* TokenTypeStr[]=
            {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan", "GreaterThan", "GreaterEqual", "LessEqual", // Comparison operators for debugging
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error", "And",     /// added AND_OP for Scanner
                "IntType", "RealType", "BoolType"  // Type keywords for debugging
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
    Token(INT_TYPE, "int"),     // Type keyword for integer type
    Token(REAL_TYPE, "real"),   // Type keyword for real (double) type
    Token(BOOL_TYPE, "bool")    // Type keyword for boolean type
};
const Token symbolic_tokens[]=
{
    Token(ASSIGN, ":="),
    Token(EQUAL, "="),
    Token(GREATER_EQUAL, ">="),  // add >=
    Token(LESS_EQUAL, "<="),     // add <=
    Token(GREATER_THAN, ">"),    // add >
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

void GetNextToken(CompilerInfo* pci, Token* ptoken){
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
        // Parse numeric variable - support both integers and real numbers
        int j=1;
        int has_decimal = 0;
        
        // Parse digits and optional decimal point for real numbers instead of integers only
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
// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt | declstmt
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

const char* NodeKindStr[]=
            {
                "If", "Repeat", "Assign", "Read", "Write",
                "Oper", "Num", "ID", "Decl"  // Added Decl for for debugging 
            };


enum ExprDataType {VOID, INTEGER, REAL, BOOLEAN}; // added REAL type


const char* ExprDataTypeStr[]=
{
    "Void", "Integer", "Real", "Boolean"  // Added Real type for debugging
};

#define MAX_CHILDREN 3
struct TreeNode
{
    TreeNode* child[MAX_CHILDREN];
    TreeNode* sibling;

    NodeKind node_kind;

    
    union{TokenType oper; int num; double real_num; char* id;}; // added real_num for real values
    
    ExprDataType expr_data_type;        // Data type of RHS expression result -> already exists
    ExprDataType var_type;              // Variable type (only for ID_NODE): INTEGER, REAL, or BOOLEAN

    int line_num;                       

    
        TreeNode() {int i; for(i=0;i<MAX_CHILDREN;i++) child[i]=0; sibling=0; expr_data_type=VOID;var_type=VOID;real_num=0.0;}

};

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
        
        // remove the loop parsing as integer only
        // check if the number contains a decimal point
        int has_decimal = 0;
        char* temp_str = num_str;
        while(*temp_str)
        {
            if(*temp_str == '.') { has_decimal = 1; break; }
            temp_str++;
        }
        
        if(has_decimal)
        {
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
            // Parse as integer -> already exists
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

// expr -> mathexpr [ (<|=) mathexpr ]
TreeNode* Expr(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Expr");

    TreeNode* tree=MathExpr(pci, ppi);

    if(ppi->next_token.type==EQUAL || ppi->next_token.type==LESS_THAN ||
        ppi->next_token.type==GREATER_THAN || ppi->next_token.type==GREATER_EQUAL ||
        ppi->next_token.type==LESS_EQUAL) // added greater than, greater equal and less equal
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

// declstmt -> type identifier := expr
// Explicit type declaration with initialization
// Examples: int x := 5; real y; bool flag;
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
    // Allow optional initializer: `int x;` or `int x := expr;`
    if(ppi->next_token.type==ASSIGN)
    {
        Match(pci, ppi, ASSIGN);
        tree->child[0]=Expr(pci, ppi);  // Parse the initializing expression
    }
    else
    {
        tree->child[0]=0; // no initializer
    }

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

    if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || node->node_kind==ASSIGN_NODE || node->node_kind==DECL_NODE) // add declnode
        if(node->id) delete[] node->id;

    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) DestroyTree(node->child[i]);
    if(node->sibling) DestroyTree(node->sibling);

    delete node;
}

struct VariableInfo
{
    char* name;
    int memloc;
    ExprDataType var_type; // Variable type: INTEGER, REAL, or BOOLEAN (only for ID_NODE)
    LineLocation* head_line;
    LineLocation* tail_line;
    VariableInfo* next_var;
};

// Insert function in SymbolTable
void Insert(const char* name, int line_num, ExprDataType type)// add type parameter
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
                // Variable already exists - check type consistency
                if(cur->var_type != type)
                {
                    // Type mismatch: variable already declared with different type
                    printf("ERROR Type mismatch: variable '%s' already declared with type '%s', attempted redeclaration with type '%s'\n", 
                           name, ExprDataTypeStr[cur->var_type], ExprDataTypeStr[type]);
                    throw 0;
                }
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
    vi->var_type=type; // Store the variable's data type

    AllocateAndCopy(&vi->name, name);

    if(!prev) var_info[h]=vi;
    else prev->next_var=vi;
}

void Analyze(TreeNode* node, SymbolTable* symbol_table)
{
    int i;

    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) Analyze(node->child[i], symbol_table);

    // add handling for explicit type declarations
    if(node->node_kind==DECL_NODE)
    {
        // Declared type is stored in node->var_type
        // RHS expression type is in node->child[0]->expr_data_type (if initializer present)
        ExprDataType rhs_type = VOID;
        if(node->child[0]) rhs_type = node->child[0]->expr_data_type;
        ExprDataType decl_type = node->var_type;
        // If initializer present, check compatibility
        if(node->child[0])
        {
            if(decl_type != rhs_type)
            {
                printf("ERROR Line %d: Declaration type mismatch: cannot assign %s to %s variable '%s'\n",
                       node->line_num,
                       ExprDataTypeStr[rhs_type],
                       ExprDataTypeStr[decl_type],
                       node->id);
                throw 0;
            }
        }

        // Register the variable with its declared type (initializer optional)
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var)
        {
            symbol_table->Insert(node->id, node->line_num, decl_type);
        }
    }
    // modify this condition to avoid boolean type comparisons with < or =
    if(node->node_kind==OPER_NODE)
    {
        ExprDataType left_type = node->child[0]->expr_data_type;
        ExprDataType right_type = node->child[1]->expr_data_type;
        
        // Comparison operators (<, =, <=, >, >=) return BOOLEAN type
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
        // Arithmetic operators: + - * / ^
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
    // Set type for NUM_NODE (integer or real value)
    else if(node->node_kind==NUM_NODE)
    {
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
                node->expr_data_type = INTEGER;  // Default integer value
            node->var_type = var->var_type;
        }
    }

    if(node->node_kind==ASSIGN_NODE)
    {
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


struct TypedValue
{
    ExprDataType type;      // Type of the value: INTEGER, REAL, or BOOLEAN
    int int_val;
    double real_val;
    int bool_val; 
    
    TypedValue() : type(VOID), int_val(0), real_val(0.0), bool_val(0) {}
    
    TypedValue(int val) : type(INTEGER), int_val(val), real_val((double)val), bool_val(0) {}
    
    TypedValue(double val) : type(REAL), int_val(0), real_val(val), bool_val(0) {}
    
    TypedValue(int val, bool) : type(BOOLEAN), int_val(val), real_val(0.0), bool_val(val) {}
};

// Power function for real numbers
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

