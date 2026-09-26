# Weekly Quests Window

Weekly quests are missions which the server operator configures on the server
(kill monsters, gain levels or resets, finish events, defeat other players).
The progress restarts every week. The window shows the quests of the current
week together with the progress of your character.

> Requires a server which offers weekly quests. Against another server the
> hotkey does nothing.

---

## Opening it

Press **Y**. **Escape** goes back from the details of a quest to the list, and
closes the window on the list.

## The quest list

Every active quest is one row, with its progress and an arrow at the right.
The row under the mouse lights up, which is where a click opens the details:

```
Cazador                250/500 >
Defensor del castillo       OK >
Duelista                     ! >
```

- `x/N` is the progress towards the objective.
- `OK` (green) means the quest is done and its rewards were handed out.
- `!` (orange) means the quest is done, but its rewards are still pending,
  usually because the inventory was full. Free some space; the rewards are
  handed out the next time you enter the game or type `/weekly`.

The line above the buttons tells how long it takes until the weekly reset.

Clicking a quest shows its details, laid out like the quest window of the
original client: the name as a cyan heading, the description below it, and
yellow headings for the progress and the rewards. **Back** returns to the list.

## Where the data comes from

The quests, their texts and the progress are sent by the server when the
character enters the game, and the progress is updated while you play. Nothing
is stored on your computer - the texts are whatever the server operator
configured, in the language the server uses.
