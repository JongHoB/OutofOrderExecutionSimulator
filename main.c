#include <stdio.h>
#include <stdlib.h>



int main(int argc, char **argv){


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
    issue();

    // If DI contains a dispatch bundle:
    // If the number of free IQ entries is less than the size of
    // the dispatch bundle in DI, then do nothing. If the number of
    // free IQ entries is greater than or equal to the size of the
    // dispatch bundle in DI, then dispatch all instructions from
    // DI to the IQ.
    dispatch();

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
    rename();

    // If DE contains a decode bundle:
    // If RN is not empty (cannot accept a new rename bundle), then
    // do nothing. If RN is empty (can accept a new rename bundle),
    // then advance the decode bundle from DE to RN.
    decode();
    
    // Do nothing if
    // (1) there are no more instructions in the trace file or
    // (2) DE is not empty (cannot accept a new decode bundle)
    //
    // If there are more instructions in the trace file and if DE
    // is empty (can accept a new decode bundle), then fetch up to
    // WIDTH instructions from the trace file into DE. Fewer than
    // WIDTH instructions will be fetched and allocated in the ROB
    // only if
    // (1) the trace file has fewer than WIDTH instructions left.
    // (2) the ROB has fewer spaces than WIDTH.
    fetch();
   
    }while(advance_cycle());
                    // advance_cycle() performs several functions.
                    // (1) It advances the simulator cycle.
                    // (2) When it becomes known that the pipeline is empty AND the
                    // trace is depleted, the function returns “false” to terminate
                    // the loop.



    return 0;

}