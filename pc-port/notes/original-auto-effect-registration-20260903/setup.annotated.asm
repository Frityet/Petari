
build/original-auto-effect-registration-20260903/setup.o:	file format elf32-powerpc

Disassembly of section .data:

800c590c <_binary_build_original_auto_effect_registration_20260903_setup_bin_start>:

# setupMultiEmitter__30@unnamed@EffectSystemUtil_cpp@FP12MultiEmitterPC14AutoEffectInfo
800c590c: 94 21 ff e0  	stwu 1, -32(1)
800c5910: 7c 08 02 a6  	mflr 0
800c5914: 90 01 00 24  	stw 0, 36(1)
800c5918: 93 e1 00 1c  	stw 31, 28(1)
800c591c: 7c 9f 23 78  	mr	31, 4
800c5920: 93 c1 00 18  	stw 30, 24(1)
800c5924: 7c 7e 1b 78  	mr	30, 3
800c5928: 90 83 00 28  	stw 4, 40(3)
800c592c: 80 84 00 4c  	lwz 4, 76(4)
800c5930: 48 00 18 ad  	bl 0x800c71dc <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x17bc> # setDrawOrder__12MultiEmitterFl
800c5934: 7f c3 f3 78  	mr	3, 30
800c5938: 38 9f 00 2c  	addi 4, 31, 44
800c593c: 48 00 14 0d  	bl 0x800c6d48 <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x1328> # setOffset__12MultiEmitterFRCQ29JGeometry8TVec3<f>
800c5940: c0 3f 00 40  	lfs 1, 64(31)
800c5944: c0 02 a0 f8  	lfs 0, -24328(2)
800c5948: c0 42 a1 04  	lfs 2, -24316(2)
800c594c: ec 21 00 28  	fsubs 1, 1, 0
800c5950: 48 32 16 71  	bl 0x803e6fc0 <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x3215a0> # isNearZero__2MRFff
800c5954: 2c 03 00 00  	cmpwi	3, 0
800c5958: 40 82 00 10  	bf	2, 0x800c5968 <_binary_build_original_auto_effect_registration_20260903_setup_bin_start+0x5c>
800c595c: c0 3f 00 40  	lfs 1, 64(31)
800c5960: 7f c3 f3 78  	mr	3, 30
800c5964: 48 00 14 05  	bl 0x800c6d68 <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x1348> # setBaseScale__12MultiEmitterFf
800c5968: c0 3f 00 44  	lfs 1, 68(31)
800c596c: c0 02 a0 f8  	lfs 0, -24328(2)
800c5970: c0 42 a1 04  	lfs 2, -24316(2)
800c5974: ec 21 00 28  	fsubs 1, 1, 0
800c5978: 48 32 16 49  	bl 0x803e6fc0 <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x3215a0> # isNearZero__2MRFff
800c597c: 2c 03 00 00  	cmpwi	3, 0
800c5980: 40 82 00 14  	bf	2, 0x800c5994 <_binary_build_original_auto_effect_registration_20260903_setup_bin_start+0x88>
800c5984: c0 3f 00 44  	lfs 1, 68(31)
800c5988: 7f c3 f3 78  	mr	3, 30
800c598c: 38 80 ff ff  	li 4, -1
800c5990: 48 00 22 c5  	bl 0x800c7c54 <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x2234> # setRate__12MultiEmitterFfl
800c5994: 88 1f 00 20  	lbz 0, 32(31)
800c5998: 2c 00 00 00  	cmpwi	0, 0
800c599c: 41 82 00 24  	bt	2, 0x800c59c0 <_binary_build_original_auto_effect_registration_20260903_setup_bin_start+0xb4>
800c59a0: 80 1f 00 1c  	lwz 0, 28(31)
800c59a4: 7f c3 f3 78  	mr	3, 30
800c59a8: 38 e0 ff ff  	li 7, -1
800c59ac: 90 01 00 0c  	stw 0, 12(1)
800c59b0: 88 81 00 0c  	lbz 4, 12(1)
800c59b4: 88 a1 00 0d  	lbz 5, 13(1)
800c59b8: 88 c1 00 0e  	lbz 6, 14(1)
800c59bc: 48 00 1f 6d  	bl 0x800c7928 <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x1f08> # setGlobalPrmColor__12MultiEmitterFUcUcUcl
800c59c0: 88 1f 00 28  	lbz 0, 40(31)
800c59c4: 2c 00 00 00  	cmpwi	0, 0
800c59c8: 41 82 00 24  	bt	2, 0x800c59ec <_binary_build_original_auto_effect_registration_20260903_setup_bin_start+0xe0>
800c59cc: 80 1f 00 24  	lwz 0, 36(31)
800c59d0: 7f c3 f3 78  	mr	3, 30
800c59d4: 38 e0 ff ff  	li 7, -1
800c59d8: 90 01 00 08  	stw 0, 8(1)
800c59dc: 88 81 00 08  	lbz 4, 8(1)
800c59e0: 88 a1 00 09  	lbz 5, 9(1)
800c59e4: 88 c1 00 0a  	lbz 6, 10(1)
800c59e8: 48 00 20 35  	bl 0x800c7a1c <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x1ffc> # setGlobalEnvColor__12MultiEmitterFUcUcUcl
800c59ec: c0 3f 00 48  	lfs 1, 72(31)
800c59f0: c0 42 a1 04  	lfs 2, -24316(2)
800c59f4: 48 32 15 cd  	bl 0x803e6fc0 <_binary_build_original_auto_effect_registration_20260903_setup_bin_end+0x3215a0> # isNearZero__2MRFff
800c59f8: 2c 03 00 00  	cmpwi	3, 0
800c59fc: 40 82 00 0c  	bf	2, 0x800c5a08 <_binary_build_original_auto_effect_registration_20260903_setup_bin_start+0xfc>
800c5a00: c0 1f 00 48  	lfs 0, 72(31)
800c5a04: d0 1e 00 2c  	stfs 0, 44(30)
800c5a08: 80 01 00 24  	lwz 0, 36(1)
800c5a0c: 83 e1 00 1c  	lwz 31, 28(1)
800c5a10: 83 c1 00 18  	lwz 30, 24(1)
800c5a14: 7c 08 03 a6  	mtlr 0
800c5a18: 38 21 00 20  	addi 1, 1, 32
800c5a1c: 4e 80 00 20  	blr
