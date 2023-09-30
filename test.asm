unresolved print
unresolved _start.end

_start:
	mov SP, 0xFFFF

	push 0x00
	push 0x0A
	push 0x21
	push 0x69
	push 0x48

	mov R1, SP
	call print
	
	jmp _start.end

_start.end:
	hltclk

print:
	push R1
	push R2
	push R3
	push R4

	mov R2, 0

	mov R3, SP
	mov SP, R1

print.loop:
	pop R1
	outc R1

	inc R4

	cmp R1, R2
	jcc NOT_EQUAL, print.loop
print.finished:

	mov SP, R3

	pop R4
	pop R3
	pop R2
	pop R1
	ret