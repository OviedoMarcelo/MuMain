#pragma once

// The look of the Cyber Shop and of its message boxes, taken from the website (mu-web/app/globals.css) and from the
// quest windows: a violet black night, gold borders and ember for the actions. Everything is drawn with colors, so
// the shop needs no textures. The positions and sizes are in coordinates of the reference resolution.

namespace SEASON3B
{
class CNewUIMessageBoxButton;
}

namespace GameShop::CyberStyle
{
constexpr unsigned int WindowFill = 0xF6120A18u;
constexpr unsigned int WindowBorder = 0xFFA07C44u;
constexpr unsigned int GridLine = 0x0EFF8C64u;
constexpr unsigned int HeaderLine = 0xFF7A5E34u;

constexpr unsigned int TabFill = 0xFF140C18u;
constexpr unsigned int TabHoverFill = 0xFF21131Fu;
constexpr unsigned int TabBorder = 0xFF5A4A3Cu;
constexpr unsigned int ActiveFill = 0xFF2A1622u;
constexpr unsigned int ActiveBorder = 0xFFF2C76Au;
constexpr unsigned int Ember = 0xFFFF8A55u;

constexpr unsigned int CardFill = 0xF00C0810u;
constexpr unsigned int CardBorder = 0xFF3C3236u;
constexpr unsigned int CardHoverBorder = 0xFFF2C76Au;
constexpr unsigned int EmptySlotMark = 0xFF2E252Cu;

constexpr unsigned int ActionFill = 0xFF3A1A14u;
constexpr unsigned int ActionHoverFill = 0xFF5A281Cu;
constexpr unsigned int ActionBorder = 0xFFFF8A55u;
constexpr unsigned int ActionHoverBorder = 0xFFFFB48Cu;

constexpr unsigned int PanelFill = 0xFF180C1Fu;
constexpr unsigned int PanelBorder = 0xFFA07C44u;
constexpr unsigned int Separator = 0xFF5A4A30u;
constexpr unsigned int ListFill = 0xF00C0810u;
constexpr unsigned int ListBorder = 0xFF4A4236u;
// The line which is selected in a list: a soft gold glow, so the text over it stays readable.
constexpr unsigned int SelectedLineFill = 0x50F2C76Au;

constexpr unsigned int CoinBorder = 0xFFFFE9A8u;
constexpr unsigned int CoinTop = 0xFFF2C76Au;
constexpr unsigned int CoinBottom = 0xFFB98A2Cu;

constexpr unsigned int IconColor = 0xFFC8B89Au;
constexpr unsigned int IconHoverColor = 0xFFFFE6A0u;
constexpr unsigned int IconFill = 0xFF1A1816u;
constexpr unsigned int IconBorder = 0xFF6A5A40u;
constexpr unsigned int CloseColor = 0xFFC8A060u;

struct TextColor
{
    unsigned char Red;
    unsigned char Green;
    unsigned char Blue;
};

constexpr TextColor GoldText = {242, 199, 106};
constexpr TextColor GoldLightText = {255, 233, 168};
constexpr TextColor WhiteText = {241, 233, 238};
constexpr TextColor MutedText = {179, 163, 180};
constexpr TextColor EmberText = {255, 138, 85};

constexpr float WindowCorner = 8.0f;
constexpr float SmallCorner = 3.0f;
constexpr float GridStep = 40.0f;

// The height of the title of a message box, with the line under it.
constexpr int MessageBoxTitleHeight = 28;

void RenderTextAt(int x, int y, const wchar_t* text, int width, TextColor color, bool isBold, int sort);
int MeasureText(const wchar_t* text);

// A button for an action, like buying or using an item.
void DrawActionButton(float x, float y, float width, float height, bool isHovered);

// A quiet button, like cancelling.
void DrawSecondaryButton(float x, float y, float width, float height, bool isHovered);

// A dark box inside a window, for a list, for the item or for a text.
void DrawInset(float x, float y, float width, float height);

// The Cyber Coin: a gold diamond.
void DrawCoin(float centerX, float centerY, float radius);

// A message box of the shop: the window with a faint grid, and the title in gold at the top.
void DrawMessageBox(float x, float y, float width, float height, const wchar_t* title);

// A button of a message box in the shop. The buttons keep their own size, text and state; only the look changes.
// A button which is disabled is drawn dim.
void RenderMessageBoxButton(SEASON3B::CNewUIMessageBoxButton& button, bool isPrimary);

// The check of a line in a list: a gold diamond when it's checked, an empty one when it isn't.
void DrawCheckMark(float x, float y, float size, bool isChecked);
} // namespace GameShop::CyberStyle
