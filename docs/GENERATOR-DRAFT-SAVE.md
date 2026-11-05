# Generated songs and explicit project saves (ADD beta)

GEN works in a RAM draft. Existing A–D sections do not prevent generation.
The first generation checkpoints pending saved-project sections and its index,
then borrows the existing four section buffers. Further generations, section
edits and idle autosave do not write that draft over a saved project.

MAKE can regenerate while playback runs: transport stops, the next draft is
generated, and playback resumes in the new draft's mode. This is not a gapless
transition. GEN 1, 2 and 3 all show real sections and arrangement progress.

SAVE > SAVE SONG AS selects a destination and SCOPE:

- SONG saves A–D and the complete arrangement, scenes, track sources and variations.
- LOOP saves the currently loaded music in A and clears B–D and the arrangement
  at that destination.

The current project is a valid destination. Occupied destinations require an
overwrite confirmation, then naming. Saving requires stopped transport. Saving
keeps the draft available for more generations; power-off discards an unsaved
draft. SAVE > PROJECTS selects saved destinations without loading them during a
draft; LOAD opens the selected saved project and discards the draft.

SAVE > PROJECTS > ERASE clears all saved parts and arrangement at the selected
destination after confirmation. SAVE > SECTIONS > ERASE clears the selected
part and removes arrangement rows referencing it, including per-track sources.
Erasing a draft part is RAM-only. Erasing a saved destination does not erase
the active draft.

Each storage object retains its existing A/B commit protection. A multi-part
save or erase is not atomic across all four parts and the index: retry an
interrupted operation. Failed saves retain the draft for retry. No flash map,
loader, project format or audio buffer changes are involved.

Validation: native NOR tests cover source preservation, both scopes, same-project
overwrite, failure/retry, project erase and dependent-row cleanup. Generator UI
tests cover 45 genre choices, distinct parts, playback traversal, regeneration
during playback and all GEN pages. Target memory checks and the ordinary pinned
audio layout must pass before publication. This release goes to beta only.
