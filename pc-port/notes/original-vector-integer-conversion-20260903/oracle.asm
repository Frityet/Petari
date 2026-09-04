
/Users/frityet/Projects/petari/build/original-vector-integer-conversion-20260903/oracle.o:	file format elf32-powerpc

Disassembly of section .text:

00000000 <convertShort__FPsfff>:
       0: fc 80 08 1e  	fctiwz 4, 1
       4: 94 21 ff e0  	stwu 1, -32(1)
       8: fc 20 10 1e  	fctiwz 1, 2
       c: fc 00 18 1e  	fctiwz 0, 3
      10: d8 81 00 08  	stfd 4, 8(1)
      14: d8 21 00 10  	stfd 1, 16(1)
      18: 80 01 00 0c  	lwz 0, 12(1)
      1c: d8 01 00 18  	stfd 0, 24(1)
      20: 80 81 00 14  	lwz 4, 20(1)
      24: 80 a1 00 1c  	lwz 5, 28(1)
      28: b0 03 00 00  	sth 0, 0(3)
      2c: b0 83 00 02  	sth 4, 2(3)
      30: b0 a3 00 04  	sth 5, 4(3)
      34: 38 21 00 20  	addi 1, 1, 32
      38: 4e 80 00 20  	blr

0000003c <convertUnsigned__FPUsff>:
      3c: fc 20 08 1e  	fctiwz 1, 1
      40: 94 21 ff e0  	stwu 1, -32(1)
      44: fc 00 10 1e  	fctiwz 0, 2
      48: d8 21 00 08  	stfd 1, 8(1)
      4c: d8 01 00 10  	stfd 0, 16(1)
      50: 80 01 00 0c  	lwz 0, 12(1)
      54: 80 81 00 14  	lwz 4, 20(1)
      58: b0 03 00 00  	sth 0, 0(3)
      5c: b0 83 00 02  	sth 4, 2(3)
      60: 38 21 00 20  	addi 1, 1, 32
      64: 4e 80 00 20  	blr

00000068 <convertWord__FPlfff>:
      68: fc 80 08 1e  	fctiwz 4, 1
      6c: 94 21 ff e0  	stwu 1, -32(1)
      70: fc 20 10 1e  	fctiwz 1, 2
      74: fc 00 18 1e  	fctiwz 0, 3
      78: d8 81 00 08  	stfd 4, 8(1)
      7c: d8 21 00 10  	stfd 1, 16(1)
      80: 80 01 00 0c  	lwz 0, 12(1)
      84: d8 01 00 18  	stfd 0, 24(1)
      88: 80 81 00 14  	lwz 4, 20(1)
      8c: 80 a1 00 1c  	lwz 5, 28(1)
      90: 90 03 00 00  	stw 0, 0(3)
      94: 90 83 00 04  	stw 4, 4(3)
      98: 90 a3 00 08  	stw 5, 8(3)
      9c: 38 21 00 20  	addi 1, 1, 32
      a0: 4e 80 00 20  	blr
