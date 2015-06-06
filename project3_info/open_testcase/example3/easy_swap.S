lw $1 0($0)
addi $4 $0 4
addi $2 $0 1
add $3 $4 $4
add $7 $4 $0
beq $1 $0 9
add $8 $7 $4
lw $9 0($7)
lw $10 0($8)
sw $10 0($7)
sw $9  0($8)
add $8 $8 $3
add $7 $7 $3
sub $1 $1 $2
j 5
lw $1 0($0)
add $5 $0 $4
add $1 $1 $1
beq $1 $0 4
lw $11 0($5)
add $5 $5 $4
sub $1 $1 $2
j 18 
halt
halt
halt
halt
halt
