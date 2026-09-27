# Slide help

The band at the top of the screen that scrolls tips such as "To save a
screenshot in your Mu folder, press the 'Print Screen' key." is the *slide
help* (`CSlideHelpMgr` / `CUISlideHelp` in `src/source/UI/Legacy/UIControls.cpp`).
Players can switch it off in the options window.

It shows two kinds of text:

- **Tips**, picked at random from `Data/Local/<Lang>/slide_<lang>.bmd`
  according to the character level.
- **Server notices**, sent in a `0x0D` packet with a type of 10 to 15
  (see `ReceiveNotice` in `src/source/Network/Server/WSclient.cpp`). Types 13 to
  15 use the bold notice band, which takes priority over the tips.

## Editing the tips

`slide_<lang>.bmd` is the `SLIDEHELP` struct encrypted with `BuxConvert`.
`tools/slide-bmd.ps1` turns it into JSON and back:

```powershell
.\tools\slide-bmd.ps1 export src\bin\Data\Local\Spn\slide_spn.bmd slide_spn.json
# edit slide_spn.json
.\tools\slide-bmd.ps1 import slide_spn.json src\bin\Data\Local\Spn\slide_spn.bmd
```

The JSON looks like this:

```json
{
  "createDelay": 60,
  "speed": 2.5,
  "groups": [
    { "maxLevel": 15, "texts": ["...", "..."] }
  ]
}
```

- `createDelay`: seconds between two tips.
- `speed`: scroll speed.
- `groups`: exactly 5 groups. A character gets the tips of the first group
  whose `maxLevel` is at or above its level. Each group holds up to 32 texts of
  at most 255 bytes of UTF-8.

The client reads the texts as UTF-8. The export also accepts the legacy
Windows-1252 texts, and the import always writes UTF-8.
