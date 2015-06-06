addi $4 $0 4
addi $8 $0 12
beq $8 $1 4
lw $2 0($1)
sw $2 12($1)
add $1 $1 $4
j 2
add $8 $8 $8
add $1 $0 $0
beq $8 $1 4
lw $3 0($1)
add $1 $1 $4
j 9
halt
halt
halt
halt
halt
