# Cosmetics

A cosmetic piece changes how one piece of the equipment of your character
looks, without changing its stats: a helm, an armor, pants, gloves, boots,
wings or a pet. While you wear it, everybody sees it instead of the item you
have equipped in that slot: you, the other players, and the players who come
into view later. Pieces of different sets can be combined, e.g. a Dragon Helm
with a Black Dragon Armor.

- **Armor and wings** are shown also when the slot is empty.
- **A pet skin** belongs to one pet or mount, e.g. the Guardian Angel, the Imp
  or the Horn of Uniria, and is only shown while you have that one equipped.
  The pet keeps moving and doing what it does (a mount still carries you); only
  its model is the one of the skin. Over another pet, or without a pet, the
  real one is shown.

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

The window shows one slot at a time: the helm, armor, pants, gloves, boots,
wings and pet. Change it with the arrows at both sides of the name of the slot,
or with the **left** and **right** keys.

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

## Skins from skin packs

The models of skin packs are added as items of their own, without stats, and
offered as cosmetic pieces (see the admin panel, "Skins"). A pet skin is a
model of the pets (`Data/Items/Models/Group13_Helper.json`) with `"skinOf"`:
the number of the pet or mount whose look it replaces, e.g. `0` for the
Guardian Angel. The server shows the skin to the other players as the item in
the pet slot; the client draws the pet the skin is of with the model of the
skin. If the skin model has fewer animations than the model it replaces, it
plays its first one instead of the missing ones.
