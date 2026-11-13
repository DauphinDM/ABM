# Design-to-code review

This is a focused maintenance review, not the whole next-update plan. The source and changelog already contain 0.11 work, while several research documents still describe a 0.10 planning baseline. Keep those documents as design and evidence records. Don't treat every proposed feature as missing.

## Reasoning and implementation

| Design requirement | What the source showed | What we improved |
| --- | --- | --- |
| [Slice C](next-update-implementation-plan.md#6-slice-c-accurate-preview-fallback-and-provenance): decode an image before accepting it. | `Previews::requestImage` used to pick the first nonempty byte array and decode only once. A corrupt mounted image also excluded loose candidates entirely. | Build an ordered, limited list of local candidates. Decode each one and move on if it's missing, corrupt, oversized, or unreadable. Keep the reported mounted origin first. |
| [Preview research](next-feature-requests.md#32-accurate-previews-without-shipping-an-image-library): never show another vehicle's photo, and label model fallbacks honestly. | `VariantCard.qml` assigned directly to `Image.source` on error, which removed its model-data binding. Reusing the card could keep the previous car's fallback and label. | Keep the source declarative. Reset failure state when inputs change. Label a model fallback only after it decodes successfully. |
| Slice C: limit decoding and cache work, and retry only after the revision changes. | The provider limited individual reads but repeated archive scans and failed decodes for repeated card loads. Loose fallback source metadata wasn't part of its identity. | Cache requested-size thumbnails and failures within a 64 MiB cost budget, and serialize misses. Include ordered candidate paths, sizes, and modification times in opaque IDs. Re-registering after a catalog refresh picks up changed source metadata. |
| [Architecture](architecture.md): C++ owns I/O; QML owns presentation. | Provider loading and UI-level model fallback were already separate. | Keep those boundaries. No protocol changes, downloads, artwork bundle, simulation actions, or generic framework. |

We kept the existing stricter **8 MiB** encoded-image limit instead of adopting the plan's proposed 16 MiB budget. A candidate is always the same virtual image path. The UI's final model fallback comes from the selected model's catalog item. Files aren't searched by display name. Source paths are approved and canonicalized at registration, then checked again before reading.

## Validation coverage

New deterministic Qt cases cover:

- Mounted origin priority, corrupt ZIP entry fallback, and corrupt loose fallback.
- Cached successful and failed decoding, repaired or removed or replaced source registration, original-size reporting, and requested-size limits.
- Oversized bytes and dimensions, symlink escapes, and a source changed after registration. Symlink cases are Unix-only.
- Real `VariantCard.qml` transitions from a failed exact image to a model fallback, then another car's exact image, no image, a failed model image, and a model-only image.

Preview-maintenance validation before the dashboard redesign, based on `182458a`:

- `./bin/test`: **43 Qt checks passed**, plus the Lua bridge, Node plugin/filter, headless CLI, and standalone lifecycle tests. The corrupt and missing-image fixtures are expected to emit decoder and image warnings. No test failed.
- Demo Garage and configuration captures at 440×700 were inspected, including loaded photos, **No preview**, and **Model preview**. Both capture commands finished without console warnings. Local evidence: `/tmp/opencode/abm-preview-review/{garage,configs}.png` (not release assets).
- `git diff --check` passed. Only documentation changed after the test run. No production or test behavior changed afterward.

Automated fixtures aren't proof of native mounted-file precedence or actual game behavior. That preview-only audit didn't cover wider windows or Windows.

## Follow-up: dashboard redesign

The UI overhaul prioritizes legibility and usable content space over the old decorative glass treatment. Solid dark surfaces, a restrained accent, larger instruments, padded Workshop groups, and consistent controls create a clearer hierarchy. The existing style identifier stays for compatibility.

Adaptive tabs and sidebar navigation plus a one-to-four-column Garage grid cover the plan's narrow and wide layout requirement. Extra filters move into a scrollable dialog instead of taking space from the gallery at small sizes. Card focus, favorites, menus, checkboxes, and confirmation buttons keep native keyboard interaction. Dangerous confirmations name the action instead of just saying "OK."

The redesign keeps the preview bindings above, plain-text rendering, the live theme accent, stable player rows, and existing game-action guards. It doesn't add new simulation behavior or complete the remaining feature plan. See the [current visual audit](ui-audit.md#unreleased-dashboard-redesign) for responsive-layout tests and capture evidence.

## Remaining boundaries

- Mounted precedence uses the bridge's existing origin descriptor, then approved same-path local fallbacks. Arbitrary mod mount ordering hasn't been verified against a running game. No default-placeholder asset or capture API is invented.
- Refresh invalidation is metadata-based and happens at registration, not through a filesystem watcher. A replacement with the same path, size, and modification time needs an ABM restart.
- Preview status is currently UI-level exact, model, or unavailable. It isn't the plan's full per-source diagnostic panel or an explicit cache-clear action.
- The registered source table keeps its existing 8192-entry limit. Cache cost limits retained images. Transient decoder memory and bookkeeping are extra.
- The larger plan still needs its own item-by-item evidence ledger. Native configuration pairing, native latency comparison, and real physics validation must not be called complete based on these tests. This review doesn't change server-policy or ownership gates and doesn't implement all of Slices B through K.
