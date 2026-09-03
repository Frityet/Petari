
build/original-jpa-overwrite-20260903/DrawStripe.o:	file format elf32-powerpc

Disassembly of section .data:

803a57dc <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start>:
803a57dc: 94 21 fe f0  	stwu 1, -272(1)
803a57e0: 7c 08 02 a6  	mflr 0
803a57e4: 90 01 01 14  	stw 0, 276(1)
803a57e8: db e1 01 00  	stfd 31, 256(1)
803a57ec: f3 e1 01 08  	xsmaddadp 31, 1, 0
803a57f0: db c1 00 f0  	stfd 30, 240(1)
803a57f4: f3 c1 00 f8  	xxsel 30, 1, 0, 35
803a57f8: db a1 00 e0  	stfd 29, 224(1)
803a57fc: f3 a1 00 e8  	<unknown>
803a5800: db 81 00 d0  	stfd 28, 208(1)
803a5804: f3 81 00 d8  	<unknown>
803a5808: db 61 00 c0  	stfd 27, 192(1)
803a580c: f3 61 00 c8  	xsmsubmsp 27, 1, 0
803a5810: db 41 00 b0  	stfd 26, 176(1)
803a5814: f3 41 00 b8  	xxsel 26, 1, 0, 34
803a5818: db 21 00 a0  	stfd 25, 160(1)
803a581c: f3 21 00 a8  	<unknown>
803a5820: 39 61 00 a0  	addi 11, 1, 160
803a5824: 48 17 31 d1  	bl 0x805189f4 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x172e2c>
803a5828: 80 c3 01 e4  	lwz 6, 484(3)
803a582c: 7c 7f 1b 78  	mr	31, 3
803a5830: 80 83 00 04  	lwz 4, 4(3)
803a5834: 83 86 00 08  	lwz 28, 8(6)
803a5838: 80 84 00 1c  	lwz 4, 28(4)
803a583c: 28 1c 00 02  	cmplwi	28, 2
803a5840: 41 80 03 38  	bt	0, 0x803a5b78 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0x39c>
803a5844: 3c 00 43 30  	lis 0, 17200
803a5848: 3c a0 80 54  	lis 5, -32684
803a584c: 93 81 00 7c  	stw 28, 124(1)
803a5850: c8 25 b8 c8  	lfd 1, -18232(5)
803a5854: 90 01 00 78  	stw 0, 120(1)
803a5858: 80 84 00 00  	lwz 4, 0(4)
803a585c: c8 01 00 78  	lfd 0, 120(1)
803a5860: c0 a2 18 28  	lfs 5, 6184(2)
803a5864: ec 20 08 28  	fsubs 1, 0, 1
803a5868: c0 03 01 4c  	lfs 0, 332(3)
803a586c: 80 04 00 08  	lwz 0, 8(4)
803a5870: ec 65 00 2a  	fadds 3, 5, 0
803a5874: c0 42 18 4c  	lfs 2, 6220(2)
803a5878: ec 81 28 28  	fsubs 4, 1, 5
803a587c: c0 23 01 44  	lfs 1, 324(3)
803a5880: ec 05 00 28  	fsubs 0, 5, 0
803a5884: 54 00 02 95  	rlwinm. 0, 0, 0, 10, 10
803a5888: ef a5 20 24  	fdivs 29, 5, 4
803a588c: c3 c2 18 2c  	lfs 30, 6188(2)
803a5890: ec 22 00 72  	fmuls 1, 2, 1
803a5894: ef 83 00 72  	fmuls 28, 3, 1
803a5898: ef 60 00 72  	fmuls 27, 0, 1
803a589c: 41 82 00 1c  	bt	2, 0x803a58b8 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0xdc>
803a58a0: ff c0 28 90  	fmr 30, 5
803a58a4: 3f 40 80 3a  	lis 26, -32710
803a58a8: ff a0 e8 50  	fneg 29, 29
803a58ac: 83 26 00 04  	lwz 25, 4(6)
803a58b0: 3b 5a 4b b4  	addi 26, 26, 19380
803a58b4: 48 00 00 10  	b 0x803a58c4 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0xe8>
803a58b8: 3f 40 80 3a  	lis 26, -32710
803a58bc: 83 26 00 00  	lwz 25, 0(6)
803a58c0: 3b 5a 4b ac  	addi 26, 26, 19372
803a58c4: 38 80 00 00  	li 4, 0
803a58c8: 38 63 01 84  	addi 3, 3, 388
803a58cc: 48 11 ad 95  	bl 0x804c0660 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x11aa98>
803a58d0: 80 1f 02 10  	lwz 0, 528(31)
803a58d4: 3c a0 80 5e  	lis 5, -32674
803a58d8: 38 a5 bd 68  	addi 5, 5, -17048
803a58dc: 7f e3 fb 78  	mr	3, 31
803a58e0: 54 00 10 3a  	slwi 0, 0, 2
803a58e4: 38 9f 01 84  	addi 4, 31, 388
803a58e8: 7d 85 00 2e  	lwzx 12, 5, 0
803a58ec: 7d 89 03 a6  	mtctr 12
803a58f0: 4e 80 04 21  	bctrl
803a58f4: 38 60 00 09  	li 3, 9
803a58f8: 38 80 00 01  	li 4, 1
803a58fc: 48 11 5b 25  	bl 0x804bb420 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x115858>
803a5900: 38 60 00 0d  	li 3, 13
803a5904: 38 80 00 01  	li 4, 1
803a5908: 48 11 5b 19  	bl 0x804bb420 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x115858>
803a590c: 57 85 0c 3c  	rlwinm 5, 28, 1, 16, 30
803a5910: 38 60 00 98  	li 3, 152
803a5914: 38 80 00 01  	li 4, 1
803a5918: 48 11 72 19  	bl 0x804bcb30 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x116f68>
803a591c: 3f 80 80 61  	lis 28, -32671
803a5920: 3f a0 80 5e  	lis 29, -32674
803a5924: c3 e2 18 2c  	lfs 31, 6188(2)
803a5928: 3b 9c fc 80  	addi 28, 28, -896
803a592c: 3b bd bd 74  	addi 29, 29, -17036
803a5930: 3f c0 cc 01  	lis 30, -13311
803a5934: 3b 60 00 00  	li 27, 0
803a5938: 48 00 02 20  	b 0x803a5b58 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0x37c>
803a593c: 3b 19 00 08  	addi 24, 25, 8
803a5940: 93 3f 01 e8  	stw 25, 488(31)
803a5944: 7f 04 c3 78  	mr	4, 24
803a5948: 38 61 00 20  	addi 3, 1, 32
803a594c: 4b c7 79 7d  	bl 0x8001d2c8 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0xffffffffffc77700>
803a5950: c0 18 00 60  	lfs 0, 96(24)
803a5954: fc 40 f8 90  	fmr 2, 31
803a5958: a8 18 00 88  	lha 0, 136(24)
803a595c: 38 61 00 2c  	addi 3, 1, 44
803a5960: fc 00 00 50  	fneg 0, 0
803a5964: 54 00 0b f8  	rlwinm 0, 0, 1, 15, 28
803a5968: d3 e1 00 30  	stfs 31, 48(1)
803a596c: 7c 9c 02 14  	add 4, 28, 0
803a5970: 7f 5c 04 2e  	lfsx 26, 28, 0
803a5974: ec 00 07 32  	fmuls 0, 0, 28
803a5978: c3 24 00 04  	lfs 25, 4(4)
803a597c: d3 e1 00 34  	stfs 31, 52(1)
803a5980: ec 20 06 72  	fmuls 1, 0, 25
803a5984: ec 60 06 b2  	fmuls 3, 0, 26
803a5988: d0 01 00 2c  	stfs 0, 44(1)
803a598c: 4b c7 19 59  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0xffffffffffc7171c>
803a5990: c0 18 00 60  	lfs 0, 96(24)
803a5994: 38 61 00 38  	addi 3, 1, 56
803a5998: c0 42 18 2c  	lfs 2, 6188(2)
803a599c: ec 20 06 f2  	fmuls 1, 0, 27
803a59a0: fc 60 10 90  	fmr 3, 2
803a59a4: 4b c7 19 41  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0xffffffffffc7171c>
803a59a8: c0 01 00 38  	lfs 0, 56(1)
803a59ac: 38 61 00 38  	addi 3, 1, 56
803a59b0: c0 42 18 2c  	lfs 2, 6188(2)
803a59b4: ec 20 06 72  	fmuls 1, 0, 25
803a59b8: ec 60 06 b2  	fmuls 3, 0, 26
803a59bc: 4b c7 19 29  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0xffffffffffc7171c>
803a59c0: 80 1f 02 00  	lwz 0, 512(31)
803a59c4: 7f e3 fb 78  	mr	3, 31
803a59c8: 7f 04 c3 78  	mr	4, 24
803a59cc: 38 a1 00 14  	addi 5, 1, 20
803a59d0: 54 00 10 3a  	slwi 0, 0, 2
803a59d4: 7d 9d 00 2e  	lwzx 12, 29, 0
803a59d8: 7d 89 03 a6  	mtctr 12
803a59dc: 4e 80 04 21  	bctrl
803a59e0: c0 22 18 48  	lfs 1, 6216(2)
803a59e4: 38 61 00 14  	addi 3, 1, 20
803a59e8: 48 04 16 01  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x41420>
803a59ec: 2c 03 00 00  	cmpwi	3, 0
803a59f0: 41 82 00 1c  	bt	2, 0x803a5a0c <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0x230>
803a59f4: c0 22 18 2c  	lfs 1, 6188(2)
803a59f8: 38 61 00 14  	addi 3, 1, 20
803a59fc: c0 42 18 28  	lfs 2, 6184(2)
803a5a00: fc 60 08 90  	fmr 3, 1
803a5a04: 4b c7 18 e1  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0xffffffffffc7171c>
803a5a08: 48 00 00 0c  	b 0x803a5a14 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0x238>
803a5a0c: 38 61 00 14  	addi 3, 1, 20
803a5a10: 48 04 09 a1  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x407e8>
803a5a14: 38 78 00 54  	addi 3, 24, 84
803a5a18: 38 81 00 14  	addi 4, 1, 20
803a5a1c: 38 a1 00 08  	addi 5, 1, 8
803a5a20: 48 11 37 1d  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x113574>
803a5a24: c0 22 18 48  	lfs 1, 6216(2)
803a5a28: 38 61 00 08  	addi 3, 1, 8
803a5a2c: 48 04 15 bd  	bl 0x803e6fe8 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x41420>
803a5a30: 2c 03 00 00  	cmpwi	3, 0
803a5a34: 41 82 00 1c  	bt	2, 0x803a5a50 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0x274>
803a5a38: c0 42 18 2c  	lfs 2, 6188(2)
803a5a3c: 38 61 00 08  	addi 3, 1, 8
803a5a40: c0 22 18 28  	lfs 1, 6184(2)
803a5a44: fc 60 10 90  	fmr 3, 2
803a5a48: 4b c7 18 9d  	bl 0x800172e4 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0xffffffffffc7171c>
803a5a4c: 48 00 00 0c  	b 0x803a5a58 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0x27c>
803a5a50: 38 61 00 08  	addi 3, 1, 8
803a5a54: 48 04 09 5d  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x407e8>
803a5a58: 38 61 00 14  	addi 3, 1, 20
803a5a5c: 38 81 00 08  	addi 4, 1, 8
803a5a60: 38 b8 00 54  	addi 5, 24, 84
803a5a64: 48 11 36 d9  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x113574>
803a5a68: 38 78 00 54  	addi 3, 24, 84
803a5a6c: 48 04 09 45  	bl 0x803e63b0 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x407e8>
803a5a70: c0 21 00 08  	lfs 1, 8(1)
803a5a74: 38 81 00 2c  	addi 4, 1, 44
803a5a78: c0 01 00 14  	lfs 0, 20(1)
803a5a7c: 7c 85 23 78  	mr	5, 4
803a5a80: d0 21 00 44  	stfs 1, 68(1)
803a5a84: 38 61 00 44  	addi 3, 1, 68
803a5a88: c0 61 00 0c  	lfs 3, 12(1)
803a5a8c: 38 c0 00 02  	li 6, 2
803a5a90: d0 01 00 48  	stfs 0, 72(1)
803a5a94: c0 41 00 18  	lfs 2, 24(1)
803a5a98: c0 18 00 54  	lfs 0, 84(24)
803a5a9c: c0 21 00 10  	lfs 1, 16(1)
803a5aa0: d0 01 00 4c  	stfs 0, 76(1)
803a5aa4: c0 01 00 1c  	lfs 0, 28(1)
803a5aa8: d3 e1 00 50  	stfs 31, 80(1)
803a5aac: d0 61 00 54  	stfs 3, 84(1)
803a5ab0: d0 41 00 58  	stfs 2, 88(1)
803a5ab4: c0 58 00 58  	lfs 2, 88(24)
803a5ab8: d0 41 00 5c  	stfs 2, 92(1)
803a5abc: d3 e1 00 60  	stfs 31, 96(1)
803a5ac0: d0 21 00 64  	stfs 1, 100(1)
803a5ac4: d0 01 00 68  	stfs 0, 104(1)
803a5ac8: c0 18 00 5c  	lfs 0, 92(24)
803a5acc: d0 01 00 6c  	stfs 0, 108(1)
803a5ad0: d3 e1 00 70  	stfs 31, 112(1)
803a5ad4: 48 11 32 95  	bl 0x804b8d68 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x1131a0>
803a5ad8: c0 21 00 2c  	lfs 1, 44(1)
803a5adc: c0 01 00 20  	lfs 0, 32(1)
803a5ae0: c0 81 00 30  	lfs 4, 48(1)
803a5ae4: c0 41 00 24  	lfs 2, 36(1)
803a5ae8: ec 21 00 2a  	fadds 1, 1, 0
803a5aec: c0 61 00 34  	lfs 3, 52(1)
803a5af0: c0 01 00 28  	lfs 0, 40(1)
803a5af4: ec 44 10 2a  	fadds 2, 4, 2
803a5af8: ec 63 00 2a  	fadds 3, 3, 0
803a5afc: 48 00 11 f5  	bl 0x803a6cf0 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x1128>
803a5b00: c0 02 18 2c  	lfs 0, 6188(2)
803a5b04: d0 1e 80 00  	stfs 0, -32768(30)
803a5b08: d3 de 80 00  	stfs 30, -32768(30)
803a5b0c: c0 21 00 38  	lfs 1, 56(1)
803a5b10: c0 01 00 20  	lfs 0, 32(1)
803a5b14: c0 81 00 3c  	lfs 4, 60(1)
803a5b18: c0 41 00 24  	lfs 2, 36(1)
803a5b1c: ec 21 00 2a  	fadds 1, 1, 0
803a5b20: c0 61 00 40  	lfs 3, 64(1)
803a5b24: c0 01 00 28  	lfs 0, 40(1)
803a5b28: ec 44 10 2a  	fadds 2, 4, 2
803a5b2c: ec 63 00 2a  	fadds 3, 3, 0
803a5b30: 48 00 11 c1  	bl 0x803a6cf0 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x1128>
803a5b34: c0 02 18 28  	lfs 0, 6184(2)
803a5b38: 7f 4c d3 78  	mr	12, 26
803a5b3c: 7f 23 cb 78  	mr	3, 25
803a5b40: d0 1e 80 00  	stfs 0, -32768(30)
803a5b44: d3 de 80 00  	stfs 30, -32768(30)
803a5b48: 7d 89 03 a6  	mtctr 12
803a5b4c: 4e 80 04 21  	bctrl
803a5b50: 7c 79 1b 78  	mr	25, 3
803a5b54: ef de e8 2a  	fadds 30, 30, 29
803a5b58: 7c 19 d8 40  	cmplw	25, 27
803a5b5c: 40 82 fd e0  	bf	2, 0x803a593c <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_start+0x160>
803a5b60: 38 60 00 09  	li 3, 9
803a5b64: 38 80 00 02  	li 4, 2
803a5b68: 48 11 58 b9  	bl 0x804bb420 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x115858>
803a5b6c: 38 60 00 0d  	li 3, 13
803a5b70: 38 80 00 02  	li 4, 2
803a5b74: 48 11 58 ad  	bl 0x804bb420 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x115858>
803a5b78: e3 e1 01 08  	<unknown>
803a5b7c: cb e1 01 00  	lfd 31, 256(1)
803a5b80: e3 c1 00 f8  	<unknown>
803a5b84: cb c1 00 f0  	lfd 30, 240(1)
803a5b88: e3 a1 00 e8  	<unknown>
803a5b8c: cb a1 00 e0  	lfd 29, 224(1)
803a5b90: e3 81 00 d8  	<unknown>
803a5b94: cb 81 00 d0  	lfd 28, 208(1)
803a5b98: e3 61 00 c8  	<unknown>
803a5b9c: cb 61 00 c0  	lfd 27, 192(1)
803a5ba0: e3 41 00 b8  	<unknown>
803a5ba4: cb 41 00 b0  	lfd 26, 176(1)
803a5ba8: e3 21 00 a8  	<unknown>
803a5bac: 39 61 00 a0  	addi 11, 1, 160
803a5bb0: cb 21 00 a0  	lfd 25, 160(1)
803a5bb4: 48 17 2e 8d  	bl 0x80518a40 <_binary_build_original_jpa_overwrite_20260903_DrawStripe_bin_end+0x172e78>
803a5bb8: 80 01 01 14  	lwz 0, 276(1)
803a5bbc: 7c 08 03 a6  	mtlr 0
803a5bc0: 38 21 01 10  	addi 1, 1, 272
803a5bc4: 4e 80 00 20  	blr
