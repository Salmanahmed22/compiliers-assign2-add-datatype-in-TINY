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
int x;
real y;
bool flag;
x := 5;
y := 3.5;
flag := 5 < 10;
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
{ Test 3: Mixed type arithmetic (int to real conversion) }
y := 5 + 3.5;
y := 10 * 2.0;
{ Test 4: Comparison operators returning boolean }
flag := 5 < 10;
flag := 10 = 10;
flag := 3 > 5;
flag := 4 >= 4;
flag := 2 <= 3;
{ Test 5: Real comparisons }
flag := 3.5 < 4.0;
flag := 2.5 = 2.5;
flag := 4.0 > 3.5;
flag := 3.0 >= 3.0;
flag := 2.5 <= 3.0;
{ Test 6: Conditional with boolean }
if flag then
x := 100
end;
{ Test 7: Repeat loop }
repeat
x := x + 1
until x = 110;
{ Test 8: Write results }
write x + 5;
write y * 2.0;
write 10 < 5;
write 5 > 3;
write 4 >= 4;
write 2 <= 3;
*/
enum TokenType
{
    ASSIGN, EQUAL, LESS_THAN, GREATER_THAN, GREATER_EQUAL, LESS_EQUAL, // Comparison operators
    INT_TYPE, REAL_TYPE, BOOL_TYPE  // Type keywords: int, real, bool
};
const char* TokenTypeStr[]=
{
    "Assign", "Equal", "LessThan", "GreaterThan", "GreaterEqual", "LessEqual", // Comparison operators for debugging
    "IntType", "RealType", "BoolType"  // Type keywords for debugging
};
const Token reserved_words[]=
{
    Token(INT_TYPE, "int"),     // Type keyword for integer type
    Token(REAL_TYPE, "real"),   // Type keyword for real (double) type
    Token(BOOL_TYPE, "bool")    // Type keyword for boolean type
};
const Token symbolic_tokens[]=
{
    Token(GREATER_EQUAL, ">="),  // add >=
    Token(LESS_EQUAL, "<="),     // add <=
    Token(GREATER_THAN, ">"),    // add >
};
void GetNextToken(CompilerInfo* pci, Token* ptoken)
{
    else if(IsDigit(s[0]))
    {
        // Parse numeric variable
        int has_decimal = 0;
        // Parse digits and optional decimal point for real numbers instead of integers only
        while(IsDigit(s[j]) || (s[j]=='.' && !has_decimal)){
            if(s[j]=='.') has_decimal=1;
            j++;
        }
    }
}
// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt | declstmt
// declstmt -> type identifier := expr       (explicit type declaration with assignment)
enum NodeKind
{
    DECL_NODE  // DECL_NODE for type declarations
};
const char* NodeKindStr[]=
{
    "Decl"  // Added Decl for for debugging 
};
enum ExprDataType {REAL}; // added REAL type
const char* ExprDataTypeStr[]=
{
    "Real"  // Added Real type for debugging
};
struct TreeNode
{
    union{TokenType oper; int num; double real_num; char* id;}; // added real_num for real values
    
    ExprDataType expr_data_type;        // Data type of RHS expression result -> already exists
    ExprDataType var_type;              // Variable type (only for ID_NODE): INTEGER, REAL, or BOOLEAN                       
    
    TreeNode() {int i; for(i=0;i<3;i++) child[i]=0; sibling=0; expr_data_type=VOID;var_type=VOID;real_num=0.0;}
};
TreeNode* NewExpr(CompilerInfo* pci, ParseInfo* ppi)
{
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
                // Handle decimal point
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
}
// expr -> mathexpr [ (<|=) mathexpr ]
TreeNode* Expr(CompilerInfo* pci, ParseInfo* ppi)
{
    if(ppi->next_token.type==EQUAL || ppi->next_token.type==LESS_THAN ||
        ppi->next_token.type==GREATER_THAN || ppi->next_token.type==GREATER_EQUAL ||
        ppi->next_token.type==LESS_EQUAL) // added greater than, greater equal and less equal
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
    tree->var_type=decl_type;
    if(ppi->next_token.type==ID) AllocateAndCopy(&tree->id, ppi->next_token.str);
    Match(pci, ppi, ID);
    pci->debug_file.Out("End DeclStmt");
    return tree;
}
// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt | declstmt
TreeNode* Stmt(CompilerInfo* pci, ParseInfo* ppi)
{
    if(ppi->next_token.type==INT_TYPE)
    {
        Match(pci, ppi, INT_TYPE);
        tree=DeclStmt(pci, ppi, INTEGER);
    }
    else if(ppi->next_token.type==REAL_TYPE) // same as INT_TYPE
    else if(ppi->next_token.type==BOOL_TYPE) // same as INT_TYPE
}
void PrintTree(TreeNode* node, int sh=0)
{
    if(node->node_kind==OPER_NODE) printf("[%s]", TokenTypeStr[node->oper]);
    else if(node->node_kind==NUM_NODE)
    {
        // Print numeric value based on type
        if(node->expr_data_type == REAL) printf("[%lf]", node->real_num);
        else printf("[%d]", node->num);
    }
    else if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || node->node_kind==ASSIGN_NODE || node->node_kind==DECL_NODE)
        printf("[%s]", node->id);

    // Print variable type for declarations and variable references
    if(node->node_kind==DECL_NODE)
        printf("[%s]", ExprDataTypeStr[node->var_type]);
    else if(node->expr_data_type!=VOID) printf("[%s]", ExprDataTypeStr[node->expr_data_type]);
}
void DestroyTree(TreeNode* node)
{
    if( node->node_kind==DECL_NODE) // add declnode
}
struct VariableInfo
{
    ExprDataType var_type; // Variable type: INTEGER, REAL, or BOOLEAN (only for ID_NODE)
};
// Insert function in SymbolTable
void Insert(const char* name, int line_num, ExprDataType type)// add type parameter
{   
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
        }
    }
    vi->var_type=type; // Store the variable's data type
}
void Analyze(TreeNode* node, SymbolTable* symbol_table)
{
    // add handling for explicit type declarations
    if(node->node_kind==DECL_NODE)
    {
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var)
        {
            symbol_table->Insert(node->id, node->line_num, node->var_type);
        }
    }
    // modify this condition to avoid boolean type comparisons with < or =
    if(node->node_kind==OPER_NODE)
    {
        if(node->oper==LESS_THAN || node->oper==LESS_EQUAL || node->oper==GREATER_THAN || node->oper==GREATER_EQUAL)
        {
            node->expr_data_type = BOOLEAN;
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
TypedValue Evaluate(TreeNode* node, SymbolTable* symbol_table, TypedValue* variables)
{
    TypedValue result;
    
    // NUM_NODE: numeric literal (int or real)
    if(node->node_kind==NUM_NODE) // for real numbers
    {
        if(node->expr_data_type == REAL)
            return TypedValue(node->real_num);
        else
            return TypedValue(node->num);
    }
    
    // ID_NODE: variable reference
    if(node->node_kind==ID_NODE)

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
    
    if(node->oper==LESS_THAN)  // same as EQUAL

    if(node->oper==GREATER_THAN)  // same as EQUAL

    if(node->oper==GREATER_EQUAL)  // same as EQUAL

    if(node->oper==LESS_EQUAL) // same as EQUAL
    

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
    
    if(node->oper==MINUS) // same as PLUS
    
    if(node->oper==TIMES) // same as PLUS
    
    if(node->oper==DIVIDE) // same as PLUS
    
    if(node->oper==POWER) // same as PLUS

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
    // DECL statement: type identifier := expression (explicit declaration with initialization)
    if(node->node_kind==DECL_NODE)
    
    // ASSIGN statement: variable := expression
    if(node->node_kind==ASSIGN_NODE)
    
    // READ statement: read variable_name
    if(node->node_kind==READ_NODE)
    // WRITE statement: write expression
    if(node->node_kind==WRITE_NODE)
    
    // REPEAT statement: repeat ... until (condition)
    if(node->node_kind==REPEAT_NODE)
    
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
}