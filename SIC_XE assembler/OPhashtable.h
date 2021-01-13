#define _CRT_SECURE_NO_WARNINGS
#define MAX_HASH 101
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int hash = 0;
struct Node
{
    char name[8];
    int code;
    struct Node *hashNext;
};

struct OPTAB
{
    char name[8];
    int code;
};

struct OPTAB optab[] = {
    {"ADD", 0x18}, {"ADDF", 0x58}, {"ADDR", 0x90}, {"AND", 0x40}, {"CLEAR", 0xB4}, {"COMP", 0x28}, {"COMPF", 0x88}, {"COMPR", 0xA0}, {"DIV", 0x24}, {"DIVF", 0x64}, {"DIVR", 0x9C}, {"FIX", 0xC4}, {"FLOAT", 0xC0}, {"HIO", 0xF4}, {"J", 0x3C}, {"JEQ", 0x30}, {"JGT", 0x34}, {"JLT", 0x38}, {"JSUB", 0x48}, {"LDA", 0x00}, {"LDB", 0x68}, {"LDCH", 0x50}, {"LDF", 0x70}, {"LDL", 0x08}, {"LDS", 0x6C}, {"LDT", 0x74}, {"LDX", 0x04}, {"LPS", 0xD0}, {"MUL", 0x20}, {"MULF", 0x60}, {"MULR", 0x98}, {"NORM", 0xC8}, {"OR", 0x44}, {"RD", 0xD8}, {"RMO", 0xAC}, {"RSUB", 0x4C}, {"SHIFTL", 0xA4}, {"SHIFTR", 0xA8}, {"SIO", 0xF0}, {"SSK", 0xEC}, {"STA", 0x0C}, {"STB", 0x78}, {"STCH", 0x54}, {"STF", 0x80}, {"STI", 0xD4}, {"STL", 0x14}, {"STS", 0x7C}, {"STSW", 0xE8}, {"STT", 0x84}, {"STX", 0x10}, {"SUB", 0x1C}, {"SUBF", 0x5C}, {"SUBR", 0x94}, {"SVC", 0xB0}, {"TD", 0xE0}, {"TIO", 0xF8}, {"TIX", 0x2C}, {"TIXR", 0xB8}, {"WD", 0xDC}, {"START", 5}, {"END", 2}, {"BYTE", 1}, {"RESW", 4}, {"RESB", 3}, {"WORD", 6}};

struct Node *hashTable[MAX_HASH];

int hashCode(char *value, int hash)
{
    int h = hash;
    if (h == 0 && strlen(value) > 0)
    {
        char *val = value;
        for (int i = 0; i < strlen(value); i++)
        {
            h = 31 * h + val[i];
        }
        hash = h;
    }
    return (abs)(h % 66);
}

void AddOPHashData(char *name, struct Node *node)
{
    int hash_key = hashCode(name, hash);
    if (hashTable[hash_key] == NULL)
    {
        hashTable[hash_key] = node;
    }
    else
    {
        node->hashNext = hashTable[hash_key];
        hashTable[hash_key] = node;
    }
}
// int FindOPTAB(char *name)
// {
//     for (int i = 0; i < 66; i++)
//     {
//         if (strcmp(optab[i].name, name) == 0)
//         {
//             return optab[i].code;
//         }
//     }
//     return 0;
// }
int FindOPHashData(char *name, char use_hash)
{
    if (use_hash == 'y')
    {
        int hash_key = hashCode(name, hash);
        if (hashTable[hash_key] == NULL)
        {
            return 0;
        }
        if (strcmp(hashTable[hash_key]->name, name) == 0)
        {
            return hashTable[hash_key]->code;
        }
        else
        {
            struct Node *node = hashTable[hash_key];
            while (node->hashNext)
            {

                if (strcmp(node->hashNext->name, name) == 0)
                {
                    return node->hashNext->code;
                }
                node = node->hashNext;
            }
        }
    }
    else
    {
        for (int i = 0; i < 66; i++)
        {
            if (strcmp(optab[i].name, name) == 0)
            {
                return optab[i].code;
            }
        }
    }
    return 0;
}
/*void PrintOPHashData()
{
    printf("###Print All Hash Data###");
    for (int i = 0; i < 66; i++)
    {
        printf("idx: %d ", i);
        if (hashTable[i] != NULL)
        {
            struct Node *node = hashTable[i];
            while (node->hashNext)
            {
                printf("%s %d ", node->name, node->code);
                node = node->hashNext;
            }
            printf("%s %d \n", node->name, node->code);
        }
    }
    printf("\n");
} */
