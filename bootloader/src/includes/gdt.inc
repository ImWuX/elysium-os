;
; GDT
;
gdt:
.null:
    dd 0
    dd 0
.ring0_code:
    dw 0x0000       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base medium
    db 10011010b    ; Access Byte
    db 10100000b    ; Flags + Limit high
    db 0x00         ; Base high
.ring0_data:
    dw 0x0000       ; Limit low
    dw 0x0000       ; Base low
    db 0x00         ; Base medium
    db 10010010b    ; Access Byte
    db 10100000b    ; Flags + Limit high
    db 0x00         ; Base high
.descriptor:
    dw .descriptor - .null - 1
    dq .null

CODE_SEGMENT equ gdt.ring0_code - gdt.null
DATA_SEGMENT equ gdt.ring0_data - gdt.null