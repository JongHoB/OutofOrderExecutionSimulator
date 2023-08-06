#include <stdio.h>
#include <stdlib.h>

#include "main.h"

void commit(void){
    // Commit up to WIDTH consecutive “ready” instructions from
    // the head of the ROB. Note that the entry of ROB should be
    // retired in the right order.
    int NR_ADVANCE=NR_ROB;
    if(NR_ADVANCE==0){
        return;
    }
    int MODIFY_BIT=-1;
    for(int i=0;i<WIDTH;i++){
        if(ROB[i].Done_BIT==NO){
            break;
        }
        if(ROB[i].inst->phy_dest_register!=-1){
            Free_List.list[Free_List.count++]=ROB[i].To_Free_Reg;
        }
        ROB[i].inst->cycles[9]=CYCLE+1;//end cycle

        printf("%d fu{%d} src{%d,%d} dst{%d} FE{%d,%d} DE{%d,%d} RN{%d,%d} DI{%d,%d} IS{%d,%d} RR{%d,%d} EX{%d,%d} WB{%d,%d} CM{%d,%d}\n",INSTRUCTION_COUNT++,ROB[i].inst->operation_type,ROB[i].inst->src1_register,ROB[i].inst->src2_register,ROB[i].inst->dest_register,ROB[i].inst->cycles[0],ROB[i].inst->cycles[1]-ROB[i].inst->cycles[0],ROB[i].inst->cycles[1],ROB[i].inst->cycles[2]-ROB[i].inst->cycles[1],ROB[i].inst->cycles[2],ROB[i].inst->cycles[3]-ROB[i].inst->cycles[2],ROB[i].inst->cycles[3],ROB[i].inst->cycles[4]-ROB[i].inst->cycles[3],ROB[i].inst->cycles[4],ROB[i].inst->cycles[5]-ROB[i].inst->cycles[4],ROB[i].inst->cycles[5],ROB[i].inst->cycles[6]-ROB[i].inst->cycles[5],ROB[i].inst->cycles[6],ROB[i].inst->cycles[7]-ROB[i].inst->cycles[6],ROB[i].inst->cycles[7],ROB[i].inst->cycles[8]-ROB[i].inst->cycles[7],ROB[i].inst->cycles[8],ROB[i].inst->cycles[9]-ROB[i].inst->cycles[8]);

        MODIFY_BIT=i;
        NR_ROB--;
    }

    //Print and Sort the ROB
    if(MODIFY_BIT>-1){
        REORDERBUFFER *temp=(REORDERBUFFER*)malloc(sizeof(REORDERBUFFER)*ROB_SIZE);
        int tempidx=0;
        for(int i=MODIFY_BIT+1;i<=MODIFY_BIT+NR_ROB;i++){
            temp[tempidx++]=ROB[i];
        }
        ROB=temp;
    }

}

void writeback(void){
    // Process the writeback bundle in WB: For each instruction in
    // WB, mark the instruction as “ready” in its entry in the ROB.
    int NR_ADVANCE=NR_WB;
    if(NR_ADVANCE==0){
        return;
    }
    for(int i=0;i<NR_ADVANCE;i++){
        for(int j=0;j<NR_ROB;j++){
            if(ROB[j].inst==WB[i].inst){
                ROB[j].Done_BIT=YES;
                WB[i].Done_BIT=YES;
                WB[i].inst->cycles[8]=CYCLE+1;//commit start cycle
                break;
            }
        }
        NR_WB--;
    }

    //sort the WB
    WRITEBACK *temp=(WRITEBACK*)malloc(sizeof(WRITEBACK)*WIDTH*5);
    int tempidx=0;
    for(int i=0;i<NR_ADVANCE;i++){
        if(WB[i].Done_BIT==NO&&tempidx<NR_WB){
            temp[tempidx++]=WB[i];
        }
    }
    WB=temp;
}

void execute(void){
    // From the execute_list, check for instructions that are
    // finishing execution this cycle, and:
    // 1) Remove the instruction from the execute_list.
    // 2) Add the instruction to WB.
    int NR_ADVANCE=NR_execute_list;
    if(NR_ADVANCE==0){
        return;
    }
    for(int i=0;i<NR_ADVANCE&&NR_WB<WIDTH*5;i++){
        if(execute_list[i].cycles>0)
            {execute_list[i].cycles--;}
        if(execute_list[i].cycles==0){
            WB[NR_WB].Done_BIT=NO;
            execute_list[i].inst->cycles[7]=CYCLE+1;//writeback start cycle
            WB[NR_WB++].inst=execute_list[i].inst;
            if(IQ[i].dest!=-1)
            {
                Ready_Table[IQ[i].dest]=YES;
            }
            NR_execute_list--;
        }
    }

    //Sort the execute list
    EXECUTE * temp=(EXECUTE*)malloc(sizeof(EXECUTE)*WIDTH*5);
    int tempidx=0;
    for(int i=0;i<NR_ADVANCE;i++){
        if(execute_list[i].cycles>0&&tempidx<NR_execute_list){
            temp[tempidx++]=execute_list[i];
        }
    }
    execute_list=temp;
}

void regRead(void){
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
    int NR_ADVANCE=NR_RR;
    if(NR_ADVANCE==0){
        return;
    }
    for(int i=0;i<NR_ADVANCE&&NR_execute_list<WIDTH*5;i++) {
        execute_list[NR_execute_list].inst=RR[i].inst;
        execute_list[NR_execute_list].inst->cycles[6]=CYCLE+1;//execute start cycle
        execute_list[NR_execute_list++].cycles=Operation_Cycle[RR[i].inst->operation_type];
        NR_RR--;
    }

}

void issue(void){
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

    int NR_INSTRUCTION=NR_IQ;
    if(NR_INSTRUCTION==0){
        return;
    }
    for(int i=0;i<NR_INSTRUCTION&&NR_RR<WIDTH;i++){
        //Remove the instruction from the IQ
        if(IQ[i].src1_BIT==YES && IQ[i].src2_BIT==YES){
            IQ[i].READY=YES;
            IQ[i].inst->cycles[5]=CYCLE+1;//regRead start cycle
            RR[NR_RR++].inst=IQ[i].inst;
            NR_IQ--;
        }
        else{
            //Wakeup dependent instruction
            IQ[i].READY=NO;
            if(IQ[i].src1==-1||Ready_Table[IQ[i].src1]==YES){
                IQ[i].src1_BIT=YES;
            }
            if(IQ[i].src2==-1||Ready_Table[IQ[i].src2]==YES){
                IQ[i].src2_BIT=YES;
            }
        }
    }
    
    //Sort the IQ
    ISSUEQUEUE *temp=(ISSUEQUEUE*)malloc(sizeof(ISSUEQUEUE)*IQ_SIZE);
    int tempidx=0;
    for(int i=0;i<NR_INSTRUCTION;i++){
        if(IQ[i].READY==NO&&tempidx<NR_IQ){
            temp[tempidx++]=IQ[i];
        }
    }

    IQ=temp;
}

void dispatch(void){
    // If DI contains a dispatch bundle:
    // If the number of free IQ entries is less than the size of
    // the dispatch bundle in DI, then do nothing. If the number of
    // free IQ entries is greater than or equal to the size of the
    // dispatch bundle in DI, then dispatch all instructions from
    // DI to the IQ.
    if(NR_DI>IQ_SIZE-NR_IQ){
        return;
    }
    int NR_ADVANCE=NR_DI;
    if(NR_ADVANCE==0){
        return;
    }
    for(int i=0;i<NR_DI;i++){

        IQ[NR_IQ].inst=DI[i].inst;
        IQ[NR_IQ].inst->cycles[4]=CYCLE+1;//issue start cycle

        IQ[NR_IQ].src1=DI[i].inst->phy_src1_register;
        IQ[NR_IQ].src2=DI[i].inst->phy_src2_register;

        if(IQ[NR_IQ].src1==-1||Ready_Table[IQ[NR_IQ].src1]==YES){
            IQ[NR_IQ].src1_BIT=YES;
        }
        else{
            IQ[NR_IQ].src1_BIT=NO;
        }

        if(IQ[NR_IQ].src2==-1||Ready_Table[IQ[NR_IQ].src2]==YES){
            IQ[NR_IQ].src2_BIT=YES;
        }
        else{
            IQ[NR_IQ].src2_BIT=NO;
        }
        
        if(DI[i].inst->phy_dest_register!=-1){
            IQ[NR_IQ].dest=DI[i].inst->phy_dest_register;
            Ready_Table[IQ[NR_IQ].dest]=NO;
        }
        else{
            IQ[NR_IQ].dest=-1;           
        }
     
        IQ[NR_IQ].birthday=NR_IQ;

        NR_IQ++;
        NR_DI--;
    }


}

void rename(void){
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
    if(NR_DI){
        return;
    }
    int NR_ADVANCE=NR_RN;
    if(NR_RN==0){
        return;
    }
    for(int i=0;i<NR_ADVANCE;i++){
        if(RN[i].inst->src1_register!=-1){
            RN[i].inst->phy_src1_register=Rename_Map_Table[RN[i].inst->src1_register];
        }
        else{
            RN[i].inst->phy_src1_register=-1;
        }
        if(RN[i].inst->src2_register!=-1)
        {
            RN[i].inst->phy_src2_register=Rename_Map_Table[RN[i].inst->src2_register];
        }
        else{
            RN[i].inst->phy_src2_register=-1;
        }
            
        if(RN[i].inst->dest_register!=-1){
            for(int j=0;j<NR_ROB;j++){
                if(ROB[j].inst==RN[i].inst){
                    ROB[j].To_Free_Reg=Rename_Map_Table[RN[i].inst->dest_register];
                }
            }

            RN[i].inst->phy_dest_register=Free_List.list[0];
            for(int i=1;i<Free_List.count;i++){
                Free_List.list[i-1]=Free_List.list[i];
            }
            Free_List.count--;

            Rename_Map_Table[RN[i].inst->dest_register]=RN[i].inst->phy_dest_register;            
        }
        else{
            RN[i].inst->phy_dest_register=-1;
        }

        DI[i].inst=RN[i].inst;
        DI[i].inst->cycles[3]=CYCLE;//dispatch start cycle
        
        NR_DI++;
        NR_RN--;
    }
}

void decode(void){
    // If DE contains a decode bundle:
    // If RN is not empty (cannot accept a new rename bundle), then
    // do nothing. If RN is empty (can accept a new rename bundle),
    // then advance the decode bundle from DE to RN.
    if(NR_RN){
        return;
    }
    int NR_ADVANCE=NR_DE;
    if(NR_ADVANCE==0){
        return;
    }
    for(int i=0;i<NR_ADVANCE;i++){
        RN[i].inst=DE[i].inst;
        RN[i].inst->cycles[2]=CYCLE+1;//rename start cycle
        NR_DE--;
        NR_RN++;
    }

}

void fetch(void){
    // Do nothing if
    // (1) there are no more instructions in the trace file or
    // (2) DE is not empty (cannot accept a new decode bundle)
    if(NR_DE>=WIDTH){
        return;
    }
    if(EOF){
        return;
    }

    // If there are more instructions in the trace file and if DE
    // is empty (can accept a new decode bundle), then fetch up to
    // WIDTH instructions from the trace file into DE. Fewer than
    // WIDTH instructions will be fetched and allocated in the ROB
    // only if
    // (1) the trace file has fewer than WIDTH instructions left.
    // (2) the ROB has fewer spaces than WIDTH.
    for(int i=0;i<WIDTH&&NR_DE<WIDTH&&NR_ROB<ROB_SIZE;i++){
        char PC_STRING[10];
        instruction *inst=(instruction*)malloc(sizeof(instruction));

        //check if the trace file has fewer than WIDTH instructions left.
        if(fscanf(tracefile,"%s %d %d %d %d",PC_STRING,&inst->operation_type,&inst->dest_register,&inst->src1_register,&inst->src2_register)!=EOF){
            inst->PC=strtol(PC_STRING,NULL,16);

            inst->cycles[0]=CYCLE;//fetch start cycle
            inst->cycles[1]=CYCLE+1;//decode start cycle

            DE[NR_DE].inst=inst;
            NR_DE++;
            
            ROB[NR_ROB].inst=inst;
            ROB[NR_ROB].Done_BIT=NO;
            NR_ROB++;

        }
        else{
            IS_EOF=TRUE;
            break;
        }

    }

    

}

int advance_cycle(void){
    // advance_cycle() performs several functions.
    // (1) It advances the simulator cycle.
    // (2) When it becomes known that the pipeline is empty AND the
    // trace is depleted, the function returns “false” to terminate
    // the loop.
    CYCLE++;
    if(IS_EOF==TRUE&&NR_ROB==0){
        return 0;
    }
    return 1;
}

void init(void){
    CYCLE=0;
    CURRENT_PC=0;
    FETCH_BIT=TRUE;
    INSTRUCTION_COUNT=0;
    IS_EOF=FALSE;

    NR_DE=0;
    NR_RN=0;
    NR_RN=0;
    NR_DI=0;
    NR_RR=0;
    NR_execute_list=0;
    NR_WB=0;
    NR_IQ=0;
    NR_ROB=0;

    Operation_Cycle[0]=1;
    Operation_Cycle[1]=2;
    Operation_Cycle[2]=5;

    Free_List.count=0;
    DE=(DECODE*)malloc(sizeof(DECODE)*WIDTH);
    RN=(RENAME*)malloc(sizeof(RENAME)*WIDTH);
    DI=(DISPATCH*)malloc(sizeof(DISPATCH)*WIDTH);
    RR=(REGISTERREAD*)malloc(sizeof(REGISTERREAD)*WIDTH);
    execute_list=(EXECUTE*)malloc(sizeof(EXECUTE)*WIDTH*5);
    WB=(WRITEBACK*)malloc(sizeof(WRITEBACK)*WIDTH*5);
    IQ=(ISSUEQUEUE*)malloc(sizeof(ISSUEQUEUE)*IQ_SIZE);
    ROB=(REORDERBUFFER*)malloc(sizeof(REORDERBUFFER)*ROB_SIZE);
    for(int i=0;i<NR_REGS;i++){
        Rename_Map_Table[i]=i;   
    }
    for(int i=0;i<MAX_PHYSICAL_REGS;i++){
        Ready_Table[i]=YES;
    }
    for(int i=NR_REGS;i<MAX_PHYSICAL_REGS;i++){
        Free_List.count++;
        Free_List.list[i-NR_REGS]=i;
    }
}


int main(int argc, char **argv){

    if(argc!=5){
        printf("Usage: ./cse561sim <ROB_SIZE> <IQ_SIZE> <WIDTH> <tracefile>\n");
        exit(1);
    }
    ROB_SIZE=atoi(argv[1]);
    IQ_SIZE=atoi(argv[2]);
    WIDTH=atoi(argv[3]);

    // Open the trace file
    tracefile=fopen(argv[4],"r");
    if(tracefile==NULL){
        printf("Invalid tracefile name: %s\n",argv[4]);
        exit(1);
    }

    // Initialize the simulator
    init();

    do{
    commit();

    writeback();

    execute();

    regRead();

    issue();

    dispatch();

    rename();

    decode();
    
    fetch();
   
    }while(advance_cycle());

    printf("# === Simulator Command =========\n");
    printf("# ./cse561sim %d %d %d %s\n", ROB_SIZE, IQ_SIZE, WIDTH, argv[4]);
    printf("# === Processor Configuration ===\n");
    printf("# ROB_SIZE = %d\n", ROB_SIZE);
    printf("# IQ_SIZE = %d\n", IQ_SIZE);
    printf("# WIDTH = %d\n", WIDTH);
    printf("# === Simulation Results ========\n");
    printf("# Dynamic Instruction Count = %d\n", INSTRUCTION_COUNT);
    printf("# Cycles = %d\n", CYCLE);
    printf("# Instructions Per Cycle (IPC) = %.2f\n", (double)INSTRUCTION_COUNT / (double)CYCLE);
           
    return 0;

}
