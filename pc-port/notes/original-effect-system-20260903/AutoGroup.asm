
build/original-effect-system-20260903/AutoGroup.o:	file format elf32-powerpc

Disassembly of section .data:

800c46e8 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start>:
800c46e8: 94 21 ff f0  	stwu 1, -16(1)
800c46ec: 7c 08 02 a6  	mflr 0
800c46f0: 90 01 00 14  	stw 0, 20(1)
800c46f4: 38 00 00 00  	li 0, 0
800c46f8: 93 e1 00 0c  	stw 31, 12(1)
800c46fc: 7c bf 2b 78  	mr	31, 5
800c4700: 93 c1 00 08  	stw 30, 8(1)
800c4704: 7c 7e 1b 78  	mr	30, 3
800c4708: 90 83 00 00  	stw 4, 0(3)
800c470c: 90 03 00 04  	stw 0, 4(3)
800c4710: 90 03 00 08  	stw 0, 8(3)
800c4714: 90 03 00 0c  	stw 0, 12(3)
800c4718: 54 a3 10 3a  	slwi 3, 5, 2
800c471c: 48 34 6d 61  	bl 0x8040b47c <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x3469e4>
800c4720: 90 7e 00 04  	stw 3, 4(30)
800c4724: 7f c3 f3 78  	mr	3, 30
800c4728: 93 fe 00 08  	stw 31, 8(30)
800c472c: 83 e1 00 0c  	lwz 31, 12(1)
800c4730: 83 c1 00 08  	lwz 30, 8(1)
800c4734: 80 01 00 14  	lwz 0, 20(1)
800c4738: 7c 08 03 a6  	mtlr 0
800c473c: 38 21 00 10  	addi 1, 1, 16
800c4740: 4e 80 00 20  	blr
800c4744: 94 21 ff e0  	stwu 1, -32(1)
800c4748: 7c 08 02 a6  	mflr 0
800c474c: 90 01 00 24  	stw 0, 36(1)
800c4750: 39 61 00 20  	addi 11, 1, 32
800c4754: 48 45 42 b5  	bl 0x80518a08 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453f70>
800c4758: 7c 7d 1b 78  	mr	29, 3
800c475c: 7c 9e 23 78  	mr	30, 4
800c4760: 38 60 00 50  	li 3, 80
800c4764: 48 34 6c f5  	bl 0x8040b458 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x3469c0>
800c4768: 2c 03 00 00  	cmpwi	3, 0
800c476c: 7c 7f 1b 78  	mr	31, 3
800c4770: 41 82 00 0c  	bt	2, 0x800c477c <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x94>
800c4774: 48 00 07 19  	bl 0x800c4e8c <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x3f4>
800c4778: 7c 7f 1b 78  	mr	31, 3
800c477c: 7f e3 fb 78  	mr	3, 31
800c4780: 7f c4 f3 78  	mr	4, 30
800c4784: 48 00 07 65  	bl 0x800c4ee8 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x450>
800c4788: 80 bd 00 0c  	lwz 5, 12(29)
800c478c: 39 61 00 20  	addi 11, 1, 32
800c4790: 80 7d 00 04  	lwz 3, 4(29)
800c4794: 38 85 00 01  	addi 4, 5, 1
800c4798: 54 a0 10 3a  	slwi 0, 5, 2
800c479c: 90 9d 00 0c  	stw 4, 12(29)
800c47a0: 7f e3 01 2e  	stwx 31, 3, 0
800c47a4: 48 45 42 b1  	bl 0x80518a54 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453fbc>
800c47a8: 80 01 00 24  	lwz 0, 36(1)
800c47ac: 7c 08 03 a6  	mtlr 0
800c47b0: 38 21 00 20  	addi 1, 1, 32
800c47b4: 4e 80 00 20  	blr
800c47b8: 94 21 ff e0  	stwu 1, -32(1)
800c47bc: 7c 08 02 a6  	mflr 0
800c47c0: 90 01 00 24  	stw 0, 36(1)
800c47c4: 39 61 00 20  	addi 11, 1, 32
800c47c8: 48 45 42 39  	bl 0x80518a00 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453f68>
800c47cc: 7c 7b 1b 78  	mr	27, 3
800c47d0: 7c 9c 23 78  	mr	28, 4
800c47d4: 7c bd 2b 78  	mr	29, 5
800c47d8: 3b c0 00 00  	li 30, 0
800c47dc: 3b e0 00 00  	li 31, 0
800c47e0: 48 00 00 20  	b 0x800c4800 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x118>
800c47e4: 80 bb 00 04  	lwz 5, 4(27)
800c47e8: 7f 83 e3 78  	mr	3, 28
800c47ec: 7f a4 eb 78  	mr	4, 29
800c47f0: 7c a5 f8 2e  	lwzx 5, 5, 31
800c47f4: 48 00 17 fd  	bl 0x800c5ff0 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x1558>
800c47f8: 3b de 00 01  	addi 30, 30, 1
800c47fc: 3b ff 00 04  	addi 31, 31, 4
800c4800: 80 1b 00 0c  	lwz 0, 12(27)
800c4804: 7c 1e 00 00  	cmpw	30, 0
800c4808: 41 80 ff dc  	bt	0, 0x800c47e4 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0xfc>
800c480c: 39 61 00 20  	addi 11, 1, 32
800c4810: 48 45 42 3d  	bl 0x80518a4c <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453fb4>
800c4814: 80 01 00 24  	lwz 0, 36(1)
800c4818: 7c 08 03 a6  	mtlr 0
800c481c: 38 21 00 20  	addi 1, 1, 32
800c4820: 4e 80 00 20  	blr
800c4824: 94 21 ff e0  	stwu 1, -32(1)
800c4828: 7c 08 02 a6  	mflr 0
800c482c: 90 01 00 24  	stw 0, 36(1)
800c4830: 39 61 00 20  	addi 11, 1, 32
800c4834: 48 45 41 cd  	bl 0x80518a00 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453f68>
800c4838: 7c 7b 1b 78  	mr	27, 3
800c483c: 7c 9c 23 78  	mr	28, 4
800c4840: 7c bd 2b 78  	mr	29, 5
800c4844: 3b c0 00 00  	li 30, 0
800c4848: 3b e0 00 00  	li 31, 0
800c484c: 48 00 00 20  	b 0x800c486c <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x184>
800c4850: 80 bb 00 04  	lwz 5, 4(27)
800c4854: 7f 83 e3 78  	mr	3, 28
800c4858: 7f a4 eb 78  	mr	4, 29
800c485c: 7c a5 f8 2e  	lwzx 5, 5, 31
800c4860: 48 00 18 81  	bl 0x800c60e0 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x1648>
800c4864: 3b de 00 01  	addi 30, 30, 1
800c4868: 3b ff 00 04  	addi 31, 31, 4
800c486c: 80 1b 00 0c  	lwz 0, 12(27)
800c4870: 7c 1e 00 00  	cmpw	30, 0
800c4874: 41 80 ff dc  	bt	0, 0x800c4850 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x168>
800c4878: 39 61 00 20  	addi 11, 1, 32
800c487c: 48 45 41 d1  	bl 0x80518a4c <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453fb4>
800c4880: 80 01 00 24  	lwz 0, 36(1)
800c4884: 7c 08 03 a6  	mtlr 0
800c4888: 38 21 00 20  	addi 1, 1, 32
800c488c: 4e 80 00 20  	blr
800c4890: 94 21 ff e0  	stwu 1, -32(1)
800c4894: 7c 08 02 a6  	mflr 0
800c4898: 90 01 00 24  	stw 0, 36(1)
800c489c: 39 61 00 20  	addi 11, 1, 32
800c48a0: 48 45 41 61  	bl 0x80518a00 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453f68>
800c48a4: 7c 7b 1b 78  	mr	27, 3
800c48a8: 7c 9c 23 78  	mr	28, 4
800c48ac: 7c bd 2b 78  	mr	29, 5
800c48b0: 3b c0 00 00  	li 30, 0
800c48b4: 3b e0 00 00  	li 31, 0
800c48b8: 48 00 00 20  	b 0x800c48d8 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x1f0>
800c48bc: 80 bb 00 04  	lwz 5, 4(27)
800c48c0: 7f 83 e3 78  	mr	3, 28
800c48c4: 7f a4 eb 78  	mr	4, 29
800c48c8: 7c a5 f8 2e  	lwzx 5, 5, 31
800c48cc: 48 00 18 6d  	bl 0x800c6138 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x16a0>
800c48d0: 3b de 00 01  	addi 30, 30, 1
800c48d4: 3b ff 00 04  	addi 31, 31, 4
800c48d8: 80 1b 00 0c  	lwz 0, 12(27)
800c48dc: 7c 1e 00 00  	cmpw	30, 0
800c48e0: 41 80 ff dc  	bt	0, 0x800c48bc <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x1d4>
800c48e4: 39 61 00 20  	addi 11, 1, 32
800c48e8: 48 45 41 65  	bl 0x80518a4c <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453fb4>
800c48ec: 80 01 00 24  	lwz 0, 36(1)
800c48f0: 7c 08 03 a6  	mtlr 0
800c48f4: 38 21 00 20  	addi 1, 1, 32
800c48f8: 4e 80 00 20  	blr
800c48fc: 94 21 ff c0  	stwu 1, -64(1)
800c4900: 7c 08 02 a6  	mflr 0
800c4904: 90 01 00 44  	stw 0, 68(1)
800c4908: 39 61 00 40  	addi 11, 1, 64
800c490c: 48 45 40 fd  	bl 0x80518a08 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453f70>
800c4910: 7c 7f 1b 78  	mr	31, 3
800c4914: 48 00 12 4d  	bl 0x800c5b60 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x10c8>
800c4918: 3c 80 80 58  	lis 4, -32680
800c491c: 7f e5 fb 78  	mr	5, 31
800c4920: 38 84 82 90  	addi 4, 4, -32112
800c4924: 38 c0 00 00  	li 6, 0
800c4928: 48 34 1a d1  	bl 0x804063f8 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x341960>
800c492c: 2c 03 00 00  	cmpwi	3, 0
800c4930: 90 81 00 24  	stw 4, 36(1)
800c4934: 38 c0 00 00  	li 6, 0
800c4938: 38 00 00 00  	li 0, 0
800c493c: 90 61 00 20  	stw 3, 32(1)
800c4940: 90 61 00 28  	stw 3, 40(1)
800c4944: 90 81 00 2c  	stw 4, 44(1)
800c4948: 41 82 00 10  	bt	2, 0x800c4958 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x270>
800c494c: 2c 04 00 00  	cmpwi	4, 0
800c4950: 41 80 00 08  	bt	0, 0x800c4958 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x270>
800c4954: 38 00 00 01  	li 0, 1
800c4958: 2c 00 00 00  	cmpwi	0, 0
800c495c: 41 82 00 34  	bt	2, 0x800c4990 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x2a8>
800c4960: 80 81 00 28  	lwz 4, 40(1)
800c4964: 80 a1 00 2c  	lwz 5, 44(1)
800c4968: 80 04 00 00  	lwz 0, 0(4)
800c496c: 2c 00 00 00  	cmpwi	0, 0
800c4970: 41 82 00 10  	bt	2, 0x800c4980 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x298>
800c4974: 80 63 00 00  	lwz 3, 0(3)
800c4978: 80 03 00 00  	lwz 0, 0(3)
800c497c: 48 00 00 08  	b 0x800c4984 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x29c>
800c4980: 38 00 00 00  	li 0, 0
800c4984: 7c 05 00 00  	cmpw	5, 0
800c4988: 40 80 00 08  	bf	0, 0x800c4990 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x2a8>
800c498c: 38 c0 00 01  	li 6, 1
800c4990: 2c 06 00 00  	cmpwi	6, 0
800c4994: 40 82 00 0c  	bf	2, 0x800c49a0 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x2b8>
800c4998: 38 60 00 00  	li 3, 0
800c499c: 48 00 00 e4  	b 0x800c4a80 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x398>
800c49a0: 38 60 00 10  	li 3, 16
800c49a4: 48 34 6a b5  	bl 0x8040b458 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x3469c0>
800c49a8: 2c 03 00 00  	cmpwi	3, 0
800c49ac: 7c 7d 1b 78  	mr	29, 3
800c49b0: 41 82 00 20  	bt	2, 0x800c49d0 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x2e8>
800c49b4: 7f e3 fb 78  	mr	3, 31
800c49b8: 48 00 11 75  	bl 0x800c5b2c <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x1094>
800c49bc: 7c 65 1b 78  	mr	5, 3
800c49c0: 7f a3 eb 78  	mr	3, 29
800c49c4: 7f e4 fb 78  	mr	4, 31
800c49c8: 4b ff fd 21  	bl 0x800c46e8 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start>
800c49cc: 7c 7d 1b 78  	mr	29, 3
800c49d0: 3f c0 80 58  	lis 30, -32680
800c49d4: 48 00 00 38  	b 0x800c4a0c <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x324>
800c49d8: 7f a3 eb 78  	mr	3, 29
800c49dc: 38 81 00 28  	addi 4, 1, 40
800c49e0: 4b ff fd 65  	bl 0x800c4744 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x5c>
800c49e4: 48 00 11 7d  	bl 0x800c5b60 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x10c8>
800c49e8: 80 c1 00 2c  	lwz 6, 44(1)
800c49ec: 7f e5 fb 78  	mr	5, 31
800c49f0: 38 9e 82 90  	addi 4, 30, -32112
800c49f4: 38 c6 00 01  	addi 6, 6, 1
800c49f8: 48 34 1a 01  	bl 0x804063f8 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x341960>
800c49fc: 90 81 00 1c  	stw 4, 28(1)
800c4a00: 90 61 00 18  	stw 3, 24(1)
800c4a04: 90 61 00 28  	stw 3, 40(1)
800c4a08: 90 81 00 2c  	stw 4, 44(1)
800c4a0c: 48 00 11 55  	bl 0x800c5b60 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x10c8>
800c4a10: 80 83 00 00  	lwz 4, 0(3)
800c4a14: 2c 04 00 00  	cmpwi	4, 0
800c4a18: 40 82 00 0c  	bf	2, 0x800c4a24 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x33c>
800c4a1c: 38 80 00 00  	li 4, 0
800c4a20: 48 00 00 08  	b 0x800c4a28 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x340>
800c4a24: 80 84 00 00  	lwz 4, 0(4)
800c4a28: 80 01 00 2c  	lwz 0, 44(1)
800c4a2c: 38 a0 00 00  	li 5, 0
800c4a30: 90 61 00 08  	stw 3, 8(1)
800c4a34: 7c 00 20 00  	cmpw	0, 4
800c4a38: 90 81 00 0c  	stw 4, 12(1)
800c4a3c: 90 61 00 10  	stw 3, 16(1)
800c4a40: 90 81 00 14  	stw 4, 20(1)
800c4a44: 40 82 00 30  	bf	2, 0x800c4a74 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x38c>
800c4a48: 80 01 00 28  	lwz 0, 40(1)
800c4a4c: 2c 00 00 00  	cmpwi	0, 0
800c4a50: 41 82 00 24  	bt	2, 0x800c4a74 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x38c>
800c4a54: 2c 03 00 00  	cmpwi	3, 0
800c4a58: 41 82 00 1c  	bt	2, 0x800c4a74 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x38c>
800c4a5c: 80 81 00 28  	lwz 4, 40(1)
800c4a60: 80 03 00 00  	lwz 0, 0(3)
800c4a64: 80 64 00 00  	lwz 3, 0(4)
800c4a68: 7c 03 00 40  	cmplw	3, 0
800c4a6c: 40 82 00 08  	bf	2, 0x800c4a74 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x38c>
800c4a70: 38 a0 00 01  	li 5, 1
800c4a74: 2c 05 00 00  	cmpwi	5, 0
800c4a78: 41 82 ff 60  	bt	2, 0x800c49d8 <_binary_build_original_effect_system_20260903_AutoGroup_bin_start+0x2f0>
800c4a7c: 7f a3 eb 78  	mr	3, 29
800c4a80: 39 61 00 40  	addi 11, 1, 64
800c4a84: 48 45 3f d1  	bl 0x80518a54 <_binary_build_original_effect_system_20260903_AutoGroup_bin_end+0x453fbc>
800c4a88: 80 01 00 44  	lwz 0, 68(1)
800c4a8c: 7c 08 03 a6  	mtlr 0
800c4a90: 38 21 00 40  	addi 1, 1, 64
800c4a94: 4e 80 00 20  	blr
