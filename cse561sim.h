#ifndef _CSE561SIM_H
#define _CSE561SIM_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NR_REGS 67
#define MAX_PHYSICAL_REGS NR_REGS*2

#define YES 1
#define NO 0

#define TRUE 1
#define FALSE 0

//Free list for Register Renaming operation
typedef struct{
    int count;
    int list[MAX_PHYSICAL_REGS+1];
}list;

/////////////////////////////////////////////////////////
//instructions
typedef struct{
    int PC;
    int operation_type;
    int dest_register;
    int src1_register;
    int src2_register;
    int phy_dest_register;
    int phy_src1_register;
    int phy_src2_register;
    int cycles[10];//cycle time for each stage
}instruction;

/////////////////////////////////////////////////////////
//Pipeline Registers

//Pipeline register between the Fetch and Decode stages
typedef struct{
    instruction *inst;

}DECODE;


//Pipeline register between the Decode and Rename stages
typedef struct{
    instruction *inst;

}RENAME;


//Pipeline register between the Rename and Dispatch stages
typedef struct{
    instruction *inst;

}DISPATCH;


//Pipeline register between the Issue and Register Read stages
typedef struct{
    instruction *inst;

}REGISTERREAD;


//execute_list represents the pipeline register between the
//Register Read and Execute stages, 
//as well as all sub‐pipeline stages within each function unit
typedef struct{
    instruction *inst;
    int cycles;
}EXECUTE;


//Pipeline register between the Execute and Writeback stages
typedef struct{
    instruction *inst;
    int Done_BIT;
}WRITEBACK;


/////////////////////////////////////////////////////////
//Queues

//Issue Queue(Reservation Station) - Holds instructions from Dispatch through Issue/Tracks ready inputs
//Instruction queue between the Dispatch and Issue stages
typedef struct{
    instruction *inst;
    int src1;
    int src1_BIT;
    int src2;
    int src2_BIT;
    int dest;
    int birthday;
    int READY;
}ISSUEQUEUE;


//Re-order Buffer - Holds instructions from Fetch through Commit
//Queue guarantees an order of inst until Commit stages
typedef struct{
    instruction * inst;
    int To_Free_Reg;
    int Done_BIT;
}REORDERBUFFER;


void commit(void);
void writeback(void);
void execute(void);
void regRead(void);
void issue(void);
void dispatch(void);
void re_name(void);
void decode(void);
void fetch(void);
int advance_cycle(void);
void init(void);

#endif