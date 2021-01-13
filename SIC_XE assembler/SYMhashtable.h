#define _CRT_SECURE_NO_WARNINGS
#define MAX_HASH 101
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int shash = 0;
int symtab_index = 0;
struct SYMNode
{
    char label[10];
    int loc;
    struct SYMNode *hashNext;
};
struct SYMNode *shashTable[MAX_HASH];
struct SYMNode *symtab[101];
int shashCode(char *value, int shash)
{
    int h = shash;
    if (h == 0 && strlen(value) > 0)
    {
        char *val = value;
        for (int i = 0; i < strlen(value); i++)
        {
            h = 31 * h + val[i];
        }
        shash = h;
    }
    return (abs)(h % 66);
}

void AddSYMHashData(char *label, int loc, char use_hash)
{
    struct SYMNode *node = (struct SYMNode *)malloc(sizeof(struct SYMNode));
    strcpy(node->label, label);
    node->loc = loc;
    node->hashNext = NULL;
    if (use_hash == 'y')
    {
        int hash_key = shashCode(label, shash);
        if (shashTable[hash_key] == NULL)
        {
            shashTable[hash_key] = node;
        }
        else
        {
            node->hashNext = shashTable[hash_key];
            shashTable[hash_key] = node;
        }
    }
    else
    {
        symtab[symtab_index] = node;
        symtab_index++;
    }
}
struct SYMNode *FindSYMTAB(struct SYMNode **symtab, char *label, char use_hash)
{
    if (use_hash == 'y')
    {
        int hash_key = shashCode(label, shash);
        if (shashTable[hash_key] == NULL)
        {
            return NULL;
        }

        if (strcmp(shashTable[hash_key]->label, label) == 0)
        {

            return shashTable[hash_key];
        }
        else
        {
            struct SYMNode *node = shashTable[hash_key];
            while (node->hashNext)
            {
                if (strcmp(node->hashNext->label, label) == 0)
                {
                    return node->hashNext;
                }
                node = node->hashNext;
            }
        }
    }
    else
    {
        for (int i = 0; i < symtab_index; i++)
        {
            if (strcmp(symtab[i]->label, label) == 0)
            {
                return symtab[i];
            }
        }
    }
    return NULL;
}
// struct SYMNode *FindSYMHashData(struct SYMNode **shashTable, char *label)
// {

//     int hash_key = shashCode(label, shash);
//     if (shashTable[hash_key] == NULL)
//     {
//         return NULL;
//     }

//     if (strcmp(shashTable[hash_key]->label, label) == 0)
//     {

//         return shashTable[hash_key];
//     }
//     else
//     {
//         struct SYMNode *node = shashTable[hash_key];
//         while (node->hashNext)
//         {
//             if (strcmp(node->hashNext->label, label) == 0)
//             {
//                 return node->hashNext;
//             }
//             node = node->hashNext;
//         }
//     }
//     return NULL;
// }
void PrintAllHashData(FILE *ffp, char use_hash)
{
    if (use_hash == 'y')
    {
        for (int i = 0; i < 66; i++)
        {
            fprintf(ffp, "%d ", i);
            if (shashTable[i] != NULL)
            {
                struct SYMNode *node = shashTable[i];
                while (node->hashNext)
                {
                    fprintf(ffp, "%s %x ", node->label, node->loc);
                    node = node->hashNext;
                }
                fprintf(ffp, "%s %x\n", node->label, node->loc);
            }
        }
    }
    else
    {
        for (int i = 0; i < symtab_index; i++)
        {
            fprintf(ffp, "%d ", i);
            fprintf(ffp, "%s %x \n", symtab[i]->label, symtab[i]->loc);
        }
    }
}
