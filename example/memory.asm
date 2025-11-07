.NVM0

; save vars
push byte 10
store 0        ; var0 = 10

push byte 20
store 1        ; var1 = 20

; load and addition
load 0         ; var0
load 1         ; var1
add            ; 10 + 20 = 30

; store result
store 2        ; var2 = 30

syscall exit
hlt