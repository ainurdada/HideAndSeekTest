## StateTree and architecture

`ST_Hider` runs on `AHideAndSeekAIController`. Custom tasks are C++; Blueprints connect components, input, animation intent, and presentation. Hider AI uses StateTree, with no Behavior Trees.

Simplified flow; arrows within a line represent successive states:

```text
Root
|-- WaitForHiding
|-- Unfound [Watch Player Sight, Watch Player Touch, Watch Revealed]
|   |-- UseHidingSlot [Claim Hiding Slot]
|   |   |-- SetMoveIntent -> MoveToGoal -> OccupyHidingSlot -> SetCrouchIntent
|   |   `-- HidingAtSpot [Hidden status]
|   |       `-- WaitForSearch -> Hidden
|   |-- Rehide -> UseHidingSlot
|   `-- WaitForSlot / MoveFailed -> Rehide
|-- Found [Found status]
|   `-- RevealNearby -> SetFoundIntent -> StartReaction -> FoundIdle
`-- SetupFailed
```

- **Round rules:** `AHideAndSeekGameMode` owns the four round phases, fixes the participant roster, and deduplicates discoveries. The final discovery skips revealing but still runs the reaction.
- **Slot ownership:** selection checks complete navigation paths and crouched cover. `UseHidingSlot` holds the claim until exit, including while hidden. Failed searches retry; the last revealed location is excluded within a configurable distance.
- **Detection:** a timer samples sight; both body sample points must be on screen and unobstructed. Touch and reveal use delegates and StateTree events. StateTree drives transitions.
- **Locomotion:** `IHiderLocomotion::SetLocomotionIntent` forwards gait and crouch intent to GASP Motion Matching.
- **Presentation:** `GE_HiderRevealed` controls outlines; `GE_HiderRevealReaction` drives `GC_HideAndSeek_FoundReaction` and its widget. `Apply Gameplay Effect` completes immediately; the GE owns its duration and outlives the task.

The ASC separates discovery status from the current hiding state:

| Tag | Meaning |
| --- | --- |
| `HideAndSeek.Status.Unfound` | Not yet found |
| `HideAndSeek.Status.Found` | Found; mutually exclusive with `Unfound` |
| `HideAndSeek.Status.Hidden` | Currently hiding; additional to `Unfound` |
| `HideAndSeek.Marker.Revealed` | Temporary reveal outline |
| `HideAndSeek.Reaction.Revealing` | Temporary cosmetic reaction |

## Limitations and next steps

- Cover visibility is checked **only at slot selection** against the current player camera. Movement afterward can expose that cover; continuous revalidation is a possible improvement.
- Dynamic participant spawning/removal is unsupported. Removing an unfound character does not reduce the round total.
- A pointing gesture, voice line, or context-sensitive betrayal message could make the reveal source clearer than the current text bubble.
- Additional AI spacing, GASP traversal, and peeking are not implemented. Animation polish and runtime path/slot-loss testing remain follow-up work.
