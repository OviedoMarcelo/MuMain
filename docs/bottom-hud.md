# Bottom HUD

The bar at the bottom of the screen uses the "S8" artwork: one connected frame
with the HP and mana orbs on its sides. From left to right:

| Area | What it shows | How to use it |
|---|---|---|
| Round buttons, left | Shop, Character, Inventory | Click to open or close; they light up while their window is open |
| Left orb | HP (green while poisoned) | Hover for current / max |
| Orange bar over Q/W/E/R | SD (shield) | Hover for current / max |
| Q / W / E / R | Potions and other quick items | Hover an item in the inventory and press the key to assign it; press the key or right-click the slot to use it |
| S slot | Current skill | Click to open the skill list above the bar and pick another skill |
| Five slots | Skill hotkeys 1-5 (or 6-0) | Click to make that skill current |
| Purple bar | AG | Hover for current / max |
| Right orb | Mana | Hover for current / max |
| Round buttons, right | Quest (T), Friends (F), Menu (U) | Click to open or close |
| Bottom strip | Experience through the current tenth of the level | Hover for the exact numbers |
| Green pill | Total experience of the level, in percent | — |

The character button blinks while a quest is waiting; the friends button
blinks on new chat messages or unread mail.

## Scaling

The bar keeps its proportions: it fills the width of a 4:3 window and is
centered on wider ones, leaving the sides free for the game world. Clicks only
count as "on the HUD" over the visible frame, the gauge row and the orbs.

## Changing the artwork

The art lives in `Data/Interface`:

| File | Part |
|---|---|
| `MenuS8_Main.OZT` | Frame (935x110, with see-through holes for the orbs) |
| `MenuS8_red`, `MenuS8_green`, `MenuS8_blue`, `MenuS8_black` (`.OZT`) | Orbs: HP, HP while poisoned, mana, empty (90x90) |
| `MenuS8_SD.OZJ`, `MenuS8_AG.OZJ` | Gauge fills (122x16) |
| `MenuS8_shop`, `_character`, `_inventory`, `_quest`, `_friend`, `_btmenu` (`.OZT`) | Buttons (27x28) |

A replacement with the same sizes and layout works as a drop-in. If the frame
changes shape, update the texel positions in
`src/source/UI/NewUI/HUD/MainFrameLayout.cpp`: every slot, gauge, orb and
button is measured there in pixels of `MenuS8_Main`, and both drawing and click
detection read from it.
