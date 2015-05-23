//
//  main.c
//  project3
//
//  Created by Blurry on 2015/5/21.
//  Copyright (c) 2015年 Mac. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADD 0x20
#define SUB 0x22
#define AND 0x24
#define OR 0x25
#define XOR 0x26
#define NOR 0x27
#define NAND 0x28
#define SLT 0x2A
#define SLL 0x00
#define SRL 0x02
#define SRA 0x03
#define JR 0x08

#define ADDI 0x08
#define LW 0x23
#define LH 0x21
#define LHU 0x25
#define LB 0x20
#define LBU 0x24
#define SW 0x2b
#define SH 0x29
#define SB 0x28
#define LUI 0x0f
#define ANDI 0x0c
#define ORI 0x0d
#define NORI 0x0e
#define SLTI 0x0a
#define BEQ 0x04
#define BNE 0x05

#define J 0x02
#define JAL 0x03

static int * code_i, * code_d;
static int i_disk[256] , d_disk[256] ,reg[32] , PC ;
static int op , rs , rt , rd , shamt , funct , imm , address ;
static int cycle_count , HaltFlag;
//int error3_re ;
int value_rs,value_imm,value_rt;
/* I-MEM-size, D-MEM-size, I-pagesize, D-pagesize, I-CACHE-size
 I-blocksize, I-set.associativity, D-CACHE-size, D-blocksize, D-set.associativity*/
static int I_MEM_block_num = 0 ,D_MEM_block_num = 0 , I_CACHE_block_num = 0 , D_CACHE_block_num = 0 ,
        I_PTE_entries = 0 , D_PTE_entries = 0 , I_TLB_entries = 0 , D_TLB_entries = 0 ,
        I_associativity = 0 , D_associativity = 0 ,I_pagesize = 0 , D_pagesize = 0 , I_blocksize = 0 , D_blocksize = 0;
static unsigned int ICache_misses = 0, ICache_hits = 0, DCache_misses = 0, DCache_hits = 0;
static unsigned int ITLB_misses = 0, ITLB_hits = 0, DTLB_misses = 0, DTLB_hits = 0;
static unsigned int IPTE_misses = 0, IPTE_hits = 0, DPTE_misses = 0, DPTE_hits = 0;
static struct CACHE{
    int *LRU_order;
    int *valid;
    int *data;
    int *tag;
} I_CACHE , D_CACHE;

static struct MEMORY{
    int *LRU_order;
    int *valid;
    int *data;
} I_MEM , D_MEM;

static struct PAGE_TABLE_ENTRY{
    int *content;
    int *valid;
} I_PTE , D_PTE;

static struct TLB{
    int *LRU_order;
    int *valid;
    int *content;
    int *tag;
} I_TLB , D_TLB;


void ReadFile();
void LoadInMem();
void decode(int instruction_code);
void R_type(int instruction_code);
void I_type(int instruction_code);
void J_type(int instruction_code);
void TestError(int in1 , int in2 , int out ,int type);
int Change_endian(int c);
void ErrorDetect(int err);
void AllocateMemory(int argc , char *argv[]);
void I_TLBupdate(int VA);

int main(int argc , char *argv[]) {
    
    FILE * fw_ptr = fopen("snapshot.rpt", "w");
    
    ReadFile();
    
    /* clean array first */
    int i;
    for (i = 0; i < 256 ; i++ ) {
        i_disk[i] = 0;
        d_disk[i] = 0;
    }
    for (i = 0; i < 32 ; i++ ) {
        reg[i] = 0;
    }
    PC = 0;
    cycle_count = 0;
    HaltFlag = 0;
    ICache_misses = ICache_hits =  DCache_misses =  DCache_hits = 0;
    ITLB_misses = ITLB_hits =  DTLB_misses = DTLB_hits = 0;
    IPTE_misses = IPTE_hits = DPTE_misses = DPTE_hits = 0;
    
    LoadInMem();
    AllocateMemory(argc, argv);
    while (1) {
        //printf("begin while.\n");
        op = rs = rt = rd = shamt = funct = imm = address = 0;
        //error3_re = 0;
        
        if (cycle_count > 500000) {
            printf("cycle counts more than 500,000 .\n");
        }
        /*
         fprinf to snapshot.rpt;
         */
        int i ;
        fprintf( fw_ptr , "cycle %d\n" , cycle_count);
        for (i = 0; i < 32; i ++) {
            fprintf( fw_ptr , "$%02d: 0x%08X\n", i , reg[i]);
        }
        fprintf( fw_ptr , "PC: 0x%08X\n\n\n" , PC );
        
        cycle_count++;
        
        
        if(PC%4 != 0 || PC > 1024 || PC < 0){
            printf("pc is wrong in cycle %d\n",cycle_count);
            exit(1);
        }
        decode(i_disk[PC/4]);
        if (op == 0x3F) {
            break;
        }
        
    }
    fclose(fw_ptr);
    
    
    return 0;
}

void ReadFile(){ //put all binary code into men and detect error
    
    FILE * fr_i = fopen("iimage.bin", "rb");
    FILE * fr_d = fopen("dimage.bin", "rb");
    
    
    if(fr_i == NULL || fr_d == NULL){
        printf("Your testcase is WRONG or can't find.\n");
        exit(2);
    }
    
    int a = fseek(fr_i, 0L , SEEK_END);
    long int filesize_i = ftell(fr_i);
    int b = fseek(fr_i, 0L, SEEK_SET);
    //  filesize_i /= 4;
    
    if(a == 0 && b == 0)  code_i= malloc(filesize_i);
    else {
        printf("fseek iimage wrong and malloc failed .\n");
        exit(3);
    }
    
    a = fseek(fr_d, 0L , SEEK_END);
    long int filesize_d = ftell(fr_d);
    b = fseek(fr_d, 0L, SEEK_SET);
    //  filesize_d /= 4;
    
    if(a == 0 && b == 0)  code_d = malloc(filesize_d);
    else {
        printf("fseek dimage wrong and malloc failed .\n");
        exit(3);
    }
    
    fread(code_i, filesize_i , 1 , fr_i);
    fread(code_d, filesize_d , 1 , fr_d);
    //printf("%ld %d\n",sizeof(code_i),filesize_i);
    //printf("filesize = %ld %ld\n",filesize_i,filesize_d);
    
    int x = 0x1234;
    char y = *(char*)&x;
    
    if(y != 0x12){ /* little endian */
        //printf("shift begin\n") ;
        int i ;
        
        code_i[1] = Change_endian(code_i[1]);
        //printf("1 = %08x\n",code_i[1]);
        
        code_i[0] = Change_endian(code_i[0]);
        //printf("0 = %08x\n",code_i[0]);
        
        for ( i = 2; i < code_i[1] + 2; i++){
            
            code_i[i] = Change_endian(code_i[i]);
            //printf("i = %d ,%08x\n",i,code_i[i]);
        }
        //printf("code_i[1] = %08x\n",code_i[1]);
        
        //printf("origainal 1 = %08x\n",code_d[1]);
        code_d[1] = Change_endian(code_d[1]);
        //printf("1 = %08x\n",code_d[1]);
        
        code_d[0] = Change_endian(code_d[0]);
        //printf("0 = %08x\n",code_d[0]);
        if (code_d[1] > 1024) {
            //printf("brokennnnnnn .\n");
            exit(1);
        }
        for ( i = 2; i < code_d[1] + 2 ; i++){
            code_d[i] = Change_endian(code_d[i]);
            //printf("i = %d ,%08x\n",i,code_d[i]);
        }
        //printf("code_d[1] = %08x\n",code_d[1]);
    }
    
    
    
    fclose(fr_i);
    fclose(fr_d);
}
int Change_endian(int c){
    
    return ((c << 24 ) & 0xff000000 ) | ((c << 8 ) & 0x00ff0000 ) |
    ((c >> 8 ) & 0x0000ff00 ) | ((c >> 24 ) & 0x000000ff) ;
    
}


void LoadInMem(){
    
    PC = code_i[0];
    reg[29] = code_d[0];
    int instr_num_i = code_i[1];
    int instr_num_d = code_d[1];
    //printf("PC = %08X, num = %08X .\n",PC ,instr_num_i);
    if ( PC + instr_num_i > 1024) {
        printf("iimage instructions more than 1K . Memory overflow .\n");
        printf("PC = %08X, num = %08X .\n",PC ,instr_num_i);
        
        exit(1);
    }
    if (instr_num_d > 1024) {
        printf("dimage instructions more than 1K . Memory overflow .\n");
        printf("$s = %08X, num = %08X .\n",reg[29] ,instr_num_d);
        exit(1);
    }
    
    int i , j;
    for (i = PC/4 , j = 2 ; j < 2 + instr_num_i ; i++, j++ )
        i_disk[i] = code_i[j];
    
    for (i = 0 , j = 2 ; j < 2 + instr_num_d ; i++, j++ )
        d_disk[i] = code_d[j];
    //printf("before free\n");
    /*free(code_i);
     free(code_d);*/
    
}

void decode(int instruction_code){
    
    I_TLBupdate(PC);
    op = (instruction_code >> 26) & 0x3f;
    
    if(op == 0x3F){
        exit(0);
    }
    
    PC = PC + 4 ; /*在執行這行的時候 ＰＣ已經跳到下一個 */
    
    if(op == 0x00){
        R_type(instruction_code);
    }
    else if(op == 0x02 || op == 0x03){
        J_type(instruction_code);
    }
    else{
        I_type(instruction_code);
    }
    
}

void R_type(int instruction_code){
    funct = instruction_code & 0x3f;
    shamt = (instruction_code >> 6) & 0x1f;
    rd = (instruction_code >> 11) & 0x1f;
    rt = (instruction_code >> 16) & 0x1f;
    rs = (instruction_code >> 21) & 0x1f;
    
    if (funct != ADD && funct != SUB && funct != AND &&
        funct != OR && funct != XOR && funct != NOR &&
        funct != NAND && funct != SLT && funct != SLL &&
        funct != SRL && funct != SRA && funct != JR ) {
        printf("R_type function is wrong , in Cycle %d .\n",cycle_count);
        exit(-1);
    }
    
    if (rd == 0 && funct!=JR) {
    
        /*if ( (funct == SLL  ) && rt == 0 && shamt == 0)
            ErrorDetect(0);
        else{
            ErrorDetect(1);*/
            if(funct == ADD){
                TestError(reg[rs],reg[rt],reg[rs] + reg[rt],2);
            }
            else if (funct == SUB){
                TestError(reg[rs],(-reg[rt]),reg[rs] + (-reg[rt]) ,2);
                // printf("sub = %08x - %08x = %08x\n",reg[rs],(-reg[rt]),reg[rs] + (-reg[rt]));
            }
        //}
    }
    
    else{
        switch (funct) {
            case ADD:
                TestError(reg[rs],reg[rt],reg[rs] + reg[rt],2);
                reg[rd] = reg[rs] + reg[rt];
                
                break;
            case SUB:
                TestError(reg[rs],(-reg[rt]),reg[rs] + (-reg[rt]),2);
                reg[rd] = reg[rs] + (-reg[rt]);
                /*  value_rs =  reg[rs] ;
                 value_rt =  reg[rt] ;
                 reg[rd] = reg[rs] - reg[rt] ;
                 // TestError2
                 int s_in1 = (value_rs >> 31) & 1 ;
                 int s_in2 = (value_rt >> 31) & 1 ;
                 int s_out = (reg[rd] >> 31) & 1 ;
                 //N-P = P OR P-N = N
                 if ((s_in1 != s_in2 ) && ( s_in2 == s_out) ) {
                 ErrorDetect(2);
                 }*/
                break;
            case AND:
                reg[rd] = reg[rs] & reg[rt];
                break;
            case OR:
                reg[rd] = reg[rs] | reg[rt];
                break;
            case XOR:
                reg[rd] = reg[rs] ^ reg[rt];
                break;
            case NOR:
                reg[rd] = ~(reg[rs] | reg[rt]);
                break;
            case NAND:
                reg[rd] = ~(reg[rs] & reg[rt]);
                break;
            case SLT:
                reg[rd] = (reg[rs] < reg[rt])?1:0;
                break;
            case SLL:
                reg[rd] = reg[rt] << shamt;
                break;
            case SRL:/* unsigned shift */
                if (shamt==0)
                    reg[rd] = reg[rt];
                else
                    reg[rd] = (reg[rt] >> shamt) & ~(0xffffffff << (32 - shamt));
                break;
            case SRA:/* signed shift */
                reg[rd] = reg[rt] >> shamt;
                break;
            case JR:
                PC = reg[rs] ;
                break;
            default:
                break;
        }
        
    }
    
}

void J_type(int instruction_code){
    
    address = instruction_code & 0x3ffffff;
    switch (op) {
        case J:
            PC = ( PC & 0xf0000000 ) | (4 * address) ;
            break;
        case JAL:
            reg[31] = PC;
            PC = ( PC & 0xf0000000 ) | (4 * address) ;
            break;
        default:
            printf("J_type opcode is wrong in cycle %d .\n",cycle_count);
            break;
    }
    
}

void I_type(int instruction_code){
    
    imm = ((instruction_code & 0xffff) << 16 ) >> 16;
    rt = (instruction_code >> 16) & 0x1f ;
    rs = (instruction_code >> 21) & 0x1f ;
    
    if (op != ADDI && op != LW && op != LH &&
        op != LHU && op != LB && op != LBU &&
        op != SW && op != SH && op != SB &&
        op != LUI && op != ANDI && op != ORI &&
        op != NORI && op != SLTI && op != BEQ && op != BNE) {
        printf("J_type function is wrong , in Cycle %d .\n",cycle_count);
        exit(-1);
    }
    
    if (rt == 0 && op != SW && op != SH && op != SB
        && op != BEQ && op != BNE) {
        //ErrorDetect(1);
        /*if(op == LW || op == LH || op == LHU || op == LB||
           op == LBU || op == SW || op == SH || op == SB){
            TestError(reg[rs], imm , reg[rs] + imm ,2);
            TestError(reg[rs], imm , 0 ,3);
            if(op == LW || op == SW){
                if ( (reg[rs] + imm) % 4 != 0)
                    ErrorDetect(4);
            }
            else if (op == LH || op == SH || op	== LHU){
                if ( (reg[rs] + imm) % 2 != 0)
                    ErrorDetect(4);
            }
            if (HaltFlag==1)
                exit(1);
        }
        else */if(op == ADDI)TestError(reg[rs], imm , reg[rs] + imm ,2);
    }
    else {
        switch ( op ) {
            case ADDI :
                value_rs =  reg[rs] ;
                value_imm =  imm;
                reg[rt] = reg[rs] + imm;
                TestError(value_rs, value_imm, reg[rt], 2);
                break;
            case LW :
                TestError(reg[rs], imm , reg[rs] + imm ,2);
                TestError(reg[rs], imm , 0 ,3);
                if ( (reg[rs] + imm) % 4 != 0)
                    ErrorDetect(4);
                if (HaltFlag==1)
                    exit(1);
                reg[rt] = d_disk[ (reg[rs] + imm) / 4 ];
                break;
            case LH : /*load 整個word的一半 所以還是 /4 */
                TestError(reg[rs], imm , reg[rs] + imm ,2);
                TestError(reg[rs], imm , 0 ,3);
                if ( (reg[rs] + imm) % 2 != 0)
                    ErrorDetect(4);
                if (HaltFlag==1)
                    exit(1);
                if ((reg[rs] + imm) % 4 == 0 )
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ]  >> 16;
                else
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ] << 16 >> 16 ;
                break;
            case LHU :
                TestError(reg[rs], imm , reg[rs] + imm ,2);
                TestError(reg[rs], imm , 0 ,3);
                if ( (reg[rs] + imm) % 2 != 0)
                    ErrorDetect(4);
                if (HaltFlag==1)
                    exit(1);
                
                if ((reg[rs] + imm) % 4 == 0 )
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ]  >> 16 & 0x0000ffff;
                else
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ] << 16 >> 16 & 0x0000ffff;
                break;
                
            case LB :
                TestError(reg[rs], imm , reg[rs] + imm ,2);
                TestError(reg[rs], imm , 0 ,3);
                if (HaltFlag==1)
                    exit(1);
                
                if ((reg[rs] + imm) %4 == 0)
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ]  >> 24;
                else if ((reg[rs] + imm) %4 == 1)
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ] << 8 >> 24;
                else if ((reg[rs] + imm) %4 == 2)
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ] << 16 >> 24;
                else
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ] << 24 >> 24 ;
                break;
            case LBU :
                TestError(reg[rs], imm , reg[rs] + imm ,2);
                TestError(reg[rs], imm , 0 ,3);
                if (HaltFlag==1)
                    exit(1);
                
                if ((reg[rs] + imm) %4 == 0)
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ]  >> 24 & 0x000000ff;
                else if ((reg[rs] + imm) %4 == 1)
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ] << 8 >> 24 & 0x000000ff;
                else if ((reg[rs] + imm) %4 == 2)
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ] << 16 >> 24 & 0x000000ff;
                else
                    reg[rt] = d_disk[ (reg[rs] + imm) / 4 ] << 24 >> 24 & 0x000000ff;
                break;
            case SW :
                TestError(reg[rs], imm , reg[rs] + imm ,2);
                TestError(reg[rs], imm , 0 ,3);
                if ( (reg[rs] + imm) % 4 != 0 )
                    ErrorDetect(4);
                if (HaltFlag==1)
                    exit(1);
                
                d_disk[ (reg[rs] + imm) / 4 ] = reg[rt];
                break;
            case SH :
                TestError(reg[rs], imm , reg[rs] + imm ,2);
                TestError(reg[rs], imm , 0 ,3);
                if ( (reg[rs] + imm) % 2 != 0)
                    ErrorDetect(4);
                if (HaltFlag==1)
                    exit(1);
                
                if ((reg[rs] + imm) % 4 == 0 ){
                    d_disk[ (reg[rs] + imm) / 4 ] &= 0x0000ffff ;
                    d_disk[ (reg[rs] + imm) / 4 ] |= ((reg[rt] & 0x0000ffff)<< 16 );
                }
                else{
                    d_disk[ (reg[rs] + imm) / 4 ] &= 0xffff0000 ;
                    d_disk[ (reg[rs] + imm) / 4 ] |= (reg[rt] & 0x0000ffff);
                }
                
                break;
            case SB :
                TestError(reg[rs], imm , reg[rs] + imm ,2);
                TestError(reg[rs], imm , 0 ,3);
                if (HaltFlag==1)
                    exit(1);
                
                if ((reg[rs] + imm) %4 == 0){
                    d_disk[ (reg[rs] + imm) / 4 ] &= 0x00ffffff ;
                    d_disk[ (reg[rs] + imm) / 4 ] |= ( (reg[rt] & 0x000000ff) << 24 );
                }
                else if ((reg[rs] + imm) %4 == 1){
                    d_disk[ (reg[rs] + imm) / 4 ] &= 0xff00ffff ;
                    d_disk[ (reg[rs] + imm) / 4 ] |= ( (reg[rt] & 0x000000ff) << 16 );
                }
                else if ((reg[rs] + imm) %4 == 2){
                    d_disk[ (reg[rs] + imm) / 4 ] &= 0xffff00ff ;
                    d_disk[ (reg[rs] + imm) / 4 ] |= ( (reg[rt] & 0x000000ff)  << 8 );
                }
                else{
                    d_disk[ (reg[rs] + imm) / 4 ] &= 0xffffff00 ;
                    d_disk[ (reg[rs] + imm) / 4 ] |=  (reg[rt] & 0x000000ff)  ;
                }
                break;
            case LUI :
                reg[rt] = imm << 16;
                break;
            case ANDI :
                reg[rt] = reg[rs] & ( imm & 0x0000ffff );
                break;
            case ORI :
                reg[rt] = reg[rs] | (imm & 0x0000ffff) ;
                break;
            case NORI :
                reg[rt] = ~(reg[rs] | (imm & 0x0000ffff));
                break;
            case SLTI :
                reg[rt] = (reg[rs] < imm)? 1 : 0 ;
                break;
            case BEQ :
                if (reg[rs] == reg [rt]) {
                    PC += imm * 4 ;
                }
                break;
            case BNE :
                if (reg[rs] != reg [rt]) {
                    PC += imm * 4 ;
                }
                break;
                
            default:
                break;
        }
        
    }
    
    
}

void TestError(int in1 , int in2 , int out ,int type ){
    int s_in1, s_in2 , s_out;
    
    switch (type) {
        case 2:/* number overflow */
            s_in1 = (in1 >> 31) & 1 ;
            s_in2 = (in2 >> 31) & 1 ;
            s_out = (out >> 31) & 1 ;
            //  printf("singned = %d %d %d in cycle = %d\n",s_in1,s_in2,s_out,cycle_count);
            //  printf("num = %8x %8x %8x in cycle = %d\n",in1,in2,out,cycle_count);
            if ( (s_in1 == s_in2 ) && ( s_out != s_in1 ) ) {
                ErrorDetect(2);
            }
            
            break;
        /*case 3: D Memory Address overflow  --   lw, lh, lhu, lb,
                lbu, sw, sh, sb,
            if( (in1 + in2) < 0 || (in1 + in2 )> 1023){
                //if(error3_re == 1){break;}
                ErrorDetect(3);
                
            }
            else if( (op == SW || op == LW) && ((in1 + in2 ) > 1020) ) ErrorDetect(3);
            else if( (op == SH || op == LH || op == LHU) && ((in1 + in2 ) > 1022) ){
                ErrorDetect(3);
            }
            break;*/
        default:
            break;
    }
}
void ErrorDetect(int err){
    FILE * fw_err = fopen("error_dump.rpt", "a");
    switch (err) {
        case 1:
            fprintf( fw_err, "In cycle %d: Write $0 Error\n", cycle_count);
            break;
        case 2:
            fprintf( fw_err , "In cycle %d: Number Overflow\n", cycle_count);
            break;
        case 3:
            fprintf( fw_err , "In cycle %d: Address Overflow\n", cycle_count);
            HaltFlag = 1;
            break;
        case 4:
            fprintf( fw_err , "In cycle %d: Misalignment Error\n", cycle_count);
            HaltFlag = 1;
            break;
        default:
            break;
    }
    fclose(fw_err);
}

void AllocateMemory(int argc , char *argv[]){
    
    int i ;
    int parameter[10] = {64, 32, 8, 16, 16, 4, 4, 16, 4, 1};
    
    /* I-MEM-size 0, D-MEM-size 1, I-pagesize 2, D-pagesize 3, I-CACHE-size 4
     I-blocksize 5, I-set.associativity 6, D-CACHE-size 7, D-blocksize 8, D-set.associativity 9*/
    
    if (argc == 11) {
        for (i = 0; i<10 ; i++) {
            parameter[i] = atoi(argv[i+1]);
            if( (parameter[i] % 4 != 0 ) && i != 6 && i != 9){
                printf("parameter %d is not a multiple of 4 .\n",i+1);
                exit(-1);
            }
        }
    }
    else if (argc != 1){
        printf("the amount of parameters should be 0 or 10 .\n" );
        exit(-1);
    }
    
    
    for (i = 0; i<10; i++) {
        printf("%d  =  %d\n",i,parameter[i]);
    }
    
    I_MEM_block_num = parameter[0] / parameter[2];
    D_MEM_block_num = parameter[1] / parameter[3];
    I_CACHE_block_num = parameter[4] / parameter[5];
    D_CACHE_block_num = parameter[7] / parameter[8];
    I_PTE_entries = 1024 / parameter[2];
    D_PTE_entries = 1024 / parameter[3];
    I_TLB_entries = I_PTE_entries /4;
    D_TLB_entries = D_PTE_entries /4;
    
    I_associativity = parameter[6];
    D_associativity = parameter[9];
    I_pagesize = parameter[2];
    D_pagesize = parameter[3];
    I_blocksize = parameter[5];
    D_blocksize = parameter[8];
    
    I_CACHE.data = (int*)calloc(parameter[4]/sizeof(int), sizeof(int));
    I_CACHE.LRU_order = (int*)calloc(I_CACHE_block_num, sizeof(int));
    I_CACHE.valid = (int*)calloc(I_CACHE_block_num, sizeof(int));
    I_CACHE.tag = (int*)calloc(I_CACHE_block_num, sizeof(int));
    D_CACHE.data = (int*)calloc(parameter[7]/sizeof(int), sizeof(int));
    D_CACHE.LRU_order = (int*)calloc(D_CACHE_block_num, sizeof(int));
    D_CACHE.valid = (int*)calloc(D_CACHE_block_num, sizeof(int));
    D_CACHE.tag = (int*)calloc(D_CACHE_block_num, sizeof(int));
    
    I_MEM.data = (int*)calloc(parameter[0]/sizeof(int), sizeof(int));
    I_MEM.LRU_order = (int*)calloc(I_MEM_block_num, sizeof(int));
    I_MEM.valid = (int*)calloc(I_MEM_block_num, sizeof(int));
    D_MEM.data = (int*)calloc(parameter[1]/sizeof(int), sizeof(int));
    D_MEM.LRU_order = (int*)calloc(D_MEM_block_num, sizeof(int));
    D_MEM.valid = (int*)calloc(D_MEM_block_num, sizeof(int));
    
    I_PTE.content = (int*)calloc(I_PTE_entries, sizeof(int));
    I_PTE.valid = (int*)calloc(I_PTE_entries, sizeof(int));
    D_PTE.content = (int*)calloc(D_PTE_entries, sizeof(int));
    D_PTE.valid = (int*)calloc(D_PTE_entries, sizeof(int));
    
    I_TLB.content = (int*)calloc(I_TLB_entries, sizeof(int));
    I_TLB.LRU_order = (int*)calloc(I_TLB_entries, sizeof(int));
    I_TLB.tag = (int*)calloc(I_TLB_entries, sizeof(int));
    I_TLB.valid = (int*)calloc(I_TLB_entries, sizeof(int));
    D_TLB.content = (int*)calloc(D_TLB_entries, sizeof(int));
    D_TLB.LRU_order = (int*)calloc(D_TLB_entries, sizeof(int));
    D_TLB.tag = (int*)calloc(D_TLB_entries, sizeof(int));
    D_TLB.valid = (int*)calloc(D_TLB_entries, sizeof(int));
    
    if (I_MEM.data == NULL || I_MEM.valid == NULL || D_MEM.data == NULL || D_MEM.valid == NULL ||
        I_PTE.content == NULL || D_PTE.content == NULL || I_CACHE.data == NULL || I_CACHE.valid == NULL ||
        D_CACHE.data == NULL || D_CACHE.valid == NULL || I_TLB.content == NULL || I_TLB.valid == NULL){
        printf("Cannot allocate dynamic memory\n");
        exit(3);
    }
    
    for (i = 0; i < I_CACHE_block_num; i++) {
        I_CACHE.LRU_order[i] = -1 ;
        I_CACHE.tag[i] = -1;
        I_CACHE.valid[i] = 0;
    }
    for (i = 0; i < D_CACHE_block_num; i++) {
        D_CACHE.LRU_order[i] = -1 ;
        D_CACHE.tag[i] = -1;
        D_CACHE.valid[i] = 0;
    }
    for (i = 0; i < I_MEM_block_num; i++) {
        I_MEM.LRU_order[i] = -1 ;
        I_MEM.valid[i] = 0;
    }
    for (i = 0; i < D_MEM_block_num; i++) {
        D_MEM.LRU_order[i] = -1 ;
        D_MEM.valid[i] = 0;
    }
    for (i = 0; i < I_PTE_entries; i++) {
        I_PTE.content[i] = -1;
        I_PTE.valid[i] = 0;
    }
    for (i = 0; i < D_PTE_entries; i++) {
        D_PTE.content[i] = -1;
        D_PTE.valid[i] = 0;
    }
    for (i = 0; i < I_TLB_entries; i++) {
        I_TLB.LRU_order[i] = -1;
        I_TLB.content[i] = -1;
        I_PTE.valid[i] = 0;
    }
    for (i = 0; i < D_TLB_entries; i++) {
        D_TLB.LRU_order[i] = -1;
        D_TLB.content[i] = -1;
        D_TLB.valid[i] = 0;
    }

}

void I_TLBupdate(int VA){
    int i , j ,k;
    
    for (i = 0; i < I_TLB_entries; i++) {
        if (I_TLB.valid[i] == 1) {
            if (I_TLB.tag[i] == VA / I_pagesize) {
                
                /*TLB hit*/
                ITLB_hits ++ ;
                
                /*LRU update*/
                for (j = 0; j < I_TLB_entries; j++) {
                    if (I_TLB.LRU_order[j] == i) {  //將後面全部往後移一位
                        for (k = j; k < I_TLB_entries - 1; k++) {
                            I_TLB.LRU_order[k] = I_TLB.LRU_order[k+1];
                        }
                        I_TLB.LRU_order[k] = -1 ;   //last one
                        for (k = j; k < I_TLB_entries; k++) {
                            if (I_TLB.LRU_order[k] == -1) {
                                I_TLB.LRU_order[k] = i;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }










}
