
build/original-predraw-scheduler-20260903/CategoryRemove.o:	file format elf32-powerpc

Disassembly of section .data:

80261e78 <_binary_build_original_predraw_scheduler_20260903_CategoryRemove_bin_start>:
80261e78: 1c 05 00 14  	mulli 0, 5, 20
80261e7c: 80 63 00 00  	lwz 3, 0(3)
80261e80: 7c c3 02 14  	add 6, 3, 0
80261e84: 7d 03 00 2e  	lwzx 8, 3, 0
80261e88: 80 e6 00 08  	lwz 7, 8(6)
80261e8c: 7d 05 43 78  	mr	5, 8
80261e90: 54 e0 10 3a  	slwi 0, 7, 2
80261e94: 7c 68 02 14  	add 3, 8, 0
80261e98: 48 00 00 08  	b 0x80261ea0 <_binary_build_original_predraw_scheduler_20260903_CategoryRemove_bin_start+0x28>
80261e9c: 38 a5 00 04  	addi 5, 5, 4
80261ea0: 7c 05 18 40  	cmplw	5, 3
80261ea4: 41 82 00 10  	bt	2, 0x80261eb4 <_binary_build_original_predraw_scheduler_20260903_CategoryRemove_bin_start+0x3c>
80261ea8: 80 05 00 00  	lwz 0, 0(5)
80261eac: 7c 00 20 40  	cmplw	0, 4
80261eb0: 40 82 ff ec  	bf	2, 0x80261e9c <_binary_build_original_predraw_scheduler_20260903_CategoryRemove_bin_start+0x24>
80261eb4: 38 67 ff ff  	addi 3, 7, -1
80261eb8: 7c 08 28 50  	sub	0, 5, 8
80261ebc: 54 63 10 3a  	slwi 3, 3, 2
80261ec0: 7c 00 16 70  	srawi 0, 0, 2
80261ec4: 7c 68 18 2e  	lwzx 3, 8, 3
80261ec8: 7c 00 01 94  	addze 0, 0
80261ecc: 54 00 10 3a  	slwi 0, 0, 2
80261ed0: 7c 68 01 2e  	stwx 3, 8, 0
80261ed4: 80 66 00 08  	lwz 3, 8(6)
80261ed8: 38 03 ff ff  	addi 0, 3, -1
80261edc: 90 06 00 08  	stw 0, 8(6)
80261ee0: 4e 80 00 20  	blr
