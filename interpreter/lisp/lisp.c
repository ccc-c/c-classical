#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 1. 定義型別：加入 TYPE_STR
typedef enum { TYPE_INT, TYPE_STR, TYPE_SYM, TYPE_LIST, TYPE_PROC } ObjType;

struct Object;
struct Env;

typedef struct Object {
    ObjType type;
    union {
        long i;
        char* s; // 符號與字串共用此欄位
        struct { struct Object *car, *cdr; } l;
        struct { struct Object *params, *body; struct Env *env; } p;
    };
} Object;

typedef struct Env {
    Object *sym, *val;
    struct Env *outer;
} Env;

// --- 構造函數 ---
Object* make_int(long i) { Object* o = malloc(sizeof(Object)); o->type = TYPE_INT; o->i = i; return o; }
Object* make_sym(char* s) { Object* o = malloc(sizeof(Object)); o->type = TYPE_SYM; o->s = strdup(s); return o; }
// 新增：字串構造函數
Object* make_str(char* s) { Object* o = malloc(sizeof(Object)); o->type = TYPE_STR; o->s = strdup(s); return o; }
Object* cons(Object* car, Object* cdr) { Object* o = malloc(sizeof(Object)); o->type = TYPE_LIST; o->l.car = car; o->l.cdr = cdr; return o; }

// --- 環境操作 ---
Object* env_get(Env* e, char* sym) {
    for (; e; e = e->outer) if (strcmp(e->sym->s, sym) == 0) return e->val;
    return NULL;
}

void env_set(Env* e, Object* sym, Object* val) {
    Env* new_node = malloc(sizeof(Env));
    *new_node = *e;
    e->sym = sym; e->val = val; e->outer = new_node;
}

// --- 解析器 ---
char* next_token(char** input) {
    while (isspace(**input)) (*input)++;
    if (**input == '\0') return NULL;
    
    // 處理括號
    if (**input == '(' || **input == ')') { 
        char* t = malloc(2); t[0] = *(*input)++; t[1] = '\0'; return t; 
    }
    
    // 2. 處理字串 (遇到 " 讀取到下一個 ")
    if (**input == '"') {
        char* start = *input;
        (*input)++; // 跳過開頭的引號
        while (**input && **input != '"') (*input)++;
        if (**input == '"') (*input)++; // 跳過結尾的引號
        int len = *input - start;
        char* t = malloc(len + 1);
        strncpy(t, start, len);
        t[len] = '\0';
        return t;
    }

    char* start = *input;
    while (**input && !isspace(**input) && **input != '(' && **input != ')' && **input != '"') (*input)++;
    return strndup(start, *input - start);
}

Object* read_expr(char** input);
Object* read_list(char** input) {
    char* t = next_token(input);
    if (!t || strcmp(t, ")") == 0) return NULL;
    char* saved_p = *input - strlen(t);
    Object* car = read_expr(&saved_p);
    *input = saved_p;
    return cons(car, read_list(input));
}

Object* read_expr(char** input) {
    char* t = next_token(input);
    if (!t) return NULL;
    if (strcmp(t, "(") == 0) return read_list(input);
    
    // 將字串 Token 轉換為 TYPE_STR
    if (t[0] == '"') {
        t[strlen(t) - 1] = '\0'; // 移除結尾引號
        return make_str(t + 1);  // 移除開頭引號並構造
    }
    
    if (isdigit(t[0]) || (t[0] == '-' && isdigit(t[1]))) return make_int(atol(t));
    return make_sym(t);
}

// --- 執行引擎 ---
Object* eval(Object* obj, Env* env);

Object* apply(Object* proc, Object* args) {
    Env* new_env = malloc(sizeof(Env)); new_env->outer = proc->p.env;
    Object *p = proc->p.params, *a = args;
    while (p && a) {
        env_set(new_env, p->l.car, a->l.car);
        p = p->l.cdr; a = a->l.cdr;
    }
    return eval(proc->p.body, new_env);
}

Object* eval(Object* obj, Env* env) {
    if (!obj) return NULL;
    // 3. 字串與整數一樣直接回傳自身
    if (obj->type == TYPE_INT || obj->type == TYPE_STR) return obj;
    if (obj->type == TYPE_SYM) return env_get(env, obj->s);
    
    if (obj->type == TYPE_LIST) {
        Object* head = obj->l.car;
        
        if (head->type == TYPE_SYM) {
            if (strcmp(head->s, "define") == 0) {
                Object* val = eval(obj->l.cdr->l.cdr->l.car, env);
                env_set(env, obj->l.cdr->l.car, val);
                return val;
            }
            if (strcmp(head->s, "if") == 0) {
                Object* cond = eval(obj->l.cdr->l.car, env);
                return eval(cond && cond->i != 0 ? obj->l.cdr->l.cdr->l.car : obj->l.cdr->l.cdr->l.cdr->l.car, env);
            }
            if (strcmp(head->s, "lambda") == 0) {
                Object* o = malloc(sizeof(Object)); o->type = TYPE_PROC;
                o->p.params = obj->l.cdr->l.car; o->p.body = obj->l.cdr->l.cdr->l.car; o->p.env = env;
                return o;
            }
            if (strcmp(head->s, "quote") == 0) return obj->l.cdr->l.car;
            if (strcmp(head->s, "cons") == 0) return cons(eval(obj->l.cdr->l.car, env), eval(obj->l.cdr->l.cdr->l.car, env));
            if (strcmp(head->s, "car") == 0) { Object* lst = eval(obj->l.cdr->l.car, env); return lst ? lst->l.car : NULL; }
            if (strcmp(head->s, "cdr") == 0) { Object* lst = eval(obj->l.cdr->l.car, env); return lst ? lst->l.cdr : NULL; }
            
            if (strpbrk(head->s, "+-*/=")) {
                Object *a = eval(obj->l.cdr->l.car, env), *b = eval(obj->l.cdr->l.cdr->l.car, env);
                if (head->s[0] == '+') return make_int(a->i + b->i);
                if (head->s[0] == '-') return make_int(a->i - b->i);
                if (head->s[0] == '*') return make_int(a->i * b->i);
                if (head->s[0] == '/') return make_int(b->i != 0 ? a->i / b->i : 0);
                if (head->s[0] == '=') return make_int(a->i == b->i);
            }
        }
        
        Object* proc = eval(head, env);
        Object* args = NULL, *tail = NULL;
        for (Object* c = obj->l.cdr; c; c = c->l.cdr) {
            Object* v = cons(eval(c->l.car, env), NULL);
            if (!args) args = tail = v; else { tail->l.cdr = v; tail = v; }
        }
        return apply(proc, args);
    }
    return obj;
}

void print_obj(Object* obj) {
    if (!obj) printf("nil");
    else if (obj->type == TYPE_INT) printf("%ld", obj->i);
    // 4. 印出時自動加上雙引號
    else if (obj->type == TYPE_STR) printf("\"%s\"", obj->s);
    else if (obj->type == TYPE_SYM) printf("%s", obj->s);
    else if (obj->type == TYPE_LIST) {
        printf("("); 
        for (; obj; obj = obj->l.cdr) { print_obj(obj->l.car); if (obj->l.cdr) printf(" "); }
        printf(")");
    } else printf("<proc>");
}

int main() {
    char line[512]; Env* global_env = calloc(1, sizeof(Env));
    printf("Mini-Lisp (String Ready)\n> ");
    while (fgets(line, sizeof(line), stdin)) {
        char* p = line;
        Object* expr = read_expr(&p);
        if (expr) { print_obj(eval(expr, global_env)); printf("\n> "); }
    }
    return 0;
}