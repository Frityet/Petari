# Remove unused player override bridge

Deleted PlayerUtilCompat.cpp/.hpp. Their only read function, active_player_system_for_player_util, had no consumers in source, scripts, or tests after original Mario owner migration. The scoped override constructors in GatewaySpinCheckpoint and four existing fixtures only wrote otherwise unread thread-local state. Removed those constructors and unused includes; actual PlayerSystemService attachment and original Mario lookup remain unchanged.

No replacement facade, compatibility alias or new tests. Existing assertions are untouched. The production build is the integration check; this removal does not claim the old bounded checkpoint proves Gateway story progression.
