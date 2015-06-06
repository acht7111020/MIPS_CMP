      lw   $2, 0x8($0)  # $2 = desired
      addi $3, $0, 1    # $3 = 1
      slt  $4, $3, $2   # $4 = ( 1 < desired )
      bne  $4, $0, else
      or   $8, $0, $2   # $8 = answer = desired
      j    end
else: lw   $6, 0x10($0) # $6 = p0
      lw   $7, 0x14($0) # $7 = p1
      addi $5, $0, 2    # $5 = i = 2
loop: slt  $4, $2, $5   # $4 = ( desired < i )
      bne  $4, $0, end
      add  $8, $6, $7   # answer = $8 = p0 + p1
      or   $6, $0, $7   # p0 = p1
      or   $7, $0, $8   # p1 = answer
      addi $5, $5, 1    # i++
      j    loop
 end: sw   $8, 0xC($0)  # store answer
      lh   $9, 0xC($0)
      halt
      halt
      halt
      halt
      halt
