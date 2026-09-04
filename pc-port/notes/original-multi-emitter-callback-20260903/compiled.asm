
/Users/frityet/Projects/petari/build/original-multi-emitter-callback-20260903/current.o:	file format elf32-powerpc

Disassembly of section .text:

00000000 <getSyntheticColor__34@unnamed@MultiEmitterCallBack_cpp@FRC6Color8RC6Color8>:
       0: 94 21 ff f0  	stwu 1, -16(1)
       4: 7c 08 02 a6  	mflr 0
       8: 7c 6b 1b 78  	mr	11, 3
       c: 88 a3 00 00  	lbz 5, 0(3)
      10: 90 01 00 14  	stw 0, 20(1)
      14: 39 20 00 ff  	li 9, 255
      18: 88 04 00 00  	lbz 0, 0(4)
      1c: 38 61 00 08  	addi 3, 1, 8
      20: 89 0b 00 01  	lbz 8, 1(11)
      24: 38 e0 00 ff  	li 7, 255
      28: 7d 45 01 d6  	mullw 10, 5, 0
      2c: 88 c4 00 01  	lbz 6, 1(4)
      30: 88 04 00 02  	lbz 0, 2(4)
      34: 88 ab 00 02  	lbz 5, 2(11)
      38: 7c 88 31 d6  	mullw 4, 8, 6
      3c: 7c 05 01 d6  	mullw 0, 5, 0
      40: 7c a4 4b d6  	divw 5, 4, 9
      44: 7c ca 4b d6  	divw 6, 10, 9
      48: 54 a5 06 3e  	clrlwi	5, 5, 24
      4c: 7c 00 4b d6  	divw 0, 0, 9
      50: 54 c4 06 3e  	clrlwi	4, 6, 24
      54: 54 06 06 3e  	clrlwi	6, 0, 24
      58: 48 00 00 01  	bl 0x58 <getSyntheticColor__34@unnamed@MultiEmitterCallBack_cpp@FRC6Color8RC6Color8+0x58>
			00000058:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
      5c: 80 01 00 14  	lwz 0, 20(1)
      60: 80 63 00 00  	lwz 3, 0(3)
      64: 7c 08 03 a6  	mtlr 0
      68: 38 21 00 10  	addi 1, 1, 16
      6c: 4e 80 00 20  	blr

00000070 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>>:
      70: 94 21 ff e0  	stwu 1, -32(1)
      74: 7c 08 02 a6  	mflr 0
      78: 90 01 00 24  	stw 0, 36(1)
      7c: 39 61 00 20  	addi 11, 1, 32
      80: 48 00 00 01  	bl 0x80 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x10>
			00000080:  R_PPC_REL24	_savegpr_28
      84: 7c 7c 1b 78  	mr	28, 3
      88: 7c 9d 23 78  	mr	29, 4
      8c: 7c be 2b 78  	mr	30, 5
      90: 48 00 00 01  	bl 0x90 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x20>
			00000090:  R_PPC_REL24	__ct__24MultiEmitterCallBackBaseFv
      94: 3c a0 00 00  	lis 5, 0
			00000096:  R_PPC_ADDR16_HA	__vt__20MultiEmitterCallBack
      98: 3b e0 00 00  	li 31, 0
      9c: 38 a5 00 00  	addi 5, 5, 0
			0000009e:  R_PPC_ADDR16_LO	__vt__20MultiEmitterCallBack
      a0: 93 bc 00 04  	stw 29, 4(28)
      a4: 7f c4 f3 78  	mr	4, 30
      a8: 38 7c 00 18  	addi 3, 28, 24
      ac: 90 bc 00 00  	stw 5, 0(28)
      b0: 93 fc 00 08  	stw 31, 8(28)
      b4: 93 fc 00 0c  	stw 31, 12(28)
      b8: 93 fc 00 10  	stw 31, 16(28)
      bc: 93 fc 00 14  	stw 31, 20(28)
      c0: 48 00 00 01  	bl 0xc0 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x50>
			000000c0:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
      c4: c0 00 00 00  	lfs 0, 0(0)
			000000c4:  Unknown	@4331
      c8: 38 7c 00 28  	addi 3, 28, 40
      cc: 38 80 00 ff  	li 4, 255
      d0: 38 a0 00 ff  	li 5, 255
      d4: d0 1c 00 24  	stfs 0, 36(28)
      d8: 38 c0 00 ff  	li 6, 255
      dc: 38 e0 00 ff  	li 7, 255
      e0: 48 00 00 01  	bl 0xe0 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x70>
			000000e0:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
      e4: 38 7c 00 2c  	addi 3, 28, 44
      e8: 38 80 00 ff  	li 4, 255
      ec: 38 a0 00 ff  	li 5, 255
      f0: 38 c0 00 ff  	li 6, 255
      f4: 38 e0 00 ff  	li 7, 255
      f8: 48 00 00 01  	bl 0xf8 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x88>
			000000f8:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
      fc: b3 fc 00 30  	sth 31, 48(28)
     100: 39 61 00 20  	addi 11, 1, 32
     104: 7f 83 e3 78  	mr	3, 28
     108: 48 00 00 01  	bl 0x108 <__ct__20MultiEmitterCallBackFPC12MultiEmitterRCQ29JGeometry8TVec3<f>+0x98>
			00000108:  R_PPC_REL24	_restgpr_28
     10c: 80 01 00 24  	lwz 0, 36(1)
     110: 7c 08 03 a6  	mtlr 0
     114: 38 21 00 20  	addi 1, 1, 32
     118: 4e 80 00 20  	blr

0000011c <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>>:
     11c: 94 21 ff d0  	stwu 1, -48(1)
     120: 7c 08 02 a6  	mflr 0
     124: 90 01 00 34  	stw 0, 52(1)
     128: 39 61 00 30  	addi 11, 1, 48
     12c: 48 00 00 01  	bl 0x12c <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x10>
			0000012c:  R_PPC_REL24	_savegpr_25
     130: 7c 79 1b 78  	mr	25, 3
     134: 7c 9a 23 78  	mr	26, 4
     138: 7c bb 2b 78  	mr	27, 5
     13c: 7c dc 33 78  	mr	28, 6
     140: 7c fd 3b 78  	mr	29, 7
     144: 7d 1e 43 78  	mr	30, 8
     148: 48 00 00 01  	bl 0x148 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x2c>
			00000148:  R_PPC_REL24	__ct__24MultiEmitterCallBackBaseFv
     14c: 3c a0 00 00  	lis 5, 0
			0000014e:  R_PPC_ADDR16_HA	__vt__20MultiEmitterCallBack
     150: 3b e0 00 00  	li 31, 0
     154: 38 a5 00 00  	addi 5, 5, 0
			00000156:  R_PPC_ADDR16_LO	__vt__20MultiEmitterCallBack
     158: 93 59 00 04  	stw 26, 4(25)
     15c: 7f c4 f3 78  	mr	4, 30
     160: 38 79 00 18  	addi 3, 25, 24
     164: 90 b9 00 00  	stw 5, 0(25)
     168: 93 79 00 08  	stw 27, 8(25)
     16c: 93 99 00 0c  	stw 28, 12(25)
     170: 93 b9 00 10  	stw 29, 16(25)
     174: 93 f9 00 14  	stw 31, 20(25)
     178: 48 00 00 01  	bl 0x178 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x5c>
			00000178:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     17c: c0 00 00 00  	lfs 0, 0(0)
			0000017c:  Unknown	@4331
     180: 38 79 00 28  	addi 3, 25, 40
     184: 38 80 00 ff  	li 4, 255
     188: 38 a0 00 ff  	li 5, 255
     18c: d0 19 00 24  	stfs 0, 36(25)
     190: 38 c0 00 ff  	li 6, 255
     194: 38 e0 00 ff  	li 7, 255
     198: 48 00 00 01  	bl 0x198 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x7c>
			00000198:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     19c: 38 79 00 2c  	addi 3, 25, 44
     1a0: 38 80 00 ff  	li 4, 255
     1a4: 38 a0 00 ff  	li 5, 255
     1a8: 38 c0 00 ff  	li 6, 255
     1ac: 38 e0 00 ff  	li 7, 255
     1b0: 48 00 00 01  	bl 0x1b0 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x94>
			000001b0:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     1b4: b3 f9 00 30  	sth 31, 48(25)
     1b8: 39 61 00 30  	addi 11, 1, 48
     1bc: 7f 23 cb 78  	mr	3, 25
     1c0: 48 00 00 01  	bl 0x1c0 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0xa4>
			000001c0:  R_PPC_REL24	_restgpr_25
     1c4: 80 01 00 34  	lwz 0, 52(1)
     1c8: 7c 08 03 a6  	mtlr 0
     1cc: 38 21 00 30  	addi 1, 1, 48
     1d0: 4e 80 00 20  	blr

000001d4 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>>:
     1d4: 94 21 ff e0  	stwu 1, -32(1)
     1d8: 7c 08 02 a6  	mflr 0
     1dc: 90 01 00 24  	stw 0, 36(1)
     1e0: 39 61 00 20  	addi 11, 1, 32
     1e4: 48 00 00 01  	bl 0x1e4 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x10>
			000001e4:  R_PPC_REL24	_savegpr_27
     1e8: 7c 7b 1b 78  	mr	27, 3
     1ec: 7c 9c 23 78  	mr	28, 4
     1f0: 7c bd 2b 78  	mr	29, 5
     1f4: 7c de 33 78  	mr	30, 6
     1f8: 48 00 00 01  	bl 0x1f8 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x24>
			000001f8:  R_PPC_REL24	__ct__24MultiEmitterCallBackBaseFv
     1fc: 3c a0 00 00  	lis 5, 0
			000001fe:  R_PPC_ADDR16_HA	__vt__20MultiEmitterCallBack
     200: 3b e0 00 00  	li 31, 0
     204: 38 a5 00 00  	addi 5, 5, 0
			00000206:  R_PPC_ADDR16_LO	__vt__20MultiEmitterCallBack
     208: 93 9b 00 04  	stw 28, 4(27)
     20c: 7f c4 f3 78  	mr	4, 30
     210: 38 7b 00 18  	addi 3, 27, 24
     214: 90 bb 00 00  	stw 5, 0(27)
     218: 93 fb 00 08  	stw 31, 8(27)
     21c: 93 fb 00 0c  	stw 31, 12(27)
     220: 93 fb 00 10  	stw 31, 16(27)
     224: 93 bb 00 14  	stw 29, 20(27)
     228: 48 00 00 01  	bl 0x228 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x54>
			00000228:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     22c: c0 00 00 00  	lfs 0, 0(0)
			0000022c:  Unknown	@4331
     230: 38 7b 00 28  	addi 3, 27, 40
     234: 38 80 00 ff  	li 4, 255
     238: 38 a0 00 ff  	li 5, 255
     23c: d0 1b 00 24  	stfs 0, 36(27)
     240: 38 c0 00 ff  	li 6, 255
     244: 38 e0 00 ff  	li 7, 255
     248: 48 00 00 01  	bl 0x248 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x74>
			00000248:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     24c: 38 7b 00 2c  	addi 3, 27, 44
     250: 38 80 00 ff  	li 4, 255
     254: 38 a0 00 ff  	li 5, 255
     258: 38 c0 00 ff  	li 6, 255
     25c: 38 e0 00 ff  	li 7, 255
     260: 48 00 00 01  	bl 0x260 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x8c>
			00000260:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     264: b3 fb 00 30  	sth 31, 48(27)
     268: 39 61 00 20  	addi 11, 1, 32
     26c: 7f 63 db 78  	mr	3, 27
     270: 48 00 00 01  	bl 0x270 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fRCQ29JGeometry8TVec3<f>+0x9c>
			00000270:  R_PPC_REL24	_restgpr_27
     274: 80 01 00 24  	lwz 0, 36(1)
     278: 7c 08 03 a6  	mtlr 0
     27c: 38 21 00 20  	addi 1, 1, 32
     280: 4e 80 00 20  	blr

00000284 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>>:
     284: 94 21 ff e0  	stwu 1, -32(1)
     288: 7c 08 02 a6  	mflr 0
     28c: 90 01 00 24  	stw 0, 36(1)
     290: 39 61 00 20  	addi 11, 1, 32
     294: 48 00 00 01  	bl 0x294 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x10>
			00000294:  R_PPC_REL24	_savegpr_26
     298: 7c 7a 1b 78  	mr	26, 3
     29c: 7c 9b 23 78  	mr	27, 4
     2a0: 7c bc 2b 78  	mr	28, 5
     2a4: 7c dd 33 78  	mr	29, 6
     2a8: 7c fe 3b 78  	mr	30, 7
     2ac: 48 00 00 01  	bl 0x2ac <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x28>
			000002ac:  R_PPC_REL24	__ct__24MultiEmitterCallBackBaseFv
     2b0: 3c a0 00 00  	lis 5, 0
			000002b2:  R_PPC_ADDR16_HA	__vt__20MultiEmitterCallBack
     2b4: 3b e0 00 00  	li 31, 0
     2b8: 38 a5 00 00  	addi 5, 5, 0
			000002ba:  R_PPC_ADDR16_LO	__vt__20MultiEmitterCallBack
     2bc: 93 7a 00 04  	stw 27, 4(26)
     2c0: 7f c4 f3 78  	mr	4, 30
     2c4: 38 7a 00 18  	addi 3, 26, 24
     2c8: 90 ba 00 00  	stw 5, 0(26)
     2cc: 93 fa 00 08  	stw 31, 8(26)
     2d0: 93 fa 00 0c  	stw 31, 12(26)
     2d4: 93 ba 00 10  	stw 29, 16(26)
     2d8: 93 9a 00 14  	stw 28, 20(26)
     2dc: 48 00 00 01  	bl 0x2dc <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x58>
			000002dc:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     2e0: c0 00 00 00  	lfs 0, 0(0)
			000002e0:  Unknown	@4331
     2e4: 38 7a 00 28  	addi 3, 26, 40
     2e8: 38 80 00 ff  	li 4, 255
     2ec: 38 a0 00 ff  	li 5, 255
     2f0: d0 1a 00 24  	stfs 0, 36(26)
     2f4: 38 c0 00 ff  	li 6, 255
     2f8: 38 e0 00 ff  	li 7, 255
     2fc: 48 00 00 01  	bl 0x2fc <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x78>
			000002fc:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     300: 38 7a 00 2c  	addi 3, 26, 44
     304: 38 80 00 ff  	li 4, 255
     308: 38 a0 00 ff  	li 5, 255
     30c: 38 c0 00 ff  	li 6, 255
     310: 38 e0 00 ff  	li 7, 255
     314: 48 00 00 01  	bl 0x314 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0x90>
			00000314:  R_PPC_REL24	__ct__6Color8FUcUcUcUc
     318: b3 fa 00 30  	sth 31, 48(26)
     31c: 39 61 00 20  	addi 11, 1, 32
     320: 7f 43 d3 78  	mr	3, 26
     324: 48 00 00 01  	bl 0x324 <__ct__20MultiEmitterCallBackFPC12MultiEmitterPA4_fPCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>+0xa0>
			00000324:  R_PPC_REL24	_restgpr_26
     328: 80 01 00 24  	lwz 0, 36(1)
     32c: 7c 08 03 a6  	mtlr 0
     330: 38 21 00 20  	addi 1, 1, 32
     334: 4e 80 00 20  	blr

00000338 <execute__20MultiEmitterCallBackFP14JPABaseEmitter>:
     338: 94 21 ff f0  	stwu 1, -16(1)
     33c: 7c 08 02 a6  	mflr 0
     340: 38 a0 00 00  	li 5, 0
     344: 90 01 00 14  	stw 0, 20(1)
     348: 93 e1 00 0c  	stw 31, 12(1)
     34c: 7c 9f 23 78  	mr	31, 4
     350: 93 c1 00 08  	stw 30, 8(1)
     354: 7c 7e 1b 78  	mr	30, 3
     358: 48 00 00 01  	bl 0x358 <execute__20MultiEmitterCallBackFP14JPABaseEmitter+0x20>
			00000358:  R_PPC_REL24	followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb
     35c: 7f c3 f3 78  	mr	3, 30
     360: 7f e4 fb 78  	mr	4, 31
     364: 48 00 00 01  	bl 0x364 <execute__20MultiEmitterCallBackFP14JPABaseEmitter+0x2c>
			00000364:  R_PPC_REL24	effectLight__20MultiEmitterCallBackFP14JPABaseEmitter
     368: 7f c3 f3 78  	mr	3, 30
     36c: 7f e4 fb 78  	mr	4, 31
     370: 48 00 00 01  	bl 0x370 <execute__20MultiEmitterCallBackFP14JPABaseEmitter+0x38>
			00000370:  R_PPC_REL24	setColor__20MultiEmitterCallBackFP14JPABaseEmitter
     374: 80 01 00 14  	lwz 0, 20(1)
     378: 83 e1 00 0c  	lwz 31, 12(1)
     37c: 83 c1 00 08  	lwz 30, 8(1)
     380: 7c 08 03 a6  	mtlr 0
     384: 38 21 00 10  	addi 1, 1, 16
     388: 4e 80 00 20  	blr

0000038c <setHostSRT__20MultiEmitterCallBackFPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>>:
     38c: 38 00 00 00  	li 0, 0
     390: 90 83 00 08  	stw 4, 8(3)
     394: 90 a3 00 0c  	stw 5, 12(3)
     398: 90 c3 00 10  	stw 6, 16(3)
     39c: 90 03 00 14  	stw 0, 20(3)
     3a0: 4e 80 00 20  	blr

000003a4 <setHostMtx__20MultiEmitterCallBackFPA4_f>:
     3a4: 38 00 00 00  	li 0, 0
     3a8: 90 83 00 14  	stw 4, 20(3)
     3ac: 90 03 00 08  	stw 0, 8(3)
     3b0: 90 03 00 0c  	stw 0, 12(3)
     3b4: 90 03 00 10  	stw 0, 16(3)
     3b8: 4e 80 00 20  	blr

000003bc <setBaseScale__20MultiEmitterCallBackFf>:
     3bc: a0 03 00 30  	lhz 0, 48(3)
     3c0: 54 00 05 ee  	rlwinm 0, 0, 0, 23, 23
     3c4: 28 00 01 00  	cmplwi	0, 256
     3c8: 41 82 00 10  	bt	2, 0x3d8 <setBaseScale__20MultiEmitterCallBackFf+0x1c>
     3cc: a0 03 00 30  	lhz 0, 48(3)
     3d0: 60 00 01 00  	ori 0, 0, 256
     3d4: b0 03 00 30  	sth 0, 48(3)
     3d8: d0 23 00 24  	stfs 1, 36(3)
     3dc: 4e 80 00 20  	blr

000003e0 <forceFollowOn__20MultiEmitterCallBackFv>:
     3e0: a0 03 00 30  	lhz 0, 48(3)
     3e4: 60 00 00 01  	ori 0, 0, 1
     3e8: b0 03 00 30  	sth 0, 48(3)
     3ec: 4e 80 00 20  	blr

000003f0 <forceFollowOff__20MultiEmitterCallBackFv>:
     3f0: a0 03 00 30  	lhz 0, 48(3)
     3f4: 60 00 00 02  	ori 0, 0, 2
     3f8: b0 03 00 30  	sth 0, 48(3)
     3fc: 4e 80 00 20  	blr

00000400 <forceScaleOn__20MultiEmitterCallBackFv>:
     400: a0 03 00 30  	lhz 0, 48(3)
     404: 60 00 00 10  	ori 0, 0, 16
     408: b0 03 00 30  	sth 0, 48(3)
     40c: 4e 80 00 20  	blr

00000410 <resetFollowCurrent__20MultiEmitterCallBackFv>:
     410: a0 03 00 30  	lhz 0, 48(3)
     414: 54 00 06 b0  	rlwinm 0, 0, 0, 26, 24
     418: b0 03 00 30  	sth 0, 48(3)
     41c: 4e 80 00 20  	blr

00000420 <init__20MultiEmitterCallBackFP14JPABaseEmitter>:
     420: 38 a0 00 01  	li 5, 1
     424: 48 00 00 00  	b 0x424 <init__20MultiEmitterCallBackFP14JPABaseEmitter+0x4>
			00000424:  R_PPC_REL24	followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb

00000428 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb>:
     428: 94 21 ff 10  	stwu 1, -240(1)
     42c: 7c 08 02 a6  	mflr 0
     430: 90 01 00 f4  	stw 0, 244(1)
     434: db e1 00 e0  	stfd 31, 224(1)
     438: f3 e1 00 e8  	<unknown>
     43c: 39 61 00 e0  	addi 11, 1, 224
     440: 48 00 00 01  	bl 0x440 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x18>
			00000440:  R_PPC_REL24	_savegpr_27
     444: e0 05 00 00  	lq 0, 0(5)
     448: 7c 9c 23 78  	mr	28, 4
     44c: e0 25 00 08  	<unknown>
     450: 39 01 00 8c  	addi 8, 1, 140
     454: e0 45 00 10  	lq 2, 16(5)
     458: 7c 7b 1b 78  	mr	27, 3
     45c: 7c dd 33 78  	mr	29, 6
     460: e0 65 00 18  	<unknown>
     464: e0 85 00 20  	lq 4, 32(5)
     468: 7c fe 3b 78  	mr	30, 7
     46c: e0 a5 00 28  	<unknown>
     470: 7d 03 43 78  	mr	3, 8
     474: 38 81 00 5c  	addi 4, 1, 92
     478: 38 a1 00 14  	addi 5, 1, 20
     47c: f0 08 00 00  	xsaddsp 0, 8, 0
     480: 38 c1 00 20  	addi 6, 1, 32
     484: f0 28 00 08  	xsmaddasp 1, 8, 0
     488: f0 48 00 10  	xxsldwi 2, 8, 0, 0
     48c: f0 68 00 18  	xscmpeqdp 3, 8, 0
     490: f0 88 00 20  	<unknown>
     494: f0 a8 00 28  	<unknown>
     498: 48 00 00 01  	bl 0x498 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x70>
			00000498:  R_PPC_REL24	JPASetRMtxSTVecfromMtx__FPA4_CfPA4_fPQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f>
     49c: 88 1d 00 00  	lbz 0, 0(29)
     4a0: 2c 00 00 00  	cmpwi	0, 0
     4a4: 41 82 00 44  	bt	2, 0x4e8 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xc0>
     4a8: 38 61 00 5c  	addi 3, 1, 92
     4ac: 38 9b 00 18  	addi 4, 27, 24
     4b0: 38 a1 00 08  	addi 5, 1, 8
     4b4: 48 00 00 01  	bl 0x4b4 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x8c>
			000004b4:  R_PPC_REL24	mult33__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRCQ29JGeometry8TVec3<f>RQ29JGeometry8TVec3<f>
     4b8: 88 1d 00 02  	lbz 0, 2(29)
     4bc: 2c 00 00 00  	cmpwi	0, 0
     4c0: 41 82 00 10  	bt	2, 0x4d0 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xa8>
     4c4: 38 61 00 08  	addi 3, 1, 8
     4c8: 38 81 00 14  	addi 4, 1, 20
     4cc: 48 00 00 01  	bl 0x4cc <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xa4>
			000004cc:  R_PPC_REL24	mul__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     4d0: 38 61 00 20  	addi 3, 1, 32
     4d4: 38 81 00 08  	addi 4, 1, 8
     4d8: 48 00 00 01  	bl 0x4d8 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xb0>
			000004d8:  R_PPC_REL24	add__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     4dc: 38 7c 00 a4  	addi 3, 28, 164
     4e0: 38 81 00 20  	addi 4, 1, 32
     4e4: 48 00 00 01  	bl 0x4e4 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xbc>
			000004e4:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     4e8: 88 1d 00 01  	lbz 0, 1(29)
     4ec: 2c 00 00 00  	cmpwi	0, 0
     4f0: 41 82 00 80  	bt	2, 0x570 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x148>
     4f4: 80 7b 00 04  	lwz 3, 4(27)
     4f8: 48 00 00 01  	bl 0x4f8 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xd0>
			000004f8:  R_PPC_REL24	isEffect2D__Q22MR6EffectFPC12MultiEmitter
     4fc: 2c 03 00 00  	cmpwi	3, 0
     500: 41 82 00 64  	bt	2, 0x564 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x13c>
     504: 38 61 00 2c  	addi 3, 1, 44
     508: 48 00 00 01  	bl 0x508 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xe0>
			00000508:  R_PPC_REL24	identity__Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>Fv
     50c: 3f e0 00 00  	lis 31, 0
			0000050e:  R_PPC_ADDR16_HA	@4397
     510: c8 3f 00 00  	lfd 1, 0(31)
			00000512:  R_PPC_ADDR16_LO	@4397
     514: 48 00 00 01  	bl 0x514 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xec>
			00000514:  R_PPC_REL24	sin
     518: ff e0 08 18  	frsp 31, 1
     51c: c8 3f 00 00  	lfd 1, 0(31)
			0000051e:  R_PPC_ADDR16_LO	@4397
     520: 48 00 00 01  	bl 0x520 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0xf8>
			00000520:  R_PPC_REL24	cos
     524: fc 60 08 18  	frsp 3, 1
     528: c0 00 00 00  	lfs 0, 0(0)
			00000528:  Unknown	@4398
     52c: fc 40 f8 50  	fneg 2, 31
     530: c0 20 00 00  	lfs 1, 0(0)
			00000530:  Unknown	@4331
     534: d3 e1 00 3c  	stfs 31, 60(1)
     538: 38 61 00 5c  	addi 3, 1, 92
     53c: d0 61 00 2c  	stfs 3, 44(1)
     540: 38 81 00 2c  	addi 4, 1, 44
     544: d0 41 00 30  	stfs 2, 48(1)
     548: d0 61 00 40  	stfs 3, 64(1)
     54c: d0 21 00 54  	stfs 1, 84(1)
     550: d0 01 00 50  	stfs 0, 80(1)
     554: d0 01 00 44  	stfs 0, 68(1)
     558: d0 01 00 4c  	stfs 0, 76(1)
     55c: d0 01 00 34  	stfs 0, 52(1)
     560: 48 00 00 01  	bl 0x560 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x138>
			00000560:  R_PPC_REL24	concat__Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>FRCQ29JGeometry13SMatrix34C<f>
     564: 38 61 00 5c  	addi 3, 1, 92
     568: 38 9c 00 68  	addi 4, 28, 104
     56c: 48 00 00 01  	bl 0x56c <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x144>
			0000056c:  R_PPC_REL24	JPASetRMtxfromMtx__FPA4_CfPA4_f
     570: 88 dd 00 02  	lbz 6, 2(29)
     574: 7f 63 db 78  	mr	3, 27
     578: 7f 84 e3 78  	mr	4, 28
     57c: 7f c7 f3 78  	mr	7, 30
     580: 38 a1 00 14  	addi 5, 1, 20
     584: 48 00 00 01  	bl 0x584 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x15c>
			00000584:  R_PPC_REL24	setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb
     588: e3 e1 00 e8  	<unknown>
     58c: 39 61 00 e0  	addi 11, 1, 224
     590: cb e1 00 e0  	lfd 31, 224(1)
     594: 48 00 00 01  	bl 0x594 <setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb+0x16c>
			00000594:  R_PPC_REL24	_restgpr_27
     598: 80 01 00 f4  	lwz 0, 244(1)
     59c: 7c 08 03 a6  	mtlr 0
     5a0: 38 21 00 f0  	addi 1, 1, 240
     5a4: 4e 80 00 20  	blr

000005a8 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb>:
     5a8: 94 21 ff 10  	stwu 1, -240(1)
     5ac: 7c 08 02 a6  	mflr 0
     5b0: 90 01 00 f4  	stw 0, 244(1)
     5b4: db e1 00 e0  	stfd 31, 224(1)
     5b8: f3 e1 00 e8  	<unknown>
     5bc: db c1 00 d0  	stfd 30, 208(1)
     5c0: f3 c1 00 d8  	<unknown>
     5c4: db a1 00 c0  	stfd 29, 192(1)
     5c8: f3 a1 00 c8  	xsmsubmsp 29, 1, 0
     5cc: db 81 00 b0  	stfd 28, 176(1)
     5d0: f3 81 00 b8  	xxsel 28, 1, 0, 34
     5d4: db 61 00 a0  	stfd 27, 160(1)
     5d8: f3 61 00 a8  	<unknown>
     5dc: db 41 00 90  	stfd 26, 144(1)
     5e0: f3 41 00 98  	xscmpgedp 26, 1, 0
     5e4: 39 61 00 90  	addi 11, 1, 144
     5e8: 48 00 00 01  	bl 0x5e8 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x40>
			000005e8:  R_PPC_REL24	_savegpr_28
     5ec: 88 05 00 00  	lbz 0, 0(5)
     5f0: 7c 7c 1b 78  	mr	28, 3
     5f4: 7c 9d 23 78  	mr	29, 4
     5f8: 7c be 2b 78  	mr	30, 5
     5fc: 2c 00 00 00  	cmpwi	0, 0
     600: 7c df 33 78  	mr	31, 6
     604: 41 82 01 48  	bt	2, 0x74c <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x1a4>
     608: 88 05 00 01  	lbz 0, 1(5)
     60c: 2c 00 00 00  	cmpwi	0, 0
     610: 41 82 00 f8  	bt	2, 0x708 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x160>
     614: 38 61 00 2c  	addi 3, 1, 44
     618: 48 00 00 01  	bl 0x618 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x70>
			00000618:  R_PPC_REL24	identity__Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>Fv
     61c: 80 9c 00 0c  	lwz 4, 12(28)
     620: 38 61 00 14  	addi 3, 1, 20
     624: 48 00 00 01  	bl 0x624 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x7c>
			00000624:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     628: c0 20 00 00  	lfs 1, 0(0)
			00000628:  Unknown	@4455
     62c: 38 61 00 14  	addi 3, 1, 20
     630: 48 00 00 01  	bl 0x630 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x88>
			00000630:  R_PPC_REL24	scale__Q29JGeometry8TVec3<f>Ff
     634: c3 a1 00 1c  	lfs 29, 28(1)
     638: c3 c1 00 18  	lfs 30, 24(1)
     63c: fc 20 e8 90  	fmr 1, 29
     640: c3 41 00 14  	lfs 26, 20(1)
     644: 48 00 00 01  	bl 0x644 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x9c>
			00000644:  R_PPC_REL24	cos
     648: ff e0 08 18  	frsp 31, 1
     64c: fc 20 f0 90  	fmr 1, 30
     650: 48 00 00 01  	bl 0x650 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xa8>
			00000650:  R_PPC_REL24	cos
     654: ff 60 08 18  	frsp 27, 1
     658: fc 20 d0 90  	fmr 1, 26
     65c: 48 00 00 01  	bl 0x65c <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xb4>
			0000065c:  R_PPC_REL24	cos
     660: ff 80 08 18  	frsp 28, 1
     664: fc 20 e8 90  	fmr 1, 29
     668: 48 00 00 01  	bl 0x668 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xc0>
			00000668:  R_PPC_REL24	sin
     66c: ff a0 08 18  	frsp 29, 1
     670: fc 20 f0 90  	fmr 1, 30
     674: 48 00 00 01  	bl 0x674 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xcc>
			00000674:  R_PPC_REL24	sin
     678: ff c0 08 18  	frsp 30, 1
     67c: fc 20 d0 90  	fmr 1, 26
     680: 48 00 00 01  	bl 0x680 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0xd8>
			00000680:  R_PPC_REL24	sin
     684: fd 00 08 18  	frsp 8, 1
     688: 38 61 00 2c  	addi 3, 1, 44
     68c: ec bc 07 f2  	fmuls 5, 28, 31
     690: 38 9c 00 18  	addi 4, 28, 24
     694: ec 1b 07 f2  	fmuls 0, 27, 31
     698: 38 a1 00 20  	addi 5, 1, 32
     69c: ec 48 07 b2  	fmuls 2, 8, 30
     6a0: d0 01 00 2c  	stfs 0, 44(1)
     6a4: ec 9b 07 72  	fmuls 4, 27, 29
     6a8: ed 3c 07 72  	fmuls 9, 28, 29
     6ac: ec 62 07 f2  	fmuls 3, 2, 31
     6b0: ec 25 07 b2  	fmuls 1, 5, 30
     6b4: d0 81 00 3c  	stfs 4, 60(1)
     6b8: ec 42 07 72  	fmuls 2, 2, 29
     6bc: ec c3 48 28  	fsubs 6, 3, 9
     6c0: ec 08 07 72  	fmuls 0, 8, 29
     6c4: ec a5 10 2a  	fadds 5, 5, 2
     6c8: fc e0 f0 50  	fneg 7, 30
     6cc: d0 c1 00 30  	stfs 6, 48(1)
     6d0: ec 61 00 2a  	fadds 3, 1, 0
     6d4: ec 88 06 f2  	fmuls 4, 8, 27
     6d8: d0 a1 00 40  	stfs 5, 64(1)
     6dc: ec 1c 06 f2  	fmuls 0, 28, 27
     6e0: ec 49 07 b2  	fmuls 2, 9, 30
     6e4: d0 e1 00 4c  	stfs 7, 76(1)
     6e8: ec 28 07 f2  	fmuls 1, 8, 31
     6ec: d0 81 00 50  	stfs 4, 80(1)
     6f0: ec 22 08 28  	fsubs 1, 2, 1
     6f4: d0 61 00 34  	stfs 3, 52(1)
     6f8: d0 01 00 54  	stfs 0, 84(1)
     6fc: d0 21 00 44  	stfs 1, 68(1)
     700: 48 00 00 01  	bl 0x700 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x158>
			00000700:  R_PPC_REL24	mult33__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRCQ29JGeometry8TVec3<f>RQ29JGeometry8TVec3<f>
     704: 48 00 00 10  	b 0x714 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x16c>
     708: 38 61 00 20  	addi 3, 1, 32
     70c: 38 9c 00 18  	addi 4, 28, 24
     710: 48 00 00 01  	bl 0x710 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x168>
			00000710:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     714: 88 1e 00 02  	lbz 0, 2(30)
     718: 2c 00 00 00  	cmpwi	0, 0
     71c: 41 82 00 18  	bt	2, 0x734 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x18c>
     720: 80 9c 00 10  	lwz 4, 16(28)
     724: 2c 04 00 00  	cmpwi	4, 0
     728: 41 82 00 0c  	bt	2, 0x734 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x18c>
     72c: 38 61 00 20  	addi 3, 1, 32
     730: 48 00 00 01  	bl 0x730 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x188>
			00000730:  R_PPC_REL24	mul__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     734: 80 9c 00 08  	lwz 4, 8(28)
     738: 38 61 00 20  	addi 3, 1, 32
     73c: 48 00 00 01  	bl 0x73c <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x194>
			0000073c:  R_PPC_REL24	add__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     740: 38 7d 00 a4  	addi 3, 29, 164
     744: 38 81 00 20  	addi 4, 1, 32
     748: 48 00 00 01  	bl 0x748 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x1a0>
			00000748:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     74c: 88 1e 00 01  	lbz 0, 1(30)
     750: 2c 00 00 00  	cmpwi	0, 0
     754: 41 82 00 5c  	bt	2, 0x7b0 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x208>
     758: 80 7c 00 0c  	lwz 3, 12(28)
     75c: 38 dd 00 68  	addi 6, 29, 104
     760: c0 60 00 00  	lfs 3, 0(0)
			00000760:  Unknown	@4456
     764: c0 43 00 08  	lfs 2, 8(3)
     768: c0 23 00 04  	lfs 1, 4(3)
     76c: c0 03 00 00  	lfs 0, 0(3)
     770: ec 43 00 b2  	fmuls 2, 3, 2
     774: ec 23 00 72  	fmuls 1, 3, 1
     778: ec 03 00 32  	fmuls 0, 3, 0
     77c: fc 40 10 1e  	fctiwz 2, 2
     780: fc 20 08 1e  	fctiwz 1, 1
     784: fc 00 00 1e  	fctiwz 0, 0
     788: d8 41 00 60  	stfd 2, 96(1)
     78c: d8 21 00 68  	stfd 1, 104(1)
     790: 80 01 00 64  	lwz 0, 100(1)
     794: 80 61 00 6c  	lwz 3, 108(1)
     798: d8 01 00 70  	stfd 0, 112(1)
     79c: 7c 05 07 34  	extsh 5, 0
     7a0: 7c 64 07 34  	extsh 4, 3
     7a4: 80 01 00 74  	lwz 0, 116(1)
     7a8: 7c 03 07 34  	extsh 3, 0
     7ac: 48 00 00 01  	bl 0x7ac <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x204>
			000007ac:  R_PPC_REL24	JPAGetXYZRotateMtx__FsssPA4_f
     7b0: c0 00 00 00  	lfs 0, 0(0)
			000007b0:  Unknown	@4331
     7b4: 7f 83 e3 78  	mr	3, 28
     7b8: 88 de 00 02  	lbz 6, 2(30)
     7bc: 7f a4 eb 78  	mr	4, 29
     7c0: d0 01 00 08  	stfs 0, 8(1)
     7c4: 7f e7 fb 78  	mr	7, 31
     7c8: 38 a1 00 08  	addi 5, 1, 8
     7cc: d0 01 00 0c  	stfs 0, 12(1)
     7d0: d0 01 00 10  	stfs 0, 16(1)
     7d4: 48 00 00 01  	bl 0x7d4 <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x22c>
			000007d4:  R_PPC_REL24	setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb
     7d8: e3 e1 00 e8  	<unknown>
     7dc: cb e1 00 e0  	lfd 31, 224(1)
     7e0: e3 c1 00 d8  	<unknown>
     7e4: cb c1 00 d0  	lfd 30, 208(1)
     7e8: e3 a1 00 c8  	<unknown>
     7ec: cb a1 00 c0  	lfd 29, 192(1)
     7f0: e3 81 00 b8  	<unknown>
     7f4: cb 81 00 b0  	lfd 28, 176(1)
     7f8: e3 61 00 a8  	<unknown>
     7fc: cb 61 00 a0  	lfd 27, 160(1)
     800: e3 41 00 98  	<unknown>
     804: 39 61 00 90  	addi 11, 1, 144
     808: cb 41 00 90  	lfd 26, 144(1)
     80c: 48 00 00 01  	bl 0x80c <setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb+0x264>
			0000080c:  R_PPC_REL24	_restgpr_28
     810: 80 01 00 f4  	lwz 0, 244(1)
     814: 7c 08 03 a6  	mtlr 0
     818: 38 21 00 f0  	addi 1, 1, 240
     81c: 4e 80 00 20  	blr

00000820 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb>:
     820: 94 21 ff d0  	stwu 1, -48(1)
     824: 7c 08 02 a6  	mflr 0
     828: 90 01 00 34  	stw 0, 52(1)
     82c: 39 61 00 30  	addi 11, 1, 48
     830: 48 00 00 01  	bl 0x830 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x10>
			00000830:  R_PPC_REL24	_savegpr_28
     834: 7c 7c 1b 78  	mr	28, 3
     838: 7c 9d 23 78  	mr	29, 4
     83c: 7c a4 2b 78  	mr	4, 5
     840: 7c de 33 78  	mr	30, 6
     844: 7c ff 3b 78  	mr	31, 7
     848: 38 61 00 08  	addi 3, 1, 8
     84c: 48 00 00 01  	bl 0x84c <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x2c>
			0000084c:  R_PPC_REL24	__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>
     850: 2c 1e 00 00  	cmpwi	30, 0
     854: 41 82 00 44  	bt	2, 0x898 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x78>
     858: 80 9c 00 10  	lwz 4, 16(28)
     85c: 2c 04 00 00  	cmpwi	4, 0
     860: 41 82 00 0c  	bt	2, 0x86c <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x4c>
     864: 38 61 00 08  	addi 3, 1, 8
     868: 48 00 00 01  	bl 0x868 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x48>
			00000868:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
     86c: a0 1c 00 30  	lhz 0, 48(28)
     870: 54 00 05 ee  	rlwinm 0, 0, 0, 23, 23
     874: 28 00 01 00  	cmplwi	0, 256
     878: 40 82 00 10  	bf	2, 0x888 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x68>
     87c: c0 3c 00 24  	lfs 1, 36(28)
     880: 38 61 00 08  	addi 3, 1, 8
     884: 48 00 00 01  	bl 0x884 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x64>
			00000884:  R_PPC_REL24	scale__Q29JGeometry8TVec3<f>Ff
     888: 7f a3 eb 78  	mr	3, 29
     88c: 38 81 00 08  	addi 4, 1, 8
     890: 48 00 00 01  	bl 0x890 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x70>
			00000890:  R_PPC_REL24	setGlobalScale__14JPABaseEmitterFRCQ29JGeometry8TVec3<f>
     894: 48 00 00 34  	b 0x8c8 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xa8>
     898: 2c 1f 00 00  	cmpwi	31, 0
     89c: 41 82 00 2c  	bt	2, 0x8c8 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xa8>
     8a0: a0 1c 00 30  	lhz 0, 48(28)
     8a4: 54 00 05 ee  	rlwinm 0, 0, 0, 23, 23
     8a8: 28 00 01 00  	cmplwi	0, 256
     8ac: 40 82 00 1c  	bf	2, 0x8c8 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xa8>
     8b0: c0 3c 00 24  	lfs 1, 36(28)
     8b4: 38 61 00 08  	addi 3, 1, 8
     8b8: 48 00 00 01  	bl 0x8b8 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0x98>
			000008b8:  R_PPC_REL24	setAll<f>__Q29JGeometry8TVec3<f>Ff_v
     8bc: 7f a3 eb 78  	mr	3, 29
     8c0: 38 81 00 08  	addi 4, 1, 8
     8c4: 48 00 00 01  	bl 0x8c4 <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xa4>
			000008c4:  R_PPC_REL24	setGlobalScale__14JPABaseEmitterFRCQ29JGeometry8TVec3<f>
     8c8: 39 61 00 30  	addi 11, 1, 48
     8cc: 48 00 00 01  	bl 0x8cc <setScaleFromHostScale__20MultiEmitterCallBackFP14JPABaseEmitterRCQ29JGeometry8TVec3<f>bb+0xac>
			000008cc:  R_PPC_REL24	_restgpr_28
     8d0: 80 01 00 34  	lwz 0, 52(1)
     8d4: 7c 08 03 a6  	mtlr 0
     8d8: 38 21 00 30  	addi 1, 1, 48
     8dc: 4e 80 00 20  	blr

000008e0 <effectLight__20MultiEmitterCallBackFP14JPABaseEmitter>:
     8e0: 80 63 00 04  	lwz 3, 4(3)
     8e4: c0 40 00 00  	lfs 2, 0(0)
			000008e4:  Unknown	@4485
     8e8: c0 23 00 2c  	lfs 1, 44(3)
     8ec: 48 00 00 00  	b 0x8ec <effectLight__20MultiEmitterCallBackFP14JPABaseEmitter+0xc>
			000008ec:  R_PPC_REL24	isNearZero__2MRFff

000008f0 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb>:
     8f0: 94 21 ff e0  	stwu 1, -32(1)
     8f4: 7c 08 02 a6  	mflr 0
     8f8: 90 01 00 24  	stw 0, 36(1)
     8fc: 39 61 00 20  	addi 11, 1, 32
     900: 48 00 00 01  	bl 0x900 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x10>
			00000900:  R_PPC_REL24	_savegpr_29
     904: 7c 9e 23 78  	mr	30, 4
     908: 7c 7d 1b 78  	mr	29, 3
     90c: 7c bf 2b 78  	mr	31, 5
     910: 38 81 00 08  	addi 4, 1, 8
     914: 48 00 00 01  	bl 0x914 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x24>
			00000914:  R_PPC_REL24	isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb
     918: 88 01 00 08  	lbz 0, 8(1)
     91c: 38 60 00 00  	li 3, 0
     920: 2c 00 00 00  	cmpwi	0, 0
     924: 40 82 00 1c  	bf	2, 0x940 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x50>
     928: 88 01 00 09  	lbz 0, 9(1)
     92c: 2c 00 00 00  	cmpwi	0, 0
     930: 40 82 00 10  	bf	2, 0x940 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x50>
     934: 88 01 00 0a  	lbz 0, 10(1)
     938: 2c 00 00 00  	cmpwi	0, 0
     93c: 41 82 00 08  	bt	2, 0x944 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x54>
     940: 38 60 00 01  	li 3, 1
     944: 2c 03 00 00  	cmpwi	3, 0
     948: 40 82 00 14  	bf	2, 0x95c <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x6c>
     94c: a0 1d 00 30  	lhz 0, 48(29)
     950: 54 00 05 ee  	rlwinm 0, 0, 0, 23, 23
     954: 28 00 01 00  	cmplwi	0, 256
     958: 40 82 00 3c  	bf	2, 0x994 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0xa4>
     95c: 80 bd 00 14  	lwz 5, 20(29)
     960: 2c 05 00 00  	cmpwi	5, 0
     964: 41 82 00 1c  	bt	2, 0x980 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x90>
     968: 7f a3 eb 78  	mr	3, 29
     96c: 7f c4 f3 78  	mr	4, 30
     970: 7f e7 fb 78  	mr	7, 31
     974: 38 c1 00 08  	addi 6, 1, 8
     978: 48 00 00 01  	bl 0x978 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0x88>
			00000978:  R_PPC_REL24	setSRTFromHostMtx__20MultiEmitterCallBackFP14JPABaseEmitterPA4_fRCQ220MultiEmitterCallBack7FlagSRTb
     97c: 48 00 00 18  	b 0x994 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0xa4>
     980: 7f a3 eb 78  	mr	3, 29
     984: 7f c4 f3 78  	mr	4, 30
     988: 7f e6 fb 78  	mr	6, 31
     98c: 38 a1 00 08  	addi 5, 1, 8
     990: 48 00 00 01  	bl 0x990 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0xa0>
			00000990:  R_PPC_REL24	setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb
     994: 39 61 00 20  	addi 11, 1, 32
     998: 48 00 00 01  	bl 0x998 <followSRT__20MultiEmitterCallBackFP14JPABaseEmitterb+0xa8>
			00000998:  R_PPC_REL24	_restgpr_29
     99c: 80 01 00 24  	lwz 0, 36(1)
     9a0: 7c 08 03 a6  	mtlr 0
     9a4: 38 21 00 20  	addi 1, 1, 32
     9a8: 4e 80 00 20  	blr

000009ac <setColor__20MultiEmitterCallBackFP14JPABaseEmitter>:
     9ac: 94 21 ff d0  	stwu 1, -48(1)
     9b0: 7c 08 02 a6  	mflr 0
     9b4: c0 40 00 00  	lfs 2, 0(0)
			000009b4:  Unknown	@4485
     9b8: 90 01 00 34  	stw 0, 52(1)
     9bc: 93 e1 00 2c  	stw 31, 44(1)
     9c0: 7c 9f 23 78  	mr	31, 4
     9c4: 93 c1 00 28  	stw 30, 40(1)
     9c8: 7c 7e 1b 78  	mr	30, 3
     9cc: 80 a3 00 04  	lwz 5, 4(3)
     9d0: c0 25 00 2c  	lfs 1, 44(5)
     9d4: 48 00 00 01  	bl 0x9d4 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x28>
			000009d4:  R_PPC_REL24	isNearZero__2MRFff
     9d8: 2c 03 00 00  	cmpwi	3, 0
     9dc: 41 82 00 30  	bt	2, 0xa0c <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x60>
     9e0: 88 9e 00 28  	lbz 4, 40(30)
     9e4: 7f e3 fb 78  	mr	3, 31
     9e8: 88 be 00 29  	lbz 5, 41(30)
     9ec: 88 de 00 2a  	lbz 6, 42(30)
     9f0: 48 00 00 01  	bl 0x9f0 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x44>
			000009f0:  R_PPC_REL24	setGlobalPrmColor__14JPABaseEmitterFUcUcUc
     9f4: 88 9e 00 2c  	lbz 4, 44(30)
     9f8: 7f e3 fb 78  	mr	3, 31
     9fc: 88 be 00 2d  	lbz 5, 45(30)
     a00: 88 de 00 2e  	lbz 6, 46(30)
     a04: 48 00 00 01  	bl 0xa04 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x58>
			00000a04:  R_PPC_REL24	setGlobalEnvColor__14JPABaseEmitterFUcUcUc
     a08: 48 00 00 b4  	b 0xabc <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x110>
     a0c: 38 61 00 24  	addi 3, 1, 36
     a10: 38 9f 00 b8  	addi 4, 31, 184
     a14: 48 00 00 01  	bl 0xa14 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x68>
			00000a14:  R_PPC_REL24	__as__8_GXColorFRC8_GXColor
     a18: 38 61 00 20  	addi 3, 1, 32
     a1c: 38 9f 00 bc  	addi 4, 31, 188
     a20: 48 00 00 01  	bl 0xa20 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x74>
			00000a20:  R_PPC_REL24	__as__8_GXColorFRC8_GXColor
     a24: 88 e1 00 24  	lbz 7, 36(1)
     a28: 38 61 00 14  	addi 3, 1, 20
     a2c: 88 c1 00 25  	lbz 6, 37(1)
     a30: 38 81 00 10  	addi 4, 1, 16
     a34: 88 a1 00 26  	lbz 5, 38(1)
     a38: 88 01 00 27  	lbz 0, 39(1)
     a3c: 98 e1 00 10  	stb 7, 16(1)
     a40: 98 c1 00 11  	stb 6, 17(1)
     a44: 98 a1 00 12  	stb 5, 18(1)
     a48: 98 01 00 13  	stb 0, 19(1)
     a4c: 48 00 00 01  	bl 0xa4c <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xa0>
			00000a4c:  R_PPC_REL24	__ct__6Color8F8_GXColor
     a50: 38 9e 00 28  	addi 4, 30, 40
     a54: 48 00 00 01  	bl 0xa54 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xa8>
			00000a54:  R_PPC_REL24	getSyntheticColor__34@unnamed@MultiEmitterCallBack_cpp@FRC6Color8RC6Color8
     a58: 90 61 00 1c  	stw 3, 28(1)
     a5c: 7f e3 fb 78  	mr	3, 31
     a60: 88 81 00 1c  	lbz 4, 28(1)
     a64: 88 a1 00 1d  	lbz 5, 29(1)
     a68: 88 c1 00 1e  	lbz 6, 30(1)
     a6c: 48 00 00 01  	bl 0xa6c <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xc0>
			00000a6c:  R_PPC_REL24	setGlobalPrmColor__14JPABaseEmitterFUcUcUc
     a70: 88 e1 00 24  	lbz 7, 36(1)
     a74: 38 61 00 0c  	addi 3, 1, 12
     a78: 88 c1 00 25  	lbz 6, 37(1)
     a7c: 38 81 00 08  	addi 4, 1, 8
     a80: 88 a1 00 26  	lbz 5, 38(1)
     a84: 88 01 00 27  	lbz 0, 39(1)
     a88: 98 e1 00 08  	stb 7, 8(1)
     a8c: 98 c1 00 09  	stb 6, 9(1)
     a90: 98 a1 00 0a  	stb 5, 10(1)
     a94: 98 01 00 0b  	stb 0, 11(1)
     a98: 48 00 00 01  	bl 0xa98 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xec>
			00000a98:  R_PPC_REL24	__ct__6Color8F8_GXColor
     a9c: 38 9e 00 2c  	addi 4, 30, 44
     aa0: 48 00 00 01  	bl 0xaa0 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0xf4>
			00000aa0:  R_PPC_REL24	getSyntheticColor__34@unnamed@MultiEmitterCallBack_cpp@FRC6Color8RC6Color8
     aa4: 90 61 00 18  	stw 3, 24(1)
     aa8: 7f e3 fb 78  	mr	3, 31
     aac: 88 81 00 18  	lbz 4, 24(1)
     ab0: 88 a1 00 19  	lbz 5, 25(1)
     ab4: 88 c1 00 1a  	lbz 6, 26(1)
     ab8: 48 00 00 01  	bl 0xab8 <setColor__20MultiEmitterCallBackFP14JPABaseEmitter+0x10c>
			00000ab8:  R_PPC_REL24	setGlobalEnvColor__14JPABaseEmitterFUcUcUc
     abc: 80 01 00 34  	lwz 0, 52(1)
     ac0: 83 e1 00 2c  	lwz 31, 44(1)
     ac4: 83 c1 00 28  	lwz 30, 40(1)
     ac8: 7c 08 03 a6  	mtlr 0
     acc: 38 21 00 30  	addi 1, 1, 48
     ad0: 4e 80 00 20  	blr

00000ad4 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb>:
     ad4: 2c 05 00 00  	cmpwi	5, 0
     ad8: 41 82 00 68  	bt	2, 0xb40 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x6c>
     adc: 80 c3 00 04  	lwz 6, 4(3)
     ae0: 80 a6 00 28  	lwz 5, 40(6)
     ae4: 2c 05 00 00  	cmpwi	5, 0
     ae8: 41 82 00 d8  	bt	2, 0xbc0 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0xec>
     aec: a0 05 00 18  	lhz 0, 24(5)
     af0: 54 05 07 38  	rlwinm 5, 0, 0, 28, 28
     af4: 38 05 ff f8  	addi 0, 5, -8
     af8: 7c 00 00 34  	cntlzw	0, 0
     afc: 54 00 d9 7e  	srwi 0, 0, 5
     b00: 98 04 00 00  	stb 0, 0(4)
     b04: 80 a6 00 28  	lwz 5, 40(6)
     b08: a0 05 00 18  	lhz 0, 24(5)
     b0c: 54 05 06 f6  	rlwinm 5, 0, 0, 27, 27
     b10: 38 05 ff f0  	addi 0, 5, -16
     b14: 7c 00 00 34  	cntlzw	0, 0
     b18: 54 00 d9 7e  	srwi 0, 0, 5
     b1c: 98 04 00 01  	stb 0, 1(4)
     b20: 80 a6 00 28  	lwz 5, 40(6)
     b24: a0 05 00 18  	lhz 0, 24(5)
     b28: 54 05 06 b4  	rlwinm 5, 0, 0, 26, 26
     b2c: 38 05 ff e0  	addi 0, 5, -32
     b30: 7c 00 00 34  	cntlzw	0, 0
     b34: 54 00 d9 7e  	srwi 0, 0, 5
     b38: 98 04 00 02  	stb 0, 2(4)
     b3c: 48 00 00 84  	b 0xbc0 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0xec>
     b40: a0 03 00 30  	lhz 0, 48(3)
     b44: 70 00 00 42  	andi. 0, 0, 66
     b48: 41 82 00 18  	bt	2, 0xb60 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x8c>
     b4c: 38 00 00 00  	li 0, 0
     b50: 98 04 00 02  	stb 0, 2(4)
     b54: 98 04 00 01  	stb 0, 1(4)
     b58: 98 04 00 00  	stb 0, 0(4)
     b5c: 4e 80 00 20  	blr
     b60: 80 c3 00 04  	lwz 6, 4(3)
     b64: 80 a6 00 28  	lwz 5, 40(6)
     b68: 2c 05 00 00  	cmpwi	5, 0
     b6c: 41 82 00 54  	bt	2, 0xbc0 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0xec>
     b70: a0 05 00 18  	lhz 0, 24(5)
     b74: 54 05 07 fe  	clrlwi	5, 0, 31
     b78: 38 05 ff ff  	addi 0, 5, -1
     b7c: 7c 00 00 34  	cntlzw	0, 0
     b80: 54 00 d9 7e  	srwi 0, 0, 5
     b84: 98 04 00 00  	stb 0, 0(4)
     b88: 80 a6 00 28  	lwz 5, 40(6)
     b8c: a0 05 00 18  	lhz 0, 24(5)
     b90: 54 05 07 bc  	rlwinm 5, 0, 0, 30, 30
     b94: 38 05 ff fe  	addi 0, 5, -2
     b98: 7c 00 00 34  	cntlzw	0, 0
     b9c: 54 00 d9 7e  	srwi 0, 0, 5
     ba0: 98 04 00 01  	stb 0, 1(4)
     ba4: 80 a6 00 28  	lwz 5, 40(6)
     ba8: a0 05 00 18  	lhz 0, 24(5)
     bac: 54 05 07 7a  	rlwinm 5, 0, 0, 29, 29
     bb0: 38 05 ff fc  	addi 0, 5, -4
     bb4: 7c 00 00 34  	cntlzw	0, 0
     bb8: 54 00 d9 7e  	srwi 0, 0, 5
     bbc: 98 04 00 02  	stb 0, 2(4)
     bc0: 80 03 00 14  	lwz 0, 20(3)
     bc4: 2c 00 00 00  	cmpwi	0, 0
     bc8: 40 82 00 1c  	bf	2, 0xbe4 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x110>
     bcc: 80 03 00 08  	lwz 0, 8(3)
     bd0: 2c 00 00 00  	cmpwi	0, 0
     bd4: 40 82 00 10  	bf	2, 0xbe4 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x110>
     bd8: 38 00 00 00  	li 0, 0
     bdc: 98 04 00 00  	stb 0, 0(4)
     be0: 48 00 00 28  	b 0xc08 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x134>
     be4: 88 04 00 00  	lbz 0, 0(4)
     be8: 2c 00 00 00  	cmpwi	0, 0
     bec: 40 82 00 1c  	bf	2, 0xc08 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x134>
     bf0: a0 03 00 30  	lhz 0, 48(3)
     bf4: 54 05 07 fe  	clrlwi	5, 0, 31
     bf8: 38 05 ff ff  	addi 0, 5, -1
     bfc: 7c 00 00 34  	cntlzw	0, 0
     c00: 54 00 d9 7e  	srwi 0, 0, 5
     c04: 98 04 00 00  	stb 0, 0(4)
     c08: 80 03 00 14  	lwz 0, 20(3)
     c0c: 2c 00 00 00  	cmpwi	0, 0
     c10: 40 82 00 1c  	bf	2, 0xc2c <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x158>
     c14: 80 03 00 0c  	lwz 0, 12(3)
     c18: 2c 00 00 00  	cmpwi	0, 0
     c1c: 40 82 00 10  	bf	2, 0xc2c <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x158>
     c20: 38 00 00 00  	li 0, 0
     c24: 98 04 00 01  	stb 0, 1(4)
     c28: 48 00 00 28  	b 0xc50 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x17c>
     c2c: 88 04 00 01  	lbz 0, 1(4)
     c30: 2c 00 00 00  	cmpwi	0, 0
     c34: 40 82 00 1c  	bf	2, 0xc50 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x17c>
     c38: a0 03 00 30  	lhz 0, 48(3)
     c3c: 54 05 07 7a  	rlwinm 5, 0, 0, 29, 29
     c40: 38 05 ff fc  	addi 0, 5, -4
     c44: 7c 00 00 34  	cntlzw	0, 0
     c48: 54 00 d9 7e  	srwi 0, 0, 5
     c4c: 98 04 00 01  	stb 0, 1(4)
     c50: 80 03 00 14  	lwz 0, 20(3)
     c54: 2c 00 00 00  	cmpwi	0, 0
     c58: 40 82 00 1c  	bf	2, 0xc74 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x1a0>
     c5c: 80 03 00 10  	lwz 0, 16(3)
     c60: 2c 00 00 00  	cmpwi	0, 0
     c64: 40 82 00 10  	bf	2, 0xc74 <isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb+0x1a0>
     c68: 38 00 00 00  	li 0, 0
     c6c: 98 04 00 02  	stb 0, 2(4)
     c70: 4e 80 00 20  	blr
     c74: 88 04 00 02  	lbz 0, 2(4)
     c78: 2c 00 00 00  	cmpwi	0, 0
     c7c: 4c 82 00 20  	bclr	4, 2
     c80: a0 03 00 30  	lhz 0, 48(3)
     c84: 54 03 06 f6  	rlwinm 3, 0, 0, 27, 27
     c88: 38 03 ff f0  	addi 0, 3, -16
     c8c: 7c 00 00 34  	cntlzw	0, 0
     c90: 54 00 d9 7e  	srwi 0, 0, 5
     c94: 98 04 00 02  	stb 0, 2(4)
     c98: 4e 80 00 20  	blr

00000c9c <__dt__20MultiEmitterCallBackFv>:
     c9c: 94 21 ff f0  	stwu 1, -16(1)
     ca0: 7c 08 02 a6  	mflr 0
     ca4: 2c 03 00 00  	cmpwi	3, 0
     ca8: 90 01 00 14  	stw 0, 20(1)
     cac: 93 e1 00 0c  	stw 31, 12(1)
     cb0: 7c 9f 23 78  	mr	31, 4
     cb4: 93 c1 00 08  	stw 30, 8(1)
     cb8: 7c 7e 1b 78  	mr	30, 3
     cbc: 41 82 00 20  	bt	2, 0xcdc <__dt__20MultiEmitterCallBackFv+0x40>
     cc0: 41 82 00 0c  	bt	2, 0xccc <__dt__20MultiEmitterCallBackFv+0x30>
     cc4: 38 80 00 00  	li 4, 0
     cc8: 48 00 00 01  	bl 0xcc8 <__dt__20MultiEmitterCallBackFv+0x2c>
			00000cc8:  R_PPC_REL24	__dt__18JPAEmitterCallBackFv
     ccc: 2c 1f 00 00  	cmpwi	31, 0
     cd0: 40 81 00 0c  	bf	1, 0xcdc <__dt__20MultiEmitterCallBackFv+0x40>
     cd4: 7f c3 f3 78  	mr	3, 30
     cd8: 48 00 00 01  	bl 0xcd8 <__dt__20MultiEmitterCallBackFv+0x3c>
			00000cd8:  R_PPC_REL24	__dl__FPv
     cdc: 7f c3 f3 78  	mr	3, 30
     ce0: 83 e1 00 0c  	lwz 31, 12(1)
     ce4: 83 c1 00 08  	lwz 30, 8(1)
     ce8: 80 01 00 14  	lwz 0, 20(1)
     cec: 7c 08 03 a6  	mtlr 0
     cf0: 38 21 00 10  	addi 1, 1, 16
     cf4: 4e 80 00 20  	blr

Disassembly of section .text:

00000000 <__ct__6Color8FUcUcUcUc>:
       0: 98 83 00 00  	stb 4, 0(3)
       4: 98 a3 00 01  	stb 5, 1(3)
       8: 98 c3 00 02  	stb 6, 2(3)
       c: 98 e3 00 03  	stb 7, 3(3)
      10: 4e 80 00 20  	blr

00000014 <__ct__6Color8F8_GXColor>:
      14: 94 21 ff f0  	stwu 1, -16(1)
      18: 88 e4 00 00  	lbz 7, 0(4)
      1c: 88 c4 00 01  	lbz 6, 1(4)
      20: 88 a4 00 02  	lbz 5, 2(4)
      24: 88 04 00 03  	lbz 0, 3(4)
      28: 98 e1 00 08  	stb 7, 8(1)
      2c: 98 c1 00 09  	stb 6, 9(1)
      30: 98 a1 00 0a  	stb 5, 10(1)
      34: 98 01 00 0b  	stb 0, 11(1)
      38: 98 e3 00 00  	stb 7, 0(3)
      3c: 98 c3 00 01  	stb 6, 1(3)
      40: 98 a3 00 02  	stb 5, 2(3)
      44: 98 03 00 03  	stb 0, 3(3)
      48: 38 21 00 10  	addi 1, 1, 16
      4c: 4e 80 00 20  	blr

Disassembly of section .text:

00000000 <__ct__24MultiEmitterCallBackBaseFv>:
       0: 3c 80 00 00  	lis 4, 0
			00000002:  R_PPC_ADDR16_HA	__vt__24MultiEmitterCallBackBase
       4: 38 84 00 00  	addi 4, 4, 0
			00000006:  R_PPC_ADDR16_LO	__vt__24MultiEmitterCallBackBase
       8: 90 83 00 00  	stw 4, 0(3)
       c: 4e 80 00 20  	blr

00000010 <__dt__24MultiEmitterCallBackBaseFv>:
      10: 94 21 ff f0  	stwu 1, -16(1)
      14: 7c 08 02 a6  	mflr 0
      18: 2c 03 00 00  	cmpwi	3, 0
      1c: 90 01 00 14  	stw 0, 20(1)
      20: 93 e1 00 0c  	stw 31, 12(1)
      24: 7c 9f 23 78  	mr	31, 4
      28: 93 c1 00 08  	stw 30, 8(1)
      2c: 7c 7e 1b 78  	mr	30, 3
      30: 41 82 00 1c  	bt	2, 0x4c <__dt__24MultiEmitterCallBackBaseFv+0x3c>
      34: 38 80 00 00  	li 4, 0
      38: 48 00 00 01  	bl 0x38 <__dt__24MultiEmitterCallBackBaseFv+0x28>
			00000038:  R_PPC_REL24	__dt__18JPAEmitterCallBackFv
      3c: 2c 1f 00 00  	cmpwi	31, 0
      40: 40 81 00 0c  	bf	1, 0x4c <__dt__24MultiEmitterCallBackBaseFv+0x3c>
      44: 7f c3 f3 78  	mr	3, 30
      48: 48 00 00 01  	bl 0x48 <__dt__24MultiEmitterCallBackBaseFv+0x38>
			00000048:  R_PPC_REL24	__dl__FPv
      4c: 7f c3 f3 78  	mr	3, 30
      50: 83 e1 00 0c  	lwz 31, 12(1)
      54: 83 c1 00 08  	lwz 30, 8(1)
      58: 80 01 00 14  	lwz 0, 20(1)
      5c: 7c 08 03 a6  	mtlr 0
      60: 38 21 00 10  	addi 1, 1, 16
      64: 4e 80 00 20  	blr

00000068 <init__24MultiEmitterCallBackBaseFP14JPABaseEmitter>:
      68: 4e 80 00 20  	blr

Disassembly of section .text:

00000000 <__ct__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>>:
       0: e0 24 00 00  	lq 1, 0(4)
       4: c0 04 00 08  	lfs 0, 8(4)
       8: f0 23 00 00  	xsaddsp 1, 3, 0
       c: d0 03 00 08  	stfs 0, 8(3)
      10: 4e 80 00 20  	blr

00000014 <mul__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>>:
      14: e0 63 00 00  	lq 3, 0(3)
      18: e0 44 00 00  	lq 2, 0(4)
      1c: c0 23 00 08  	lfs 1, 8(3)
      20: c0 04 00 08  	lfs 0, 8(4)
      24: 10 43 00 b2  	<unknown>
      28: ec 01 00 32  	fmuls 0, 1, 0
      2c: f0 43 00 00  	xsaddsp 2, 3, 0
      30: d0 03 00 08  	stfs 0, 8(3)
      34: 4e 80 00 20  	blr

00000038 <add__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>>:
      38: e0 63 00 00  	lq 3, 0(3)
      3c: e0 44 00 00  	lq 2, 0(4)
      40: e0 23 80 08  	<unknown>
      44: e0 04 80 08  	<unknown>
      48: 10 43 10 2a  	vsel 2, 3, 2, 0
      4c: 10 01 00 2a  	vsel 0, 1, 0, 0
      50: f0 43 00 00  	xsaddsp 2, 3, 0
      54: f0 03 80 08  	xsmaddasp 0, 3, 16
      58: 4e 80 00 20  	blr

0000005c <set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v>:
      5c: c0 44 00 00  	lfs 2, 0(4)
      60: c0 24 00 04  	lfs 1, 4(4)
      64: c0 04 00 08  	lfs 0, 8(4)
      68: d0 43 00 00  	stfs 2, 0(3)
      6c: d0 23 00 04  	stfs 1, 4(3)
      70: d0 03 00 08  	stfs 0, 8(3)
      74: 4e 80 00 20  	blr

00000078 <set<f>__Q29JGeometry8TVec3<f>Ffff_v>:
      78: d0 23 00 00  	stfs 1, 0(3)
      7c: d0 43 00 04  	stfs 2, 4(3)
      80: d0 63 00 08  	stfs 3, 8(3)
      84: 4e 80 00 20  	blr

00000088 <scale__Q29JGeometry8TVec3<f>Ff>:
      88: c0 63 00 00  	lfs 3, 0(3)
      8c: c0 43 00 04  	lfs 2, 4(3)
      90: c0 03 00 08  	lfs 0, 8(3)
      94: ec 63 00 72  	fmuls 3, 3, 1
      98: ec 42 00 72  	fmuls 2, 2, 1
      9c: ec 00 00 72  	fmuls 0, 0, 1
      a0: d0 63 00 00  	stfs 3, 0(3)
      a4: d0 43 00 04  	stfs 2, 4(3)
      a8: d0 03 00 08  	stfs 0, 8(3)
      ac: 4e 80 00 20  	blr

Disassembly of section .text:

00000000 <mult33__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRCQ29JGeometry8TVec3<f>RQ29JGeometry8TVec3<f>>:
       0: 7c 66 1b 78  	mr	6, 3
       4: c0 23 00 20  	lfs 1, 32(3)
       8: c0 04 00 00  	lfs 0, 0(4)
       c: 7c a3 2b 78  	mr	3, 5
      10: c0 66 00 10  	lfs 3, 16(6)
      14: c0 a6 00 00  	lfs 5, 0(6)
      18: ec 20 00 72  	fmuls 1, 0, 1
      1c: c0 46 00 24  	lfs 2, 36(6)
      20: ec 60 00 f2  	fmuls 3, 0, 3
      24: c0 84 00 04  	lfs 4, 4(4)
      28: ec a0 01 72  	fmuls 5, 0, 5
      2c: c0 c6 00 14  	lfs 6, 20(6)
      30: ec 04 00 b2  	fmuls 0, 4, 2
      34: c0 e6 00 04  	lfs 7, 4(6)
      38: ec 44 01 b2  	fmuls 2, 4, 6
      3c: c1 06 00 28  	lfs 8, 40(6)
      40: ec 84 01 f2  	fmuls 4, 4, 7
      44: c0 e4 00 08  	lfs 7, 8(4)
      48: c0 c6 00 08  	lfs 6, 8(6)
      4c: ec 43 10 2a  	fadds 2, 3, 2
      50: c1 26 00 18  	lfs 9, 24(6)
      54: ec a5 20 2a  	fadds 5, 5, 4
      58: ec c7 01 b2  	fmuls 6, 7, 6
      5c: ec 87 02 72  	fmuls 4, 7, 9
      60: ec 01 00 2a  	fadds 0, 1, 0
      64: ec 67 02 32  	fmuls 3, 7, 8
      68: ec 26 28 2a  	fadds 1, 6, 5
      6c: ec 44 10 2a  	fadds 2, 4, 2
      70: ec 63 00 2a  	fadds 3, 3, 0
      74: 48 00 00 00  	b 0x74 <mult33__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRCQ29JGeometry8TVec3<f>RQ29JGeometry8TVec3<f>+0x74>
			00000074:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>Ffff_v

Disassembly of section .text:

00000000 <setGlobalScale__14JPABaseEmitterFRCQ29JGeometry8TVec3<f>>:
       0: 94 21 ff f0  	stwu 1, -16(1)
       4: 7c 08 02 a6  	mflr 0
       8: 90 01 00 14  	stw 0, 20(1)
       c: 93 e1 00 0c  	stw 31, 12(1)
      10: 7c 9f 23 78  	mr	31, 4
      14: 93 c1 00 08  	stw 30, 8(1)
      18: 7c 7e 1b 78  	mr	30, 3
      1c: 38 63 00 98  	addi 3, 3, 152
      20: 48 00 00 01  	bl 0x20 <setGlobalScale__14JPABaseEmitterFRCQ29JGeometry8TVec3<f>+0x20>
			00000020:  R_PPC_REL24	set<f>__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>_v
      24: c0 3f 00 04  	lfs 1, 4(31)
      28: c0 1f 00 00  	lfs 0, 0(31)
      2c: d0 3e 00 b4  	stfs 1, 180(30)
      30: d0 1e 00 b0  	stfs 0, 176(30)
      34: 83 e1 00 0c  	lwz 31, 12(1)
      38: 83 c1 00 08  	lwz 30, 8(1)
      3c: 80 01 00 14  	lwz 0, 20(1)
      40: 7c 08 03 a6  	mtlr 0
      44: 38 21 00 10  	addi 1, 1, 16
      48: 4e 80 00 20  	blr

0000004c <setGlobalPrmColor__14JPABaseEmitterFUcUcUc>:
      4c: 98 83 00 b8  	stb 4, 184(3)
      50: 98 a3 00 b9  	stb 5, 185(3)
      54: 98 c3 00 ba  	stb 6, 186(3)
      58: 4e 80 00 20  	blr

0000005c <setGlobalEnvColor__14JPABaseEmitterFUcUcUc>:
      5c: 98 83 00 bc  	stb 4, 188(3)
      60: 98 a3 00 bd  	stb 5, 189(3)
      64: 98 c3 00 be  	stb 6, 190(3)
      68: 4e 80 00 20  	blr

0000006c <__as__8_GXColorFRC8_GXColor>:
      6c: 88 e4 00 00  	lbz 7, 0(4)
      70: 88 c4 00 01  	lbz 6, 1(4)
      74: 88 a4 00 02  	lbz 5, 2(4)
      78: 88 04 00 03  	lbz 0, 3(4)
      7c: 98 e3 00 00  	stb 7, 0(3)
      80: 98 c3 00 01  	stb 6, 1(3)
      84: 98 a3 00 02  	stb 5, 2(3)
      88: 98 03 00 03  	stb 0, 3(3)
      8c: 4e 80 00 20  	blr

Disassembly of section .text:

00000000 <drawAfter__18JPAEmitterCallBackFP14JPABaseEmitter>:
       0: 4e 80 00 20  	blr

00000004 <draw__18JPAEmitterCallBackFP14JPABaseEmitter>:
       4: 4e 80 00 20  	blr

00000008 <executeAfter__18JPAEmitterCallBackFP14JPABaseEmitter>:
       8: 4e 80 00 20  	blr

0000000c <execute__18JPAEmitterCallBackFP14JPABaseEmitter>:
       c: 4e 80 00 20  	blr

00000010 <init__18JPAEmitterCallBackFP14JPABaseEmitter>:
      10: 4e 80 00 20  	blr
