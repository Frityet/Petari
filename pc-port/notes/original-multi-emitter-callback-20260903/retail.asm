
/Users/frityet/Projects/petari/build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Effect/MultiEmitterCallBack.o:	file format elf32-powerpc

Disassembly of section .text:

00000000 <mul__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>>:
       0: e0 63 00 00  	lq 3, 0(3)
       4: e0 44 00 00  	lq 2, 0(4)
       8: c0 23 00 08  	lfs 1, 8(3)
       c: c0 04 00 08  	lfs 0, 8(4)
      10: 10 43 00 b2  	<unknown>
      14: ec 01 00 32  	fmuls 0, 1, 0
      18: f0 43 00 00  	xsaddsp 2, 3, 0
      1c: d0 03 00 08  	stfs 0, 8(3)
      20: 4e 80 00 20  	blr

00000024 <getSyntheticColor__34@unnamed@MultiEmitterCallBack_cpp@FRC6Color8RC6Color8>:
      24: 94 21 ff f0  	stwu 1, -16(1)
      28: 7c 08 02 a6  	mflr 0
      2c: 7c 6b 1b 78  	mr	11, 3
      30: 88 a3 00 00  	lbz 5, 0(3)
      34: 90 01 00 14  	stw 0, 20(1)
      38: 39 20 00 ff  	li 9, 255
      3c: 88 04 00 00  	lbz 0, 0(4)
      40: 38 61 00 08  	addi 3, 1, 8
      44: 89 0b 00 01  	lbz 8, 1(11)
      48: 38 e0 00 ff  	li 7, 255
      4c: 7d 45 01 d6  	mullw 10, 5, 0
      50: 88 c4 00 01  	lbz 6, 1(4)
      54: 88 04 00 02  	lbz 0, 2(4)
      58: 88 ab 00 02  	lbz 5, 2(11)
      5c: 7c 88 31 d6  	mullw 4, 8, 6
      60: 7c 05 01 d6  	mullw 0, 5, 0
      64: 7c a4 4b d6  	divw 5, 4, 9
      68: 7c ca 4b d6  	divw 6, 10, 9
      6c: 54 a5 06 3e  	clrlwi	5, 5, 24
      70: 7c 00 4b d6  	divw 0, 0, 9
      74: 54 c4 06 3e  	clrlwi	4, 6, 24
      78: 54 06 06 3e  	clrlwi	6, 0, 24
      7c: 48 00 00 01  	bl 0x7c <getSyntheticColor__34@unnamed@MultiEmitterCallBack_cpp@FRC6Color8RC6Color8+0x58>
			0000007c:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
      80: 80 01 00 14  	lwz 0, 20(1)
      84: 80 63 00 00  	lwz 3, 0(3)
      88: 7c 08 03 a6  	mtlr 0
      8c: 38 21 00 10  	addi 1, 1, 16
      90: 4e 80 00 20  	blr

00000094 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>>:
      94: 94 21 ff e0  	stwu 1, -32(1)
      98: 7c 08 02 a6  	mflr 0
      9c: 90 01 00 24  	stw 0, 36(1)
      a0: 39 61 00 20  	addi 11, 1, 32
      a4: 48 00 00 01  	bl 0xa4 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x10>
			000000a4:  R_PPC_REL24	_savegpr_28
      a8: 7c 7c 1b 78  	mr	28, 3
      ac: 7c 9d 23 78  	mr	29, 4
      b0: 7c be 2b 78  	mr	30, 5
      b4: 48 00 00 01  	bl 0xb4 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x20>
			000000b4:  R_PPC_REL24	__ct__24MultiEmitterCallBackBaseFv
      b8: 3c a0 00 00  	lis 5, 0
			000000ba:  R_PPC_ADDR16_HA	__vt__20MultiEmitterCallBack
      bc: 3b e0 00 00  	li 31, 0
      c0: 38 a5 00 00  	addi 5, 5, 0
			000000c2:  R_PPC_ADDR16_LO	__vt__20MultiEmitterCallBack
      c4: 93 bc 00 04  	stw 29, 4(28)
      c8: 7f c4 f3 78  	mr	4, 30
      cc: 38 7c 00 18  	addi 3, 28, 24
      d0: 90 bc 00 00  	stw 5, 0(28)
      d4: 93 fc 00 08  	stw 31, 8(28)
      d8: 93 fc 00 0c  	stw 31, 12(28)
      dc: 93 fc 00 10  	stw 31, 16(28)
      e0: 93 fc 00 14  	stw 31, 20(28)
      e4: 48 00 00 01  	bl 0xe4 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x50>
			000000e4:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
      e8: c0 00 00 00  	lfs 0, 0(0)
			000000e8:  Unknown	@55723
      ec: 38 7c 00 28  	addi 3, 28, 40
      f0: 38 80 00 ff  	li 4, 255
      f4: 38 a0 00 ff  	li 5, 255
      f8: d0 1c 00 24  	stfs 0, 36(28)
      fc: 38 c0 00 ff  	li 6, 255
     100: 38 e0 00 ff  	li 7, 255
     104: 48 00 00 01  	bl 0x104 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x70>
			00000104:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     108: 38 7c 00 2c  	addi 3, 28, 44
     10c: 38 80 00 ff  	li 4, 255
     110: 38 a0 00 ff  	li 5, 255
     114: 38 c0 00 ff  	li 6, 255
     118: 38 e0 00 ff  	li 7, 255
     11c: 48 00 00 01  	bl 0x11c <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x88>
			0000011c:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     120: b3 fc 00 30  	sth 31, 48(28)
     124: 39 61 00 20  	addi 11, 1, 32
     128: 7f 83 e3 78  	mr	3, 28
     12c: 48 00 00 01  	bl 0x12c <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x98>
			0000012c:  R_PPC_REL24	_restgpr_28
     130: 80 01 00 24  	lwz 0, 36(1)
     134: 7c 08 03 a6  	mtlr 0
     138: 38 21 00 20  	addi 1, 1, 32
     13c: 4e 80 00 20  	blr

00000140 <__ct__24MultiEmitterCallBackBaseFv>:
     140: 3c 80 00 00  	lis 4, 0
			00000142:  R_PPC_ADDR16_HA	__vt__24MultiEmitterCallBackBase
     144: 38 84 00 00  	addi 4, 4, 0
			00000146:  R_PPC_ADDR16_LO	__vt__24MultiEmitterCallBackBase
     148: 90 83 00 00  	stw 4, 0(3)
     14c: 4e 80 00 20  	blr

00000150 <__dt__24MultiEmitterCallBackBaseFv>:
     150: 94 21 ff f0  	stwu 1, -16(1)
     154: 7c 08 02 a6  	mflr 0
     158: 2c 03 00 00  	cmpwi	3, 0
     15c: 90 01 00 14  	stw 0, 20(1)
     160: 93 e1 00 0c  	stw 31, 12(1)
     164: 7c 9f 23 78  	mr	31, 4
     168: 93 c1 00 08  	stw 30, 8(1)
     16c: 7c 7e 1b 78  	mr	30, 3
     170: 41 82 00 1c  	bt	2, 0x18c <__dt__24MultiEmitterCallBackBaseFv+0x3c>
     174: 38 80 00 00  	li 4, 0
     178: 48 00 00 01  	bl 0x178 <__dt__24MultiEmitterCallBackBaseFv+0x28>
			00000178:  R_PPC_REL24	__dt__18JPAEmitterCallBackFv
     17c: 2c 1f 00 00  	cmpwi	31, 0
     180: 40 81 00 0c  	bf	1, 0x18c <__dt__24MultiEmitterCallBackBaseFv+0x3c>
     184: 7f c3 f3 78  	mr	3, 30
     188: 48 00 00 01  	bl 0x188 <__dt__24MultiEmitterCallBackBaseFv+0x38>
			00000188:  R_PPC_REL24	__dl__FPv
     18c: 7f c3 f3 78  	mr	3, 30
     190: 83 e1 00 0c  	lwz 31, 12(1)
     194: 83 c1 00 08  	lwz 30, 8(1)
     198: 80 01 00 14  	lwz 0, 20(1)
     19c: 7c 08 03 a6  	mtlr 0
     1a0: 38 21 00 10  	addi 1, 1, 16
     1a4: 4e 80 00 20  	blr

000001a8 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>>:
     1a8: 94 21 ff d0  	stwu 1, -48(1)
     1ac: 7c 08 02 a6  	mflr 0
     1b0: 90 01 00 34  	stw 0, 52(1)
     1b4: 39 61 00 30  	addi 11, 1, 48
     1b8: 48 00 00 01  	bl 0x1b8 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x10>
			000001b8:  R_PPC_REL24	_savegpr_25
     1bc: 7c 79 1b 78  	mr	25, 3
     1c0: 7c 9a 23 78  	mr	26, 4
     1c4: 7c bb 2b 78  	mr	27, 5
     1c8: 7c dc 33 78  	mr	28, 6
     1cc: 7c fd 3b 78  	mr	29, 7
     1d0: 7d 1e 43 78  	mr	30, 8
     1d4: 48 00 00 01  	bl 0x1d4 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x2c>
			000001d4:  R_PPC_REL24	__ct__24MultiEmitterCallBackBaseFv
     1d8: 3c a0 00 00  	lis 5, 0
			000001da:  R_PPC_ADDR16_HA	__vt__20MultiEmitterCallBack
     1dc: 3b e0 00 00  	li 31, 0
     1e0: 38 a5 00 00  	addi 5, 5, 0
			000001e2:  R_PPC_ADDR16_LO	__vt__20MultiEmitterCallBack
     1e4: 93 59 00 04  	stw 26, 4(25)
     1e8: 7f c4 f3 78  	mr	4, 30
     1ec: 38 79 00 18  	addi 3, 25, 24
     1f0: 90 b9 00 00  	stw 5, 0(25)
     1f4: 93 79 00 08  	stw 27, 8(25)
     1f8: 93 99 00 0c  	stw 28, 12(25)
     1fc: 93 b9 00 10  	stw 29, 16(25)
     200: 93 f9 00 14  	stw 31, 20(25)
     204: 48 00 00 01  	bl 0x204 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x5c>
			00000204:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     208: c0 00 00 00  	lfs 0, 0(0)
			00000208:  Unknown	@55723
     20c: 38 79 00 28  	addi 3, 25, 40
     210: 38 80 00 ff  	li 4, 255
     214: 38 a0 00 ff  	li 5, 255
     218: d0 19 00 24  	stfs 0, 36(25)
     21c: 38 c0 00 ff  	li 6, 255
     220: 38 e0 00 ff  	li 7, 255
     224: 48 00 00 01  	bl 0x224 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x7c>
			00000224:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     228: 38 79 00 2c  	addi 3, 25, 44
     22c: 38 80 00 ff  	li 4, 255
     230: 38 a0 00 ff  	li 5, 255
     234: 38 c0 00 ff  	li 6, 255
     238: 38 e0 00 ff  	li 7, 255
     23c: 48 00 00 01  	bl 0x23c <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x94>
			0000023c:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     240: b3 f9 00 30  	sth 31, 48(25)
     244: 39 61 00 30  	addi 11, 1, 48
     248: 7f 23 cb 78  	mr	3, 25
     24c: 48 00 00 01  	bl 0x24c <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0xa4>
			0000024c:  R_PPC_REL24	_restgpr_25
     250: 80 01 00 34  	lwz 0, 52(1)
     254: 7c 08 03 a6  	mtlr 0
     258: 38 21 00 30  	addi 1, 1, 48
     25c: 4e 80 00 20  	blr

00000260 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>>:
     260: 94 21 ff e0  	stwu 1, -32(1)
     264: 7c 08 02 a6  	mflr 0
     268: 90 01 00 24  	stw 0, 36(1)
     26c: 39 61 00 20  	addi 11, 1, 32
     270: 48 00 00 01  	bl 0x270 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x10>
			00000270:  R_PPC_REL24	_savegpr_27
     274: 7c 7b 1b 78  	mr	27, 3
     278: 7c 9c 23 78  	mr	28, 4
     27c: 7c bd 2b 78  	mr	29, 5
     280: 7c de 33 78  	mr	30, 6
     284: 48 00 00 01  	bl 0x284 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x24>
			00000284:  R_PPC_REL24	__ct__24MultiEmitterCallBackBaseFv
     288: 3c a0 00 00  	lis 5, 0
			0000028a:  R_PPC_ADDR16_HA	__vt__20MultiEmitterCallBack
     28c: 3b e0 00 00  	li 31, 0
     290: 38 a5 00 00  	addi 5, 5, 0
			00000292:  R_PPC_ADDR16_LO	__vt__20MultiEmitterCallBack
     294: 93 9b 00 04  	stw 28, 4(27)
     298: 7f c4 f3 78  	mr	4, 30
     29c: 38 7b 00 18  	addi 3, 27, 24
     2a0: 90 bb 00 00  	stw 5, 0(27)
     2a4: 93 fb 00 08  	stw 31, 8(27)
     2a8: 93 fb 00 0c  	stw 31, 12(27)
     2ac: 93 fb 00 10  	stw 31, 16(27)
     2b0: 93 bb 00 14  	stw 29, 20(27)
     2b4: 48 00 00 01  	bl 0x2b4 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x54>
			000002b4:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     2b8: c0 00 00 00  	lfs 0, 0(0)
			000002b8:  Unknown	@55723
     2bc: 38 7b 00 28  	addi 3, 27, 40
     2c0: 38 80 00 ff  	li 4, 255
     2c4: 38 a0 00 ff  	li 5, 255
     2c8: d0 1b 00 24  	stfs 0, 36(27)
     2cc: 38 c0 00 ff  	li 6, 255
     2d0: 38 e0 00 ff  	li 7, 255
     2d4: 48 00 00 01  	bl 0x2d4 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x74>
			000002d4:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     2d8: 38 7b 00 2c  	addi 3, 27, 44
     2dc: 38 80 00 ff  	li 4, 255
     2e0: 38 a0 00 ff  	li 5, 255
     2e4: 38 c0 00 ff  	li 6, 255
     2e8: 38 e0 00 ff  	li 7, 255
     2ec: 48 00 00 01  	bl 0x2ec <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x8c>
			000002ec:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     2f0: b3 fb 00 30  	sth 31, 48(27)
     2f4: 39 61 00 20  	addi 11, 1, 32
     2f8: 7f 63 db 78  	mr	3, 27
     2fc: 48 00 00 01  	bl 0x2fc <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x9c>
			000002fc:  R_PPC_REL24	_restgpr_27
     300: 80 01 00 24  	lwz 0, 36(1)
     304: 7c 08 03 a6  	mtlr 0
     308: 38 21 00 20  	addi 1, 1, 32
     30c: 4e 80 00 20  	blr

00000310 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>>:
     310: 94 21 ff e0  	stwu 1, -32(1)
     314: 7c 08 02 a6  	mflr 0
     318: 90 01 00 24  	stw 0, 36(1)
     31c: 39 61 00 20  	addi 11, 1, 32
     320: 48 00 00 01  	bl 0x320 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x10>
			00000320:  R_PPC_REL24	_savegpr_26
     324: 7c 7a 1b 78  	mr	26, 3
     328: 7c 9b 23 78  	mr	27, 4
     32c: 7c bc 2b 78  	mr	28, 5
     330: 7c dd 33 78  	mr	29, 6
     334: 7c fe 3b 78  	mr	30, 7
     338: 48 00 00 01  	bl 0x338 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x28>
			00000338:  R_PPC_REL24	__ct__24MultiEmitterCallBackBaseFv
     33c: 3c a0 00 00  	lis 5, 0
			0000033e:  R_PPC_ADDR16_HA	__vt__20MultiEmitterCallBack
     340: 3b e0 00 00  	li 31, 0
     344: 38 a5 00 00  	addi 5, 5, 0
			00000346:  R_PPC_ADDR16_LO	__vt__20MultiEmitterCallBack
     348: 93 7a 00 04  	stw 27, 4(26)
     34c: 7f c4 f3 78  	mr	4, 30
     350: 38 7a 00 18  	addi 3, 26, 24
     354: 90 ba 00 00  	stw 5, 0(26)
     358: 93 fa 00 08  	stw 31, 8(26)
     35c: 93 fa 00 0c  	stw 31, 12(26)
     360: 93 ba 00 10  	stw 29, 16(26)
     364: 93 9a 00 14  	stw 28, 20(26)
     368: 48 00 00 01  	bl 0x368 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x58>
			00000368:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     36c: c0 00 00 00  	lfs 0, 0(0)
			0000036c:  Unknown	@55723
     370: 38 7a 00 28  	addi 3, 26, 40
     374: 38 80 00 ff  	li 4, 255
     378: 38 a0 00 ff  	li 5, 255
     37c: d0 1a 00 24  	stfs 0, 36(26)
     380: 38 c0 00 ff  	li 6, 255
     384: 38 e0 00 ff  	li 7, 255
     388: 48 00 00 01  	bl 0x388 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x78>
			00000388:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     38c: 38 7a 00 2c  	addi 3, 26, 44
     390: 38 80 00 ff  	li 4, 255
     394: 38 a0 00 ff  	li 5, 255
     398: 38 c0 00 ff  	li 6, 255
     39c: 38 e0 00 ff  	li 7, 255
     3a0: 48 00 00 01  	bl 0x3a0 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x90>
			000003a0:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     3a4: b3 fa 00 30  	sth 31, 48(26)
     3a8: 39 61 00 20  	addi 11, 1, 32
     3ac: 7f 43 d3 78  	mr	3, 26
     3b0: 48 00 00 01  	bl 0x3b0 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0xa0>
			000003b0:  R_PPC_REL24	_restgpr_26
     3b4: 80 01 00 24  	lwz 0, 36(1)
     3b8: 7c 08 03 a6  	mtlr 0
     3bc: 38 21 00 20  	addi 1, 1, 32
     3c0: 4e 80 00 20  	blr

000003c4 <execute__20MultiEmitterCallBackFP14JPABaseEmitter>:
     3c4: 94 21 ff f0  	stwu 1, -16(1)
     3c8: 7c 08 02 a6  	mflr 0
     3cc: 38 a0 00 00  	li 5, 0
     3d0: 90 01 00 14  	stw 0, 20(1)
     3d4: 93 e1 00 0c  	stw 31, 12(1)
     3d8: 7c 9f 23 78  	mr	31, 4
     3dc: 93 c1 00 08  	stw 30, 8(1)
     3e0: 7c 7e 1b 78  	mr	30, 3
     3e4: 48 00 00 01  	bl 0x3e4 <execute__20MultiEmitterCallBackFP14JPABaseEmitter+0x20>
			000003e4:  R_PPC_REL24	followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb
     3e8: 7f c3 f3 78  	mr	3, 30
     3ec: 7f e4 fb 78  	mr	4, 31
     3f0: 48 00 00 01  	bl 0x3f0 <execute__20MultiEmitterCallBackFP14JPABaseEmitter+0x2c>
			000003f0:  R_PPC_REL24	effectLight__20MultiEmitterCallBackFP14JPABaseEmitter
     3f4: 7f c3 f3 78  	mr	3, 30
     3f8: 7f e4 fb 78  	mr	4, 31
     3fc: 48 00 00 01  	bl 0x3fc <execute__20MultiEmitterCallBackFP14JPABaseEmitter+0x38>
			000003fc:  R_PPC_REL24	setColor__20MultiEmitterCallBackFP14JPABaseEmitter
     400: 80 01 00 14  	lwz 0, 20(1)
     404: 83 e1 00 0c  	lwz 31, 12(1)
     408: 83 c1 00 08  	lwz 30, 8(1)
     40c: 7c 08 03 a6  	mtlr 0
     410: 38 21 00 10  	addi 1, 1, 16
     414: 4e 80 00 20  	blr

00000418 <setHostSRT__20MultiEmitterCallBackFPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>>:
     418: 38 00 00 00  	li 0, 0
     41c: 90 83 00 08  	stw 4, 8(3)
     420: 90 a3 00 0c  	stw 5, 12(3)
     424: 90 c3 00 10  	stw 6, 16(3)
     428: 90 03 00 14  	stw 0, 20(3)
     42c: 4e 80 00 20  	blr

00000430 <setHostMtx__20MultiEmitterCallBackFPA4_f>:
     430: 38 00 00 00  	li 0, 0
     434: 90 83 00 14  	stw 4, 20(3)
     438: 90 03 00 08  	stw 0, 8(3)
     43c: 90 03 00 0c  	stw 0, 12(3)
     440: 90 03 00 10  	stw 0, 16(3)
     444: 4e 80 00 20  	blr

00000448 <setBaseScale__20MultiEmitterCallBackFf>:
     448: a0 03 00 30  	lhz 0, 48(3)
     44c: 54 00 05 ee  	rlwinm 0, 0, 0, 23, 23
     450: 28 00 01 00  	cmplwi	0, 256
     454: 41 82 00 10  	bt	2, 0x464 <setBaseScale__20MultiEmitterCallBackFf+0x1c>
     458: a0 03 00 30  	lhz 0, 48(3)
     45c: 60 00 01 00  	ori 0, 0, 256
     460: b0 03 00 30  	sth 0, 48(3)
     464: d0 23 00 24  	stfs 1, 36(3)
     468: 4e 80 00 20  	blr

0000046c <forceFollowOn__20MultiEmitterCallBackFv>:
     46c: a0 03 00 30  	lhz 0, 48(3)
     470: 60 00 00 01  	ori 0, 0, 1
     474: b0 03 00 30  	sth 0, 48(3)
     478: 4e 80 00 20  	blr

0000047c <forceFollowOff__20MultiEmitterCallBackFv>:
     47c: a0 03 00 30  	lhz 0, 48(3)
     480: 60 00 00 02  	ori 0, 0, 2
     484: b0 03 00 30  	sth 0, 48(3)
     488: 4e 80 00 20  	blr

0000048c <forceScaleOn__20MultiEmitterCallBackFv>:
     48c: a0 03 00 30  	lhz 0, 48(3)
     490: 60 00 00 10  	ori 0, 0, 16
     494: b0 03 00 30  	sth 0, 48(3)
     498: 4e 80 00 20  	blr

0000049c <resetFollowCurrent__20MultiEmitterCallBackFv>:
     49c: a0 03 00 30  	lhz 0, 48(3)
     4a0: 54 00 06 b0  	rlwinm 0, 0, 0, 26, 24
     4a4: b0 03 00 30  	sth 0, 48(3)
     4a8: 4e 80 00 20  	blr

000004ac <init__20MultiEmitterCallBackFP14JPABaseEmitter>:
     4ac: 38 a0 00 01  	li 5, 1
     4b0: 48 00 00 00  	b 0x4b0 <init__20MultiEmitterCallBackFP14JPABaseEmitter+0x4>
			000004b0:  R_PPC_REL24	followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb

000004b4 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb>:
     4b4: 94 21 ff 10  	stwu 1, -240(1)
     4b8: 7c 08 02 a6  	mflr 0
     4bc: 90 01 00 f4  	stw 0, 244(1)
     4c0: db e1 00 e0  	stfd 31, 224(1)
     4c4: f3 e1 00 e8  	<unknown>
     4c8: 39 61 00 e0  	addi 11, 1, 224
     4cc: 48 00 00 01  	bl 0x4cc <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x18>
			000004cc:  R_PPC_REL24	_savegpr_27
     4d0: e0 05 00 00  	lq 0, 0(5)
     4d4: 7c 9c 23 78  	mr	28, 4
     4d8: e0 25 00 08  	<unknown>
     4dc: 39 01 00 8c  	addi 8, 1, 140
     4e0: e0 45 00 10  	lq 2, 16(5)
     4e4: 7c 7b 1b 78  	mr	27, 3
     4e8: 7c dd 33 78  	mr	29, 6
     4ec: e0 65 00 18  	<unknown>
     4f0: e0 85 00 20  	lq 4, 32(5)
     4f4: 7c fe 3b 78  	mr	30, 7
     4f8: e0 a5 00 28  	<unknown>
     4fc: 7d 03 43 78  	mr	3, 8
     500: 38 81 00 5c  	addi 4, 1, 92
     504: 38 a1 00 14  	addi 5, 1, 20
     508: f0 08 00 00  	xsaddsp 0, 8, 0
     50c: 38 c1 00 20  	addi 6, 1, 32
     510: f0 28 00 08  	xsmaddasp 1, 8, 0
     514: f0 48 00 10  	xxsldwi 2, 8, 0, 0
     518: f0 68 00 18  	xscmpeqdp 3, 8, 0
     51c: f0 88 00 20  	<unknown>
     520: f0 a8 00 28  	<unknown>
     524: 48 00 00 01  	bl 0x524 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x70>
			00000524:  R_PPC_REL24	JPASetRMtxSTVecfromMtx__FPA4_CfPA4_fPQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f>
     528: 88 1d 00 00  	lbz 0, 0(29)
     52c: 2c 00 00 00  	cmpwi	0, 0
     530: 41 82 00 44  	bt	2, 0x574 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xc0>
     534: 38 61 00 5c  	addi 3, 1, 92
     538: 38 9b 00 18  	addi 4, 27, 24
     53c: 38 a1 00 08  	addi 5, 1, 8
     540: 48 00 00 01  	bl 0x540 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x8c>
			00000540:  R_PPC_REL24	mult33__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRCQ29JGeometry8TVec3<f>RQ29JGeometry8TVec3<f>
     544: 88 1d 00 02  	lbz 0, 2(29)
     548: 2c 00 00 00  	cmpwi	0, 0
     54c: 41 82 00 10  	bt	2, 0x55c <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xa8>
     550: 38 61 00 08  	addi 3, 1, 8
     554: 38 81 00 14  	addi 4, 1, 20
     558: 48 00 00 01  	bl 0x558 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xa4>
			00000558:  R_PPC_REL24	mul__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     55c: 38 61 00 20  	addi 3, 1, 32
     560: 38 81 00 08  	addi 4, 1, 8
     564: 48 00 00 01  	bl 0x564 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xb0>
			00000564:  R_PPC_REL24	add__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     568: 38 7c 00 a4  	addi 3, 28, 164
     56c: 38 81 00 20  	addi 4, 1, 32
     570: 48 00 00 01  	bl 0x570 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xbc>
			00000570:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     574: 88 1d 00 01  	lbz 0, 1(29)
     578: 2c 00 00 00  	cmpwi	0, 0
     57c: 41 82 00 80  	bt	2, 0x5fc <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x148>
     580: 80 7b 00 04  	lwz 3, 4(27)
     584: 48 00 00 01  	bl 0x584 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xd0>
			00000584:  R_PPC_REL24	isEffect2D__Q22MR6EffectFPC12MultiEmitter
     588: 2c 03 00 00  	cmpwi	3, 0
     58c: 41 82 00 64  	bt	2, 0x5f0 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x13c>
     590: 38 61 00 2c  	addi 3, 1, 44
     594: 48 00 00 01  	bl 0x594 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xe0>
			00000594:  R_PPC_REL24	identity__Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>Fv
     598: 3f e0 00 00  	lis 31, 0
			0000059a:  R_PPC_ADDR16_HA	lbl_80531A98
     59c: c8 3f 00 00  	lfd 1, 0(31)
			0000059e:  R_PPC_ADDR16_LO	lbl_80531A98
     5a0: 48 00 00 01  	bl 0x5a0 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xec>
			000005a0:  R_PPC_REL24	sin
     5a4: ff e0 08 18  	frsp 31, 1
     5a8: c8 3f 00 00  	lfd 1, 0(31)
			000005aa:  R_PPC_ADDR16_LO	lbl_80531A98
     5ac: 48 00 00 01  	bl 0x5ac <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xf8>
			000005ac:  R_PPC_REL24	cos
     5b0: fc 60 08 18  	frsp 3, 1
     5b4: c0 00 00 00  	lfs 0, 0(0)
			000005b4:  Unknown	@55724
     5b8: fc 40 f8 50  	fneg 2, 31
     5bc: c0 20 00 00  	lfs 1, 0(0)
			000005bc:  Unknown	@55723
     5c0: d3 e1 00 3c  	stfs 31, 60(1)
     5c4: 38 61 00 5c  	addi 3, 1, 92
     5c8: d0 61 00 2c  	stfs 3, 44(1)
     5cc: 38 81 00 2c  	addi 4, 1, 44
     5d0: d0 41 00 30  	stfs 2, 48(1)
     5d4: d0 61 00 40  	stfs 3, 64(1)
     5d8: d0 21 00 54  	stfs 1, 84(1)
     5dc: d0 01 00 50  	stfs 0, 80(1)
     5e0: d0 01 00 44  	stfs 0, 68(1)
     5e4: d0 01 00 4c  	stfs 0, 76(1)
     5e8: d0 01 00 34  	stfs 0, 52(1)
     5ec: 48 00 00 01  	bl 0x5ec <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x138>
			000005ec:  R_PPC_REL24	concat__Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>FRCQ29JGeometry13SMatrix34C<f>
     5f0: 38 61 00 5c  	addi 3, 1, 92
     5f4: 38 9c 00 68  	addi 4, 28, 104
     5f8: 48 00 00 01  	bl 0x5f8 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x144>
			000005f8:  R_PPC_REL24	JPASetRMtxfromMtx__FPA4_CfPA4_f
     5fc: 88 dd 00 02  	lbz 6, 2(29)
     600: 7f 63 db 78  	mr	3, 27
     604: 7f 84 e3 78  	mr	4, 28
     608: 7f c7 f3 78  	mr	7, 30
     60c: 38 a1 00 14  	addi 5, 1, 20
     610: 48 00 00 01  	bl 0x610 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x15c>
			00000610:  R_PPC_REL24	setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb
     614: e3 e1 00 e8  	<unknown>
     618: 39 61 00 e0  	addi 11, 1, 224
     61c: cb e1 00 e0  	lfd 31, 224(1)
     620: 48 00 00 01  	bl 0x620 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x16c>
			00000620:  R_PPC_REL24	_restgpr_27
     624: 80 01 00 f4  	lwz 0, 244(1)
     628: 7c 08 03 a6  	mtlr 0
     62c: 38 21 00 f0  	addi 1, 1, 240
     630: 4e 80 00 20  	blr

00000634 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb>:
     634: 94 21 ff 00  	stwu 1, -256(1)
     638: 7c 08 02 a6  	mflr 0
     63c: 90 01 01 04  	stw 0, 260(1)
     640: db e1 00 f0  	stfd 31, 240(1)
     644: f3 e1 00 f8  	xxsel 31, 1, 0, 35
     648: db c1 00 e0  	stfd 30, 224(1)
     64c: f3 c1 00 e8  	<unknown>
     650: db a1 00 d0  	stfd 29, 208(1)
     654: f3 a1 00 d8  	<unknown>
     658: db 81 00 c0  	stfd 28, 192(1)
     65c: f3 81 00 c8  	xsmsubmsp 28, 1, 0
     660: db 61 00 b0  	stfd 27, 176(1)
     664: f3 61 00 b8  	xxsel 27, 1, 0, 34
     668: db 41 00 a0  	stfd 26, 160(1)
     66c: f3 41 00 a8  	<unknown>
     670: db 21 00 90  	stfd 25, 144(1)
     674: f3 21 00 98  	xscmpgedp 25, 1, 0
     678: 39 61 00 90  	addi 11, 1, 144
     67c: 48 00 00 01  	bl 0x67c <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x48>
			0000067c:  R_PPC_REL24	_savegpr_28
     680: 88 05 00 00  	lbz 0, 0(5)
     684: 7c 7c 1b 78  	mr	28, 3
     688: 7c 9d 23 78  	mr	29, 4
     68c: 7c be 2b 78  	mr	30, 5
     690: 2c 00 00 00  	cmpwi	0, 0
     694: 7c df 33 78  	mr	31, 6
     698: 41 82 01 48  	bt	2, 0x7e0 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x1ac>
     69c: 88 05 00 01  	lbz 0, 1(5)
     6a0: 2c 00 00 00  	cmpwi	0, 0
     6a4: 41 82 00 f8  	bt	2, 0x79c <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x168>
     6a8: 38 61 00 2c  	addi 3, 1, 44
     6ac: 48 00 00 01  	bl 0x6ac <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x78>
			000006ac:  R_PPC_REL24	identity__Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>Fv
     6b0: 80 9c 00 0c  	lwz 4, 12(28)
     6b4: 38 61 00 14  	addi 3, 1, 20
     6b8: 48 00 00 01  	bl 0x6b8 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x84>
			000006b8:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     6bc: c0 20 00 00  	lfs 1, 0(0)
			000006bc:  Unknown	@57184
     6c0: 38 61 00 14  	addi 3, 1, 20
     6c4: 48 00 00 01  	bl 0x6c4 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x90>
			000006c4:  R_PPC_REL24	scale__Q29JGeometry8TVec3<f>Ff
     6c8: c3 81 00 1c  	lfs 28, 28(1)
     6cc: c3 61 00 18  	lfs 27, 24(1)
     6d0: fc 20 e0 90  	fmr 1, 28
     6d4: c3 41 00 14  	lfs 26, 20(1)
     6d8: 48 00 00 01  	bl 0x6d8 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xa4>
			000006d8:  R_PPC_REL24	cos
     6dc: ff e0 08 18  	frsp 31, 1
     6e0: fc 20 d8 90  	fmr 1, 27
     6e4: 48 00 00 01  	bl 0x6e4 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xb0>
			000006e4:  R_PPC_REL24	cos
     6e8: ff c0 08 18  	frsp 30, 1
     6ec: fc 20 d0 90  	fmr 1, 26
     6f0: 48 00 00 01  	bl 0x6f0 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xbc>
			000006f0:  R_PPC_REL24	cos
     6f4: ff a0 08 18  	frsp 29, 1
     6f8: fc 20 e0 90  	fmr 1, 28
     6fc: 48 00 00 01  	bl 0x6fc <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xc8>
			000006fc:  R_PPC_REL24	sin
     700: ff 20 08 18  	frsp 25, 1
     704: fc 20 d8 90  	fmr 1, 27
     708: 48 00 00 01  	bl 0x708 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xd4>
			00000708:  R_PPC_REL24	sin
     70c: ff 80 08 18  	frsp 28, 1
     710: fc 20 d0 90  	fmr 1, 26
     714: 48 00 00 01  	bl 0x714 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xe0>
			00000714:  R_PPC_REL24	sin
     718: fd 20 08 18  	frsp 9, 1
     71c: 38 61 00 2c  	addi 3, 1, 44
     720: ec bd 07 f2  	fmuls 5, 29, 31
     724: 38 9c 00 18  	addi 4, 28, 24
     728: ec 1e 07 f2  	fmuls 0, 30, 31
     72c: 38 a1 00 20  	addi 5, 1, 32
     730: ec 49 07 32  	fmuls 2, 9, 28
     734: d0 01 00 2c  	stfs 0, 44(1)
     738: ec 9e 06 72  	fmuls 4, 30, 25
     73c: ed 1d 06 72  	fmuls 8, 29, 25
     740: ec 62 07 f2  	fmuls 3, 2, 31
     744: ec 25 07 32  	fmuls 1, 5, 28
     748: d0 81 00 3c  	stfs 4, 60(1)
     74c: ec 42 06 72  	fmuls 2, 2, 25
     750: ec c3 40 28  	fsubs 6, 3, 8
     754: ec 09 06 72  	fmuls 0, 9, 25
     758: ec a5 10 2a  	fadds 5, 5, 2
     75c: fc e0 e0 50  	fneg 7, 28
     760: d0 c1 00 30  	stfs 6, 48(1)
     764: ec 61 00 2a  	fadds 3, 1, 0
     768: ec 89 07 b2  	fmuls 4, 9, 30
     76c: d0 a1 00 40  	stfs 5, 64(1)
     770: ec 1d 07 b2  	fmuls 0, 29, 30
     774: ec 48 07 32  	fmuls 2, 8, 28
     778: d0 e1 00 4c  	stfs 7, 76(1)
     77c: ec 29 07 f2  	fmuls 1, 9, 31
     780: d0 81 00 50  	stfs 4, 80(1)
     784: ec 22 08 28  	fsubs 1, 2, 1
     788: d0 61 00 34  	stfs 3, 52(1)
     78c: d0 01 00 54  	stfs 0, 84(1)
     790: d0 21 00 44  	stfs 1, 68(1)
     794: 48 00 00 01  	bl 0x794 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x160>
			00000794:  R_PPC_REL24	mult33__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRCQ29JGeometry8TVec3<f>RQ29JGeometry8TVec3<f>
     798: 48 00 00 10  	b 0x7a8 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x174>
     79c: 38 61 00 20  	addi 3, 1, 32
     7a0: 38 9c 00 18  	addi 4, 28, 24
     7a4: 48 00 00 01  	bl 0x7a4 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x170>
			000007a4:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     7a8: 88 1e 00 02  	lbz 0, 2(30)
     7ac: 2c 00 00 00  	cmpwi	0, 0
     7b0: 41 82 00 18  	bt	2, 0x7c8 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x194>
     7b4: 80 9c 00 10  	lwz 4, 16(28)
     7b8: 2c 04 00 00  	cmpwi	4, 0
     7bc: 41 82 00 0c  	bt	2, 0x7c8 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x194>
     7c0: 38 61 00 20  	addi 3, 1, 32
     7c4: 48 00 00 01  	bl 0x7c4 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x190>
			000007c4:  R_PPC_REL24	mul__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     7c8: 80 9c 00 08  	lwz 4, 8(28)
     7cc: 38 61 00 20  	addi 3, 1, 32
     7d0: 48 00 00 01  	bl 0x7d0 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x19c>
			000007d0:  R_PPC_REL24	add__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     7d4: 38 7d 00 a4  	addi 3, 29, 164
     7d8: 38 81 00 20  	addi 4, 1, 32
     7dc: 48 00 00 01  	bl 0x7dc <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x1a8>
			000007dc:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     7e0: 88 1e 00 01  	lbz 0, 1(30)
     7e4: 2c 00 00 00  	cmpwi	0, 0
     7e8: 41 82 00 5c  	bt	2, 0x844 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x210>
     7ec: 80 7c 00 0c  	lwz 3, 12(28)
     7f0: 38 dd 00 68  	addi 6, 29, 104
     7f4: c0 00 00 00  	lfs 0, 0(0)
			000007f4:  Unknown	@57185
     7f8: c0 43 00 08  	lfs 2, 8(3)
     7fc: c0 23 00 04  	lfs 1, 4(3)
     800: ec 40 00 b2  	fmuls 2, 0, 2
     804: c0 63 00 00  	lfs 3, 0(3)
     808: ec 20 00 72  	fmuls 1, 0, 1
     80c: ec 00 00 f2  	fmuls 0, 0, 3
     810: fc 40 10 1e  	fctiwz 2, 2
     814: fc 20 08 1e  	fctiwz 1, 1
     818: fc 00 00 1e  	fctiwz 0, 0
     81c: d8 41 00 60  	stfd 2, 96(1)
     820: d8 21 00 68  	stfd 1, 104(1)
     824: 80 01 00 64  	lwz 0, 100(1)
     828: 80 61 00 6c  	lwz 3, 108(1)
     82c: d8 01 00 70  	stfd 0, 112(1)
     830: 7c 05 07 34  	extsh 5, 0
     834: 7c 64 07 34  	extsh 4, 3
     838: 80 01 00 74  	lwz 0, 116(1)
     83c: 7c 03 07 34  	extsh 3, 0
     840: 48 00 00 01  	bl 0x840 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x20c>
			00000840:  R_PPC_REL24	JPAGetXYZRotateMtx__FsssPA4_f
     844: c0 00 00 00  	lfs 0, 0(0)
			00000844:  Unknown	@55723
     848: 7f 83 e3 78  	mr	3, 28
     84c: 88 de 00 02  	lbz 6, 2(30)
     850: 7f a4 eb 78  	mr	4, 29
     854: d0 01 00 08  	stfs 0, 8(1)
     858: 7f e7 fb 78  	mr	7, 31
     85c: 38 a1 00 08  	addi 5, 1, 8
     860: d0 01 00 0c  	stfs 0, 12(1)
     864: d0 01 00 10  	stfs 0, 16(1)
     868: 48 00 00 01  	bl 0x868 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x234>
			00000868:  R_PPC_REL24	setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb
     86c: e3 e1 00 f8  	<unknown>
     870: cb e1 00 f0  	lfd 31, 240(1)
     874: e3 c1 00 e8  	<unknown>
     878: cb c1 00 e0  	lfd 30, 224(1)
     87c: e3 a1 00 d8  	<unknown>
     880: cb a1 00 d0  	lfd 29, 208(1)
     884: e3 81 00 c8  	<unknown>
     888: cb 81 00 c0  	lfd 28, 192(1)
     88c: e3 61 00 b8  	<unknown>
     890: cb 61 00 b0  	lfd 27, 176(1)
     894: e3 41 00 a8  	<unknown>
     898: cb 41 00 a0  	lfd 26, 160(1)
     89c: e3 21 00 98  	<unknown>
     8a0: 39 61 00 90  	addi 11, 1, 144
     8a4: cb 21 00 90  	lfd 25, 144(1)
     8a8: 48 00 00 01  	bl 0x8a8 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x274>
			000008a8:  R_PPC_REL24	_restgpr_28
     8ac: 80 01 01 04  	lwz 0, 260(1)
     8b0: 7c 08 03 a6  	mtlr 0
     8b4: 38 21 01 00  	addi 1, 1, 256
     8b8: 4e 80 00 20  	blr

000008bc <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb>:
     8bc: 94 21 ff d0  	stwu 1, -48(1)
     8c0: 7c 08 02 a6  	mflr 0
     8c4: 90 01 00 34  	stw 0, 52(1)
     8c8: 39 61 00 30  	addi 11, 1, 48
     8cc: 48 00 00 01  	bl 0x8cc <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x10>
			000008cc:  R_PPC_REL24	_savegpr_28
     8d0: 7c 7c 1b 78  	mr	28, 3
     8d4: 7c 9d 23 78  	mr	29, 4
     8d8: 7c a4 2b 78  	mr	4, 5
     8dc: 7c de 33 78  	mr	30, 6
     8e0: 7c ff 3b 78  	mr	31, 7
     8e4: 38 61 00 08  	addi 3, 1, 8
     8e8: 48 00 00 01  	bl 0x8e8 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x2c>
			000008e8:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     8ec: 2c 1e 00 00  	cmpwi	30, 0
     8f0: 41 82 00 44  	bt	2, 0x934 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x78>
     8f4: 80 9c 00 10  	lwz 4, 16(28)
     8f8: 2c 04 00 00  	cmpwi	4, 0
     8fc: 41 82 00 0c  	bt	2, 0x908 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x4c>
     900: 38 61 00 08  	addi 3, 1, 8
     904: 48 00 00 01  	bl 0x904 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x48>
			00000904:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     908: a0 1c 00 30  	lhz 0, 48(28)
     90c: 54 00 05 ee  	rlwinm 0, 0, 0, 23, 23
     910: 28 00 01 00  	cmplwi	0, 256
     914: 40 82 00 10  	bf	2, 0x924 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x68>
     918: c0 3c 00 24  	lfs 1, 36(28)
     91c: 38 61 00 08  	addi 3, 1, 8
     920: 48 00 00 01  	bl 0x920 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x64>
			00000920:  R_PPC_REL24	scale__Q29JGeometry8TVec3<f>Ff
     924: 7f a3 eb 78  	mr	3, 29
     928: 38 81 00 08  	addi 4, 1, 8
     92c: 48 00 00 01  	bl 0x92c <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x70>
			0000092c:  R_PPC_REL24	setGlobalScale__14JPABaseEmitterFRCQ29JGeometry8TVec3<f>
     930: 48 00 00 34  	b 0x964 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xa8>
     934: 2c 1f 00 00  	cmpwi	31, 0
     938: 41 82 00 2c  	bt	2, 0x964 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xa8>
     93c: a0 1c 00 30  	lhz 0, 48(28)
     940: 54 00 05 ee  	rlwinm 0, 0, 0, 23, 23
     944: 28 00 01 00  	cmplwi	0, 256
     948: 40 82 00 1c  	bf	2, 0x964 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xa8>
     94c: c0 3c 00 24  	lfs 1, 36(28)
     950: 38 61 00 08  	addi 3, 1, 8
     954: 48 00 00 01  	bl 0x954 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x98>
			00000954:  R_PPC_REL24	setAll<f>__Q29JGeometry8TVec3<f>Ff_v
     958: 7f a3 eb 78  	mr	3, 29
     95c: 38 81 00 08  	addi 4, 1, 8
     960: 48 00 00 01  	bl 0x960 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xa4>
			00000960:  R_PPC_REL24	setGlobalScale__14JPABaseEmitterFRCQ29JGeometry8TVec3<f>
     964: 39 61 00 30  	addi 11, 1, 48
     968: 48 00 00 01  	bl 0x968 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xac>
			00000968:  R_PPC_REL24	_restgpr_28
     96c: 80 01 00 34  	lwz 0, 52(1)
     970: 7c 08 03 a6  	mtlr 0
     974: 38 21 00 30  	addi 1, 1, 48
     978: 4e 80 00 20  	blr

0000097c <effectLight__20MultiEmitterCallBackFP14JPABaseEmitter>:
     97c: 94 21 ff f0  	stwu 1, -16(1)
     980: 7c 08 02 a6  	mflr 0
     984: c0 40 00 00  	lfs 2, 0(0)
			00000984:  Unknown	@57287
     988: 90 01 00 14  	stw 0, 20(1)
     98c: 80 63 00 04  	lwz 3, 4(3)
     990: c0 23 00 2c  	lfs 1, 44(3)
     994: 48 00 00 01  	bl 0x994 <effectLight__20MultiEmitterCallBackFP14JPABaseEmitter+0x18>
			00000994:  R_PPC_REL24	isNearZero__2MRFff
     998: 80 01 00 14  	lwz 0, 20(1)
     99c: 7c 08 03 a6  	mtlr 0
     9a0: 38 21 00 10  	addi 1, 1, 16
     9a4: 4e 80 00 20  	blr

000009a8 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb>:
     9a8: 94 21 ff e0  	stwu 1, -32(1)
     9ac: 7c 08 02 a6  	mflr 0
     9b0: 90 01 00 24  	stw 0, 36(1)
     9b4: 39 61 00 20  	addi 11, 1, 32
     9b8: 48 00 00 01  	bl 0x9b8 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x10>
			000009b8:  R_PPC_REL24	_savegpr_29
     9bc: 7c 9e 23 78  	mr	30, 4
     9c0: 7c 7d 1b 78  	mr	29, 3
     9c4: 7c bf 2b 78  	mr	31, 5
     9c8: 38 81 00 08  	addi 4, 1, 8
     9cc: 48 00 00 01  	bl 0x9cc <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x24>
			000009cc:  R_PPC_REL24	isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb
     9d0: 88 01 00 08  	lbz 0, 8(1)
     9d4: 38 60 00 00  	li 3, 0
     9d8: 2c 00 00 00  	cmpwi	0, 0
     9dc: 40 82 00 1c  	bf	2, 0x9f8 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x50>
     9e0: 88 01 00 09  	lbz 0, 9(1)
     9e4: 2c 00 00 00  	cmpwi	0, 0
     9e8: 40 82 00 10  	bf	2, 0x9f8 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x50>
     9ec: 88 01 00 0a  	lbz 0, 10(1)
     9f0: 2c 00 00 00  	cmpwi	0, 0
     9f4: 41 82 00 08  	bt	2, 0x9fc <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x54>
     9f8: 38 60 00 01  	li 3, 1
     9fc: 2c 03 00 00  	cmpwi	3, 0
     a00: 40 82 00 14  	bf	2, 0xa14 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x6c>
     a04: a0 1d 00 30  	lhz 0, 48(29)
     a08: 54 00 05 ee  	rlwinm 0, 0, 0, 23, 23
     a0c: 28 00 01 00  	cmplwi	0, 256
     a10: 40 82 00 3c  	bf	2, 0xa4c <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0xa4>
     a14: 80 bd 00 14  	lwz 5, 20(29)
     a18: 2c 05 00 00  	cmpwi	5, 0
     a1c: 41 82 00 1c  	bt	2, 0xa38 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x90>
     a20: 7f a3 eb 78  	mr	3, 29
     a24: 7f c4 f3 78  	mr	4, 30
     a28: 7f e7 fb 78  	mr	7, 31
     a2c: 38 c1 00 08  	addi 6, 1, 8
     a30: 48 00 00 01  	bl 0xa30 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x88>
			00000a30:  R_PPC_REL24	setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb
     a34: 48 00 00 18  	b 0xa4c <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0xa4>
     a38: 7f a3 eb 78  	mr	3, 29
     a3c: 7f c4 f3 78  	mr	4, 30
     a40: 7f e6 fb 78  	mr	6, 31
     a44: 38 a1 00 08  	addi 5, 1, 8
     a48: 48 00 00 01  	bl 0xa48 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0xa0>
			00000a48:  R_PPC_REL24	setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb
     a4c: 39 61 00 20  	addi 11, 1, 32
     a50: 48 00 00 01  	bl 0xa50 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0xa8>
			00000a50:  R_PPC_REL24	_restgpr_29
     a54: 80 01 00 24  	lwz 0, 36(1)
     a58: 7c 08 03 a6  	mtlr 0
     a5c: 38 21 00 20  	addi 1, 1, 32
     a60: 4e 80 00 20  	blr

00000a64 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter>:
     a64: 94 21 ff d0  	stwu 1, -48(1)
     a68: 7c 08 02 a6  	mflr 0
     a6c: c0 40 00 00  	lfs 2, 0(0)
			00000a6c:  Unknown	@57287
     a70: 90 01 00 34  	stw 0, 52(1)
     a74: 93 e1 00 2c  	stw 31, 44(1)
     a78: 7c 9f 23 78  	mr	31, 4
     a7c: 93 c1 00 28  	stw 30, 40(1)
     a80: 7c 7e 1b 78  	mr	30, 3
     a84: 80 a3 00 04  	lwz 5, 4(3)
     a88: c0 25 00 2c  	lfs 1, 44(5)
     a8c: 48 00 00 01  	bl 0xa8c <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x28>
			00000a8c:  R_PPC_REL24	isNearZero__2MRFff
     a90: 2c 03 00 00  	cmpwi	3, 0
     a94: 41 82 00 30  	bt	2, 0xac4 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x60>
     a98: 88 9e 00 28  	lbz 4, 40(30)
     a9c: 7f e3 fb 78  	mr	3, 31
     aa0: 88 be 00 29  	lbz 5, 41(30)
     aa4: 88 de 00 2a  	lbz 6, 42(30)
     aa8: 48 00 00 01  	bl 0xaa8 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x44>
			00000aa8:  R_PPC_REL24	setGlobalPrmColor__14JPABaseEmitterFUcUcUc
     aac: 88 9e 00 2c  	lbz 4, 44(30)
     ab0: 7f e3 fb 78  	mr	3, 31
     ab4: 88 be 00 2d  	lbz 5, 45(30)
     ab8: 88 de 00 2e  	lbz 6, 46(30)
     abc: 48 00 00 01  	bl 0xabc <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x58>
			00000abc:  R_PPC_REL24	setGlobalEnvColor__14JPABaseEmitterFUcUcUc
     ac0: 48 00 00 bc  	b 0xb7c <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x118>
     ac4: 38 61 00 24  	addi 3, 1, 36
     ac8: 38 9f 00 b8  	addi 4, 31, 184
     acc: 48 00 00 01  	bl 0xacc <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x68>
			00000acc:  R_PPC_REL24	__as__8_GXColorFRC8_GXColor
     ad0: 38 61 00 20  	addi 3, 1, 32
     ad4: 38 9f 00 bc  	addi 4, 31, 188
     ad8: 48 00 00 01  	bl 0xad8 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x74>
			00000ad8:  R_PPC_REL24	__as__8_GXColorFRC8_GXColor
     adc: 88 e1 00 24  	lbz 7, 36(1)
     ae0: 38 61 00 14  	addi 3, 1, 20
     ae4: 88 c1 00 25  	lbz 6, 37(1)
     ae8: 38 81 00 0c  	addi 4, 1, 12
     aec: 88 a1 00 26  	lbz 5, 38(1)
     af0: 88 01 00 27  	lbz 0, 39(1)
     af4: 98 e1 00 0c  	stb 7, 12(1)
     af8: 98 c1 00 0d  	stb 6, 13(1)
     afc: 98 a1 00 0e  	stb 5, 14(1)
     b00: 98 01 00 0f  	stb 0, 15(1)
     b04: 48 00 00 01  	bl 0xb04 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xa0>
			00000b04:  R_PPC_REL24	__as__8_GXColorFRC8_GXColor
     b08: 38 61 00 14  	addi 3, 1, 20
     b0c: 38 9e 00 28  	addi 4, 30, 40
     b10: 48 00 00 01  	bl 0xb10 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xac>
			00000b10:  R_PPC_REL24	getSyntheticColor__34@unnamed@MultiEmitterCallBack_cpp@FRC6Color8RC6Color8
     b14: 90 61 00 1c  	stw 3, 28(1)
     b18: 7f e3 fb 78  	mr	3, 31
     b1c: 88 81 00 1c  	lbz 4, 28(1)
     b20: 88 a1 00 1d  	lbz 5, 29(1)
     b24: 88 c1 00 1e  	lbz 6, 30(1)
     b28: 48 00 00 01  	bl 0xb28 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xc4>
			00000b28:  R_PPC_REL24	setGlobalPrmColor__14JPABaseEmitterFUcUcUc
     b2c: 88 e1 00 24  	lbz 7, 36(1)
     b30: 38 61 00 10  	addi 3, 1, 16
     b34: 88 c1 00 25  	lbz 6, 37(1)
     b38: 38 81 00 08  	addi 4, 1, 8
     b3c: 88 a1 00 26  	lbz 5, 38(1)
     b40: 88 01 00 27  	lbz 0, 39(1)
     b44: 98 e1 00 08  	stb 7, 8(1)
     b48: 98 c1 00 09  	stb 6, 9(1)
     b4c: 98 a1 00 0a  	stb 5, 10(1)
     b50: 98 01 00 0b  	stb 0, 11(1)
     b54: 48 00 00 01  	bl 0xb54 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xf0>
			00000b54:  R_PPC_REL24	__as__8_GXColorFRC8_GXColor
     b58: 38 61 00 10  	addi 3, 1, 16
     b5c: 38 9e 00 2c  	addi 4, 30, 44
     b60: 48 00 00 01  	bl 0xb60 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xfc>
			00000b60:  R_PPC_REL24	getSyntheticColor__34@unnamed@MultiEmitterCallBack_cpp@FRC6Color8RC6Color8
     b64: 90 61 00 18  	stw 3, 24(1)
     b68: 7f e3 fb 78  	mr	3, 31
     b6c: 88 81 00 18  	lbz 4, 24(1)
     b70: 88 a1 00 19  	lbz 5, 25(1)
     b74: 88 c1 00 1a  	lbz 6, 26(1)
     b78: 48 00 00 01  	bl 0xb78 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x114>
			00000b78:  R_PPC_REL24	setGlobalEnvColor__14JPABaseEmitterFUcUcUc
     b7c: 80 01 00 34  	lwz 0, 52(1)
     b80: 83 e1 00 2c  	lwz 31, 44(1)
     b84: 83 c1 00 28  	lwz 30, 40(1)
     b88: 7c 08 03 a6  	mtlr 0
     b8c: 38 21 00 30  	addi 1, 1, 48
     b90: 4e 80 00 20  	blr

00000b94 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb>:
     b94: 2c 05 00 00  	cmpwi	5, 0
     b98: 41 82 00 68  	bt	2, 0xc00 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x6c>
     b9c: 80 c3 00 04  	lwz 6, 4(3)
     ba0: 80 a6 00 28  	lwz 5, 40(6)
     ba4: 2c 05 00 00  	cmpwi	5, 0
     ba8: 41 82 00 d8  	bt	2, 0xc80 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0xec>
     bac: a0 05 00 18  	lhz 0, 24(5)
     bb0: 54 05 07 38  	rlwinm 5, 0, 0, 28, 28
     bb4: 38 05 ff f8  	addi 0, 5, -8
     bb8: 7c 00 00 34  	cntlzw	0, 0
     bbc: 54 00 d9 7e  	srwi 0, 0, 5
     bc0: 98 04 00 00  	stb 0, 0(4)
     bc4: 80 a6 00 28  	lwz 5, 40(6)
     bc8: a0 05 00 18  	lhz 0, 24(5)
     bcc: 54 05 06 f6  	rlwinm 5, 0, 0, 27, 27
     bd0: 38 05 ff f0  	addi 0, 5, -16
     bd4: 7c 00 00 34  	cntlzw	0, 0
     bd8: 54 00 d9 7e  	srwi 0, 0, 5
     bdc: 98 04 00 01  	stb 0, 1(4)
     be0: 80 a6 00 28  	lwz 5, 40(6)
     be4: a0 05 00 18  	lhz 0, 24(5)
     be8: 54 05 06 b4  	rlwinm 5, 0, 0, 26, 26
     bec: 38 05 ff e0  	addi 0, 5, -32
     bf0: 7c 00 00 34  	cntlzw	0, 0
     bf4: 54 00 d9 7e  	srwi 0, 0, 5
     bf8: 98 04 00 02  	stb 0, 2(4)
     bfc: 48 00 00 84  	b 0xc80 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0xec>
     c00: a0 03 00 30  	lhz 0, 48(3)
     c04: 70 00 00 42  	andi. 0, 0, 66
     c08: 41 82 00 18  	bt	2, 0xc20 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x8c>
     c0c: 38 00 00 00  	li 0, 0
     c10: 98 04 00 02  	stb 0, 2(4)
     c14: 98 04 00 01  	stb 0, 1(4)
     c18: 98 04 00 00  	stb 0, 0(4)
     c1c: 4e 80 00 20  	blr
     c20: 80 c3 00 04  	lwz 6, 4(3)
     c24: 80 a6 00 28  	lwz 5, 40(6)
     c28: 2c 05 00 00  	cmpwi	5, 0
     c2c: 41 82 00 54  	bt	2, 0xc80 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0xec>
     c30: a0 05 00 18  	lhz 0, 24(5)
     c34: 54 05 07 fe  	clrlwi	5, 0, 31
     c38: 38 05 ff ff  	addi 0, 5, -1
     c3c: 7c 00 00 34  	cntlzw	0, 0
     c40: 54 00 d9 7e  	srwi 0, 0, 5
     c44: 98 04 00 00  	stb 0, 0(4)
     c48: 80 a6 00 28  	lwz 5, 40(6)
     c4c: a0 05 00 18  	lhz 0, 24(5)
     c50: 54 05 07 bc  	rlwinm 5, 0, 0, 30, 30
     c54: 38 05 ff fe  	addi 0, 5, -2
     c58: 7c 00 00 34  	cntlzw	0, 0
     c5c: 54 00 d9 7e  	srwi 0, 0, 5
     c60: 98 04 00 01  	stb 0, 1(4)
     c64: 80 a6 00 28  	lwz 5, 40(6)
     c68: a0 05 00 18  	lhz 0, 24(5)
     c6c: 54 05 07 7a  	rlwinm 5, 0, 0, 29, 29
     c70: 38 05 ff fc  	addi 0, 5, -4
     c74: 7c 00 00 34  	cntlzw	0, 0
     c78: 54 00 d9 7e  	srwi 0, 0, 5
     c7c: 98 04 00 02  	stb 0, 2(4)
     c80: 80 03 00 14  	lwz 0, 20(3)
     c84: 2c 00 00 00  	cmpwi	0, 0
     c88: 40 82 00 1c  	bf	2, 0xca4 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x110>
     c8c: 80 03 00 08  	lwz 0, 8(3)
     c90: 2c 00 00 00  	cmpwi	0, 0
     c94: 40 82 00 10  	bf	2, 0xca4 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x110>
     c98: 38 00 00 00  	li 0, 0
     c9c: 98 04 00 00  	stb 0, 0(4)
     ca0: 48 00 00 28  	b 0xcc8 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x134>
     ca4: 88 04 00 00  	lbz 0, 0(4)
     ca8: 2c 00 00 00  	cmpwi	0, 0
     cac: 40 82 00 1c  	bf	2, 0xcc8 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x134>
     cb0: a0 03 00 30  	lhz 0, 48(3)
     cb4: 54 05 07 fe  	clrlwi	5, 0, 31
     cb8: 38 05 ff ff  	addi 0, 5, -1
     cbc: 7c 00 00 34  	cntlzw	0, 0
     cc0: 54 00 d9 7e  	srwi 0, 0, 5
     cc4: 98 04 00 00  	stb 0, 0(4)
     cc8: 80 03 00 14  	lwz 0, 20(3)
     ccc: 2c 00 00 00  	cmpwi	0, 0
     cd0: 40 82 00 1c  	bf	2, 0xcec <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x158>
     cd4: 80 03 00 0c  	lwz 0, 12(3)
     cd8: 2c 00 00 00  	cmpwi	0, 0
     cdc: 40 82 00 10  	bf	2, 0xcec <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x158>
     ce0: 38 00 00 00  	li 0, 0
     ce4: 98 04 00 01  	stb 0, 1(4)
     ce8: 48 00 00 28  	b 0xd10 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x17c>
     cec: 88 04 00 01  	lbz 0, 1(4)
     cf0: 2c 00 00 00  	cmpwi	0, 0
     cf4: 40 82 00 1c  	bf	2, 0xd10 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x17c>
     cf8: a0 03 00 30  	lhz 0, 48(3)
     cfc: 54 05 07 7a  	rlwinm 5, 0, 0, 29, 29
     d00: 38 05 ff fc  	addi 0, 5, -4
     d04: 7c 00 00 34  	cntlzw	0, 0
     d08: 54 00 d9 7e  	srwi 0, 0, 5
     d0c: 98 04 00 01  	stb 0, 1(4)
     d10: 80 03 00 14  	lwz 0, 20(3)
     d14: 2c 00 00 00  	cmpwi	0, 0
     d18: 40 82 00 1c  	bf	2, 0xd34 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x1a0>
     d1c: 80 03 00 10  	lwz 0, 16(3)
     d20: 2c 00 00 00  	cmpwi	0, 0
     d24: 40 82 00 10  	bf	2, 0xd34 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x1a0>
     d28: 38 00 00 00  	li 0, 0
     d2c: 98 04 00 02  	stb 0, 2(4)
     d30: 4e 80 00 20  	blr
     d34: 88 04 00 02  	lbz 0, 2(4)
     d38: 2c 00 00 00  	cmpwi	0, 0
     d3c: 4c 82 00 20  	bclr	4, 2
     d40: a0 03 00 30  	lhz 0, 48(3)
     d44: 54 03 06 f6  	rlwinm 3, 0, 0, 27, 27
     d48: 38 03 ff f0  	addi 0, 3, -16
     d4c: 7c 00 00 34  	cntlzw	0, 0
     d50: 54 00 d9 7e  	srwi 0, 0, 5
     d54: 98 04 00 02  	stb 0, 2(4)
     d58: 4e 80 00 20  	blr

00000d5c <setGlobalScale__14JPABaseEmitterFRCQ29JGeometry8TVec3<f>>:
     d5c: 94 21 ff f0  	stwu 1, -16(1)
     d60: 7c 08 02 a6  	mflr 0
     d64: 90 01 00 14  	stw 0, 20(1)
     d68: 93 e1 00 0c  	stw 31, 12(1)
     d6c: 7c 9f 23 78  	mr	31, 4
     d70: 93 c1 00 08  	stw 30, 8(1)
     d74: 7c 7e 1b 78  	mr	30, 3
     d78: 38 63 00 98  	addi 3, 3, 152
     d7c: 48 00 00 01  	bl 0xd7c <setGlobalScale__14JPABaseEmitterFRCQ29JGeometry8TVec3<f>+0x20>
			00000d7c:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     d80: c0 3f 00 04  	lfs 1, 4(31)
     d84: c0 1f 00 00  	lfs 0, 0(31)
     d88: d0 3e 00 b4  	stfs 1, 180(30)
     d8c: d0 1e 00 b0  	stfs 0, 176(30)
     d90: 83 e1 00 0c  	lwz 31, 12(1)
     d94: 83 c1 00 08  	lwz 30, 8(1)
     d98: 80 01 00 14  	lwz 0, 20(1)
     d9c: 7c 08 03 a6  	mtlr 0
     da0: 38 21 00 10  	addi 1, 1, 16
     da4: 4e 80 00 20  	blr

00000da8 <setGlobalPrmColor__14JPABaseEmitterFUcUcUc>:
     da8: 98 83 00 b8  	stb 4, 184(3)
     dac: 98 a3 00 b9  	stb 5, 185(3)
     db0: 98 c3 00 ba  	stb 6, 186(3)
     db4: 4e 80 00 20  	blr

00000db8 <setGlobalEnvColor__14JPABaseEmitterFUcUcUc>:
     db8: 98 83 00 bc  	stb 4, 188(3)
     dbc: 98 a3 00 bd  	stb 5, 189(3)
     dc0: 98 c3 00 be  	stb 6, 190(3)
     dc4: 4e 80 00 20  	blr

00000dc8 <__as__8_GXColorFRC8_GXColor>:
     dc8: 88 e4 00 00  	lbz 7, 0(4)
     dcc: 88 c4 00 01  	lbz 6, 1(4)
     dd0: 88 a4 00 02  	lbz 5, 2(4)
     dd4: 88 04 00 03  	lbz 0, 3(4)
     dd8: 98 e3 00 00  	stb 7, 0(3)
     ddc: 98 c3 00 01  	stb 6, 1(3)
     de0: 98 a3 00 02  	stb 5, 2(3)
     de4: 98 03 00 03  	stb 0, 3(3)
     de8: 4e 80 00 20  	blr

00000dec <drawAfter__18JPAEmitterCallBackFP14JPABaseEmitter>:
     dec: 4e 80 00 20  	blr

00000df0 <draw__18JPAEmitterCallBackFP14JPABaseEmitter>:
     df0: 4e 80 00 20  	blr

00000df4 <executeAfter__18JPAEmitterCallBackFP14JPABaseEmitter>:
     df4: 4e 80 00 20  	blr

00000df8 <execute__18JPAEmitterCallBackFP14JPABaseEmitter>:
     df8: 4e 80 00 20  	blr

00000dfc <init__24MultiEmitterCallBackBaseFP14JPABaseEmitter>:
     dfc: 4e 80 00 20  	blr

00000e00 <__dt__20MultiEmitterCallBackFv>:
     e00: 94 21 ff f0  	stwu 1, -16(1)
     e04: 7c 08 02 a6  	mflr 0
     e08: 2c 03 00 00  	cmpwi	3, 0
     e0c: 90 01 00 14  	stw 0, 20(1)
     e10: 93 e1 00 0c  	stw 31, 12(1)
     e14: 7c 9f 23 78  	mr	31, 4
     e18: 93 c1 00 08  	stw 30, 8(1)
     e1c: 7c 7e 1b 78  	mr	30, 3
     e20: 41 82 00 20  	bt	2, 0xe40 <__dt__20MultiEmitterCallBackFv+0x40>
     e24: 41 82 00 0c  	bt	2, 0xe30 <__dt__20MultiEmitterCallBackFv+0x30>
     e28: 38 80 00 00  	li 4, 0
     e2c: 48 00 00 01  	bl 0xe2c <__dt__20MultiEmitterCallBackFv+0x2c>
			00000e2c:  R_PPC_REL24	__dt__18JPAEmitterCallBackFv
     e30: 2c 1f 00 00  	cmpwi	31, 0
     e34: 40 81 00 0c  	bf	1, 0xe40 <__dt__20MultiEmitterCallBackFv+0x40>
     e38: 7f c3 f3 78  	mr	3, 30
     e3c: 48 00 00 01  	bl 0xe3c <__dt__20MultiEmitterCallBackFv+0x3c>
			00000e3c:  R_PPC_REL24	__dl__FPv
     e40: 7f c3 f3 78  	mr	3, 30
     e44: 83 e1 00 0c  	lwz 31, 12(1)
     e48: 83 c1 00 08  	lwz 30, 8(1)
     e4c: 80 01 00 14  	lwz 0, 20(1)
     e50: 7c 08 03 a6  	mtlr 0
     e54: 38 21 00 10  	addi 1, 1, 16
     e58: 4e 80 00 20  	blr
