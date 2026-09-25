# Delete preview playback and simulated audio state

Deleted eight PCM/stream/modifier compat files, the JAudioPlaybackService pair, and the unused scenario/particle ownership pairs. RuntimeContext no longer owns an audio backend, accepts playback injection, or simulates/queries audio requests. RuntimeServices and ParityTrace no longer retain simulated audio events or fade state. The actual Game owners supply required behavior; there is no replacement runtime audio service.

OriginalProcess explicitly destroys AudSystemWrapper after scene/actor retirement and before FileLoader/GameSystem/heap release. Actual wrapper destructor unregisters its heap finalizer. The shared ownership contract keeps registered sound-object cleanup independent of global publication.

Build wiring enables the restored 18 AudioLib sources and five canonical JAU/JAS implementation units. Original scalar audio arithmetic disables FMA contraction. Remaining DSP/rhythm/speaker providers stay inactive unless a concrete link needs their proper owner. No build/tests run for this partial lane; root validates only the complete integrated batch.

Additional canonical links: SpkSystem/SpkSpeakerCtrl reconnect and volume query methods are copied from the existing donor. Aurora exposes WPAD speaker-enabled status (false for its current speakerless host devices) and the canonical SC-backed speaker volume query. JAUSection's wave-load/erase entry points explicitly return unavailable; JAUSectionHeap's existing donor loaded-state query is restored. No archive or voice is fabricated.
