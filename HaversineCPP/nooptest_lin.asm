global NOOP1ASM
global NOOP2ASM
global NOOP4ASM
global NOOP8ASM
global NOOP16ASM
global READ_8x1
global READ_8x2
global READ_8x3
global READ_8x4
global READ_16x2
global READ_32x2
global READ_32x4
global TEST_CACHE
section .text

NOOP1ASM:
    xor rax, rax
.loop:
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    inc rax
    cmp rax, rdi
    jb .loop
    ret

NOOP2ASM:
    xor rax, rax
.loop:
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    inc rax
    cmp rax, rdi
    jb .loop
    ret

NOOP4ASM:
    xor rax, rax
.loop:
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    inc rax
    cmp rax, rdi
    jb .loop
    ret

NOOP8ASM:
    xor rax, rax
.loop:
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    inc rax
    cmp rax, rdi
    jb .loop
    ret

NOOP16ASM:
    xor rax, rax
.loop:
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    db 0x0f, 0x1f, 0x00 ; NOTE(casey): This is the byte sequence for a 3-byte NOP
    inc rax
    cmp rax, rdi
    jb .loop
    ret

    READ_8x1:
        xor rax, rax
        align 64
    .loop:
        mov r8, [rsi]
        add rax, 8
        cmp rax, rdi
        jb .loop
        ret

    READ_8x2:
        xor rax, rax
        align 64
    .loop:
        mov r8, [rsi]
        mov r8, [rsi + 8]
        add rax, 16
        cmp rax, rdi
        jb .loop
        ret

    READ_8x3:
        xor rax, rax
        align 64
    .loop:
        ret

    READ_8x4:
        xor rax, rax
        align 64
    .loop:
        ret

        

READ_16x2:
    xor rax, rax
    align 64
.loop:
    vmovdqu xmm0, [rsi]
    vmovdqu xmm1, [rsi + 16]
    add rax, 32
    cmp rax, rdi
    jb .loop
    ret

READ_32x2:
    xor rax, rax
    align 64
.loop:
    vmovdqu ymm0, [rsi]
    vmovdqu ymm1, [rsi + 32]
    add rax, 64
    cmp rax, rdi
    jb .loop
    ret

READ_32x4:
    xor rax, rax
    align 64
.loop:
    vmovdqu ymm0, [rsi]
    vmovdqu ymm1, [rsi + 32]
    vmovdqu ymm2, [rsi + 64]
    vmovdqu ymm3, [rsi + 96]
    add rax, 128
    cmp rax, rdi
    jb .loop
    ret

;rdi: count
;rsi: data
;rdx: mask
TEST_CACHE:
    xor rax, rax
    xor rcx, rcx
    add rcx, rsi
    align 64
.loop:
    vmovdqu ymm0, [rcx]
    vmovdqu ymm1, [rcx + 32]
    vmovdqu ymm2, [rcx + 64]
    vmovdqu ymm3, [rcx + 96]
    add rax, 128
    mov rcx, rax
    and rcx, rdx
    add rcx, rsi
    cmp rax, rdi
    jb .loop
    ret