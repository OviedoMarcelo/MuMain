# Cosmetics

A cosmetic piece changes how one piece of the armor of your character looks,
without changing its stats: a helm, an armor, pants, gloves or boots. While you
wear it, everybody sees it instead of the item you have equipped in that slot:
you, the other players, and the players who come into view later. Pieces of
different sets can be combined, e.g. a Dragon Helm with a Black Dragon Armor.

The server operator configures which pieces exist. A character unlocks them one
by one, as a reward (quests, achievements, the season pass, bosses) or from a
game master.

> Requires a server which offers cosmetic pieces. Against another server the
> button doesn't show up.

---

## Opening it

Open the inventory (**I** or **V**) and press the cosmetics button in the row of
buttons at the bottom. The window opens at the left of the inventory, where the
character window would be. **Escape** closes it.

## The window

The window shows one slot at a time. Change it with the arrows at both sides of
the name of the slot, or with the **left** and **right** keys.

Each row shows a piece of that slot which you unlocked, its name and its state.
The pieces you haven't unlocked yet aren't shown.

- **Worn** - you wear this piece.
- **Other class** - you unlocked it, but your class can't wear it, because the
  server only allows the pieces your class could equip.

Click a row to pick it. **Wear** puts on the picked piece instead of the one you
wore in that slot; **Take off** shows your real item of that slot again. The
worn pieces are remembered: you still wear them after logging in again.

## Chat commands

The window uses these commands, which can also be typed:

- `/cosmetico <number>` wears the piece with that number.
- `/cosmetico off` takes off all pieces, `/cosmetico off <number>` only that one.
- `/cosmeticos` lists your unlocked pieces with their numbers.

Game masters use `/darcosmetico <character> <number>` and
`/quitarcosmetico <character> <number>` to give and remove pieces.
