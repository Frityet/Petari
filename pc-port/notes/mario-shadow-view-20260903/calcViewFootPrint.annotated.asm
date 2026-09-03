
build/mario-shadow-view-20260903/calcViewFootPrint.o:	file format elf32-powerpc

Disassembly of section .data:

802c107c <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start>:
802c107c: 94 21 ff f0  	stwu 1, -16(1)
802c1080: 7c 08 02 a6  	mflr 0
802c1084: 90 01 00 14  	stw 0, 20(1)
802c1088: 93 e1 00 0c  	stw 31, 12(1)
802c108c: 7c 7f 1b 78  	mr	31, 3
802c1090: 88 03 09 34  	lbz 0, 2356(3)
802c1094: 2c 00 00 00  	cmpwi	0, 0
802c1098: 40 82 00 c8  	bf	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c109c: 80 83 02 30  	lwz 4, 560(3)
802c10a0: 80 04 00 08  	lwz 0, 8(4)
802c10a4: 54 00 17 ff  	rlwinm. 0, 0, 2, 31, 31
802c10a8: 41 82 00 b8  	bt	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c10ac: 88 04 07 1c  	lbz 0, 1820(4)
802c10b0: 2c 00 00 00  	cmpwi	0, 0
802c10b4: 40 82 00 34  	bf	2, 0x802c10e8 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0x6c>
802c10b8: 80 63 02 30  	lwz 3, 560(3)
802c10bc: 80 03 00 1c  	lwz 0, 28(3)
802c10c0: 54 00 9f ff  	rlwinm. 0, 0, 19, 31, 31
802c10c4: 40 82 00 24  	bf	2, 0x802c10e8 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0x6c>
802c10c8: c0 24 02 78  	lfs 1, 632(4)
802c10cc: c0 02 fb c0  	lfs 0, -1088(2)
802c10d0: fc 01 00 40  	fcmpo 0, 1, 0
802c10d4: 41 81 00 14  	bt	1, 0x802c10e8 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0x6c>
802c10d8: 7c 83 23 78  	mr	3, 4
802c10dc: 48 02 7d f1  	bl 0x802e8ecc isPlayerModeHopper__11MarioModuleCFv <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_end+0x27d20>
802c10e0: 2c 03 00 00  	cmpwi	3, 0
802c10e4: 41 82 00 7c  	bt	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c10e8: 80 7f 02 30  	lwz 3, 560(31)
802c10ec: 88 03 07 35  	lbz 0, 1845(3)
802c10f0: 2c 00 00 00  	cmpwi	0, 0
802c10f4: 40 82 00 6c  	bf	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c10f8: 80 83 00 18  	lwz 4, 24(3)
802c10fc: 54 80 57 ff  	rlwinm. 0, 4, 10, 31, 31
802c1100: 40 82 00 60  	bf	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c1104: 54 80 5f ff  	rlwinm. 0, 4, 11, 31, 31
802c1108: 40 82 00 58  	bf	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c110c: 80 83 00 1c  	lwz 4, 28(3)
802c1110: 54 80 a7 ff  	rlwinm. 0, 4, 20, 31, 31
802c1114: 41 82 00 4c  	bt	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c1118: 54 80 af ff  	rlwinm. 0, 4, 21, 31, 31
802c111c: 41 82 00 44  	bt	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c1120: 38 80 00 0d  	li 4, 13
802c1124: 48 01 10 25  	bl 0x802d2148 checkCurrentFloorCodeSevere__5MarioCFUl <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_end+0x10f9c>
802c1128: 2c 03 00 00  	cmpwi	3, 0
802c112c: 40 82 00 18  	bf	2, 0x802c1144 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xc8>
802c1130: 80 7f 02 30  	lwz 3, 560(31)
802c1134: 38 80 00 1a  	li 4, 26
802c1138: 48 01 10 11  	bl 0x802d2148 checkCurrentFloorCodeSevere__5MarioCFUl <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_end+0x10f9c>
802c113c: 2c 03 00 00  	cmpwi	3, 0
802c1140: 41 82 00 20  	bt	2, 0x802c1160 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0xe4>
802c1144: 80 df 02 30  	lwz 6, 560(31)
802c1148: 38 9f 00 0c  	addi 4, 31, 12
802c114c: 80 7f 0b 48  	lwz 3, 2888(31)
802c1150: 38 e0 00 00  	li 7, 0
802c1154: 38 a6 02 08  	addi 5, 6, 520
802c1158: 38 c6 03 68  	addi 6, 6, 872
802c115c: 48 10 f0 21  	bl 0x803d017c addPrint__9FootPrintFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>b <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_end+0x10efd0>
802c1160: 80 7f 0b 48  	lwz 3, 2888(31)
802c1164: 80 9f 03 7c  	lwz 4, 892(31)
802c1168: 48 10 f4 1d  	bl 0x803d0584 isValid__9FootPrintCFUl <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_end+0x10f3d8>
802c116c: 2c 03 00 00  	cmpwi	3, 0
802c1170: 41 82 00 28  	bt	2, 0x802c1198 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0x11c>
802c1174: 80 7f 0b 48  	lwz 3, 2888(31)
802c1178: 80 9f 03 7c  	lwz 4, 892(31)
802c117c: 48 10 f3 c1  	bl 0x803d053c getPrintPos__9FootPrintCFUl <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_end+0x10f390>
802c1180: 48 12 f3 51  	bl 0x803f04d0 isInWater__2MRFRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_end+0x12f324>
802c1184: 2c 03 00 00  	cmpwi	3, 0
802c1188: 41 82 00 10  	bt	2, 0x802c1198 <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_start+0x11c>
802c118c: 80 7f 0b 48  	lwz 3, 2888(31)
802c1190: 80 9f 03 7c  	lwz 4, 892(31)
802c1194: 48 10 f3 c9  	bl 0x803d055c invalidate__9FootPrintFUl <_binary_build_mario_shadow_view_20260903_calcViewFootPrint_bin_end+0x10f3b0>
802c1198: 80 01 00 14  	lwz 0, 20(1)
802c119c: 83 e1 00 0c  	lwz 31, 12(1)
802c11a0: 7c 08 03 a6  	mtlr 0
802c11a4: 38 21 00 10  	addi 1, 1, 16
802c11a8: 4e 80 00 20  	blr
