# 1. Safe update, backups and recovery

[All tutorials](README.md) · [Full FAQ](../FAQ.md) · [Detailed recovery](../../INSTALL_RECOVERY.txt)

1. Stop playback and turn MASTER fully down. Keep USB and power reliable; close other MIDI clients.
2. Save your own current project. Export it twice using `tools/x0x_backup.py backup before-01.x0b` and a different second filename. Stopped export can commit unsaved work. Preserve both and require byte-identical files with valid CRCs.
3. Retain your exact previous firmware and compatible data, plus any separate sample/calibration backups you depend on. Do not use somebody else's full project to install a preset.
4. Check the `.fwsc` SHA-256 against the supplied checksum file. Use `python3 tools/fm1_install.py --info` to read identity, then the documented installer with the candidate `.fwsc`. Keep the confirmation prompt; do not add `--force` or `--yes` for this workflow.
5. Keep power and USB connected while writing. Wait for HOME and completed startup. Read identity: `FM-1_9000160`.
6. Verify the saved project, presets and calibration before playback. Begin at a quiet level. The measured test history is in the release notes; it is not a guarantee for arbitrary workloads.

If an update fails, retain its log and inspect the actual running/loader state before another action. Do not repeatedly retry or automatically restore a project. The included .59 asset is a specific engineering rollback, not universal factory firmware; read the format compatibility and restore-replacement notes first.
