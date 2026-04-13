#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include "instruction.h"

void usage() {
    fprintf(stderr, "Usage: ./xas file");
    exit(1);
}

typedef struct instructions {
    struct instructions_t *next;
    char inst[10];
    char op1[10];
    char op2[10];
    char op3[10];
} instructions_t;

typedef struct labels {
    instructions_t *first;
    instructions_t *last;
    int size;
    struct label_t* next;
    char name[25];
    int value;
} label_t;

int getNextArg(char* line, char* input, int i){
    for (; i < strlen(line) &&
        (line[i] == ' ' || line[i] == '\t'); i++){}


    int j = i;
    for (; i < strlen(line) &&
        (line[i] != ' ' && line[i] != '\t'); i++){
        if (line[i] == '#') break;
        input[i - j] = line[i];
    }
    return i;
}

uint16_t findLabel(label_t* cur, char* label){
    // printf("searching for label: %s\n", label);
    uint16_t off = 0;
    label_t* start = cur;
    // printf("Current label: %s\n", start->name);
    while (start != NULL){
        if (strcmp(start->name, label) == 0){
            return off;
        }
        off+=start->size;
        start = (label_t*)start->next;
        // printf("Current label: %s\n", start->name);
    }
    exit(2);
    return -1;
}
bool isRegister(char* input){
    if (strncmp(input, "%r", 2) == 0){
        return true;
    }
    return false;
}
reg_t getRegister(char* input){
    bool isRegister = false;
    int output = -1;
    for (int i = 0; i < strlen(input); i++){
        if (input[i] == '%') isRegister = true;
        if (isdigit(input[i])){
            output =  input[i] - '0';
        }
    }
    if (isRegister) return output;
    exit(2);
}
uint16_t getVal(int pc, label_t* start, char* input){
    // printf("input: %s\n",input);
    if (strlen(input) < 2) return -1;
    if (input[0] == '$') {
        return atoi(&input[1]);
    } else {
        return findLabel(start, input) - (pc + 1);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        usage();
    }

    FILE *fptr;
    int size = 0;
    char line[100];
    label_t* label = (label_t*)calloc(1, sizeof(label_t));
    label->first = NULL;
    label_t* current = label;
    fptr = fopen(argv[1], "r");
    while (fgets(line, 100, fptr))
    {
        line[strlen(line) - 1] = '\0';
        if (strlen(line) == 0) continue;
        if (line[strlen(line) - 1] == ':'){
            int i;
            for (i = 0; line[i] == ' '; i++){}
            int j = i;
            current->next = (struct label_t*)calloc(1, sizeof(label_t));
            current = (label_t*)current->next;
            for (; i < strlen(line); i++){
                if (line[i] == ':') break;
                if (line[i] == '#') break;
                if (line[i] == ' ') return -1;
                current->name[i - j] = line[i];
            }
            current->name[i - j + 1]= '\0';

            continue;
        }
        int i;
        for (i = 0; i < strlen(line) &&
            (line[i] == ' ' || line[i] == '\t'); i++){}

        if ((i >= (strlen(line) - 1)) || (line[i] == '#')) continue;

        if (current == NULL || current->first == NULL){
            current->first = calloc(1, sizeof(instructions_t));
            current->last = current->first;
            current->size++;
            size++;
        } else {
            current->last->next = calloc(1, sizeof(instructions_t));
            current->last = (instructions_t*)current->last->next;
            current->size++;
            size++;
        }

        i = getNextArg(line, current->last->inst, i);
        i = getNextArg(line, current->last->op1, i);
        i = getNextArg(line, current->last->op2, i);
        i = getNextArg(line, current->last->op3, i);
    }
    current = label;
    uint16_t* instructionList = calloc(size, sizeof(uint16_t));
    int i = 0;
    while (current != NULL){
        instructions_t *cur = current->first;
        // printf("cur: %s\n", current->name);
        for (; cur != NULL; i++) {
            char* inst = cur->inst;
            if (0 == strcmp(inst, "add")){
                // printf("Adding\n");
                if (isRegister(cur->op3)){
                    // printf("Add reg\n");
                    instructionList[i] = emit_add_reg(getRegister(cur->op1),
                                                      getRegister(cur->op2),
                                                      getRegister(cur->op3));
                } else {
                    instructionList[i] = emit_add_imm(getRegister(cur->op1),
                                                getRegister(cur->op2),
                                                getVal(i, label, cur->op3));
                }
            }
            if (0 == strcmp(inst, "and")){
                if (isRegister(cur->op3)){
                    instructionList[i] = emit_and_reg(getRegister(cur->op1),
                                                      getRegister(cur->op2),
                                                      getRegister(cur->op3));
                } else {
                    instructionList[i] = emit_and_imm(getRegister(cur->op1),
                                                    getRegister(cur->op2),
                                                    getVal(i, label, cur->op3));
                }
            }
            if (0 == strncmp(inst, "br", 2)){
                bool n = false;
                bool z = false;
                bool p = false;
                for (int i = 2; i < strlen(inst); i++){
                    if (inst[i] == 'n') n = true;
                    if (inst[i] == 'p') p = true;
                    if (inst[i] == 'z') z = true;
                }

                instructionList[i] = emit_br(n, z, p,
                    getVal(i, label, cur->op1));
            }
            if (0 == strcmp(inst, "ld")){
                instructionList[i] = emit_ld(getRegister(cur->op1),
                    getVal(i, label, cur->op2));
            }
            if (0 == strcmp(inst, "ldi")){
                instructionList[i] = emit_ldi(getRegister(cur->op1),
                    getVal(i, label, cur->op2));
            }
            if (0 == strcmp(inst, "ldr")){
                instructionList[i] = emit_ldr(getRegister(cur->op1),
                                             getRegister(cur->op2),
                                             getVal(i, label, cur->op3));
            }
            if (0 == strcmp(inst, "lea")){
                instructionList[i] = emit_lea(getRegister(cur->op1),
                    getVal(i, label, cur->op2));
            }
            if (0 == strcmp(inst, "not")){
                instructionList[i] = emit_not(getRegister(cur->op1),
                                                  getRegister(cur->op2));
            }
            if (0 == strcmp(inst, "jmp")){
                instructionList[i] = emit_jmp(getRegister(cur->op1));
            }
            if (0 == strcmp(inst, "jsr")){
                instructionList[i] = emit_jsr(getVal(i, label, cur->op1));
            }
            if (0 == strcmp(inst, "jsrr")){
                instructionList[i] = emit_jsrr(getRegister(cur->op1));
            }
            if (0 == strcmp(inst, "st")){
                instructionList[i] = emit_st(getRegister(cur->op1),
                    getVal(i, label, cur->op2));
            }
            if (0 == strcmp(inst, "sti")){
                instructionList[i] = emit_sti(getRegister(cur->op1),
                    getVal(i, label, cur->op2));
            }
            if (0 == strcmp(inst, "str")){
                instructionList[i] = emit_str(getRegister(cur->op1),
                    getRegister(cur->op2), getVal(i, label, cur->op3));
            }
            if (0 == strcmp(inst, "getc")){
                instructionList[i] = 0xf020;
            }
            if (0 == strcmp(inst, "putc")){
                instructionList[i] = 0xf021;
            }
            if (0 == strcmp(inst, "puts")){
                instructionList[i] = 0xf022;
            }
            if (0 == strcmp(inst, "putsp")){
                instructionList[i] = 0xf024;
            }
            if (0 == strcmp(inst, "enter")){
                instructionList[i] = 0xf023;
            }
            if (0 == strcmp(inst, "halt")){
                instructionList[i] = 0xf025;
            }
            if (0 == strcmp(inst, "val")){
                instructionList[i] = getVal(i, label, cur->op1);
            }

            cur = (instructions_t*)cur->next;
        }
        current = (label_t*)current->next;
    }
    // printf("Succcessfully made array\n");
    FILE* outptr = fopen("a.obj", "w");
    uint16_t origin[1];
    origin[0] = 0x0030;
    fseek(outptr, 0, SEEK_SET);
    fwrite(origin, sizeof(origin), 1, outptr);
    // fseek(outptr, 0x3000, SEEK_SET);
    if (outptr == NULL) {
        printf("Failed to make file\n");
        return -1;
    }
    for (int i = 0; i < size; i++){
        instructionList[i] = ((instructionList[i] & 0xff00) >> 8)
            + (((instructionList[i] & 0xff) << 8));
    }
    int flag = fwrite(instructionList, sizeof(*instructionList), size, outptr);
    free(instructionList);
    current = label;
    while (current != NULL){
        instructions_t *cur = current->first;
        // printf("cur: %s\n", current->name);
        for (; cur != NULL; i++) {
            instructions_t* temp = cur;
            cur = (instructions_t*)cur->next;
            free(temp);
        }
        label_t* temp = current;
        current = (label_t*)current->next;
        free(temp);
    }
    if (!flag) {
        printf("Write failed");
    }
    return 0;
}
