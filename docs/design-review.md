# Design-to-code review

This is a focused maintenance review, not completion of the entire next-update
plan. The source/changelog already contain 0.11 work while several research
documents still describe a 0.10 planning baseline. Preserve those documents as
design/evidence records rather than treating every proposed feature as absent.

## Reasoning and implementation

| Design requirement | Source finding | Improvement |
| --- | --- | --- |
| [Slice C](next-update-implementation-plan.md#6-slice-c--accurate-preview-fallback-and-provenance): decode before accepting an image. | `Previews::requestImage` previously selected the first nonempty byte array and decoded only once. A corrupt mounted image also excluded loose candidates entirely. | Build a bounded ordered local candidate list; decode each candidate and advance on missing, corrupt, oversized or unreadable content. Keep the reported mounted origin first. |
| [Preview research](next-feature-requests.md#32-accurate-previews-without-shipping-an-image-library): never show another vehicle's photo and label model fallbacks honestly. | `VariantCard.qml` assigned directly to `Image.source` on error, removing its model-data binding. Reusing the card could retain the prior car's fallback and label. | Keep the source declarative, reset failure state when inputs change and label model fallback only after successful decoding. |
| Slice C: bound decoding/cache work and retry only after revision changes. | The provider bounded individual reads but repeated archive scans and failed decodes for repeated card loads. Loose fallback source metadata was absent from its identity. | Cache requested-size thumbnails and failures with a 64 MiB cost budget; serialize misses. Include ordered candidate paths, sizes and modification times in opaque IDs. Re-registering after a catalog refresh picks up changed source metadata. |
| [Architecture](architecture.md): C++ owns I/O; QML owns presentation. | Provider loading and UI-level model fallback are already separate. | Retain those boundaries; no protocol changes, downloads, artwork bundle, simulation actions or generic framework. |

The existing stricter **8 MiB** encoded-image limit is retained rather than
silently adopting the plan's proposed 16 MiB budget. A candidate is always the
same virtual image path; the UI's final model fallback comes from the selected
model's catalog item. Files are not searched by display name. Source paths are
approved/canonicalized at registration and checked again before reading.

## Validation coverage

New deterministic Qt cases exercise:

- Mounted origin priority, corrupt ZIP entry fallback, corrupt loose fallback.
- Cached successful and failed decoding, repaired/removed/replaced source
  registration, original-size reporting and requested-size limits.
- Oversized bytes/dimensions and symlink escapes, including a source changed
  after registration (symlink cases are Unix-only).
- Real `VariantCard.qml` transitions from failed exact image to model fallback,
  then another car's exact image, no image, failed model image and model-only image.

Preview-maintenance validation before the subsequent dashboard redesign, based
on `182458a`:

- `./bin/test`: **43 Qt checks passed**, plus Lua bridge, Node plugin/filter,
  headless CLI and standalone lifecycle tests. The corrupt/missing-image fixtures
  intentionally emit decoder/image warnings; no test failed.
- Demo Garage and configuration captures at 440×700 were inspected, including
  loaded photos, **No preview** and **Model preview**. Both capture commands
   completed without console warnings. Local evidence:
  `/tmp/opencode/abm-preview-review/{garage,configs}.png` (not release assets).
- `git diff --check` passed. Only documentation changed after the test run;
  no production/test behavior was changed afterward.

Automated fixtures are not proof of native mounted-file precedence or actual
game behavior. That preview-only audit did not cover wider windows or Windows.

## Follow-up: dashboard redesign

The requested UI overhaul prioritizes legibility and usable content space over
the old decorative glass treatment. Solid dark surfaces, restrained accent,
larger instruments, padded Workshop groups and consistent controls establish a
clearer hierarchy. The existing style identifier remains for compatibility.

Adaptive tabs/sidebar navigation and a one-to-four-column Garage grid address
the plan's narrow/wide layout requirement. Extra filters move into a scrollable
dialog rather than consuming the gallery at small sizes. Card focus, favorites,
menus, checkboxes and confirmation buttons keep native keyboard interaction;
dangerous confirmations name the action rather than just saying “OK”.

The redesign preserves the preview bindings above, plain-text rendering,
live theme accent, stable player rows, and existing game-action guards. It does
not implement new simulation behavior or complete the remaining feature plan.
See the [current visual audit](ui-audit.md#unreleased--dashboard-redesign) for
responsive-layout tests and capture evidence.

## Remaining boundaries

- Mounted precedence uses the bridge's existing origin descriptor, then approved
  same-path local fallbacks. Arbitrary mod mount ordering has not been verified
  against a running game. No default-placeholder asset or capture API is invented.
- Refresh invalidation is metadata-based and happens on registration, not via a
  filesystem watcher. Equal-path/size/mtime replacements need an ABM restart.
- Preview status is currently UI-level exact/model/unavailable, not the plan's
  full per-source diagnostic/provenance panel or explicit cache-clear action.
- The registered source table retains its existing 8192-entry bound. Cache cost
  bounds retained images; transient decoder memory/bookkeeping are additional.
- The larger plan still requires its own item-by-item evidence ledger. In
  particular, native configuration pairing, native latency comparison and real
  physics validation must not be called complete from these tests. This review
  neither changes server-policy/ownership gates nor implements all Slices B–K.
