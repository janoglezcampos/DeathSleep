[BITS 64]

GLOBAL getRsp
GLOBAL moveRsp

[SECTION .text]

getRsp:
    mov rax, rsp
    add rax, 8
    ret

;;rcx hold size to increase, rdx the epilogue size
moveRsp:
    mov r8, [rsp] ;; Backup addr to come back
    add rsp, 8

    mov r9, rcx

    mov rcx, 0x28 ;; rcx hold value for rep
    add rcx, rdx ;; epilogue + 8 byte ret addr + 32 bytes homing space
    
    add r9, rcx 
    
    ;; rcx holds the number of bytes to move
    ;; r9 holds the total displacement

    ;;backup rsi, rdi
    mov r10, rsi
    mov r11, rdi

    mov rsi, rsp

    ;; rdi holds the source address

    mov rdi, rsi
    sub rdi, r9

    ;; rdi holds the destination address
    ;; repeat operation movsb rcx times
    rep movsb

    ;;restore rsi, rdi
    mov rsi, r10
    mov rdi, r11

    sub rsp, r9
    jmp r8