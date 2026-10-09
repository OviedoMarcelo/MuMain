# MU Cyber branding

Scripts that put the MU Cyber look into the client data (`src/bin/Data`). They need Python 3 and Pillow.

- `make_branding.py <logo.png> <banner> <src/bin/Data> <preview dir>` writes the login logo
  (`Logo/MU-logo.OZT` and its glow `Logo/MU-logo_g.OZJ`) and the loading screens: the first one, before
  the login (`Interface/New_lo_back_0*`, `lo_back_s5_0*`, `lo_back_im0*`, `lo_back_s5_im0*`), and the one
  shown when entering the world (`Interface/LSBg0*`). It also saves PNG previews.
  The logo and banner are the website's: `mu-web/public/brand/logo.png` and `banner.webp`.
- `serverlist.py <Local/ServerList.bmd>` shows the server groups; with `--rename OLD NEW` it renames one.

Formats: an OZJ is a JPEG behind a 24-byte header the client skips; an OZT is a 32-bit uncompressed TGA
behind a 4-byte header. The loading screens are drawn in pieces at fixed places (see
`CUIMng::CreateTitleSceneUI` and `CLoadingScene::Create`), so every piece must keep its original size.
