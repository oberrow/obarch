unresolved symbol

start:
	mov R1, 0
	mov R2, 1
	mov R3, 0x0a
	mov R4, 0x00
	
	mov SP, 0xFF

	out R4
	outc R3
	inc R4
	out R4
	outc R3
	mov R4, 0
	mov R3, 0

start.loop:
	add R1, R2
	mov R3, R1

	mov R1, R2

	mov R2, R3

	out R3

	push R4
	mov R4, 0x0A
	outc R4
	pop R4

	inc R4
	cmp R4, 20
	jcc LESS start.loop

	hltclk