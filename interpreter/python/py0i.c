#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --- 1. 定義 Token 類型 ---
typedef enum {
    T_DEF, T_IF, T_RETURN, T_PRINT, T_ID, T_NUM,
    T_ASSIGN, T_EQ, T_PLUS, T_MINUS, T_MUL,
    T_LPAREN, T_RPAREN, T_COLON, T_EOF
} TokenType;

typedef struct {
    TokenType type;
    int value;
    char name[32];
    int indent;   // 非常重要：紀錄該 Token 所在行的縮排空格數
} Token;

Token tokens[1000];
int token_count = 0;

// --- 2. 函式表 (儲存定義的函式) ---
typedef struct {
    char name[32];
    char param[32];   // 為了簡化，只支援單一參數
    int body_ip;      // 函式本體的第一個 Token 索引 (Instruction Pointer)
    int def_indent;   // def 關鍵字所在的縮排層級
} Function;

Function functions[10];
int func_count = 0;

// --- 3. 呼叫堆疊 (Call Stack) 支援遞迴的區域變數 ---
#define MAX_FRAMES 100
#define MAX_VARS 20

typedef struct {
    char name[32];
    int value;
} Variable;

typedef struct {
    Variable vars[MAX_VARS];
    int var_count;
} StackFrame;

StackFrame stack[MAX_FRAMES];
int sp = 0; // sp=0 代表全域，sp>0 代表函式內部

void set_var(const char *name, int value) {
    for (int i = 0; i < stack[sp].var_count; i++) {
        if (strcmp(stack[sp].vars[i].name, name) == 0) {
            stack[sp].vars[i].value = value;
            return;
        }
    }
    strcpy(stack[sp].vars[stack[sp].var_count].name, name);
    stack[sp].vars[stack[sp].var_count].value = value;
    stack[sp].var_count++;
}

int get_var(const char *name) {
    // 優先找區域變數
    for (int i = 0; i < stack[sp].var_count; i++) {
        if (strcmp(stack[sp].vars[i].name, name) == 0) 
            return stack[sp].vars[i].value;
    }
    // 再找全域變數
    for (int i = 0; i < stack[0].var_count; i++) {
        if (strcmp(stack[0].vars[i].name, name) == 0) 
            return stack[0].vars[i].value;
    }
    printf("NameError: '%s' not defined\n", name);
    exit(1);
}

// --- 4. 詞法分析器 (Lexer) 支援縮排 ---
void tokenize(const char *src) {
    int line_start = 1;
    int spaces = 0;
    
    while (*src) {
        if (*src == '\n' || *src == '\r') {
            line_start = 1; spaces = 0; src++; continue;
        }
        if (line_start && *src == ' ') {
            spaces++; src++; continue;
        }
        if (isspace(*src)) { src++; continue; }
        
        line_start = 0; // 遇到非空白字元，確定該行的縮排數
        Token *t = &tokens[token_count++];
        t->indent = spaces;
        
        if (isdigit(*src)) {
            t->type = T_NUM; t->value = 0;
            while (isdigit(*src)) t->value = t->value * 10 + (*src++ - '0');
            continue;
        }
        if (isalpha(*src)) {
            int i = 0;
            while (isalnum(*src) || *src == '_') t->name[i++] = *src++;
            t->name[i] = '\0';
            
            if (strcmp(t->name, "def") == 0) t->type = T_DEF;
            else if (strcmp(t->name, "if") == 0) t->type = T_IF;
            else if (strcmp(t->name, "return") == 0) t->type = T_RETURN;
            else if (strcmp(t->name, "print") == 0) t->type = T_PRINT;
            else t->type = T_ID;
            continue;
        }
        
        if (*src == '=' && *(src+1) == '=') { t->type = T_EQ; src += 2; continue; }
        
        switch (*src++) {
            case '=': t->type = T_ASSIGN; break;
            case '+': t->type = T_PLUS; break;
            case '-': t->type = T_MINUS; break;
            case '*': t->type = T_MUL; break;
            case '(': t->type = T_LPAREN; break;
            case ')': t->type = T_RPAREN; break;
            case ':': t->type = T_COLON; break;
        }
    }
    tokens[token_count].type = T_EOF;
}

// --- 5. 語法解析與執行 (Parser & Evaluator) ---
int eval_expr(int *ip);
int run_block(int ip, int base_indent);

// 執行函式呼叫
int call_function(const char *name, int arg_val) {
    for (int i = 0; i < func_count; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            sp++; // 進入新的 Stack Frame (遞迴核心)
            stack[sp].var_count = 0;
            set_var(functions[i].param, arg_val);
            
            // 執行函式區塊
            int ret_val = run_block(functions[i].body_ip, functions[i].def_indent);
            
            sp--; // 離開 Stack Frame
            return ret_val;
        }
    }
    printf("NameError: function '%s' not defined\n", name);
    exit(1);
}

// 解析數學與比較算式 (遞迴下降)
int eval_factor(int *ip) {
    int val = 0;
    if (tokens[*ip].type == T_NUM) {
        val = tokens[(*ip)++].value;
    } else if (tokens[*ip].type == T_ID) {
        char name[32]; strcpy(name, tokens[*ip].name);
        (*ip)++;
        if (tokens[*ip].type == T_LPAREN) { // 函式呼叫: func(x)
            (*ip)++;
            int arg = eval_expr(ip);
            (*ip)++; // skip ')'
            return call_function(name, arg);
        }
        val = get_var(name); // 一般變數
    } else if (tokens[*ip].type == T_LPAREN) {
        (*ip)++; val = eval_expr(ip); (*ip)++;
    }
    return val;
}

int eval_mul(int *ip) {
    int val = eval_factor(ip);
    while (tokens[*ip].type == T_MUL) {
        (*ip)++; val *= eval_factor(ip);
    }
    return val;
}

int eval_add_sub(int *ip) {
    int val = eval_mul(ip);
    while (tokens[*ip].type == T_PLUS || tokens[*ip].type == T_MINUS) {
        if (tokens[*ip].type == T_PLUS) { (*ip)++; val += eval_mul(ip); }
        else { (*ip)++; val -= eval_mul(ip); }
    }
    return val;
}

int eval_expr(int *ip) {
    int val = eval_add_sub(ip);
    if (tokens[*ip].type == T_EQ) {
        (*ip)++; val = (val == eval_add_sub(ip));
    }
    return val;
}

// 執行一個程式區塊
int run_block(int ip, int base_indent) {
    while (tokens[ip].type != T_EOF) {
        // 如果縮排退回原本的層級，代表離開目前的區塊 (例如函式結束)
        if (base_indent >= 0 && tokens[ip].indent <= base_indent) break;

        if (tokens[ip].type == T_DEF) {
            int def_indent = tokens[ip].indent; ip++;
            strcpy(functions[func_count].name, tokens[ip++].name);
            ip++; // '('
            strcpy(functions[func_count].param, tokens[ip++].name);
            ip++; // ')'
            ip++; // ':'
            functions[func_count].body_ip = ip;
            functions[func_count].def_indent = def_indent;
            func_count++;
            
            // 跳過函式本體，不立刻執行
            while (tokens[ip].type != T_EOF && tokens[ip].indent > def_indent) ip++;
            continue;
        }
        
        if (tokens[ip].type == T_IF) {
            int if_indent = tokens[ip].indent; ip++;
            int cond = eval_expr(&ip); ip++; // skip ':'
            
            if (!cond) {
                // 條件不成立，跳過整個 if 區塊 (尋找縮排 <= if縮排 的下一行)
                while (tokens[ip].type != T_EOF && tokens[ip].indent > if_indent) ip++;
            }
            continue;
        }

        if (tokens[ip].type == T_RETURN) {
            ip++; return eval_expr(&ip);
        }

        if (tokens[ip].type == T_PRINT) {
            ip++; ip++; // print(
            printf("%d\n", eval_expr(&ip));
            ip++; // )
            continue;
        }

        if (tokens[ip].type == T_ID && tokens[ip+1].type == T_ASSIGN) {
            char name[32]; strcpy(name, tokens[ip].name);
            ip += 2;
            set_var(name, eval_expr(&ip));
            continue;
        }
        
        ip++;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    // 檢查是否提供檔案路徑
    if (argc < 2) {
        printf("用法: %s <path_to_python_file>\n", argv[0]);
        return 1;
    }

    // 開啟檔案
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        printf("錯誤: 無法開啟檔案 '%s'\n", argv[1]);
        return 1;
    }

    // 移動到檔案尾端來取得檔案大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file); // 將檔案指標移回開頭

    // 配置記憶體來存放程式碼字串 (+1 是為了結尾的 '\0')
    char *source_code = (char *)malloc(file_size + 1);
    if (!source_code) {
        printf("錯誤: 記憶體配置失敗\n");
        fclose(file);
        return 1;
    }

    // 讀取檔案內容並加上字串結尾符號
    size_t read_size = fread(source_code, 1, file_size, file);
    source_code[read_size] = '\0';
    fclose(file);

    // 1. 詞法分析 (Lexer)
    tokenize(source_code);

    // 2. 啟動虛擬機執行區塊 (全域層級 base_indent = -1)
    run_block(0, -1);

    // 釋放記憶體
    free(source_code);

    return 0;
}