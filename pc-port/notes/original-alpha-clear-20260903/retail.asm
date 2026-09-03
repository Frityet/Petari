
build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Util/DrawUtil.o:	file format elf32-powerpc

Disassembly of section .text:

000006a0 <clearAlphaBuffer__2MRFUc>:
     6a0: 94 21 ff d0  	stwu 1, -48(1)
     6a4: 7c 08 02 a6  	mflr 0
     6a8: 3c c0 00 00  	lis 6, 0
			000006aa:  R_PPC_ADDR16_HA	lbl_8053EB40
     6ac: c0 00 00 00  	lfs 0, 0(0)
			000006ac:  Unknown	@60329
     6b0: 90 01 00 34  	stw 0, 52(1)
     6b4: 3c 00 43 30  	lis 0, 17200
     6b8: c8 66 00 00  	lfd 3, 0(6)
			000006ba:  R_PPC_ADDR16_LO	lbl_8053EB40
     6bc: 38 a1 00 08  	addi 5, 1, 8
     6c0: 80 80 00 00  	lwz 4, 0(0)
			000006c0:  Unknown	sManager__8JUTVideo
     6c4: 90 01 00 18  	stw 0, 24(1)
     6c8: 81 04 00 04  	lwz 8, 4(4)
     6cc: 38 81 00 10  	addi 4, 1, 16
     6d0: 90 01 00 20  	stw 0, 32(1)
     6d4: a0 e8 00 06  	lhz 7, 6(8)
     6d8: a0 08 00 04  	lhz 0, 4(8)
     6dc: 90 e1 00 1c  	stw 7, 28(1)
     6e0: 90 01 00 24  	stw 0, 36(1)
     6e4: c8 41 00 18  	lfd 2, 24(1)
     6e8: c8 21 00 20  	lfd 1, 32(1)
     6ec: ec 42 18 28  	fsubs 2, 2, 3
     6f0: d0 01 00 10  	stfs 0, 16(1)
     6f4: ec 21 18 28  	fsubs 1, 1, 3
     6f8: d0 01 00 14  	stfs 0, 20(1)
     6fc: d0 21 00 08  	stfs 1, 8(1)
     700: d0 41 00 0c  	stfs 2, 12(1)
     704: 48 00 00 01  	bl 0x704 <clearAlphaBuffer__2MRFUc+0x64>
			00000704:  R_PPC_REL24	clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>
     708: 80 01 00 34  	lwz 0, 52(1)
     70c: 7c 08 03 a6  	mtlr 0
     710: 38 21 00 30  	addi 1, 1, 48
     714: 4e 80 00 20  	blr

00000718 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>>:
     718: 94 21 ff 50  	stwu 1, -176(1)
     71c: 7c 08 02 a6  	mflr 0
     720: 90 01 00 b4  	stw 0, 180(1)
     724: 39 61 00 b0  	addi 11, 1, 176
     728: 48 00 00 01  	bl 0x728 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x10>
			00000728:  R_PPC_REL24	_savegpr_27
     72c: c0 25 00 00  	lfs 1, 0(5)
     730: 3c 00 43 30  	lis 0, 17200
     734: c0 05 00 04  	lfs 0, 4(5)
     738: 7c 7b 1b 78  	mr	27, 3
     73c: fc 20 08 1e  	fctiwz 1, 1
     740: 90 01 00 78  	stw 0, 120(1)
     744: fc 00 00 1e  	fctiwz 0, 0
     748: 7c 9c 23 78  	mr	28, 4
     74c: 90 01 00 80  	stw 0, 128(1)
     750: d8 21 00 88  	stfd 1, 136(1)
     754: d8 01 00 90  	stfd 0, 144(1)
     758: 83 c1 00 8c  	lwz 30, 140(1)
     75c: 83 a1 00 94  	lwz 29, 148(1)
     760: 48 00 00 01  	bl 0x760 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x48>
			00000760:  R_PPC_REL24	getScreenHeight__2MRFv
     764: 7c 7f 1b 78  	mr	31, 3
     768: 48 00 00 01  	bl 0x768 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x50>
			00000768:  R_PPC_REL24	getFrameBufferWidth__2MRFv
     76c: 6c 60 80 00  	xoris 0, 3, 32768
     770: 3c 80 00 00  	lis 4, 0
			00000772:  R_PPC_ADDR16_HA	lbl_8053EB38
     774: 90 01 00 7c  	stw 0, 124(1)
     778: 6f e0 80 00  	xoris 0, 31, 32768
     77c: c0 20 00 00  	lfs 1, 0(0)
			0000077c:  Unknown	@60329
     780: 38 61 00 38  	addi 3, 1, 56
     784: c8 44 00 00  	lfd 2, 0(4)
			00000786:  R_PPC_ADDR16_LO	lbl_8053EB38
     788: c8 01 00 78  	lfd 0, 120(1)
     78c: fc 60 08 90  	fmr 3, 1
     790: 90 01 00 84  	stw 0, 132(1)
     794: ec 80 10 28  	fsubs 4, 0, 2
     798: c0 a0 00 00  	lfs 5, 0(0)
			00000798:  Unknown	@61471
     79c: c8 01 00 80  	lfd 0, 128(1)
     7a0: c0 c0 00 00  	lfs 6, 0(0)
			000007a0:  Unknown	@60328
     7a4: ec 40 10 28  	fsubs 2, 0, 2
     7a8: 48 00 00 01  	bl 0x7a8 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x90>
			000007a8:  R_PPC_REL24	C_MTXOrtho
     7ac: 38 61 00 38  	addi 3, 1, 56
     7b0: 38 80 00 01  	li 4, 1
     7b4: 48 00 00 01  	bl 0x7b4 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x9c>
			000007b4:  R_PPC_REL24	GXSetProjection
     7b8: 38 60 00 00  	li 3, 0
     7bc: 48 00 00 01  	bl 0x7bc <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xa4>
			000007bc:  R_PPC_REL24	GXSetCurrentMtx
     7c0: 38 61 00 08  	addi 3, 1, 8
     7c4: 48 00 00 01  	bl 0x7c4 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xac>
			000007c4:  R_PPC_REL24	PSMTXIdentity
     7c8: 38 61 00 08  	addi 3, 1, 8
     7cc: 38 80 00 00  	li 4, 0
     7d0: 48 00 00 01  	bl 0x7d0 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xb8>
			000007d0:  R_PPC_REL24	GXLoadPosMtxImm
     7d4: 48 00 00 01  	bl 0x7d4 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xbc>
			000007d4:  R_PPC_REL24	GXClearVtxDesc
     7d8: 38 60 00 09  	li 3, 9
     7dc: 38 80 00 01  	li 4, 1
     7e0: 48 00 00 01  	bl 0x7e0 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xc8>
			000007e0:  R_PPC_REL24	GXSetVtxDesc
     7e4: 38 60 00 00  	li 3, 0
     7e8: 38 80 00 09  	li 4, 9
     7ec: 38 a0 00 01  	li 5, 1
     7f0: 38 c0 00 04  	li 6, 4
     7f4: 38 e0 00 00  	li 7, 0
     7f8: 48 00 00 01  	bl 0x7f8 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xe0>
			000007f8:  R_PPC_REL24	GXSetVtxAttrFmt
     7fc: 38 60 00 00  	li 3, 0
     800: 48 00 00 01  	bl 0x800 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xe8>
			00000800:  R_PPC_REL24	GXSetNumTexGens
     804: 38 60 00 01  	li 3, 1
     808: 48 00 00 01  	bl 0x808 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xf0>
			00000808:  R_PPC_REL24	GXSetNumTevStages
     80c: 38 60 00 00  	li 3, 0
     810: 48 00 00 01  	bl 0x810 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0xf8>
			00000810:  R_PPC_REL24	GXSetTevDirect
     814: 38 60 00 00  	li 3, 0
     818: 38 80 00 04  	li 4, 4
     81c: 48 00 00 01  	bl 0x81c <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x104>
			0000081c:  R_PPC_REL24	GXSetTevOp
     820: 38 60 00 01  	li 3, 1
     824: 48 00 00 01  	bl 0x824 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x10c>
			00000824:  R_PPC_REL24	GXSetNumChans
     828: 38 60 00 04  	li 3, 4
     82c: 38 80 00 00  	li 4, 0
     830: 38 a0 00 00  	li 5, 0
     834: 38 c0 00 00  	li 6, 0
     838: 38 e0 00 00  	li 7, 0
     83c: 39 00 00 00  	li 8, 0
     840: 39 20 00 02  	li 9, 2
     844: 48 00 00 01  	bl 0x844 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x12c>
			00000844:  R_PPC_REL24	GXSetChanCtrl
     848: 38 60 00 05  	li 3, 5
     84c: 38 80 00 00  	li 4, 0
     850: 38 a0 00 00  	li 5, 0
     854: 38 c0 00 00  	li 6, 0
     858: 38 e0 00 00  	li 7, 0
     85c: 39 00 00 00  	li 8, 0
     860: 39 20 00 02  	li 9, 2
     864: 48 00 00 01  	bl 0x864 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x14c>
			00000864:  R_PPC_REL24	GXSetChanCtrl
     868: 38 60 00 00  	li 3, 0
     86c: 48 00 00 01  	bl 0x86c <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x154>
			0000086c:  R_PPC_REL24	GXSetCoPlanar
     870: 38 60 00 01  	li 3, 1
     874: 48 00 00 01  	bl 0x874 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x15c>
			00000874:  R_PPC_REL24	GXSetClipMode
     878: 38 60 00 00  	li 3, 0
     87c: 48 00 00 01  	bl 0x87c <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x164>
			0000087c:  R_PPC_REL24	GXSetCullMode
     880: 38 60 00 00  	li 3, 0
     884: 38 80 00 07  	li 4, 7
     888: 38 a0 00 00  	li 5, 0
     88c: 48 00 00 01  	bl 0x88c <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x174>
			0000088c:  R_PPC_REL24	GXSetZMode
     890: 38 60 00 07  	li 3, 7
     894: 38 80 00 00  	li 4, 0
     898: 38 a0 00 00  	li 5, 0
     89c: 38 c0 00 07  	li 6, 7
     8a0: 38 e0 00 00  	li 7, 0
     8a4: 48 00 00 01  	bl 0x8a4 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x18c>
			000008a4:  R_PPC_REL24	GXSetAlphaCompare
     8a8: 38 60 00 00  	li 3, 0
     8ac: 38 80 00 00  	li 4, 0
     8b0: 38 a0 00 00  	li 5, 0
     8b4: 38 c0 00 03  	li 6, 3
     8b8: 48 00 00 01  	bl 0x8b8 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x1a0>
			000008b8:  R_PPC_REL24	GXSetBlendMode
     8bc: 38 60 00 00  	li 3, 0
     8c0: 48 00 00 01  	bl 0x8c0 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x1a8>
			000008c0:  R_PPC_REL24	GXSetColorUpdate
     8c4: 38 60 00 01  	li 3, 1
     8c8: 48 00 00 01  	bl 0x8c8 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x1b0>
			000008c8:  R_PPC_REL24	GXSetAlphaUpdate
     8cc: 7f 64 db 78  	mr	4, 27
     8d0: 38 60 00 01  	li 3, 1
     8d4: 48 00 00 01  	bl 0x8d4 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x1bc>
			000008d4:  R_PPC_REL24	GXSetDstAlpha
     8d8: 38 60 00 90  	li 3, 144
     8dc: 38 80 00 00  	li 4, 0
     8e0: 38 a0 00 06  	li 5, 6
     8e4: 48 00 00 01  	bl 0x8e4 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x1cc>
			000008e4:  R_PPC_REL24	GXBegin
     8e8: c0 3c 00 00  	lfs 1, 0(28)
     8ec: c0 5c 00 04  	lfs 2, 4(28)
     8f0: c0 60 00 00  	lfs 3, 0(0)
			000008f0:  Unknown	@60329
     8f4: 48 00 00 01  	bl 0x8f4 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x1dc>
			000008f4:  R_PPC_REL24	GXPosition3f32
     8f8: 57 c0 04 3e  	clrlwi	0, 30, 16
     8fc: 3f e0 00 00  	lis 31, 0
			000008fe:  R_PPC_ADDR16_HA	lbl_8053EB40
     900: 90 01 00 7c  	stw 0, 124(1)
     904: c8 5f 00 00  	lfd 2, 0(31)
			00000906:  R_PPC_ADDR16_LO	lbl_8053EB40
     908: c8 21 00 78  	lfd 1, 120(1)
     90c: c0 1c 00 00  	lfs 0, 0(28)
     910: ec 21 10 28  	fsubs 1, 1, 2
     914: c0 5c 00 04  	lfs 2, 4(28)
     918: c0 60 00 00  	lfs 3, 0(0)
			00000918:  Unknown	@60329
     91c: ec 21 00 2a  	fadds 1, 1, 0
     920: 48 00 00 01  	bl 0x920 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x208>
			00000920:  R_PPC_REL24	GXPosition3f32
     924: 57 c3 04 3e  	clrlwi	3, 30, 16
     928: 57 a0 04 3e  	clrlwi	0, 29, 16
     92c: 90 61 00 84  	stw 3, 132(1)
     930: c8 5f 00 00  	lfd 2, 0(31)
			00000932:  R_PPC_ADDR16_LO	lbl_8053EB40
     934: 90 01 00 7c  	stw 0, 124(1)
     938: c8 21 00 80  	lfd 1, 128(1)
     93c: c8 01 00 78  	lfd 0, 120(1)
     940: ec 81 10 28  	fsubs 4, 1, 2
     944: c0 3c 00 00  	lfs 1, 0(28)
     948: ec 40 10 28  	fsubs 2, 0, 2
     94c: c0 1c 00 04  	lfs 0, 4(28)
     950: c0 60 00 00  	lfs 3, 0(0)
			00000950:  Unknown	@60329
     954: ec 24 08 2a  	fadds 1, 4, 1
     958: ec 42 00 2a  	fadds 2, 2, 0
     95c: 48 00 00 01  	bl 0x95c <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x244>
			0000095c:  R_PPC_REL24	GXPosition3f32
     960: c0 3c 00 00  	lfs 1, 0(28)
     964: c0 5c 00 04  	lfs 2, 4(28)
     968: c0 60 00 00  	lfs 3, 0(0)
			00000968:  Unknown	@60329
     96c: 48 00 00 01  	bl 0x96c <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x254>
			0000096c:  R_PPC_REL24	GXPosition3f32
     970: 57 c3 04 3e  	clrlwi	3, 30, 16
     974: 57 a0 04 3e  	clrlwi	0, 29, 16
     978: 90 61 00 84  	stw 3, 132(1)
     97c: c8 5f 00 00  	lfd 2, 0(31)
			0000097e:  R_PPC_ADDR16_LO	lbl_8053EB40
     980: 90 01 00 7c  	stw 0, 124(1)
     984: c8 21 00 80  	lfd 1, 128(1)
     988: c8 01 00 78  	lfd 0, 120(1)
     98c: ec 81 10 28  	fsubs 4, 1, 2
     990: c0 3c 00 00  	lfs 1, 0(28)
     994: ec 40 10 28  	fsubs 2, 0, 2
     998: c0 1c 00 04  	lfs 0, 4(28)
     99c: c0 60 00 00  	lfs 3, 0(0)
			0000099c:  Unknown	@60329
     9a0: ec 24 08 2a  	fadds 1, 4, 1
     9a4: ec 42 00 2a  	fadds 2, 2, 0
     9a8: 48 00 00 01  	bl 0x9a8 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x290>
			000009a8:  R_PPC_REL24	GXPosition3f32
     9ac: 57 a0 04 3e  	clrlwi	0, 29, 16
     9b0: c8 7f 00 00  	lfd 3, 0(31)
			000009b2:  R_PPC_ADDR16_LO	lbl_8053EB40
     9b4: 90 01 00 84  	stw 0, 132(1)
     9b8: c0 1c 00 04  	lfs 0, 4(28)
     9bc: c8 41 00 80  	lfd 2, 128(1)
     9c0: c0 3c 00 00  	lfs 1, 0(28)
     9c4: ec 42 18 28  	fsubs 2, 2, 3
     9c8: c0 60 00 00  	lfs 3, 0(0)
			000009c8:  Unknown	@60329
     9cc: ec 42 00 2a  	fadds 2, 2, 0
     9d0: 48 00 00 01  	bl 0x9d0 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x2b8>
			000009d0:  R_PPC_REL24	GXPosition3f32
     9d4: 38 60 00 00  	li 3, 0
     9d8: 38 80 00 00  	li 4, 0
     9dc: 48 00 00 01  	bl 0x9dc <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x2c4>
			000009dc:  R_PPC_REL24	GXSetDstAlpha
     9e0: 38 60 00 00  	li 3, 0
     9e4: 48 00 00 01  	bl 0x9e4 <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x2cc>
			000009e4:  R_PPC_REL24	GXSetClipMode
     9e8: 39 61 00 b0  	addi 11, 1, 176
     9ec: 48 00 00 01  	bl 0x9ec <clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>+0x2d4>
			000009ec:  R_PPC_REL24	_restgpr_27
     9f0: 80 01 00 b4  	lwz 0, 180(1)
     9f4: 7c 08 03 a6  	mtlr 0
     9f8: 38 21 00 b0  	addi 1, 1, 176
     9fc: 4e 80 00 20  	blr
