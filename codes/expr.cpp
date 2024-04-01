#include <iostream>
#include <vector>
#include <unordered_map>
#include <cassert>

using namespace std;

int CalcExpr(const char *expr);
using Operator = char;
unordered_map<Operator, int> priority{
    {')', 0},
    {'+', 1},
    {'-', 1},
    {'*', 2},
    {'/', 2},
    {'(', 3}};
int OpCmp(const Operator a, const Operator b)
{
    if (priority[a] == priority[b])
    {
        return 1;
    }
    return priority[a] - priority[b];
}
vector<int> numStack;
vector<Operator> opStack;
bool ShouldDelay()
{
    if (opStack.size() < 2)
    {
        return true;
    }
    int size = opStack.size();
    Operator preOp = opStack[size - 2];
    if (preOp == '(')
    {
        return true;
    }
    Operator topOp = opStack[size - 1];
    return OpCmp(preOp, topOp) < 0;
}

void ComputeOp(const Operator op)
{
    int topNum = numStack.back();
    numStack.pop_back();
    int preNum = numStack.back();
    numStack.pop_back();
    if (op == '*')
    {
        numStack.push_back(topNum * preNum);
    }
    else if (op == '/')
    {
        numStack.push_back(preNum / topNum);
    }
    else if (op == '+')
    {
        numStack.push_back(preNum + topNum);
    }
    else if (op == '-')
    {
        numStack.push_back(preNum - topNum);
    }
    else
    {
        cout << "invalid op1: " << op << endl;
    };
}

void ComputeOne()
{
    Operator op = opStack.back();
    if (op == '+' || op == '-' || op == '*' || op == '/')
    {
        ComputeOp(op);
        opStack.pop_back();
    }
    else if (op == ')')
    {
        opStack.pop_back();
        op = opStack.back();
        while (op != '(')
        {
            ComputeOp(op);
            opStack.pop_back();
            op = opStack.back();
        }
        if (op == '(')
        {
            opStack.pop_back();
        }
    }
    else
    {
        cout << "invalid op2: " << op << endl;
        exit(1);
    }
}

int CalcExpr(const char *expr)
{
    int ret = 0;
    const char *oriExpr = expr;
    while (*expr != '\0')
    {
        if (*expr == '(')
        {
            opStack.push_back('(');
            ++expr;
        }
        else if (*expr <= '9' && *expr >= '0')
        {
            int num = atoi(expr);
            while (*expr <= '9' && *expr >= '0')
            {
                ++expr;
            }
            numStack.push_back(num);
            // handle num
            if (ShouldDelay())
            {
                continue;
            }
            while (!ShouldDelay())
            {
                ComputeOne();
            }
        }
        else if (*expr == '*' || *expr == '/' || *expr == '+' || *expr == '-')
        {
            Operator op = *expr;
            ++expr;
            opStack.push_back(op);
            while (!ShouldDelay())
            {
                opStack.pop_back();
                ComputeOne();
                opStack.push_back(op);
            }
        }
        else if (*expr == ')')
        {
            opStack.push_back(*expr);
            while (!ShouldDelay())
            {
                ComputeOne();
            }
            ++expr;
        }
        else
        {
            cout << "unexpected [" << *expr << "] at " << (expr - oriExpr) << endl;
            break;
        }
    }
    while (!opStack.empty())
    {
        ComputeOne();
    }
    assert(opStack.empty());
    return numStack.back();
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "error" << endl;
        return 1;
    }
    cout << argv[1] << " = " << CalcExpr(argv[1]) << endl;
}
