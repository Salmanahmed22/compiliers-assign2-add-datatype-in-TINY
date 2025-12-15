#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
using namespace std;

// Only the changed parts from myfile.cpp for optional declaration initialization

// Modified DeclStmt to allow optional initializer
TreeNode* DeclStmt(CompilerInfo* pci, ParseInfo* ppi, ExprDataType decl_type)
{
    pci->debug_file.Out("Start DeclStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=DECL_NODE;
    tree->line_num=pci->in_file.cur_line_num;
    tree->var_type=decl_type;

    if(ppi->next_token.type==ID) AllocateAndCopy(&tree->id, ppi->next_token.str);
    Match(pci, ppi, ID);
    // Allow optional initializer: `int x;` or `int x := expr;`
    if(ppi->next_token.type==ASSIGN)
    {
        Match(pci, ppi, ASSIGN);
        tree->child[0]=Expr(pci, ppi);
    }
    else
    {
        tree->child[0]=0; // no initializer
    }

    pci->debug_file.Out("End DeclStmt");
    return tree;
}

// Modified Analyze for DECL_NODE
if(node->node_kind==DECL_NODE)
{
    ExprDataType rhs_type = VOID;
    if(node->child[0]) rhs_type = node->child[0]->expr_data_type;
    ExprDataType decl_type = node->var_type;
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
    VariableInfo* var = symbol_table->Find(node->id);
    if(!var)
    {
        symbol_table->Insert(node->id, node->line_num, decl_type);
    }
}

// Modified RunProgram for DECL_NODE
if(node->node_kind==DECL_NODE)
{
    TypedValue v;
    VariableInfo* var = symbol_table->Find(node->id);
    if(node->child[0])
    {
        v = Evaluate(node->child[0], symbol_table, variables);
    }
    else
    {
        if(var)
        {
            if(var->var_type == REAL) v = TypedValue(0.0);
            else if(var->var_type == BOOLEAN) v = TypedValue(0, true);
            else v = TypedValue(0);
        }
        else
        {
            v = TypedValue(0);
        }
    }
    if(var)
    {
        variables[var->memloc] = v;
        variables[var->memloc].type = var->var_type;
    }
}

/*
30 Test Cases for input.txt:

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
11. x := 2 & 3;  // 4 - 9 = -5
12. y := 4.0 & 2.0;  // 16 - 4 = 12
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
26. flag := not flag;  // assuming not is added, but since not required, skip
27. x := 10 - x;
28. y := y + 1.5;
29. flag := x >= 5;
30. write x + y;  // mixed type
*/

int main() {
    // Minimal main to test
    CompilerInfo compiler_info("input.txt", "output.txt", "debug.txt");
    StartCompiler(&compiler_info);
    return 0;
}