#include "9cc.h"

//
// global variables
//

Token *token;     // current token
char *user_input; // input program

// entrypoint
Node *parse(char *program)
{
    user_input = program;
    token = tokenize();
    return expr();
}

void error_at(char *loc, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//
// Tokenizer
//

bool consume(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
    {
        return false;
    }
    else
    {
        token = token->next;
        return true;
    }
}

void expect(char *op)
{
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
    {
        error_at(token->str, "\"%s\"ではありません", op);
    }
    else
    {
        token = token->next;
    }
}

int expect_number()
{
    if (token->kind != TK_NUM)
    {
        error_at(token->str, "数ではありません");
    }
    else
    {
        int val = token->val;
        token = token->next;
        return val;
    }
}

// pがqで始まっているかどうか
bool startswith(char *p, char *q)
{
    return memcmp(p, q, strlen(q)) == 0;
}

bool at_eof()
{
    return token->kind == TK_EOF;
}

// 新しいtokenを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize()
{
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p)
    {
        if (isspace(*p))
        {
            p++;
        }
        // multi-letter 記号
        else if (startswith(p, "==") || startswith(p, "!=") ||
                 startswith(p, "<=") || startswith(p, ">="))
        {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
        }
        // single-letter 記号
        else if (strchr("+-*/()<>", *p))
        {
            cur = new_token(TK_RESERVED, cur, p++, 1);
        }
        else if (isdigit(*p))
        {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
        }
        else
        {
            error_at(token->str, "tokenizeできません");
        }
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

//
// ast
//

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

// expr = equality
Node *expr()
{
    return equality();
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality()
{
    Node *node = relational();

    for (;;)
    {
        if (consume("=="))
        {
            node = new_node(ND_EQ, node, relational());
        }
        else if (consume("!="))
        {
            node = new_node(ND_NE, node, relational());
        }
        else
        {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational()
{
    Node *node = add();

    for (;;)
    {
        if (consume("<"))
        {
            node = new_node(ND_LT, node, add());
        }
        else if (consume("<="))
        {
            node = new_node(ND_LE, node, add());
        }
        else if (consume(">"))
        {
            node = new_node(ND_LT, add(), node);
        }
        else if (consume(">="))
        {
            node = new_node(ND_LE, add(), node);
        }
        else
        {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add()
{
    Node *node = mul();

    for (;;)
    {
        if (consume("+"))
        {
            node = new_node(ND_ADD, node, mul());
        }
        else if (consume("-"))
        {
            node = new_node(ND_SUB, node, mul());
        }
        else
        {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul()
{
    Node *node = unary();

    for (;;)
    {
        if (consume("*"))
        {
            node = new_node(ND_MUL, node, unary());
        }
        else if (consume("/"))
        {
            node = new_node(ND_DIV, node, unary());
        }
        else
        {
            return node;
        }
    }
}

// unary   = ("+" | "-")? primary
Node *unary()
{
    if (consume("+"))
    {
        return unary();
    }
    else if (consume("-"))
    {
        return new_node(ND_SUB, new_node_num(0), unary());
    }
    else
    {
        return primary();
    }
}

// primary = "(" expr ")" | num
Node *primary()
{
    // 次のtokenが "(" なら、"(" expr ")" のはず
    if (consume("("))
    {
        Node *node = expr();
        expect(")");
        return node;
    }
    else
    {
        // そうでなければ数値のはず
        return new_node_num(expect_number());
    }
}