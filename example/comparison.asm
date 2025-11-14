.NVM0
; Test comparison operations
push 10
push 20

cmp     ; Compare 10 and 20 (should be -1 since 10 < 20)

syscall exit
hlt