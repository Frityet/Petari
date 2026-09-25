# Original SDK API surface needed by the broad Game import

Aurora GD now provides GDPosition3f32, GDTexCoord2f32, GDColor4u8, GDBegin and GDEnd through its existing bounded GD byte writer. These retain display-list recording semantics. GXCmd1f32 writes to Aurora's normal FIFO decoder. RVLFaceLib exposes the original 0x4C-byte RFLStoreData record.

JPA's previously misdeclared pointer at 0x78 is the original 32-bit user work word; accessor methods and particle initialization/copy now use that actual type. JKRArchive owns getExpandedResSize instead of a definition hidden inside KinopioAstro; mounted native resources are already expanded, and its implementation returns the existing resource size.

Missing animation defaults, joint-index controller creation, DrawType_None, Color10 GX conversion and Mercator area declarations were restored from existing donor code. Original pointer-to-temporary clipping offsets are kept as the port's existing by-value return convention in all imported overrides.

No standalone SDK test suite was added or run in this batch. Integrated build and opening-run results are recorded at the batch root.

The broader original OceanSphere source exposed GDTexCoord2f32 being defined only in the port Revolution wrapper. It now belongs to Aurora GDBase; the port header only forwards to that owner. GDBegin explicitly narrows its two enum values before combining the command byte.
