#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NR_REGS 67
#define MAX_PHYSICAL_REGS NR_REGS*2

#define YES 1
#define NO 0

#define TRUE 1
#define FALSE 0

//For tracefile
FILE *tracefile;

extern int CURRENT_PC;

extern int WIDTH;
extern int IQ_SIZE;
extern int ROB_SIZE;

extern int CYCLE;
extern int FETCH_BIT;	
extern int INSTRUCTION_COUNT;

extern int IS_EOF;

// Operation Cycle for Operation Types
extern int Operation_Cycle[3]={1,2,5};

//Free list for Register Renaming operation
typedef struct{
    int count;
    int list[MAX_PHYSICAL_REGS+1];
}list;
extern list Free_List;


extern int Rename_Map_Table[NR_REGS+1];//Arch_Reg -> Physical_Reg
extern int Ready_Table[MAX_PHYSICAL_REGS+1];//Physical_Reg -> yes/no

/////////////////////////////////////////////////////////
//instructions
typedef struct{
    int PC;
    int operation_type;
    int dest_register;
    int src1_register;
    int src2_register;
}instruction;

/////////////////////////////////////////////////////////
//Pipeline Registers

//Pipeline register between the Fetch and Decode stages
typedef struct{
    instruction *inst;

}DE;
extern int NR_DE;

//Pipeline register between the Decode and Rename stages
typedef struct{
    instruction *inst;

}RN;
extern int NR_RN;

//Pipeline register between the Rename and Dispatch stages
typedef struct{
    instruction *inst;

}DI;
extern int NR_DI;

//Pipeline register between the Issue and Register Read stages
typedef struct{
    instruction *inst;

}RR;
extern int NR_RR;

//execute_list represents the pipeline register between the
//Register Read and Execute stages, 
//as well as all sub‐pipeline stages within each function unit
typedef struct{
    instruction *inst;

}execute_list;
extern int NR_execute_list;

//Pipeline register between the Execute and Writeback stages
typedef struct{
    instruction *inst;

}WB;
extern int NR_WB;

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
}IQ;
extern int NR_IQ;

//Re-order Buffer - Holds instructions from Fetch through Commit
//Queue guarantees an order of inst until Commit stages
typedef struct{
    instruction * inst;
    int TO_Free_Reg;
    int Done_BIT;
}ROB;
extern int NR_ROB;

extern DE *DE;
extern RN *RN;
extern DI *DI;
extern RR *RR;
extern excute_list *excute_list;
extern WB *WB;
extern IQ *IQ;
extern ROB *ROB;
