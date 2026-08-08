# Stage Placement Report
phase: preflight
stage: FileSelect
scenario: 1
total_objects: 4
complete_objects: 2
blocked_objects: 2
intentionally_ignored_objects: 0

## Objects
- status: complete
  object: InvisibleWall10x10
  zone: FileSelect
  zone_id: 0
  table: jmp/placement/common/objinfo
  row: 0
  child_count: 0
  common_path_id: -1
  rail_info_attached: false
  rail_path_row: -1
  rail_point_count: 0
  support_reason: original_factory
  archive: /ObjectData/InvisibleWall10x10.arc
- status: blocked
  object: FileSelector
  zone: FileSelect
  zone_id: 0
  table: jmp/placement/common/objinfo
  row: 1
  child_count: 0
  common_path_id: -1
  rail_info_attached: false
  rail_path_row: -1
  rail_point_count: 0
  support_reason: retail_file_select_actor_runtime_unavailable
  archive:
- status: blocked
  object: SphereSelectorHandle
  zone: FileSelect
  zone_id: 0
  table: jmp/placement/common/objinfo
  row: 2
  child_count: 0
  common_path_id: -1
  rail_info_attached: false
  rail_path_row: -1
  rail_point_count: 0
  support_reason: atmosphere_level_sound_playback_runtime_unavailable
  archive:
- status: complete
  object: GlobalPlaneGravityInBox
  zone: FileSelect
  zone_id: 0
  table: jmp/placement/common/planetobjinfo
  row: 0
  child_count: 0
  common_path_id: -1
  rail_info_attached: false
  rail_path_row: -1
  rail_point_count: 0
  support_reason: original_factory
  archive:
