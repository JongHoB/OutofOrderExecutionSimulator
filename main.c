#include <stdio.h>
#include <stdlib.h>

#include "main.h"

void commit(){

}

void writeback(){

}

void execute(){

}

void regRead(){

}

void issue(){
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
    for(int i=0;i<NR_INSTRUCTION;i++){
        //Remove the instruction from the IQ
        if(IQ[i].src1_BIT==YES && IQ[i].src2_BIT==YES){
            IQ[i].READY=YES;
            Ready_Table[IQ[i].dest]=YES;
            RR[NR_RR++].inst=IQ[i].inst;
            NR_IQ--;
        }
        else{
            //Wakeup dependent instruction
            IQ[i].READY=NO;
            if(Ready_Table[IQ[i].src1]==YES){
                IQ[i].src1_BIT=YES;
            }
            if(Ready_Table[IQ[i].src2]==YES){
                IQ[i].src2_BIT=YES;
            }
        }
    }
    
    //Sort the IQ
    IQ *temp=(IQ*)malloc(sizeof(IQ)*IQ_SIZE);
    int tempidx=0;
    for(int i=0;i<NR_INSTRUCTION;i++){
        if(IQ[i].READY==NO&&tempidx<NR_IQ){
            temp[tempidx++]=IQ[i];
        }
    }
    IQ=temp;
}

void dispatch(){
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
    for(int i=0;i<NR_DI;i++){
        IQ[NR_IQ].inst=DI[i].inst;
        IQ[NR_IQ].src1=DI[i].inst->phy_src1_register;
        IQ[NR_IQ].src2=DI[i].inst->phy_src2_register;
        
        if(Ready_Table[IQ[NR_IQ].src1]==NO){
            IQ[NR_IQ].src1_BIT=NO;
        }
        else{
            IQ[NR_IQ].src1_BIT=YES;
        }
        
        if(Ready_Table[IQ[NR_IQ].src2]==NO){
            IQ[NR_IQ].src2_BIT=NO;
        }
        else{
            IQ[NR_IQ].src1_BIT=YES;
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

void rename(){
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
    for(int i=0;i<NR_ADVANCE;i++){
        RN[i].inst->phy_src1_register=Rename_Map_Table[RN[i].inst->src1_register];
        RN[i].inst->phy_src2_register=Rename_Map_Table[RN[i].inst->src2_register];
        if(RN[i].inst->dest_register!=-1){
            RN[i].inst->phy_dest_register=Free_List.list[MAX_Free_List-Free_List.count];
            Rename_Map_Table[RN[i].inst->dest_register]=RN[i].inst->phy_dest_register;
            Free_List.count--;
        }
        else{
            RN[i].inst->phy_dest_register=-1;
        }
        DI[i].inst=RN[i].inst;
        NR_DI++;
        NR_RN--;
    }
}

void decode(){
    // If DE contains a decode bundle:
    // If RN is not empty (cannot accept a new rename bundle), then
    // do nothing. If RN is empty (can accept a new rename bundle),
    // then advance the decode bundle from DE to RN.
    if(NR_RN){
        return;
    }
    int NR_ADVANCE=NR_DE;
    for(int i=0;i<NR_ADVANCE;i++){
        RN[i].inst=DE[i].inst;
        NR_DE--;
        NR_RN++;
    }

}

void fetch(){
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
        intstuction *inst=(intstuction*)malloc(sizeof(intstuction));

        //check if the trace file has fewer than WIDTH instructions left.
        if(fscanf(tracefile,"%s %d %d %d %d",PC_STRING,&inst->operation_type,&inst->dest_register,&inst->src1_register,&inst->src2_register)!=EOF){
            inst->PC=strtol(PC_STRING,NULL,16);

            DE[NR_DE].inst=inst;
            NR_DE++;
            
            ROB[NR_ROB].inst=inst;
            ROB[NR_ROB].Done_BIT=NO;
            NR_ROB++;

            INSTRUCTION_COUNT++;

        }
        else{
            IS_EOF=TRUE;
            break;
        }

    }

    

}

void advance_cycle(){

}

void init(){
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

    Free_List.count=0;
    DE=(DE*)malloc(sizeof(DE)*WIDTH);
    RN=(RN*)malloc(sizeof(RN)*WIDTH);
    DI=(DI*)malloc(sizeof(DI)*WIDTH);
    RR=(RR*)malloc(sizeof(RR)*WIDTH);
    execute_list=(execute_list*)malloc(sizeof(execute_list)*WIDTH*5);
    WB=(WB*)malloc(sizeof(WB)*WIDTH*5);
    IQ=(IQ*)malloc(sizeof(IQ)*IQ_SIZE);
    ROB=(ROB*)malloc(sizeof(ROB)*ROB_SIZE);
    for(int i=1;i<=NR_REGS;i++){
        Rename_Map_Table[i]=i;   
    }
    for(int i=1;i<=MAX_PHYSICAL_REGS;i++){
        Ready_Table[i]=YES;
    }
    for(int i=NR_REGS+1;i<=MAX_PHYSICAL_REGS;i++){
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
    // Commit up to WIDTH consecutive “ready” instructions from
    // the head of the ROB. Note that the entry of ROB should be
    // retired in the right order.
    commit();

    // Process the writeback bundle in WB: For each instruction in
    // WB, mark the instruction as “ready” in its entry in the ROB.
    writeback();

    // From the execute_list, check for instructions that are
    // finishing execution this cycle, and:
    // 1) Remove the instruction from the execute_list.
    // 2) Add the instruction to WB.
    execute();

    // If RR contains a register-read bundle:
    // then process (see below) the register-read bundle up to
    // WIDTH instructions and advance it from RR to execute_list.
    //
    // How to process the register-read bundle:
    // Since values are not explicitly modeled, the sole purpose of
    // the Register Read stage is to pass the information of each
    // instruction in the register-read bundle (e.g. the renamed
    // source operands and operation type).
    // Aside from adding the instruction to the execute_list, set a // timer for the instruction in the execute_list that will
    // allow you to model its execution latency.
    regRead();

    issue();

    dispatch();

    rename();

    decode();
    
    fetch();
   
    }while(advance_cycle());
                    // advance_cycle() performs several functions.
                    // (1) It advances the simulator cycle.
                    // (2) When it becomes known that the pipeline is empty AND the
                    // trace is depleted, the function returns “false” to terminate
                    // the loop.



    return 0;

}