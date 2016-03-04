# MIPS_CMP
Use C language to simulate MIPS processor, contains the design of cache, memory, page table, and TLB.

For pipeline stage : IF -> ID -> EX -> DM ->WB.


stage explain:

IF : Instructionfetch , get PC from i_memory.

ID : InstructionDecode , according to R,I,J type , decode its instruction 
	information about that type , and also evaluate conditional/
	unconditional branch .
	
EX : ALUexecution , do all arithmetic/bitwise shift/logical operations , and 
	also computer the address to be accessed in D memory.
	
DM : DatamemoryAccess , Load/store access data memory .

WB : WriteBack , executions targeting to $0~$31 , are done during the first half of tke cycle.




It contain three types MIPS instruction.

  R-type:
  
	"add", "sub", "and", "or", "xor", "nor", "nand", "slt",
	"sll", "srl", "sra", "jr" .
	
  I-type:
  
	"addi", "lw", "lh", "lhu", "lb", "lbu", "sw", "sh",
	"sb", "lui", "andi", "ori", "nori", "slti", "beq", "bne" .
	
  J-type:
  
	"j", "jal" .
	
  Specialized Instruction: 
  
	"halt" .




PS : 
	
1. You can use "make" instruction, that can compiler out a .exe file.
	
2. You can use "make clean" instruction to clean *.exe , *.o , *.out , *.bin , *.rpt.
	Which can make your file looked clear.
	
