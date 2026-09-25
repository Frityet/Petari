# Native entry points use the actual Scene

OriginalGameApplication obtains GameSystemSceneController::mScene and calls beginNativeFrame on that actual scene, when one exists. SceneFunction's effect initialization uses that same owner. The Game Xmake source glob for ../scene is removed.

The prior scene-initialization scope and current-state getter had no production callers outside their own definitions. Production already drives the actual controller state through original MR setters. Their singleton binding is therefore deleted without a replacement service; existing retained fixtures now use the actual controller fields.

A follow-up ownership correction retains the previous SceneScheduler::clear behavior by clearing the actual draw category callbacks after registration removal. It does not restore callback history. Scene teardown already cleared all callback lists through its executor; this also covers direct clear/reconnect use.

No additional tests or fixtures added. One integrated build and a single fresh-save opening run will validate the batch.
