
build/mario-shadow-view-20260903/calcViewSilhouetteModel.o:	file format elf32-powerpc

Disassembly of section .data:

802c0c20 <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_start>:
802c0c20: 94 21 ff 80  	stwu 1, -128(1)
802c0c24: 7c 08 02 a6  	mflr 0
802c0c28: 90 01 00 84  	stw 0, 132(1)
802c0c2c: 39 61 00 80  	addi 11, 1, 128
802c0c30: 48 25 7d d5  	bl 0x80518a04 _savegpr_28 <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x257c50>
802c0c34: 7c 7c 1b 78  	mr	28, 3
802c0c38: 4b ff 65 ed  	bl 0x802b7224 getJ3DModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffff6470>
802c0c3c: 7c 7f 1b 78  	mr	31, 3
802c0c40: 7f 83 e3 78  	mr	3, 28
802c0c44: 4b ff 66 0d  	bl 0x802b7250 getSimpleModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffff649c>
802c0c48: 7c 7e 1b 78  	mr	30, 3
802c0c4c: 38 61 00 5c  	addi 3, 1, 92
802c0c50: 38 9c 00 0c  	addi 4, 28, 12
802c0c54: 4b d5 82 9d  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd5813c>
802c0c58: 38 61 00 5c  	addi 3, 1, 92
802c0c5c: 38 9c 0f 78  	addi 4, 28, 3960
802c0c60: 4b d5 f4 99  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd5f344>
802c0c64: 38 61 00 5c  	addi 3, 1, 92
802c0c68: 48 12 58 d5  	bl 0x803e653c normalizeOrZero__2MRFPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x125788>
802c0c6c: 38 7f 00 24  	addi 3, 31, 36
802c0c70: 38 9e 00 24  	addi 4, 30, 36
802c0c74: 48 1f 77 19  	bl 0x804b838c PSMTXCopy <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x1f75d8>
802c0c78: 3b be 00 24  	addi 29, 30, 36
802c0c7c: 38 81 00 50  	addi 4, 1, 80
802c0c80: 7f a3 eb 78  	mr	3, 29
802c0c84: 4b d5 fc 49  	bl 0x800208cc getXDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd5fb18>
802c0c88: 38 61 00 50  	addi 3, 1, 80
802c0c8c: 38 81 00 5c  	addi 4, 1, 92
802c0c90: 7c 65 1b 78  	mr	5, 3
802c0c94: 48 12 68 a1  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x126780>
802c0c98: 7f a3 eb 78  	mr	3, 29
802c0c9c: 38 81 00 44  	addi 4, 1, 68
802c0ca0: 4b d5 fc 45  	bl 0x800208e4 getYDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd5fb30>
802c0ca4: 38 61 00 44  	addi 3, 1, 68
802c0ca8: 38 81 00 5c  	addi 4, 1, 92
802c0cac: 7c 65 1b 78  	mr	5, 3
802c0cb0: 48 12 68 85  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x126780>
802c0cb4: 7f a3 eb 78  	mr	3, 29
802c0cb8: 38 81 00 38  	addi 4, 1, 56
802c0cbc: 4b d5 fc 41  	bl 0x800208fc getZDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd5fb48>
802c0cc0: 38 61 00 38  	addi 3, 1, 56
802c0cc4: 38 81 00 5c  	addi 4, 1, 92
802c0cc8: 7c 65 1b 78  	mr	5, 3
802c0ccc: 48 12 68 69  	bl 0x803e7534 vecKillElement__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x126780>
802c0cd0: 7f a3 eb 78  	mr	3, 29
802c0cd4: 38 81 00 50  	addi 4, 1, 80
802c0cd8: 4b db b4 bd  	bl 0x8007c194 setXDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffdbb3e0>
802c0cdc: 7f a3 eb 78  	mr	3, 29
802c0ce0: 38 81 00 44  	addi 4, 1, 68
802c0ce4: 4b db b4 cd  	bl 0x8007c1b0 setYDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffdbb3fc>
802c0ce8: 7f a3 eb 78  	mr	3, 29
802c0cec: 38 81 00 38  	addi 4, 1, 56
802c0cf0: 4b db b4 dd  	bl 0x8007c1cc setZDir__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffdbb418>
802c0cf4: 88 1c 0e a4  	lbz 0, 3748(28)
802c0cf8: 2c 00 00 00  	cmpwi	0, 0
802c0cfc: 41 82 00 60  	bt	2, 0x802c0d5c <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_start+0x13c>
802c0d00: 3c 80 80 5c  	lis 4, -32676
802c0d04: 7f 83 e3 78  	mr	3, 28
802c0d08: 38 84 8e 27  	addi 4, 4, -29145
802c0d0c: 38 a1 00 2c  	addi 5, 1, 44
802c0d10: 4b ff 1d 89  	bl 0x802b2a98 getRealPos__10MarioActorCFPCcPQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffff1ce4>
802c0d14: 80 bc 02 3c  	lwz 5, 572(28)
802c0d18: 38 61 00 08  	addi 3, 1, 8
802c0d1c: 38 81 00 5c  	addi 4, 1, 92
802c0d20: 80 05 00 08  	lwz 0, 8(5)
802c0d24: 54 00 10 3a  	slwi 0, 0, 2
802c0d28: 7c a5 00 2e  	lwzx 5, 5, 0
802c0d2c: c0 25 07 24  	lfs 1, 1828(5)
802c0d30: 4b d5 81 5d  	bl 0x80018e8c __ml__Q29JGeometry8TVec3<f>CFf <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd580d8>
802c0d34: 38 61 00 14  	addi 3, 1, 20
802c0d38: 38 81 00 2c  	addi 4, 1, 44
802c0d3c: 4b d5 81 b5  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd5813c>
802c0d40: 38 61 00 14  	addi 3, 1, 20
802c0d44: 38 81 00 08  	addi 4, 1, 8
802c0d48: 4b d5 f3 b1  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd5f344>
802c0d4c: 38 61 00 20  	addi 3, 1, 32
802c0d50: 38 81 00 14  	addi 4, 1, 20
802c0d54: 4b d5 81 25  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd580c4>
802c0d58: 48 00 00 10  	b 0x802c0d68 <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_start+0x148>
802c0d5c: 38 61 00 20  	addi 3, 1, 32
802c0d60: 38 9c 00 0c  	addi 4, 28, 12
802c0d64: 4b d5 81 15  	bl 0x80018e78 __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xffffffffffd580c4>
802c0d68: c0 21 00 20  	lfs 1, 32(1)
802c0d6c: 7f a3 eb 78  	mr	3, 29
802c0d70: c0 41 00 24  	lfs 2, 36(1)
802c0d74: c0 61 00 28  	lfs 3, 40(1)
802c0d78: 48 12 c2 39  	bl 0x803ecfb0 setMtxTrans__2MRFPA4_ffff <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x12c1fc>
802c0d7c: 7f a3 eb 78  	mr	3, 29
802c0d80: 7f a5 eb 78  	mr	5, 29
802c0d84: 38 9c 0b c8  	addi 4, 28, 3016
802c0d88: 48 1f 76 39  	bl 0x804b83c0 PSMTXConcat <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x1f760c>
802c0d8c: 7f c3 f3 78  	mr	3, 30
802c0d90: 7f e5 fb 78  	mr	5, 31
802c0d94: 38 80 00 01  	li 4, 1
802c0d98: 4b fe 5e bd  	bl 0x802a6c54 viewCalcRef__9J3DModelXFUlP8J3DModel <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0xfffffffffffe5ea0>
802c0d9c: 39 61 00 80  	addi 11, 1, 128
802c0da0: 48 25 7c b1  	bl 0x80518a50 _restgpr_28 <_binary_build_mario_shadow_view_20260903_calcViewSilhouetteModel_bin_end+0x257c9c>
802c0da4: 80 01 00 84  	lwz 0, 132(1)
802c0da8: 7c 08 03 a6  	mtlr 0
802c0dac: 38 21 00 80  	addi 1, 1, 128
802c0db0: 4e 80 00 20  	blr
