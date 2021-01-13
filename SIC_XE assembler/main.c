// 추가 보완할 것
// 1. 에러처리(assembly listing file)

#include "OPhashtable.h"
#include "SYMhashtable.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

clock_t start, end, res;
char use_hash;
char opcode[10], label[10], operand[10];
int LOC;
int startadd; //시작주소
int Total_Length;
int objcode;
int M_code;
char objcode_txt[100];
char txt_rec[100];
int txt_cnt = 0;
int operandAddr;
int M_check; // +opcode 만났을때 체크
int TS;      //레코드 시작주소
int find;
FILE *fp, *ffp, *obj_file;
struct SYMNode *SYMTAB[MAX_HASH];
struct SYMNode *SYMTAB_ns[101];
struct M_rec
{
    int M_code;
    int root_ck;
    struct M_rec *next;
};
struct M_rec *M_REC;

void getLine(char *buffer) //label, opcode, operand 구분
{

    int cnt = 0;
    // char *copy_buffer;
    // strcpy(copy_buffer, buffer);
    char *buf = strtok(buffer, " \t\n");

    strcpy(opcode, "");
    strcpy(label, "");
    strcpy(operand, "");
    while (buf != NULL)
    {
        if (buf[0] == '.')
            return;
        if (buf[0] == '+')
        {
            M_check = 1;
            char *temp = (char *)malloc(sizeof(char) * strlen(buf));
            for (int i = 1; i <= strlen(buf); i++)
            {
                temp[i - 1] = buf[i];
            }
            find = FindOPHashData(temp, use_hash);
        }
        else
            find = FindOPHashData(buf, use_hash);

        if (find != 0 || strcmp(buf, "LDA") == 0)
        {
            if (strcmp(opcode, "") == 0)
                strcpy(opcode, buf); //유형 1 OPCODE
            else
            {
                strcpy(operand, buf);
            }
        }
        else if (find == 0)
        {
            if (strcmp(label, "") != 0 && strcmp(opcode, "") == 0 && strcmp(buf, "BASE") != 0)
            {
                FILE *ffp = fopen("Intermediate file.asm", "w");
                fprintf(ffp, "Invalid Operation Code!\n");
                fclose(ffp);
                exit(1);
            }
            else
            {
                if (strcmp(buf, "BASE") == 0) //유형 3 LABEL OPCODE OPERAND
                {
                    strcpy(opcode, buf);
                    buf = strtok(NULL, " \n\t");
                    strcpy(operand, buf);
                }
                else if (strcmp(opcode, "") == 0)
                {
                    strcpy(label, buf);
                }
                else
                {
                    strcpy(operand, buf); //유형 2 OPCODE OPERAND
                }
            }
        }
        //fprintf(stderr, "l: %s\toc: %s\tor: %s\n", label, opcode, operand);
        buf = strtok(NULL, " \n\t");
        cnt++;
    }
}
void change_loc(char *opcode)
{
    if (!strcmp(opcode, "WORD"))
        LOC += 3;
    else if (!strcmp(opcode, "RESW"))
        LOC += 3 * atoi(operand);
    else if (!strcmp(opcode, "RESB"))
    {
        LOC += atoi(operand);
    }
    else if (!strcmp(opcode, "BYTE"))
    {
        if (operand[0] == 'C')
            LOC += strlen(operand) - 3;
        else if (operand[0] == 'X')
            LOC += (strlen(operand) - 3) / 2;
    }
    else if (!strcmp(opcode, "BASE"))
    {
        return;
    }
    else if (!strcmp(opcode, "CLEAR"))
    {
        LOC += 2;
    }
    else if (!strcmp(opcode, "TIXR"))
    {
        LOC += 2;
    }
    else if (!strcmp(opcode, "COMPR"))
    {
        LOC += 2;
    }
    else if (opcode[0] == '+')
    {
        LOC += 4;
    }
    else
        LOC += 3;
}
int Byte_case(char *a)
{
    int i, n, nsum, k, j;
    if (operand[0] == 'C')
    { // C의 경우
        nsum = 0;
        for (i = 2; i < strlen(a) - 2; ++i)
        {
            nsum += (int)a[i];
            nsum = nsum << 8;
        }
        nsum += a[strlen(a) - 2];
    }
    else if (operand[0] == 'X')
    { // X의 경우
        nsum = 0;
        for (i = 2; i < strlen(a) - 2; ++i)
        {
            if (a[i] > 65 && a[i] < 90) // 알파벳이라면
                n = (int)a[i] - 55;
            else // 숫자라면
                n = (int)a[i] - 48;
            nsum += n;
            nsum = nsum << 4;
        }
        if (a[strlen(a) - 2] > 65 && a[strlen(a) - 2] < 90)
            nsum += a[strlen(a) - 2] - 55;
        else
            nsum += a[strlen(a) - 2] - 48;
    }
    else
    {
        nsum = atoi(operand);
    }
    return nsum;
}
void print_T_rec()
{
    fprintf(obj_file, "T%.6X%.2X%s\n", TS, ((unsigned int)strlen(txt_rec) / 2), txt_rec);
    strcpy(txt_rec, "");
    txt_cnt = 0;
}
void print_M_rec(struct M_rec *M_REC)
{
    if (M_REC->next == NULL)
        return;
    M_REC = M_REC->next->next;
    while (M_REC->root_ck != 1)
    {
        fprintf(obj_file, "M%.6X\n", M_REC->M_code);
        M_REC = M_REC->next;
    }
}

void readSYMTAB(FILE *ffp)
{
    char line[65];
    int index;

    while (fgets(line, sizeof(line), ffp) != NULL)
    {
        char *buf = strtok(line, " \t\n");
        if (strcmp(line, "START_CODE") == 0)
        {
            break;
        }
        while (buf != NULL)
        {
            if (buf[0] >= 48 && buf[0] <= 57) //숫자이면
            {
                index = atoi(buf);
            }
            else //문자
            {
                struct SYMNode *TempNode = (struct SYMNode *)malloc(sizeof(struct SYMNode));
                strcpy(TempNode->label, buf);
                buf = strtok(NULL, " \t\n");
                TempNode->loc = strtoul(buf, NULL, 16);
                TempNode->hashNext = NULL;
                if (use_hash == 'y')
                {
                    if (SYMTAB[index] == NULL)
                    {
                        SYMTAB[index] = TempNode;
                    }
                    else
                    {
                        TempNode->hashNext = SYMTAB[index];
                        SYMTAB[index] = TempNode;
                    }
                }
                else
                {
                    SYMTAB_ns[index] = TempNode;
                }
            }
            buf = strtok(NULL, " \t\n");
        }
    }
}

void openp1file(char *argv)
{
    fp = fopen(argv, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "file not found\n");
        exit(1);
    }
}
void openp2file()
{
    ffp = fopen("Intermediate file.asm", "r");
    if (ffp == NULL)
    {
        fprintf(stderr, "file not found\n");
        exit(1);
    }
}
void Copy_sample_file(FILE *fp, FILE *ffp)
{
    char line[100];
    fprintf(ffp, "\n%s\n", "START_CODE");
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        fprintf(ffp, "%s", line);
    }
}
void makeIntermediateFILE(char *argv, int type)
{
    fp = fopen(argv, "r");
    FILE *ffp = fopen("Intermediate file.asm", "w");
    if (type == 1) //Duplicated Error
    {
        fprintf(ffp, "Duplicated Symbol Error!\n");
    }
    else if (type == 2)
    {
        fprintf(ffp, "Invalid Operation Code!\n");
    }
    else
    {
        PrintAllHashData(ffp, use_hash);
        //sample code 복사
        Copy_sample_file(fp, ffp);
    }
    fclose(ffp);
}
void pass1(FILE *fp)
{
    char buffer[100];
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        getLine(buffer);
        if (buffer[0] == '.')
            continue;
        if (!strcmp(opcode, "START"))
        {
            startadd = strtoul(operand, NULL, 16);
            LOC = startadd; //?
            continue;
        }
        if (!strcmp(opcode, "BASE"))
        {
            continue;
            //값을넣어줘야함
        }
        if (strcmp(opcode, "END"))
        {
            if (use_hash == 'y')
            {
                if (FindSYMTAB(shashTable, label, use_hash) != 0 && strcmp(label, "") != 0) //
                {
                    makeIntermediateFILE(NULL, 1); //Duplicated Error
                    exit(1);
                }
                else
                {
                    if (strcmp(label, ""))
                    {
                        AddSYMHashData(label, LOC, use_hash);
                    }
                }
                if (opcode[0] == '+')
                {
                    char *temp;
                    change_loc(opcode);
                }
                else if (FindOPHashData != 0)
                    change_loc(opcode);
            }
            else
            {
                //fprintf(stderr, "\n!%s!\n", label);
                if (FindSYMTAB(symtab, label, use_hash) && strcmp(label, "") != 0)
                {
                    makeIntermediateFILE(NULL, 1); //Duplicated Error
                    exit(1);
                }
                else
                {
                    if (strcmp(label, ""))
                    {
                        AddSYMHashData(label, LOC, use_hash);
                    }
                }
                if (opcode[0] == '+')
                {
                    char *temp;
                    change_loc(opcode);
                }
                else if (FindOPHashData != 0)
                    change_loc(opcode);
            }
            //printf("OP : %s LOC : %X \n", opcode, LOC);
        }
    }
    Total_Length = LOC - startadd;
}
void pass2(FILE *ffp, char *argv)
{
    readSYMTAB(ffp);
    LOC = startadd;
    int n;
    int indirect_check, immediate_check;
    TS = LOC;
    char buffer[80];
    struct M_rec *M_REC = (struct M_rec *)malloc(sizeof(struct M_rec));
    char *BASE_name;
    int BASE_reg, BASE_ck = 0;
    int L_reg = 0;
    char list_operand[10]; //listing file출력을 위한 보관소
    obj_file = fopen(argv, "w");
    FILE *listing_file = fopen("assembly listing file.s", "w");
    fgets(buffer, sizeof(buffer), ffp);
    getLine(buffer);

    if (!strcmp(opcode, "START"))
    {
        objcode += strtoul(operand, NULL, 16);
        fprintf(listing_file, "%.4x\t\t%s\t\t%s\t\t%s\n", objcode, label, opcode, operand);
    }
    fprintf(obj_file, "H%s\t%.6X%.6x\n", label, objcode, Total_Length); //H레코드
    while (fgets(buffer, sizeof(buffer), ffp) != NULL)
    {
        M_check = 0;
        if (buffer[0] == '.')
        {
            fprintf(listing_file, "\t\t%s", buffer);
        }
        getLine(buffer);
        strcpy(list_operand, operand);
        //operand input error 처리//
        char *er_tmp;
        if (strcmp(operand, "") != 0)
        {
            er_tmp = strtok(operand, "#@");
        }

        if ((er_tmp[0] < 48 || er_tmp[0] > 57) && !(strlen(er_tmp) == 1 && er_tmp[strlen(er_tmp) - 1] == 'X') && !(strlen(er_tmp) == 1 && er_tmp[strlen(er_tmp) - 1] == 'F') && !(strlen(er_tmp) == 1 && er_tmp[strlen(er_tmp) - 1] == 'A') && !(strlen(er_tmp) == 1 && er_tmp[strlen(er_tmp) - 1] == 'S') && !(strlen(er_tmp) == 1 && er_tmp[strlen(er_tmp) - 1] == 'B') && !(strlen(er_tmp) == 1 && er_tmp[strlen(er_tmp) - 1] == 'T') && !(strlen(er_tmp) == 1 && er_tmp[strlen(er_tmp) - 1] == 'L') && strcmp(er_tmp, "SW") != 0 && strcmp(er_tmp, "PC") != 0 && !(er_tmp[0] == 'C' && er_tmp[strlen(er_tmp) - 1] == '\'') && !(er_tmp[0] == 'X' && er_tmp[strlen(er_tmp) - 1] == '\'') && strcmp(operand, "") != 0)
        {

            char *err_tmp = strtok(er_tmp, ",");
            //printf("%s\n", er_tmp);
            while (err_tmp)
            {
                if ((strlen(err_tmp) == 1 && err_tmp[strlen(err_tmp) - 1] == 'X') || (strlen(err_tmp) == 1 && err_tmp[strlen(err_tmp) - 1] == 'F') || (strlen(err_tmp) == 1 && err_tmp[strlen(err_tmp) - 1] == 'A') || (strlen(err_tmp) == 1 && err_tmp[strlen(err_tmp) - 1] == 'S') || (strlen(err_tmp) == 1 && err_tmp[strlen(err_tmp) - 1] == 'B') || (strlen(err_tmp) == 1 && err_tmp[strlen(err_tmp) - 1] == 'T') || (strlen(err_tmp) == 1 && err_tmp[strlen(err_tmp) - 1] == 'L') || strcmp(err_tmp, "SW") == 0 || strcmp(err_tmp, "PC") == 0)
                {
                    err_tmp = strtok(NULL, ",");
                }
                else
                {
                    if (use_hash == 'y')
                    {
                        if (FindSYMTAB(SYMTAB, err_tmp, use_hash) == NULL)
                        {
                            FILE *ffp = fopen("Intermediate file.asm", "w");
                            fprintf(ffp, "Undefined symbol!\n"); //Undefined symbol ERROR
                            fclose(ffp);
                            exit(1);
                        }
                    }
                    else
                    {
                        if (FindSYMTAB(SYMTAB_ns, err_tmp, use_hash) == NULL)
                        {
                            FILE *ffp = fopen("Intermediate file.asm", "w");
                            fprintf(ffp, "Undefined symbol!\n"); //Undefined symbol ERROR
                            fclose(ffp);
                            exit(1);
                        }
                    }
                    err_tmp = strtok(NULL, ",");
                }
            }
        }
        //assemble listing file print
        if (!strcmp(opcode, "BASE"))
        {
            fprintf(listing_file, "%s", label);
            if (!strcmp(label, ""))
                fprintf(listing_file, "\t");
            fprintf(listing_file, "\t\t%s\t\t%s\n", opcode, list_operand);
        }
        else if (buffer[0] == '.')
        {
        }
        else
        {
            fprintf(listing_file, "%.4X\t\t%s", LOC, label);
            if (!strcmp(label, ""))
                fprintf(listing_file, "\t");
            fprintf(listing_file, "\t\t%s\t\t%s", opcode, list_operand);
        }
        //size계산(출력할지 말지)
        int nextsize = 0;
        if (opcode[0] == '+')
            nextsize = 4;
        else if (!strcmp(opcode, "CLEAR") || !strcmp(opcode, "TIXR") || !strcmp(opcode, "COMPR"))
            nextsize = 2;
        else if (!strcmp(opcode, "RESW") && !strcmp(opcode, "RESB"))
        {
            if (operand[0] == 'C')
            {
                nextsize = (strlen(operand) - 3) * 2;
            }
            else
            {
                nextsize = (strlen(operand) - 3);
            }
        }
        else
            nextsize = 3;

        if (strcmp(opcode, "END"))
        {
            if (buffer[0] != '.')
            {
                if (txt_cnt + nextsize > 30)
                {
                    print_T_rec();
                    TS = LOC;
                }
                if (FindOPHashData(strtok(opcode, "+"), use_hash) != 0 || strcmp(opcode, "LDA") == 0)
                {
                    change_loc(opcode); //?
                    if (!strcmp(opcode, "BYTE"))
                    {
                        objcode = Byte_case(operand); ///선언전
                        n = strlen(operand) - 3;
                        sprintf(objcode_txt, "%.*X", n, objcode);
                        strcat(txt_rec, objcode_txt);
                        if (operand[0] == 'C')
                        {
                            txt_cnt += (strlen(operand) - 3) * 2;
                        }
                        else
                        {
                            txt_cnt += (strlen(operand) - 3);
                        }
                    }
                    else if (!strcmp(opcode, "WORD"))
                    {
                        objcode = 0x000000;
                        objcode += atoi(operand);
                        sprintf(objcode_txt, "%.6X", objcode);
                        strcat(txt_rec, objcode_txt);
                        txt_cnt += 3;
                    }
                    else if (!strcmp(opcode, "CLEAR") || !strcmp(opcode, "TIXR") || !strcmp(opcode, "COMPR"))
                    {
                        if (!strcmp(opcode, "CLEAR"))
                        {
                            objcode = 0xB4;
                            objcode = objcode << 8;
                        }
                        else if (!strcmp(opcode, "TIXR"))
                        {
                            objcode = 0xB8;
                            objcode = objcode << 8;
                        }
                        else if (!strcmp(opcode, "COMPR"))
                        {
                            objcode = 0xA0;
                            objcode = objcode << 8;
                            char *cmp_tmp = strtok(list_operand, ",");
                            cmp_tmp = strtok(NULL, ",");
                            //fprintf(stderr, "%s\n", cmp_tmp);
                            if (cmp_tmp[strlen(cmp_tmp) - 1] == 'X')
                                objcode += 0x01;
                            else if (cmp_tmp[strlen(cmp_tmp) - 1] == 'L')
                                objcode += 0x02;
                            else if (cmp_tmp[strlen(cmp_tmp) - 1] == 'B')
                                objcode += 0x03;
                            else if (cmp_tmp[strlen(cmp_tmp) - 1] == 'S')
                                objcode += 0x04;
                            else if (cmp_tmp[strlen(cmp_tmp) - 1] == 'T')
                                objcode += 0x05;
                            else if (cmp_tmp[strlen(cmp_tmp) - 1] == 'F')
                                objcode += 0x06;
                            else if (cmp_tmp[strlen(cmp_tmp) - 1] == 'C') //PC
                                objcode += 0x08;
                            else if (cmp_tmp[strlen(cmp_tmp) - 1] == 'W') //SW
                                objcode += 0x09;
                            sprintf(objcode_txt, "%.4X", objcode);
                            strcat(txt_rec, objcode_txt);
                            txt_cnt += 2;
                            fprintf(listing_file, "\t\t%.X\n", objcode);
                            continue;
                        }

                        if (!strcmp(operand, "X"))
                            objcode += 0x10;
                        else if (!strcmp(operand, "L"))
                            objcode += 0x20;
                        else if (!strcmp(operand, "B"))
                            objcode += 0x30;
                        else if (!strcmp(operand, "S"))
                            objcode += 0x40;
                        else if (!strcmp(operand, "T"))
                            objcode += 0x50;
                        else if (!strcmp(operand, "F"))
                            objcode += 0x60;
                        else if (!strcmp(operand, "PC"))
                            objcode += 0x80;
                        else if (!strcmp(operand, "SW"))
                            objcode += 0x90;
                        sprintf(objcode_txt, "%.4X", objcode);
                        strcat(txt_rec, objcode_txt);
                        txt_cnt += 2;
                    }
                    else if (strcmp(opcode, "RESW") != 0 && strcmp(opcode, "RESB") != 0)
                    {
                        struct SYMNode *temp = (struct SYMNode *)malloc(sizeof(struct SYMNode));
                        char *tempop;
                        int ni_bit = 3;
                        //printf("&%s %s&", opcode, operand);

                        if (operand[0] == '#')
                        {
                            ni_bit = 1;
                            tempop = strtok(operand, "#");
                            strcpy(operand, tempop);
                        }
                        else if (operand[0] == '@')
                        {
                            ni_bit = 2;
                            tempop = strtok(operand, "@");
                            strcpy(operand, tempop);
                        }
                        if (use_hash == 'y')
                            temp = FindSYMTAB(SYMTAB, operand, use_hash); //
                        else
                        {
                            temp = FindSYMTAB(SYMTAB_ns, operand, use_hash);
                        }
                        int im_ck = 0;
                        if (temp != NULL)
                        {
                            operandAddr = temp->loc;
                        }
                        else
                        {
                            if (operand[0] >= 48 && operand[0] <= 57) //immediate addressing 숫자일경우 예외처리
                            {
                                im_ck = 1;
                            }
                            else if (strcmp(operand, "") != 0)
                            {
                                operandAddr = 0;
                                //fprintf(stderr, "Operand does not exist");
                            }
                            else
                            {
                                operandAddr = 0;
                            }
                        }
                        if (im_ck == 1 && opcode[0] != '+')
                        {
                            operandAddr = strtoul(operand, NULL, 16);
                            objcode = FindOPHashData(opcode, use_hash);
                            objcode += ni_bit;
                            objcode = objcode << 16;
                            objcode += operandAddr;
                            sprintf(objcode_txt, "%.6X", objcode);
                            strcat(txt_rec, objcode_txt);
                            txt_cnt += 3;
                        }
                        else
                        {
                            objcode = FindOPHashData(strtok(opcode, "+"), use_hash);
                            if (opcode[0] == '+')
                            {
                                objcode += ni_bit;
                                objcode = objcode << 24;
                                objcode += 0x00100000;
                                if (operand[0] >= 48 && operand[0] <= 57) //숫자는 그대로
                                {
                                    operandAddr = atoi(operand);
                                }
                                objcode += operandAddr;
                                sprintf(objcode_txt, "%.X", objcode);
                                strcat(txt_rec, objcode_txt);
                                txt_cnt += 4;
                                //M_rec저장
                                if (opcode[1] == 'J')
                                {
                                    M_code = LOC - 3;
                                    M_code = M_code << 8;
                                    M_code += 5;
                                    struct M_rec *Temp_rec = (struct M_rec *)malloc(sizeof(struct M_rec));
                                    Temp_rec->M_code = M_code;
                                    if (M_REC->next == NULL)
                                    {
                                        M_REC->root_ck = 1;
                                        M_REC->next = Temp_rec;
                                        Temp_rec->next = M_REC;
                                        M_REC = Temp_rec;
                                    }
                                    else
                                    {
                                        Temp_rec->next = M_REC->next;
                                        M_REC->next = Temp_rec;
                                        M_REC = Temp_rec;
                                    }
                                }
                                fprintf(listing_file, "\t\t %.X\n", objcode);
                                continue;
                            }
                            else if (!strcmp(opcode, "RSUB"))
                            {
                                objcode += ni_bit;
                                objcode = objcode << 16;
                                objcode += L_reg;
                            }
                            else //n,i bit
                            {
                                objcode += ni_bit;
                                objcode = objcode << 16;
                                objcode += operandAddr;
                            }
                            if (operand[strlen(operand) - 2] == ',' && operand[strlen(operand) - 1] == 'X')
                            {
                                char *tp;
                                int tmp;
                                objcode += 0x008000; //X비트 더해주기
                                if (BASE_ck == 1)
                                {
                                    objcode += 0x004000; //B비트 더해주기
                                }
                                tp = strtok(operand, ",");
                                if (use_hash == 'y')
                                    tmp = FindSYMTAB(SYMTAB, tp, use_hash)->loc; //
                                else
                                {
                                    tmp = FindSYMTAB(SYMTAB_ns, tp, use_hash)->loc;
                                }

                                tmp -= BASE_reg;
                                objcode += tmp;
                            }
                            else if (!strcmp(opcode, "RSUB")) //RSUB의경우 pc or base relative addressing필없음
                            {
                            }
                            else if (!(ni_bit == 1 && (operand[1] >= 48 && operand[1] <= 57))) //pc relative
                            {
                                if (LOC - operandAddr > 0x1000) //Base relative
                                {
                                    objcode += 0x004000;
                                    objcode -= BASE_reg;
                                }
                                else
                                {
                                    objcode += 0x002000;
                                    objcode -= LOC; // TA-(PC) // PC relative
                                    if (operandAddr < LOC)
                                        objcode += 0x001000; //objcode가 음수가될경우
                                }
                                //printf("OP: %s objcode : %X\n", opcode, objcode);
                            }
                            sprintf(objcode_txt, "%.6X", objcode);
                            strcat(txt_rec, objcode_txt);
                            txt_cnt += 3;
                        }
                    }
                }
                if (!strcmp(opcode, "BASE"))
                {
                    //strcpy(BASE_name, operand);
                    if (use_hash == 'y')
                        BASE_reg = FindSYMTAB(SYMTAB, operand, use_hash)->loc; //
                    else
                    {
                        BASE_reg = FindSYMTAB(SYMTAB_ns, operand, use_hash)->loc;
                    }
                    BASE_ck = 1;
                }
                else if (!strcmp(opcode, "LDL"))
                {
                    L_reg = operandAddr;
                }
                if (strcmp(opcode, "RESW") == 0 || strcmp(opcode, "RESB") == 0)
                {
                    //RESW와RESB는 objcode가 없고, T레코드에서 이 다음부터 출력이 됨
                    if (strcmp(txt_rec, "") != 0)
                    {
                        print_T_rec();
                        TS = LOC; //T_rec 시작주소변경
                    }
                    TS = LOC;
                }
            }
        }
        if (!strcmp(opcode, "RESW") || !strcmp(opcode, "RESB") || !strcmp(opcode, "END"))
        {
            fprintf(listing_file, "\n");
        }
        else if (!strcmp(opcode, "BYTE") && operand[0] == 'X')
        {
            char *q;
            q = strtok(operand, "'");
            q = strtok(NULL, "'");
            fprintf(listing_file, "\t\t%s\n", q);
        }
        else if (strcmp(opcode, "BASE") && buffer[0] != '.')
        {
            fprintf(listing_file, "\t\t%.6X\n", objcode);
        }
    }
    print_T_rec();
    print_M_rec(M_REC);
    fprintf(obj_file, "E%.6x\n", startadd);
    fclose(obj_file);
    fclose(listing_file);
}
int main(int argc, char *argv[])
{
    start = clock();
    printf("Do you want to use hashtable?(y/n) : ");
    scanf("%c", &use_hash);
    //char *saveidx[66] = {};
    if (use_hash == 'y')
    {
        for (int i = 0; i < 65; i++) //OPTAB hashtable 생성
        {
            struct Node *node = (struct Node *)malloc(sizeof(struct Node));
            strcpy(node->name, optab[i].name);
            node->code = optab[i].code;
            node->hashNext = NULL;
            AddOPHashData(node->name, node);
            //saveidx[i] = (char *)malloc(sizeof(char) * 8);
            //strcpy(saveidx[i], node->name);
        }
    }
    openp1file(argv[1]);
    pass1(fp);
    //fprintf(stderr, "Pass1 Done");
    //intermediate file print
    makeIntermediateFILE(argv[1], 0);
    openp2file();
    pass2(ffp, argv[2]);
    end = clock();
    clock_t res = (double)(end - start);
    printf("\n%lf\n", (double)(end - start) / 1000);
    return 0;
}