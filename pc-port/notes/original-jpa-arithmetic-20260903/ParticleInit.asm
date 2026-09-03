
build/original-jpa-arithmetic-20260903/ParticleInit.o:	file format elf32-powerpc

Disassembly of section .data:

8044a90c <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start>:
8044a90c: 94 21 ff 40  	stwu 1, -192(1)
8044a910: 7c 08 02 a6  	mflr 0
8044a914: 90 01 00 c4  	stw 0, 196(1)
8044a918: db e1 00 b0  	stfd 31, 176(1)
8044a91c: f3 e1 00 b8  	xxsel 31, 1, 0, 34
8044a920: db c1 00 a0  	stfd 30, 160(1)
8044a924: f3 c1 00 a8  	<unknown>
8044a928: 39 61 00 a0  	addi 11, 1, 160
8044a92c: 48 0c e0 cd  	bl 0x805189f8 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xcdb68>
8044a930: 80 a4 00 04  	lwz 5, 4(4)
8044a934: 38 00 ff ff  	li 0, -1
8044a938: 83 e4 00 00  	lwz 31, 0(4)
8044a93c: 7c 7a 1b 78  	mr	26, 3
8044a940: 83 c5 00 20  	lwz 30, 32(5)
8044a944: 7c 9b 23 78  	mr	27, 4
8044a948: 83 a5 00 1c  	lwz 29, 28(5)
8044a94c: 83 85 00 2c  	lwz 28, 44(5)
8044a950: b0 03 00 80  	sth 0, 128(3)
8044a954: 38 7f 00 c4  	addi 3, 31, 196
8044a958: 4b ff e2 2d  	bl 0x80448b84 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdcf4>
8044a95c: 80 dc 00 00  	lwz 6, 0(28)
8044a960: 3c 60 43 30  	lis 3, 17200
8044a964: a8 9f 00 52  	lha 4, 82(31)
8044a968: 3c a0 80 56  	lis 5, -32682
8044a96c: c0 06 00 54  	lfs 0, 84(6)
8044a970: 38 00 00 00  	li 0, 0
8044a974: 6c 84 80 00  	xoris 4, 4, 32768
8044a978: 90 61 00 68  	stw 3, 104(1)
8044a97c: ec 20 00 72  	fmuls 1, 0, 1
8044a980: c0 02 20 18  	lfs 0, 8216(2)
8044a984: 90 81 00 6c  	stw 4, 108(1)
8044a988: 38 7b 00 d8  	addi 3, 27, 216
8044a98c: c8 65 c3 e8  	lfd 3, -15384(5)
8044a990: 38 9b 00 10  	addi 4, 27, 16
8044a994: c8 41 00 68  	lfd 2, 104(1)
8044a998: ec 20 08 28  	fsubs 1, 0, 1
8044a99c: c0 02 20 1c  	lfs 0, 8220(2)
8044a9a0: 38 ba 00 0c  	addi 5, 26, 12
8044a9a4: ec 42 18 28  	fsubs 2, 2, 3
8044a9a8: 90 1a 00 7c  	stw 0, 124(26)
8044a9ac: d0 1a 00 84  	stfs 0, 132(26)
8044a9b0: ec 02 00 72  	fmuls 0, 2, 1
8044a9b4: fc 00 00 1e  	fctiwz 0, 0
8044a9b8: d8 01 00 70  	stfd 0, 112(1)
8044a9bc: 80 01 00 74  	lwz 0, 116(1)
8044a9c0: b0 1a 00 82  	sth 0, 130(26)
8044a9c4: 48 06 e3 51  	bl 0x804b8d14 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0x6de84>
8044a9c8: 7f e3 fb 78  	mr	3, 31
8044a9cc: 38 80 00 08  	li 4, 8
8044a9d0: 4b ff e1 f5  	bl 0x80448bc4 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdd34>
8044a9d4: 2c 03 00 00  	cmpwi	3, 0
8044a9d8: 41 82 00 10  	bt	2, 0x8044a9e8 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0xdc>
8044a9dc: 80 1a 00 7c  	lwz 0, 124(26)
8044a9e0: 60 00 00 20  	ori 0, 0, 32
8044a9e4: 90 1a 00 7c  	stw 0, 124(26)
8044a9e8: 38 7a 00 18  	addi 3, 26, 24
8044a9ec: 38 9b 01 38  	addi 4, 27, 312
8044a9f0: 4b bd 28 d9  	bl 0x8001d2c8 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbd2438>
8044a9f4: c0 7a 00 0c  	lfs 3, 12(26)
8044a9f8: 7f 43 d3 78  	mr	3, 26
8044a9fc: c0 5b 01 2c  	lfs 2, 300(27)
8044aa00: c0 3a 00 10  	lfs 1, 16(26)
8044aa04: ec 83 00 b2  	fmuls 4, 3, 2
8044aa08: c0 1b 01 30  	lfs 0, 304(27)
8044aa0c: c0 7a 00 18  	lfs 3, 24(26)
8044aa10: ec a1 00 32  	fmuls 5, 1, 0
8044aa14: c0 5a 00 14  	lfs 2, 20(26)
8044aa18: ec 23 20 2a  	fadds 1, 3, 4
8044aa1c: c0 1b 01 34  	lfs 0, 308(27)
8044aa20: c0 9a 00 1c  	lfs 4, 28(26)
8044aa24: ec 62 00 32  	fmuls 3, 2, 0
8044aa28: c0 1a 00 20  	lfs 0, 32(26)
8044aa2c: ec 44 28 2a  	fadds 2, 4, 5
8044aa30: ec 60 18 2a  	fadds 3, 0, 3
8044aa34: 4b bc c8 b1  	bl 0x800172e4 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcc454>
8044aa38: c0 3f 00 34  	lfs 1, 52(31)
8044aa3c: c0 02 20 1c  	lfs 0, 8220(2)
8044aa40: fc 01 00 00  	fcmpu 0, 1, 0
8044aa44: 41 82 00 14  	bt	2, 0x8044aa58 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x14c>
8044aa48: 38 61 00 2c  	addi 3, 1, 44
8044aa4c: 38 9b 00 1c  	addi 4, 27, 28
8044aa50: 48 00 0e 05  	bl 0x8044b854 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0x9c4>
8044aa54: 48 00 00 0c  	b 0x8044aa60 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x154>
8044aa58: 38 61 00 2c  	addi 3, 1, 44
8044aa5c: 4b bc e3 a1  	bl 0x80018dfc <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcdf6c>
8044aa60: c0 3f 00 38  	lfs 1, 56(31)
8044aa64: c0 02 20 1c  	lfs 0, 8220(2)
8044aa68: fc 01 00 00  	fcmpu 0, 1, 0
8044aa6c: 41 82 00 14  	bt	2, 0x8044aa80 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x174>
8044aa70: 38 61 00 20  	addi 3, 1, 32
8044aa74: 38 9b 00 28  	addi 4, 27, 40
8044aa78: 48 00 0d dd  	bl 0x8044b854 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0x9c4>
8044aa7c: 48 00 00 0c  	b 0x8044aa88 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x17c>
8044aa80: 38 61 00 20  	addi 3, 1, 32
8044aa84: 4b bc e3 79  	bl 0x80018dfc <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcdf6c>
8044aa88: c0 3f 00 3c  	lfs 1, 60(31)
8044aa8c: c0 02 20 1c  	lfs 0, 8220(2)
8044aa90: fc 01 00 00  	fcmpu 0, 1, 0
8044aa94: 41 82 00 90  	bt	2, 0x8044ab24 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x218>
8044aa98: 3c 60 00 19  	lis 3, 25
8044aa9c: 80 9f 00 c4  	lwz 4, 196(31)
8044aaa0: 38 03 66 0d  	addi 0, 3, 26125
8044aaa4: 7c 84 01 d6  	mullw 4, 4, 0
8044aaa8: 7f e3 fb 78  	mr	3, 31
8044aaac: 3c 84 3c 6f  	addis 4, 4, 15471
8044aab0: 3b 24 f3 5f  	addi 25, 4, -3233
8044aab4: 93 3f 00 c4  	stw 25, 196(31)
8044aab8: 4b ff e1 4d  	bl 0x80448c04 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdd74>
8044aabc: c0 42 20 20  	lfs 2, 8224(2)
8044aac0: 57 20 84 3e  	srwi 0, 25, 16
8044aac4: c0 1f 00 40  	lfs 0, 64(31)
8044aac8: 7c 04 07 34  	extsh 4, 0
8044aacc: ec 22 00 72  	fmuls 1, 2, 1
8044aad0: 38 a1 00 38  	addi 5, 1, 56
8044aad4: ec 00 00 72  	fmuls 0, 0, 1
8044aad8: fc 00 00 1e  	fctiwz 0, 0
8044aadc: d8 01 00 70  	stfd 0, 112(1)
8044aae0: 80 01 00 74  	lwz 0, 116(1)
8044aae4: 7c 03 07 34  	extsh 3, 0
8044aae8: 48 00 0f 41  	bl 0x8044ba28 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xb98>
8044aaec: 38 81 00 38  	addi 4, 1, 56
8044aaf0: 38 7b 00 48  	addi 3, 27, 72
8044aaf4: 7c 85 23 78  	mr	5, 4
8044aaf8: 48 06 d8 c9  	bl 0x804b83c0 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0x6d530>
8044aafc: c0 7f 00 3c  	lfs 3, 60(31)
8044ab00: 38 61 00 14  	addi 3, 1, 20
8044ab04: c0 21 00 40  	lfs 1, 64(1)
8044ab08: c0 41 00 50  	lfs 2, 80(1)
8044ab0c: c0 01 00 60  	lfs 0, 96(1)
8044ab10: ec 23 00 72  	fmuls 1, 3, 1
8044ab14: ec 43 00 b2  	fmuls 2, 3, 2
8044ab18: ec 63 00 32  	fmuls 3, 3, 0
8044ab1c: 4b bc c7 c9  	bl 0x800172e4 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcc454>
8044ab20: 48 00 00 0c  	b 0x8044ab2c <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x220>
8044ab24: 38 61 00 14  	addi 3, 1, 20
8044ab28: 4b bc e2 d5  	bl 0x80018dfc <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcdf6c>
8044ab2c: c0 3f 00 44  	lfs 1, 68(31)
8044ab30: c0 02 20 1c  	lfs 0, 8220(2)
8044ab34: fc 01 00 00  	fcmpu 0, 1, 0
8044ab38: 41 82 00 50  	bt	2, 0x8044ab88 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x27c>
8044ab3c: 38 7f 00 c4  	addi 3, 31, 196
8044ab40: 4b ff e0 45  	bl 0x80448b84 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdcf4>
8044ab44: c0 02 20 24  	lfs 0, 8228(2)
8044ab48: 38 7f 00 c4  	addi 3, 31, 196
8044ab4c: ef e1 00 28  	fsubs 31, 1, 0
8044ab50: 4b ff e0 35  	bl 0x80448b84 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdcf4>
8044ab54: c0 02 20 24  	lfs 0, 8228(2)
8044ab58: 38 7f 00 c4  	addi 3, 31, 196
8044ab5c: ef c1 00 28  	fsubs 30, 1, 0
8044ab60: 4b ff e0 25  	bl 0x80448b84 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdcf4>
8044ab64: c0 42 20 24  	lfs 2, 8228(2)
8044ab68: 38 61 00 08  	addi 3, 1, 8
8044ab6c: c0 1f 00 44  	lfs 0, 68(31)
8044ab70: ec 21 10 28  	fsubs 1, 1, 2
8044ab74: ec 40 07 b2  	fmuls 2, 0, 30
8044ab78: ec 60 07 f2  	fmuls 3, 0, 31
8044ab7c: ec 20 00 72  	fmuls 1, 0, 1
8044ab80: 4b bc c7 65  	bl 0x800172e4 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcc454>
8044ab84: 48 00 00 0c  	b 0x8044ab90 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x284>
8044ab88: 38 61 00 08  	addi 3, 1, 8
8044ab8c: 4b bc e2 71  	bl 0x80018dfc <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcdf6c>
8044ab90: 80 9c 00 00  	lwz 4, 0(28)
8044ab94: 7f e3 fb 78  	mr	3, 31
8044ab98: c3 c4 00 48  	lfs 30, 72(4)
8044ab9c: 4b ff e0 69  	bl 0x80448c04 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdd74>
8044aba0: ec e1 07 b2  	fmuls 7, 1, 30
8044aba4: c0 61 00 2c  	lfs 3, 44(1)
8044aba8: c0 21 00 20  	lfs 1, 32(1)
8044abac: 38 7a 00 30  	addi 3, 26, 48
8044abb0: c0 41 00 30  	lfs 2, 48(1)
8044abb4: ec a3 08 2a  	fadds 5, 3, 1
8044abb8: c0 01 00 24  	lfs 0, 36(1)
8044abbc: c0 61 00 14  	lfs 3, 20(1)
8044abc0: ec 82 00 2a  	fadds 4, 2, 0
8044abc4: c0 21 00 34  	lfs 1, 52(1)
8044abc8: c0 01 00 28  	lfs 0, 40(1)
8044abcc: ec a3 28 2a  	fadds 5, 3, 5
8044abd0: c0 61 00 18  	lfs 3, 24(1)
8044abd4: ec 41 00 2a  	fadds 2, 1, 0
8044abd8: c0 21 00 1c  	lfs 1, 28(1)
8044abdc: ec 83 20 2a  	fadds 4, 3, 4
8044abe0: c0 01 00 08  	lfs 0, 8(1)
8044abe4: c0 c2 20 18  	lfs 6, 8216(2)
8044abe8: ec 41 10 2a  	fadds 2, 1, 2
8044abec: c0 61 00 0c  	lfs 3, 12(1)
8044abf0: ec a0 28 2a  	fadds 5, 0, 5
8044abf4: c0 01 00 10  	lfs 0, 16(1)
8044abf8: ec c6 38 2a  	fadds 6, 6, 7
8044abfc: ec 63 20 2a  	fadds 3, 3, 4
8044ac00: ec 00 10 2a  	fadds 0, 0, 2
8044ac04: ec 26 01 72  	fmuls 1, 6, 5
8044ac08: ec 46 00 f2  	fmuls 2, 6, 3
8044ac0c: ec 66 00 32  	fmuls 3, 6, 0
8044ac10: 4b bc c6 d5  	bl 0x800172e4 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcc454>
8044ac14: 7f e3 fb 78  	mr	3, 31
8044ac18: 38 80 00 04  	li 4, 4
8044ac1c: 4b ff df a9  	bl 0x80448bc4 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdd34>
8044ac20: 2c 03 00 00  	cmpwi	3, 0
8044ac24: 41 82 00 24  	bt	2, 0x8044ac48 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x33c>
8044ac28: e0 1a 00 30  	lq 0, 48(26)
8044ac2c: e0 5f 00 00  	lq 2, 0(31)
8044ac30: c0 3a 00 38  	lfs 1, 56(26)
8044ac34: 10 00 00 b2  	<unknown>
8044ac38: f0 1a 00 30  	xxsel 0, 26, 0, 0
8044ac3c: c0 1f 00 08  	lfs 0, 8(31)
8044ac40: ec 01 00 32  	fmuls 0, 1, 0
8044ac44: d0 1a 00 38  	stfs 0, 56(26)
8044ac48: 38 9a 00 30  	addi 4, 26, 48
8044ac4c: 38 7b 00 a8  	addi 3, 27, 168
8044ac50: 7c 85 23 78  	mr	5, 4
8044ac54: 48 06 e0 c1  	bl 0x804b8d14 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0x6de84>
8044ac58: 38 7a 00 3c  	addi 3, 26, 60
8044ac5c: 4b bc e1 a1  	bl 0x80018dfc <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcdf6c>
8044ac60: 38 7f 00 c4  	addi 3, 31, 196
8044ac64: 4b ff df 21  	bl 0x80448b84 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdcf4>
8044ac68: 80 9c 00 00  	lwz 4, 0(28)
8044ac6c: 38 00 00 00  	li 0, 0
8044ac70: c0 02 20 18  	lfs 0, 8216(2)
8044ac74: 38 7a 00 54  	addi 3, 26, 84
8044ac78: c0 44 00 64  	lfs 2, 100(4)
8044ac7c: ec 22 00 72  	fmuls 1, 2, 1
8044ac80: d0 1a 00 74  	stfs 0, 116(26)
8044ac84: 90 1a 00 78  	stw 0, 120(26)
8044ac88: ec 00 08 28  	fsubs 0, 0, 1
8044ac8c: d0 1a 00 70  	stfs 0, 112(26)
8044ac90: c0 3b 00 ac  	lfs 1, 172(27)
8044ac94: c0 5b 00 bc  	lfs 2, 188(27)
8044ac98: c0 7b 00 cc  	lfs 3, 204(27)
8044ac9c: 4b bc c6 49  	bl 0x800172e4 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffbcc454>
8044aca0: 38 7a 00 8c  	addi 3, 26, 140
8044aca4: 38 9f 01 08  	addi 4, 31, 264
8044aca8: 4b c7 e2 cd  	bl 0x800c8f74 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffc7e0e4>
8044acac: 38 7a 00 90  	addi 3, 26, 144
8044acb0: 38 9f 01 0c  	addi 4, 31, 268
8044acb4: 4b c7 e2 c1  	bl 0x800c8f74 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffc7e0e4>
8044acb8: 80 9d 00 00  	lwz 4, 0(29)
8044acbc: 38 7f 00 c4  	addi 3, 31, 196
8044acc0: 8b 64 00 2e  	lbz 27, 46(4)
8044acc4: 4b ff de c1  	bl 0x80448b84 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdcf4>
8044acc8: 3c 00 43 30  	lis 0, 17200
8044accc: 3c 60 80 56  	lis 3, -32682
8044acd0: 93 61 00 74  	stw 27, 116(1)
8044acd4: 2c 1e 00 00  	cmpwi	30, 0
8044acd8: c8 43 c3 f0  	lfd 2, -15376(3)
8044acdc: 90 01 00 70  	stw 0, 112(1)
8044ace0: c8 01 00 70  	lfd 0, 112(1)
8044ace4: ec 00 10 28  	fsubs 0, 0, 2
8044ace8: ec 01 00 32  	fmuls 0, 1, 0
8044acec: fc 00 00 1e  	fctiwz 0, 0
8044acf0: d8 01 00 68  	stfd 0, 104(1)
8044acf4: 80 01 00 6c  	lwz 0, 108(1)
8044acf8: 98 1a 00 95  	stb 0, 149(26)
8044acfc: 41 82 00 44  	bt	2, 0x8044ad40 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x434>
8044ad00: 80 7e 00 00  	lwz 3, 0(30)
8044ad04: 80 03 00 08  	lwz 0, 8(3)
8044ad08: 54 00 07 ff  	clrlwi.	0, 0, 31
8044ad0c: 41 82 00 34  	bt	2, 0x8044ad40 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x434>
8044ad10: c3 c3 00 24  	lfs 30, 36(3)
8044ad14: 7f e3 fb 78  	mr	3, 31
8044ad18: 4b ff de ed  	bl 0x80448c04 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdd74>
8044ad1c: ec 41 07 b2  	fmuls 2, 1, 30
8044ad20: c0 22 20 18  	lfs 1, 8216(2)
8044ad24: c0 1f 00 fc  	lfs 0, 252(31)
8044ad28: ec 21 10 2a  	fadds 1, 1, 2
8044ad2c: ec 00 00 72  	fmuls 0, 0, 1
8044ad30: d0 1a 00 68  	stfs 0, 104(26)
8044ad34: d0 1a 00 64  	stfs 0, 100(26)
8044ad38: d0 1a 00 60  	stfs 0, 96(26)
8044ad3c: 48 00 00 14  	b 0x8044ad50 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x444>
8044ad40: c0 1f 00 fc  	lfs 0, 252(31)
8044ad44: d0 1a 00 68  	stfs 0, 104(26)
8044ad48: d0 1a 00 64  	stfs 0, 100(26)
8044ad4c: d0 1a 00 60  	stfs 0, 96(26)
8044ad50: 38 00 00 ff  	li 0, 255
8044ad54: 2c 1e 00 00  	cmpwi	30, 0
8044ad58: 98 1a 00 96  	stb 0, 150(26)
8044ad5c: 41 82 00 34  	bt	2, 0x8044ad90 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x484>
8044ad60: 80 7e 00 00  	lwz 3, 0(30)
8044ad64: 80 03 00 08  	lwz 0, 8(3)
8044ad68: 54 00 03 9d  	rlwinm. 0, 0, 0, 14, 14
8044ad6c: 41 82 00 24  	bt	2, 0x8044ad90 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x484>
8044ad70: c3 c3 00 44  	lfs 30, 68(3)
8044ad74: 7f e3 fb 78  	mr	3, 31
8044ad78: 4b ff de 8d  	bl 0x80448c04 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdd74>
8044ad7c: ec 21 07 b2  	fmuls 1, 1, 30
8044ad80: c0 02 20 18  	lfs 0, 8216(2)
8044ad84: ec 00 08 2a  	fadds 0, 0, 1
8044ad88: d0 1a 00 6c  	stfs 0, 108(26)
8044ad8c: 48 00 00 0c  	b 0x8044ad98 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x48c>
8044ad90: c0 02 20 18  	lfs 0, 8216(2)
8044ad94: d0 1a 00 6c  	stfs 0, 108(26)
8044ad98: 2c 1e 00 00  	cmpwi	30, 0
8044ad9c: 41 82 00 c0  	bt	2, 0x8044ae5c <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x550>
8044ada0: 80 7e 00 00  	lwz 3, 0(30)
8044ada4: 80 03 00 08  	lwz 0, 8(3)
8044ada8: 54 00 01 cf  	rlwinm. 0, 0, 0, 7, 7
8044adac: 41 82 00 a0  	bt	2, 0x8044ae4c <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x540>
8044adb0: 38 7f 00 c4  	addi 3, 31, 196
8044adb4: 4b ff dd d1  	bl 0x80448b84 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdcf4>
8044adb8: c0 02 20 24  	lfs 0, 8228(2)
8044adbc: 7f e3 fb 78  	mr	3, 31
8044adc0: 80 9e 00 00  	lwz 4, 0(30)
8044adc4: ec 01 00 28  	fsubs 0, 1, 0
8044adc8: c0 24 00 50  	lfs 1, 80(4)
8044adcc: c0 44 00 4c  	lfs 2, 76(4)
8044add0: ec 01 00 32  	fmuls 0, 1, 0
8044add4: ec 02 00 2a  	fadds 0, 2, 0
8044add8: fc 00 00 1e  	fctiwz 0, 0
8044addc: d8 01 00 70  	stfd 0, 112(1)
8044ade0: 80 01 00 74  	lwz 0, 116(1)
8044ade4: b0 1a 00 88  	sth 0, 136(26)
8044ade8: 80 9e 00 00  	lwz 4, 0(30)
8044adec: c3 c4 00 58  	lfs 30, 88(4)
8044adf0: c3 e4 00 54  	lfs 31, 84(4)
8044adf4: 4b ff de 11  	bl 0x80448c04 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdd74>
8044adf8: ec 3e 00 72  	fmuls 1, 30, 1
8044adfc: c0 02 20 18  	lfs 0, 8216(2)
8044ae00: 7f e3 fb 78  	mr	3, 31
8044ae04: ec 00 08 2a  	fadds 0, 0, 1
8044ae08: ec 1f 00 32  	fmuls 0, 31, 0
8044ae0c: fc 00 00 1e  	fctiwz 0, 0
8044ae10: d8 01 00 68  	stfd 0, 104(1)
8044ae14: 80 01 00 6c  	lwz 0, 108(1)
8044ae18: b0 1a 00 8a  	sth 0, 138(26)
8044ae1c: 80 9e 00 00  	lwz 4, 0(30)
8044ae20: c3 c4 00 5c  	lfs 30, 92(4)
8044ae24: 4b ff dd e1  	bl 0x80448c04 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xffffffffffffdd74>
8044ae28: fc 01 f0 40  	fcmpo 0, 1, 30
8044ae2c: 40 80 00 0c  	bf	0, 0x8044ae38 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x52c>
8044ae30: a8 1a 00 8a  	lha 0, 138(26)
8044ae34: 48 00 00 10  	b 0x8044ae44 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x538>
8044ae38: a8 1a 00 8a  	lha 0, 138(26)
8044ae3c: 7c 00 00 d0  	neg 0, 0
8044ae40: 7c 00 07 34  	extsh 0, 0
8044ae44: b0 1a 00 8a  	sth 0, 138(26)
8044ae48: 48 00 00 20  	b 0x8044ae68 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x55c>
8044ae4c: 38 00 00 00  	li 0, 0
8044ae50: b0 1a 00 88  	sth 0, 136(26)
8044ae54: b0 1a 00 8a  	sth 0, 138(26)
8044ae58: 48 00 00 10  	b 0x8044ae68 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_start+0x55c>
8044ae5c: 38 00 00 00  	li 0, 0
8044ae60: b0 1a 00 88  	sth 0, 136(26)
8044ae64: b0 1a 00 8a  	sth 0, 138(26)
8044ae68: e3 e1 00 b8  	<unknown>
8044ae6c: cb e1 00 b0  	lfd 31, 176(1)
8044ae70: e3 c1 00 a8  	<unknown>
8044ae74: 39 61 00 a0  	addi 11, 1, 160
8044ae78: cb c1 00 a0  	lfd 30, 160(1)
8044ae7c: 48 0c db c9  	bl 0x80518a44 <_binary_build_original_jpa_arithmetic_20260903_ParticleInit_bin_end+0xcdbb4>
8044ae80: 80 01 00 c4  	lwz 0, 196(1)
8044ae84: 7c 08 03 a6  	mtlr 0
8044ae88: 38 21 00 c0  	addi 1, 1, 192
8044ae8c: 4e 80 00 20  	blr
