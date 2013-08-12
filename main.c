#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "symbol.c"

token lookahead;
int token_value;
char token_value_str[20] = "";

int label_count = 0;

void new_label(char *c) {
    sprintf(c, "label%d", label_count);
    label_count++;
}

token lex() {
    int num = 0;
    int num_i = 0;
    char str[20] = "";
    int char_i = 0;

    while (1) {
        char c = getchar();

        if (c == ';') {
            return SEMICOLON;
        } else if (c == '(') {
            return OPEN_BRACKET;
        } else if (c == ')') {
            return CLOSE_BRACKET;
        } else if (c == '=') {
            return EQUALS;
        } else if (isalpha(c)) {
            str[char_i] = c;
            char_i++;
        } else if (isdigit(c)) {
            num = (num_i * 10) + atoi(&c);
            num_i++;
        } else if (c == ' ' || c == '\t' || c == '\n') {
            if (char_i > 0) {
                // string "collection" is complete.
                str[char_i] = '\0';

                symtable_record_ptr rec = symtable_get(str);
                if (rec) {
                    strcpy(token_value_str, rec->lexeme);
                    return rec->token;
                } else {
                    strcpy(token_value_str, str);
                    return ID;
                }
            } else if (num_i > 0) {
                token_value = num;
                return NUM;
            }
        } else if (feof(stdin)) {
            return END;
        } else {
            error();
        }
    }
}

void emit(char *s) {
    printf("%s", s);
}

void match(token expected) {

    if (lookahead != expected) {
        error();
    } else {
        // fetch next token
        lookahead = lex();
    }
}





void literal() {
    if (lookahead == TRUE_T) {
        match(TRUE_T);
        emit("1");
    } else if (lookahead == FALSE_T) {
        match(FALSE_T);
        emit("0");
    } else if (lookahead == ID) {
        symtable_record_ptr rec = symtable_get(token_value_str);
        match(ID);
        char s[33];
        sprintf(s, "%d", rec->value);
        emit(s);
    } else {
        char s[33];
        sprintf(s, "%d", token_value);
        match(NUM);
        emit(s);
    }
}

void stmt_list();

void stmt(int depth) {
    if (lookahead == PRINT) {
        match(PRINT);         
        emit("li $v0,1 \n");
        emit("li $t0,");
        literal();
        emit("\n");
        emit("move $a0,$t0 \n");
        emit("syscall \n");

        // print newline
        emit("li $v0,4 \n");
        emit("la $a0,nl \n");
        emit("syscall \n");

    } else if (lookahead == IF) {
        char after_label[10];
        new_label(after_label);
        char else_label[10]; 
        new_label(else_label);

        match(IF); emit("li $t0,");
        literal(); emit("\n beq $t0,0,"); emit(else_label); emit("\n");
        match(OPEN_BRACKET);
        stmt_list(depth+1);
        match(CLOSE_BRACKET); emit("b "); emit(after_label); emit("\n");
        match(ELSE); emit(else_label); emit(":");
        match(OPEN_BRACKET);
        stmt_list(depth+1);
        match(CLOSE_BRACKET); emit(after_label); emit(":");
    } else if (lookahead == DO_TIMES) {
        // the register used is based on the depth,
        // so that structures can be nested without conflict.
        char reg_id[5];
        sprintf(reg_id, "$t%d", depth+1);


        char start_label[10];
        new_label(start_label);
        char end_label[10]; 
        new_label(end_label);

        match(DO_TIMES);
        emit("li "); emit(reg_id); emit(",0 \n");
        emit(start_label); emit(":");
        emit("bge "); emit(reg_id); emit(",");
        literal();
        emit(","); emit(end_label); emit("\n");
        match(OPEN_BRACKET);
        stmt_list(depth+1);
        match(CLOSE_BRACKET);
        emit("addi "); emit(reg_id); emit(","); emit(reg_id); emit(",1 \n");
        emit("b "); emit(start_label); emit("\n");
        emit(end_label); emit(":");
    } else if (lookahead == ID) {
        char lexeme[22] = ""; 
        strcpy(lexeme, token_value_str);

        match(ID);
        match(EQUALS);

        int value = token_value;
        match(NUM);

        symtable_insert(lexeme, ID, value);
    }
}

void stmt_list(int depth) {
    if (lookahead == PRINT || lookahead == IF || 
        lookahead == DO_TIMES || lookahead == ID) {
        stmt(depth); match(SEMICOLON); stmt_list(depth);
    }
}


void parse() {
    emit(".data \n nl: .asciiz \"\\n\" \n");
    emit(".text \n main:");

    stmt_list(0);

    emit("li $v0, 10 \n");
    emit("syscall");
}



error() {
    printf("Syntax error\n");
    exit(1);
}

int main() {
    // put keywords into symbol table.
    symtable_insert("print", PRINT, 0);
    symtable_insert("if", IF, 0);
    symtable_insert("else", ELSE, 0);
    symtable_insert("dotimes", DO_TIMES, 0);
    symtable_insert("true", TRUE_T, 0);
    symtable_insert("false", FALSE_T, 0);

    lookahead = lex();

    parse();

    return 0;
}
