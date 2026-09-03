
build/mario-shadow-view-20260903/drawSilhouette.o:	file format elf32-powerpc

Disassembly of section .data:

802c11ac <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start>:
802c11ac: 94 21 ff d0  	stwu 1, -48(1)
802c11b0: 7c 08 02 a6  	mflr 0
802c11b4: 38 80 00 00  	li 4, 0
802c11b8: 90 01 00 34  	stw 0, 52(1)
802c11bc: 88 03 04 82  	lbz 0, 1154(3)
802c11c0: 93 e1 00 2c  	stw 31, 44(1)
802c11c4: 7c 7f 1b 78  	mr	31, 3
802c11c8: 2c 00 00 00  	cmpwi	0, 0
802c11cc: 40 82 00 1c  	bf	2, 0x802c11e8 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x3c>
802c11d0: 88 03 04 83  	lbz 0, 1155(3)
802c11d4: 2c 00 00 00  	cmpwi	0, 0
802c11d8: 40 82 00 10  	bf	2, 0x802c11e8 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x3c>
802c11dc: 88 03 04 81  	lbz 0, 1153(3)
802c11e0: 2c 00 00 00  	cmpwi	0, 0
802c11e4: 41 82 00 08  	bt	2, 0x802c11ec <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x40>
802c11e8: 38 80 00 01  	li 4, 1
802c11ec: 2c 04 00 00  	cmpwi	4, 0
802c11f0: 40 82 01 28  	bf	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c11f4: 80 63 02 30  	lwz 3, 560(3)
802c11f8: 38 80 00 1a  	li 4, 26
802c11fc: 48 03 34 d5  	bl 0x802f46d0 isStatusActive__5MarioCFUl <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0x333a4>
802c1200: 2c 03 00 00  	cmpwi	3, 0
802c1204: 40 82 01 14  	bf	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c1208: 80 7f 02 30  	lwz 3, 560(31)
802c120c: 38 80 00 1b  	li 4, 27
802c1210: 48 03 34 c1  	bl 0x802f46d0 isStatusActive__5MarioCFUl <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0x333a4>
802c1214: 2c 03 00 00  	cmpwi	3, 0
802c1218: 41 82 00 5c  	bt	2, 0x802c1274 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0xc8>
802c121c: 38 61 00 14  	addi 3, 1, 20
802c1220: 38 9f 00 0c  	addi 4, 31, 12
802c1224: 4b d5 7c cd  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0xffffffffffd57bc4>
802c1228: 38 61 00 14  	addi 3, 1, 20
802c122c: 38 9f 0f 78  	addi 4, 31, 3960
802c1230: 4b d5 ee c9  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0xffffffffffd5edcc>
802c1234: 38 7f 0f 78  	addi 3, 31, 3960
802c1238: 38 81 00 14  	addi 4, 1, 20
802c123c: 48 12 11 a5  	bl 0x803e23e0 isExistMapCollision__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0x1210b4>
802c1240: 2c 03 00 00  	cmpwi	3, 0
802c1244: 41 82 00 d4  	bt	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c1248: 38 61 00 08  	addi 3, 1, 8
802c124c: 38 9f 02 ac  	addi 4, 31, 684
802c1250: 4b d5 7c a1  	bl 0x80018ef0 __ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0xffffffffffd57bc4>
802c1254: 38 61 00 08  	addi 3, 1, 8
802c1258: 38 9f 0f 78  	addi 4, 31, 3960
802c125c: 4b d5 ee 9d  	bl 0x800200f8 __ami__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0xffffffffffd5edcc>
802c1260: 38 7f 0f 78  	addi 3, 31, 3960
802c1264: 38 81 00 08  	addi 4, 1, 8
802c1268: 48 12 11 79  	bl 0x803e23e0 isExistMapCollision__2MRFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f> <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0x1210b4>
802c126c: 2c 03 00 00  	cmpwi	3, 0
802c1270: 41 82 00 a8  	bt	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c1274: 80 7f 02 30  	lwz 3, 560(31)
802c1278: 88 03 07 35  	lbz 0, 1845(3)
802c127c: 2c 00 00 00  	cmpwi	0, 0
802c1280: 40 82 00 98  	bf	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c1284: a0 1f 03 d4  	lhz 0, 980(31)
802c1288: 28 00 00 06  	cmplwi	0, 6
802c128c: 41 82 00 8c  	bt	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c1290: 38 80 00 12  	li 4, 18
802c1294: 48 03 34 3d  	bl 0x802f46d0 isStatusActive__5MarioCFUl <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0x333a4>
802c1298: 2c 03 00 00  	cmpwi	3, 0
802c129c: 41 82 00 10  	bt	2, 0x802c12ac <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x100>
802c12a0: 88 1f 01 a1  	lbz 0, 417(31)
802c12a4: 2c 00 00 00  	cmpwi	0, 0
802c12a8: 40 82 00 70  	bf	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c12ac: 88 1f 0e a4  	lbz 0, 3748(31)
802c12b0: 2c 00 00 00  	cmpwi	0, 0
802c12b4: 40 82 00 64  	bf	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c12b8: 88 1f 09 34  	lbz 0, 2356(31)
802c12bc: 2c 00 00 00  	cmpwi	0, 0
802c12c0: 40 82 00 58  	bf	2, 0x802c1318 <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_start+0x16c>
802c12c4: 7f e3 fb 78  	mr	3, 31
802c12c8: 4b ff 5f 89  	bl 0x802b7250 getSimpleModel__10MarioActorCFv <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0xffffffffffff5f24>
802c12cc: 7c 7f 1b 78  	mr	31, 3
802c12d0: 38 80 00 01  	li 4, 1
802c12d4: 4b fe 58 55  	bl 0x802a6b28 setDrawView__9J3DModelXFUl <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0xfffffffffffe57fc>
802c12d8: 38 00 00 00  	li 0, 0
802c12dc: 7f e3 fb 78  	mr	3, 31
802c12e0: 60 00 00 08  	ori 0, 0, 8
802c12e4: 38 80 00 00  	li 4, 0
802c12e8: 90 1f 01 b0  	stw 0, 432(31)
802c12ec: 4b fe 5d fd  	bl 0x802a70e8 directDraw__9J3DModelXFP8J3DModel <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0xfffffffffffe5dbc>
802c12f0: 80 1f 01 b0  	lwz 0, 432(31)
802c12f4: 38 60 00 01  	li 3, 1
802c12f8: 54 00 07 76  	rlwinm 0, 0, 0, 29, 27
802c12fc: 90 1f 01 b0  	stw 0, 432(31)
802c1300: 48 1f ec 95  	bl 0x804bff94 GXSetAlphaUpdate <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0x1fec68>
802c1304: 38 60 00 01  	li 3, 1
802c1308: 48 1f ec 61  	bl 0x804bff68 GXSetColorUpdate <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0x1fec3c>
802c130c: 38 60 00 01  	li 3, 1
802c1310: 38 80 00 00  	li 4, 0
802c1314: 48 1f ed e5  	bl 0x804c00f8 GXSetDstAlpha <_binary_build_mario_shadow_view_20260903_drawSilhouette_bin_end+0x1fedcc>
802c1318: 80 01 00 34  	lwz 0, 52(1)
802c131c: 83 e1 00 2c  	lwz 31, 44(1)
802c1320: 7c 08 03 a6  	mtlr 0
802c1324: 38 21 00 30  	addi 1, 1, 48
802c1328: 4e 80 00 20  	blr
