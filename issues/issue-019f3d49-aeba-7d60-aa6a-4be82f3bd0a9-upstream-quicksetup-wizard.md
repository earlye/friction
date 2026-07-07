# Upstream pull: Quick Setup wizard

## Context

Part of the upstream-pull breakdown tracked in
[issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md](issue-019f3869-037a-7843-94b7-da4b8f32ee02-pull-upstream-fixes.md).

Upstream added a first-run "Quick Setup" wizard (general settings +
install presets), built from new reusable wizard widgets. This is a
genuinely new feature, not a fix — larger review surface than the
other batches.

**This batch is a dependency for two other tracked pulls**:
[upstream-cpu-raster-default](issue-019f3d49-ae9f-77d0-ac16-20917aa9d657-upstream-cpu-raster-default.md)
and
[upstream-flatpak-earlysettings-fixes](issue-019f3d49-aeac-7bb1-a46c-eabe1fd4330a-upstream-flatpak-earlysettings-fixes.md),
each of which edit files this batch creates. Pull this one first.

Commits (oldest first):

- `41029ece5` — Ui: added WizardPage
- `400373669` — Ui: added CheckBoxes widget
- `d1cfa6d24` — Ui: added QuickSetupPresetsPage
- `c7ec58a7d` — Ui: added QuickSetupGeneralPage
- `092211f6a` — Update quicksetupgeneral.cpp
- `6fa448fc2` — Ui: added InstallPresets
- `34d0b0ff9` — Ui: added Quick Setup
- `6aa0cb58a` — App: use wizard for install presets from help
- `3f6dc71e0` — Update QuickSetup
- `f3c6e673c` — Add option to run Quick Setup from help menu
- `c0cdc5d71` — QuickSetup: enable "Backup On Save" as default

## Relevant files

- `src/ui/wizards/*` (new: `wizardpage.*`, `checkboxes.*`,
  `quicksetuppresets.*`, `quicksetupgeneral.*`, `quicksetup.*`,
  `installpresets.*`)
- `src/app/GUI/menu.cpp` / help-menu wiring

No overlap with fork-specific SVG import/flipbook-track code.
`f3c6e673c` and `6aa0cb58a` do touch `mainwindow.cpp`/`menu.cpp`, which
the fork also modifies (mechanical qDebug→qCDebug conversion) and
which
[upstream-misc](issue-019f3d49-af0e-7712-9466-95c9fc7e29f7-upstream-misc.md)
also touches (`e6d16e4e9`) — land this batch before `upstream-misc` to
avoid an avoidable textual conflict on `menu.cpp`.

## Next steps

- Cherry-pick the 11 commits above (in order) from `upstream/main`.
- Build and manually run through the first-run wizard end to end.
- Open a PR against `earlye/friction`.
