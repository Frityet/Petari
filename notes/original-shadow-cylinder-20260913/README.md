# Original cylindrical shadow rendering

Recovered ShadowVolumeCylinder::loadModelDrawMtx from retail 0x8016E55C in decomp first, then imported the complete source/header to the PC port. It orients the actual cylinder model against the drop direction, starts at the controller offset, follows the original optional host X scale, stretches to the original cut/drop length and loads the camera-relative GX matrix. The reference compiler succeeds; object comparison is 99.90% (248-byte retail function).

The common native shadow controller owner now constructs the actual original sphere or cylinder drawer from the authored definition, with shared offset/cut configuration and ordinary scene draw registration. Other shadow shapes remain separate unfinished work; this does not activate a fallback drawer or fabricate geometry for them.

The production smg-pc sixth build succeeds with this drawer included. No component test or pixel claim was added; actual startup stops before game frames at the layout owner boundary.
