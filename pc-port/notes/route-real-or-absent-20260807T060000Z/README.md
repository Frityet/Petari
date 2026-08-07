# Title route real-or-absent checkpoint

## Command

```text
xmake aurora-route-smoke --no-build \
  --disc=/workspaces/pcport/RMGK01.iso \
  --work-dir=/workspaces/pcport/pc-port/notes/route-real-or-absent-20260807T060000Z \
  --timeout=90 --display=500 title
```

## Result

The Aurora application opened the real RMGK01 image and initialized the real
Vulkan renderer, but exited before the frame-90 trace and screenshot were
written. The first reported missing closure was:

```text
GameDataHolder operation is unavailable without retail backing data: retail BinaryDataChunkHolder serialization
```

This is an honest failure rather than a forced title/file-select transition.
The complete application log and failed route manifest are retained beside
this note.

The run also produced `title/save/config1`, whose first four bytes are the old
invented `CFG1` signature. That binary is retained as direct evidence for the
next save-system cleanup: the host config serializer and partial/legacy save
container paths must be removed, and persistence must use the actual retail
`BinaryDataChunkHolder` closure or not exist.

No screenshot exists because the application exited before the requested
capture frame.
