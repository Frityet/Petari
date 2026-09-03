
build/original-jpa-overwrite-20260903/FieldVortexCalc.o:	file format elf32-powerpc

Disassembly of section .data:

803a63b0 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_start>:
803a63b0: 94 21 ff c0  	stwu 1, -64(1)
803a63b4: 7c 08 02 a6  	mflr 0
803a63b8: 90 01 00 44  	stw 0, 68(1)
803a63bc: db e1 00 30  	stfd 31, 48(1)
803a63c0: f3 e1 00 38  	xxsel 31, 1, 0, 32
803a63c4: 39 61 00 30  	addi 11, 1, 48
803a63c8: 48 17 26 41  	bl 0x80518a08 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0x17256c>
803a63cc: 7c 7d 1b 78  	mr	29, 3
803a63d0: 7c be 2b 78  	mr	30, 5
803a63d4: 7c df 33 78  	mr	31, 6
803a63d8: 38 63 00 10  	addi 3, 3, 16
803a63dc: 38 86 00 0c  	addi 4, 6, 12
803a63e0: 4b c7 6e c9  	bl 0x8001d2a8 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0xffffffffffc76e0c>
803a63e4: 38 61 00 08  	addi 3, 1, 8
803a63e8: 38 9d 00 10  	addi 4, 29, 16
803a63ec: 4b ca ec ed  	bl 0x800550d8 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0xffffffffffcaec3c>
803a63f0: 38 81 00 08  	addi 4, 1, 8
803a63f4: 38 7f 00 0c  	addi 3, 31, 12
803a63f8: 7c 85 23 78  	mr	5, 4
803a63fc: 4b c7 7b dd  	bl 0x8001dfd8 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0xffffffffffc77b3c>
803a6400: 38 61 00 08  	addi 3, 1, 8
803a6404: 4b cd 13 ad  	bl 0x800777b0 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0xffffffffffcd1314>
803a6408: c0 1d 00 1c  	lfs 0, 28(29)
803a640c: fc 01 00 40  	fcmpo 0, 1, 0
803a6410: 40 81 00 10  	bf	1, 0x803a6420 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_start+0x70>
803a6414: 80 7e 00 00  	lwz 3, 0(30)
803a6418: c3 e3 00 28  	lfs 31, 40(3)
803a641c: 48 00 00 2c  	b 0x803a6448 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_start+0x98>
803a6420: c0 1d 00 20  	lfs 0, 32(29)
803a6424: 80 7e 00 00  	lwz 3, 0(30)
803a6428: ec 81 00 32  	fmuls 4, 1, 0
803a642c: c0 02 18 28  	lfs 0, 6184(2)
803a6430: c0 43 00 28  	lfs 2, 40(3)
803a6434: c0 7e 00 28  	lfs 3, 40(30)
803a6438: ec 20 20 28  	fsubs 1, 0, 4
803a643c: ec 04 00 b2  	fmuls 0, 4, 2
803a6440: ec 21 00 f2  	fmuls 1, 1, 3
803a6444: ef e1 00 2a  	fadds 31, 1, 0
803a6448: 38 61 00 08  	addi 3, 1, 8
803a644c: 48 04 00 f1  	bl 0x803e653c <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0x400a0>
803a6450: 38 61 00 08  	addi 3, 1, 8
803a6454: 38 9d 00 10  	addi 4, 29, 16
803a6458: 38 bd 00 04  	addi 5, 29, 4
803a645c: 48 11 2c e1  	bl 0x804b913c <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0x112ca0>
803a6460: fc 20 f8 90  	fmr 1, 31
803a6464: 38 7d 00 04  	addi 3, 29, 4
803a6468: 4b c7 9c 69  	bl 0x800200d0 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0xffffffffffc79c34>
803a646c: 7f a3 eb 78  	mr	3, 29
803a6470: 7f c4 f3 78  	mr	4, 30
803a6474: 7f e5 fb 78  	mr	5, 31
803a6478: 48 0a 27 bd  	bl 0x80448c34 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0xa2798>
803a647c: e3 e1 00 38  	<unknown>
803a6480: 39 61 00 30  	addi 11, 1, 48
803a6484: cb e1 00 30  	lfd 31, 48(1)
803a6488: 48 17 25 cd  	bl 0x80518a54 <_binary_build_original_jpa_overwrite_20260903_FieldVortexCalc_bin_end+0x1725b8>
803a648c: 80 01 00 44  	lwz 0, 68(1)
803a6490: 7c 08 03 a6  	mtlr 0
803a6494: 38 21 00 40  	addi 1, 1, 64
803a6498: 4e 80 00 20  	blr
