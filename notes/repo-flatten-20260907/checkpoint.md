Flattened pc-port/ into the root and added decomp/ tracking pcp-decomp.
Moved existing submodules with git mv. Preserved existing PC edits unchanged.
Copied the two modified original decomp files into the decomp checkout, along
with AGENT_DECOMP_GUIDE.md. The branch differs from the previously embedded
source tree; the full original tree and initial patch are backed up externally.
Five headers used through the old parent include directory are now in src/Game.
Updated launcher, audit, source fixture paths, debug discovery, and container paths.
User requested stopping and committing the current checkpoint before validation.
Outstanding: native builds/tests, CI/editor updates, complete path audit.
Initial game/player mirror checks failed; baseline results are in the backup.
Backup: /Users/frityet/Projects/petari-cleanup-backup-20260907T091745
