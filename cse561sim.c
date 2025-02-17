#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cse561sim.h"

DECODE *DE;
RENAME *RN;
DISPATCH *DI;
REGISTERREAD *RR;
EXECUTE *execute_list;
WRITEBACK *WB;
ISSUEQUEUE *IQ;
REORDERBUFFER *ROB;

list Free_List;

int NR_DE;
int NR_RN;
int NR_DI;
int NR_RR;
int NR_execute_list;
int NR_WB;
int NR_IQ;
int NR_ROB;

int birthday = 0;

// For tracefile
FILE *tracefile;

int WIDTH;
int IQ_SIZE;
int ROB_SIZE;

int CYCLE;
int INSTRUCTION_COUNT;

int IS_EOF;

// Operation Cycle for Operation Types
int Operation_Cycle[3];

int Rename_Map_Table[NR_REGS];      // Arch_Reg -> Physical_Reg
int Ready_Table[MAX_PHYSICAL_REGS]; // Physical_Reg -> yes/no

void commit(void)
{
    // Commit up to WIDTH consecutive “ready” instructions from
    // the head of the ROB. Note that the entry of ROB should be
    // retired in the right order.
    for (int i = 0; i < WIDTH && NR_ROB > 0; i++)
    {
        if (ROB[0].Done_BIT == NO)
        {
            break;
        }
        if (ROB[0].inst->phy_dest_register != -1)
        {
            Free_List.list[Free_List.count++] = ROB[0].To_Free_Reg;
        }
        ROB[0].inst->cycles[9] = CYCLE + 1; // end cycle

        printf("%d fu{%d} src{%d,%d} dst{%d} FE{%d,%d} DE{%d,%d} RN{%d,%d} DI{%d,%d} IS{%d,%d} RR{%d,%d} EX{%d,%d} WB{%d,%d} CM{%d,%d}\n", INSTRUCTION_COUNT++, ROB[0].inst->operation_type, ROB[0].inst->src1_register, ROB[0].inst->src2_register, ROB[0].inst->dest_register, ROB[0].inst->cycles[0], ROB[0].inst->cycles[1] - ROB[0].inst->cycles[0], ROB[0].inst->cycles[1], ROB[0].inst->cycles[2] - ROB[0].inst->cycles[1], ROB[0].inst->cycles[2], ROB[0].inst->cycles[3] - ROB[0].inst->cycles[2], ROB[0].inst->cycles[3], ROB[0].inst->cycles[4] - ROB[0].inst->cycles[3], ROB[0].inst->cycles[4], ROB[0].inst->cycles[5] - ROB[0].inst->cycles[4], ROB[0].inst->cycles[5], ROB[0].inst->cycles[6] - ROB[0].inst->cycles[5], ROB[0].inst->cycles[6], ROB[0].inst->cycles[7] - ROB[0].inst->cycles[6], ROB[0].inst->cycles[7], ROB[0].inst->cycles[8] - ROB[0].inst->cycles[7], ROB[0].inst->cycles[8], ROB[0].inst->cycles[9] - ROB[0].inst->cycles[8]);

        for (int j = 1; j < NR_ROB; j++)
        {
            ROB[j-1]=ROB[j];
            // memcpy(&ROB[j - 1], &ROB[j], sizeof(REORDERBUFFER));
            ROB[j - 1].inst->rob_idx = j - 1;
        }
        NR_ROB--;
    }
}

void writeback(void)
{
    // Process the writeback bundle in WB: For each instruction in
    // WB, mark the instruction as “ready” in its entry in the ROB.
    int NR_ADVANCE = NR_WB;

    for (int i = 0; i < NR_ADVANCE && NR_WB > 0; i++)
    {
        ROB[WB[0].inst->rob_idx].Done_BIT = YES;
        WB[0].inst->cycles[8] = CYCLE + 1; // commit start cycle

        for (int k = 1; k < NR_WB; k++)
        {
            WB[k - 1]= WB[k];
        }
        NR_WB--;
    }
}

void execute(void)
{
    // From the execute_list, check for instructions that are
    // finishing execution this cycle, and:
    // 1) Remove the instruction from the execute_list.
    // 2) Add the instruction to WB.
    int NR_ADVANCE = NR_execute_list;

    for (int i = 0; i < NR_execute_list && NR_execute_list > 0 && NR_WB < WIDTH * 5; i++)
    {
        if (execute_list[i].cycles > 0)
        {
            execute_list[i].cycles--;
        }
        if (execute_list[i].cycles == 0)
        {
            execute_list[i].inst->cycles[7] = CYCLE + 1; // writeback start cycle
            WB[NR_WB++].inst = execute_list[i].inst;
            if (execute_list[i].inst-> phy_dest_register!= -1)
            {
                Ready_Table[execute_list[i].inst-> phy_dest_register] = YES;
            }

            for (int j = i + 1; j < NR_execute_list; j++)
            {
                // memcpy(&execute_list[j - 1], &execute_list[j], sizeof(EXECUTE));
                execute_list[j - 1]= execute_list[j];
            }
            NR_execute_list--;
            i--;
        }
    }
}

void regRead(void)
{
    // If RR contains a register-read bundle:
    // then process (see below) the register-read bundle up to
    // WIDTH instructions and advance it from RR to execute_list.
    //
    // How to process the register-read bundle:
    // Since values are not explicitly modeled, the sole purpose of
    // the Register Read stage is to pass the information of each
    // instruction in the register-read bundle (e.g. the renamed
    // source operands and operation type).
    // Aside from adding the instruction to the execute_list, set a
    // timer for the instruction in the execute_list that will
    // allow you to model its execution latency.
    int NR_ADVANCE = NR_RR;

    for (int i = 0; i < NR_ADVANCE && NR_RR > 0 && NR_execute_list < WIDTH * 5; i++)
    {
        execute_list[NR_execute_list].inst = RR[0].inst;
        execute_list[NR_execute_list].inst->cycles[6] = CYCLE + 1; // execute start cycle
        execute_list[NR_execute_list++].cycles = Operation_Cycle[RR[0].inst->operation_type];

        for (int j = 1; j < NR_RR; j++)
        {
            RR[j - 1].inst = RR[j].inst;
            RR[j - 1] = RR[j];
        }
        NR_RR--;
    }
}

void issue(void)
{
    // Issue up to WIDTH oldest instructions from the IQ. (One
    // approach to implement oldest-first issuing is to make
    // multiple passes through the IQ, each time finding the next
    // oldest ready instruction and then issuing it. One way to
    // annotate the age of an instruction is to assign an
    // incrementing sequence number to each instruction as it is
    // fetched from the trace file.)
    //
    // To issue an instruction:
    // 1) Remove the instruction from the IQ.
    // 2) Wakeup dependent instructions (set their source operand
    // ready flags) in the IQ, so that in the next cycle IQ should
    // properly handle the dependent instructions.
    int NR_INSTRUCTION = NR_IQ;

    for (int i = 0; i < NR_IQ && NR_RR < WIDTH && NR_IQ > 0; i++)
    {
        if (IQ[i].src1 != -1 && Ready_Table[IQ[i].src1] == YES)
        {
            IQ[i].src1_BIT = YES;
        }

        if (IQ[i].src2 != -1 && Ready_Table[IQ[i].src2] == YES)
        {
            IQ[i].src2_BIT = YES;
        }

        // Remove the instruction from the IQ
        if (IQ[i].src1_BIT == YES && IQ[i].src2_BIT == YES)
        {
            IQ[i].inst->cycles[5] = CYCLE + 1; // regRead start cycle
            RR[NR_RR++].inst = IQ[i].inst;

            for (int j = i + 1; j < NR_IQ; j++)
            {
                // memcpy(&IQ[j - 1], &IQ[j], sizeof(ISSUEQUEUE));
                IQ[j - 1]=IQ[j];
            }
            NR_IQ--;
            i--;
        }
    }
}

void dispatch(void)
{
    // If DI contains a dispatch bundle:
    // If the number of free IQ entries is less than the size of
    // the dispatch bundle in DI, then do nothing. If the number of
    // free IQ entries is greater than or equal to the size of the
    // dispatch bundle in DI, then dispatch all instructions from
    // DI to the IQ.
    if (NR_DI > IQ_SIZE - NR_IQ)
    {
        return;
    }
    int NR_ADVANCE = NR_DI;
    for (int i = 0; i < NR_ADVANCE && NR_IQ < IQ_SIZE && NR_DI > 0; i++)
    {
        IQ[NR_IQ].inst = DI[0].inst;
        IQ[NR_IQ].inst->cycles[4] = CYCLE + 1; // issue start cycle

        IQ[NR_IQ].src1 = DI[0].inst->phy_src1_register;
        IQ[NR_IQ].src2 = DI[0].inst->phy_src2_register;

        IQ[NR_IQ].src1_BIT = YES;
        IQ[NR_IQ].src2_BIT = YES;

        if (IQ[NR_IQ].src1 > -1 && Ready_Table[IQ[NR_IQ].src1] == NO)
        {
            IQ[NR_IQ].src1_BIT = NO;
        }

        if (IQ[NR_IQ].src2 > -1 && Ready_Table[IQ[NR_IQ].src2] == NO)
        {
            IQ[NR_IQ].src2_BIT = NO;
        }

        IQ[NR_IQ].dest = DI[0].inst->phy_dest_register;

        IQ[NR_IQ++].birthday = birthday++;

        for (int j = 1; j < NR_DI; j++)
        {
            DI[j - 1] = DI[j];
        }
        NR_DI--;
    }
}

void re_name(void)
{
    // If RN contains a rename bundle:
    // If either DI is not empty (cannot accept a new register-read
    // bundle) then do nothing. If DI is empty (can accept a new
    // dispatch bundle), then process (see below) the rename
    // bundle and advance it from RN to DI.
    //
    // How to process the rename bundle:
    // Apply your learning from the class lectures/notes on the
    // steps for renaming:
    // (1) Rename its source registers, and
    // (2) Rename its destination register (if it has one). If you
    // are not sure how to implement the register renaming, apply
    // the algorithm that you’ve learned from lectures and notes.
    // Note that the rename bundle must be renamed in program
    // order. Fortunately, the instructions in the rename bundle
    // are in program order).

    int NR_ADVANCE = NR_RN;
    for (int i = 0; i < NR_ADVANCE && NR_DI < WIDTH && NR_RN > 0 && Free_List.count > 0; i++)
    {
        RN[0].inst->phy_src1_register = RN[0].inst->src1_register != -1 ? Rename_Map_Table[RN[0].inst->src1_register] : -1;
        RN[0].inst->phy_src2_register = RN[0].inst->src2_register != -1 ? Rename_Map_Table[RN[0].inst->src2_register] : -1;

        if (RN[0].inst->dest_register != -1)
        {
            ROB[RN[0].inst->rob_idx].To_Free_Reg = Rename_Map_Table[RN[0].inst->dest_register];

            RN[0].inst->phy_dest_register = Free_List.list[0];
            Ready_Table[RN[0].inst->phy_dest_register] = NO;
            for (int j = 1; j < Free_List.count; j++)
            {
                Free_List.list[j - 1] = Free_List.list[j];
            }
            Free_List.count--;

            Rename_Map_Table[RN[0].inst->dest_register] = RN[0].inst->phy_dest_register;
        }
        else
        {
            RN[0].inst->phy_dest_register = -1;
        }

        DI[NR_DI].inst = RN[0].inst;
        DI[NR_DI++].inst->cycles[3] = CYCLE + 1; // dispatch start cycle

        for (int j = 1; j < NR_RN; j++)
        {
            RN[j - 1].inst = RN[j].inst;
            RN[j - 1]= RN[j];
        }
        NR_RN--;
    }
}

void decode(void)
{
    // If DE contains a decode bundle:
    // If RN is not empty (cannot accept a new rename bundle), then
    // do nothing. If RN is empty (can accept a new rename bundle),
    // then advance the decode bundle from DE to RN.

    int NR_ADVANCE = NR_DE;
    for (int i = 0; i < NR_ADVANCE && NR_RN < WIDTH && NR_DE > 0; i++)
    {
        RN[NR_RN].inst = DE[0].inst;
        RN[NR_RN++].inst->cycles[2] = CYCLE + 1; // rename start cycle

        for (int j = 1; j < NR_DE; j++)
        {
            //DE[j - 1].inst = DE[j].inst;
            DE[j - 1] = DE[j];
        }
        NR_DE--;
    }
}

void fet_ch(void)
{
    // Do nothing if
    // (1) there are no more instructions in the trace file or
    // (2) DE is not empty (cannot accept a new decode bundle)

    if (IS_EOF)
    {
        return;
    }

    // If there are more instructions in the trace file and if DE
    // is empty (can accept a new decode bundle), then fetch up to
    // WIDTH instructions from the trace file into DE. Fewer than
    // WIDTH instructions will be fetched and allocated in the ROB
    // only if
    // (1) the trace file has fewer than WIDTH instructions left.
    // (2) the ROB has fewer spaces than WIDTH.
    for (int i = 0; i < WIDTH && NR_DE < WIDTH && NR_ROB < ROB_SIZE; i++)
    {
        char PC_STRING[10];
        instruction *inst = (instruction *)malloc(sizeof(instruction));

        // check if the trace file has fewer than WIDTH instructions left.
        if (fscanf(tracefile, "%s %d %d %d %d", PC_STRING, &inst->operation_type, &inst->dest_register, &inst->src1_register, &inst->src2_register) != EOF)
        {
            inst->PC = strtol(PC_STRING, NULL, 16);

            inst->cycles[0] = CYCLE;     // fetch start cycle
            inst->cycles[1] = CYCLE + 1; // decode start cycle

            DE[NR_DE++].inst = inst;

            ROB[NR_ROB].inst = inst;
            inst->rob_idx = NR_ROB;
            ROB[NR_ROB++].Done_BIT = NO;
        }
        else
        {
            IS_EOF = TRUE;
            break;
        }
    }
}

int advance_cycle(void)
{
    // advance_cycle() performs several functions.
    // (1) It advances the simulator cycle.
    // (2) When it becomes known that the pipeline is empty AND the
    // trace is depleted, the function returns “false” to terminate
    // the loop. 
    if (IS_EOF == TRUE && NR_ROB == 0)
    {
        return 0;
    }
    CYCLE++;
    return 1;
}

void init(void)
{
    CYCLE = 0;
    INSTRUCTION_COUNT = 0;
    IS_EOF = FALSE;

    NR_DE = 0;
    NR_RN = 0;
    NR_RN = 0;
    NR_DI = 0;
    NR_RR = 0;
    NR_execute_list = 0;
    NR_WB = 0;
    NR_IQ = 0;
    NR_ROB = 0;

    Free_List.count = 0;

    Operation_Cycle[0] = 1;
    Operation_Cycle[1] = 2;
    Operation_Cycle[2] = 5;

    Free_List.count = 0;
    DE = (DECODE *)malloc(sizeof(DECODE) * WIDTH);
    RN = (RENAME *)malloc(sizeof(RENAME) * WIDTH);
    DI = (DISPATCH *)malloc(sizeof(DISPATCH) * WIDTH);
    RR = (REGISTERREAD *)malloc(sizeof(REGISTERREAD) * WIDTH);
    execute_list = (EXECUTE *)malloc(sizeof(EXECUTE) * WIDTH * 5);
    WB = (WRITEBACK *)malloc(sizeof(WRITEBACK) * WIDTH * 5);
    IQ = (ISSUEQUEUE *)malloc(sizeof(ISSUEQUEUE) * IQ_SIZE);
    ROB = (REORDERBUFFER *)malloc(sizeof(REORDERBUFFER) * ROB_SIZE);
    for (int i = 0; i < NR_REGS; i++)
    {
        Rename_Map_Table[i] = i;
    }
    for (int i = 0; i < MAX_PHYSICAL_REGS; i++)
    {
        Ready_Table[i] = YES;
    }
    for (int i = NR_REGS; i < MAX_PHYSICAL_REGS; i++)
    {
        Free_List.list[Free_List.count++] = i;
    }
}

int main(int argc, char **argv)
{

    if (argc != 5)
    {
        printf("Usage: ./cse561sim <ROB_SIZE> <IQ_SIZE> <WIDTH> <tracefile>\n");
        exit(1);
    }
    ROB_SIZE = atoi(argv[1]);
    IQ_SIZE = atoi(argv[2]);
    WIDTH = atoi(argv[3]);

    // Open the trace file
    tracefile = fopen(argv[4], "r");
    if (tracefile == NULL)
    {
        printf("Invalid tracefile name: %s\n", argv[4]);
        exit(1);
    }

    // Initialize the simulator
    init();

    do
    {
        commit();

        writeback();

        execute();

        regRead();

        issue();

        dispatch();

        re_name();

        decode();

        fet_ch();
    } while (advance_cycle());

    printf("# === Simulator Command =========\n");
    printf("# ./cse561sim %d %d %d %s\n", ROB_SIZE, IQ_SIZE, WIDTH, argv[4]);
    printf("# === Processor Configuration ===\n");
    printf("# ROB_SIZE = %d\n", ROB_SIZE);
    printf("# IQ_SIZE  = %d\n", IQ_SIZE);
    printf("# WIDTH    = %d\n", WIDTH);
    printf("# === Simulation Results ========\n");
    printf("# Dynamic Instruction Count = %d\n", INSTRUCTION_COUNT);
    printf("# Cycles                    = %d\n", CYCLE);
    printf("# Instructions Per Cycle    = %.2f\n", (double)INSTRUCTION_COUNT / (double)CYCLE);

    return 0;
}
