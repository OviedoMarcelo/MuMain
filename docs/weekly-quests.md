# Quests Window

Quests are missions which the server operator configures on the server: story
chapters, daily, weekly, class and zone quests. A quest has a single objective
(kill monsters, gain levels or resets, finish events, defeat other players,
collect items) or several steps, e.g. talk to an NPC, kill 50 Skeletons and
defeat a boss. The window shows the quests which are available for your
character, together with its progress.

When the progress starts over depends on the quest:

- **Daily** quests start over every day, **weekly** quests every week.
- **Story** chapters are done once. The next chapter appears when the previous
  one is completed.

> Requires a server which offers quests. Against another server the hotkey
> does nothing. A server which doesn't send categories and steps shows every
> quest as a weekly one with a single objective, like before.

---

## Opening it

Press **Y**. **Escape** goes back from the details of a quest to the list, and
closes the window on the list.

## The tracker

While you play, the quests which aren't done yet are shown at the top right of
the screen, in a frame titled **Misiones** (at most four; the story first, then
the quests with progress). An open quest shows:

- its name, with its type as a label (`HISTORIA`, `SEMANAL`, `DIARIA`...),
- what to do now, e.g. `Matá 50 Skeletons`,
- **Progreso** with the count and a bar, when the step has something to count,
- its rewards, next to a chest.

A folded quest shows only its name and its progress (`2/5`, or an orange `!`
when the reward waits for free space). A click on a quest opens or folds it, in
its place; at first only the first quest is open. The arrow in the title folds
the whole tracker. The quest window opens with **Y**. With a party, the tracker
moves to the left of the party list.

## Talking to an NPC

When the step to do now is to talk to an NPC, a click on the NPC opens a dialog
at the bottom of the screen: the portrait of the NPC with its name, the name of
the quest and what the NPC says. The talk only counts when you choose **Acepto
la misión.** **Ahora no, gracias.**, the cross or escape close it without
counting, and you can talk to the NPC again later. Walking away from the NPC
closes it, too. When the NPC has nothing to say, the talk counts right away.

The text is the "Texto del NPC" of the step in the admin panel; each line of it
is a line in the dialog. A long text is shown in pages of up to eight lines:
until the last one, the first button is **Continuar leyendo...**, and **Acepto
la misión.** shows up on the last page. The portrait is
`Data/Interface/NpcPortraits/<number of the NPC>.OZJ`, made from an image by
`tools/branding/make_portrait.ps1`; an NPC without one shows the MU Cyber
emblem. Some NPCs have particles over their portrait: golden motes over Elf Lala,
sparks of fire over the guards, blue magic over Marce (`NpcPortraitEffects.cpp`). Older clients don't show the dialog: the NPC says the text above its
head instead, and the talk counts right away.

## The quest list

The quests are grouped by their category, in the order story, daily, weekly,
class and zone, each group under its heading - also when there is only one
category, so that the type of a quest is always visible. Every quest is one row, with its progress and an arrow at the
right. The row under the mouse lights up, which is where a click opens the
details:

```
Historia
Capítulo I              1/4 >
Diarias
Patrulla diaria      40/100 >
Semanales
Cazador             250/500 >
Defensor del castillo    OK >
Duelista                  ! >
```

- `x/N` is the progress towards the objective. For a quest with several steps,
  it's the number of the steps which are done.
- `OK` (green) means the quest is done and its rewards were handed out.
- `!` (orange) means the quest is done, but its rewards are still pending,
  usually because the inventory was full. Free some space; the rewards are
  handed out the next time you enter the game or type `/weekly`.

The line above the buttons tells how long it takes until the weekly reset. When
there are no weekly quests, it tells the daily reset instead, and when all
quests are done once (like story chapters), it's empty.

Clicking a quest shows its details, laid out like the quest window of the
original client: the name as a cyan heading, below it the type of the quest
and when it resets (e.g. `Semanal · Se reinicia en 1d 0h 24m`, or
`Historia · No se reinicia`), the description, and yellow headings for the
progress and the rewards. A quest with several steps
shows them as a checklist:

```
Pasos:
[x] Habla con el Guardián
[  ] Mata 50 Skeletons (12/50)
[  ] Encuentra el Fragmento del Chaos
[  ] Derrota al jefe
```

Done steps are green. When the steps have to be done in their order, only the
current one counts, and the ones after it are grey. **Back** returns to the list.

## Where the data comes from

The quests, their texts and the progress are sent by the server when the
character enters the game, and the progress is updated while you play. Nothing
is stored on your computer - the texts are whatever the server operator
configured, in the language the server uses.

## Season pass

When the server runs a season pass, the list page has a **Pass** button at the
bottom right. It opens the pass of your account: the season, whether it's the
free or the premium pass, when the season ends, your level and the experience
towards the next one, and every level with its rewards.

- Quests and playing give experience of the pass. All characters of the account
  share it.
- The free rewards are for everyone, the premium ones only with the premium
  pass. Rewards of a level which isn't reached yet, or premium rewards without
  the premium pass, are greyed out. Received rewards are marked.
- **Claim** hands out all rewards of the reached levels which you didn't receive
  yet. It's the same as typing `/pase reclamar`, and only shows up while there
  is something to claim. When the inventory is full, the rewards stay claimable.
