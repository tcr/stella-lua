//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"
#include "Command.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "FBSurface.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"

#include "Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::Widget(GuiObject* boss, const GUI::Font& font,
               int x, int y, int w, int h)
  : GuiObject(boss->instance(), boss->parent(), boss->dialog(), x, y, w, h),
    _boss(boss),
    _font(font),
    _id(-1),
    _flags(0),
    _hasFocus(false),
    _bgcolor(kWidColor),
    _bgcolorhi(kWidColor),
    _textcolor(kTextColor),
    _textcolorhi(kTextColorHi)
{
  // Insert into the widget list of the boss
  _next = _boss->_firstWidget;
  _boss->_firstWidget = this;

  _fontWidth  = _font.getMaxCharWidth();
  _fontHeight = _font.getLineHeight();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::~Widget()
{
  delete _next;
  _next = nullptr;

  _focusList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::draw()
{
  if(!_dirty || !isVisible() || !_boss->isVisible())
    return;

  _dirty = false;

  FBSurface& s = _boss->dialog().surface();

  bool hasBorder = _flags & WIDGET_BORDER;
  int oldX = _x, oldY = _y;

  // Account for our relative position in the dialog
  _x = getAbsX();
  _y = getAbsY();

  // Clear background (unless alpha blending is enabled)
  if(_flags & WIDGET_CLEARBG)
  {
    int x = _x, y = _y, w = _w, h = _h;
    if(hasBorder)
    {
      x++; y++; w-=2; h-=2;
    }
#ifndef FLAT_UI
    s.fillRect(x, y, w, h, (_flags & WIDGET_HILITED) && isEnabled() ? _bgcolorhi : _bgcolor);
#else
    s.fillRect(x, y, w, h, (_flags & WIDGET_HILITED) && isEnabled() ? _bgcolorhi : _bgcolor);
#endif
  }

  // Draw border
  if(hasBorder)
  {
#ifndef FLAT_UI
    s.box(_x, _y, _w, _h, kColor, kShadowColor);
#else
    s.frameRect(_x, _y, _w, _h, (_flags & WIDGET_HILITED) && isEnabled() ? kScrollColorHi : kColor);
#endif // !FLAT_UI
    _x += 4;
    _y += 4;
    _w -= 8;
    _h -= 8;
  }

  // Now perform the actual widget draw
  drawWidget((_flags & WIDGET_HILITED) ? true : false);

  // Restore x/y
  if (hasBorder)
  {
    _x -= 4;
    _y -= 4;
    _w += 8;
    _h += 8;
  }

  _x = oldX;
  _y = oldY;

  // Draw all children
  Widget* w = _firstWidget;
  while(w)
  {
    w->draw();
    w = w->_next;
  }

  // Tell the framebuffer this area is dirty
  s.setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::receivedFocus()
{
  if(_hasFocus)
    return;

  _hasFocus = true;
  receivedFocusWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::lostFocus()
{
  if(!_hasFocus)
    return;

  _hasFocus = false;
  lostFocusWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setEnabled(bool e)
{
  if(e) setFlags(WIDGET_ENABLED);
  else  clearFlags(WIDGET_ENABLED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::findWidgetInChain(Widget* w, int x, int y)
{
  while(w)
  {
    // Stop as soon as we find a widget that contains the point (x,y)
    if(x >= w->_x && x < w->_x + w->_w && y >= w->_y && y < w->_y + w->_h)
      break;
    w = w->_next;
  }

  if(w)
    w = w->findWidget(x - w->_x, y - w->_y);

  return w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Widget::isWidgetInChain(Widget* w, Widget* find)
{
  while(w)
  {
    // Stop as soon as we find the widget
    if(w == find)  return true;
    w = w->_next;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Widget::isWidgetInChain(WidgetArray& list, Widget* find)
{
  for(const auto& w: list)
    if(w == find)
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::setFocusForChain(GuiObject* boss, WidgetArray& arr,
                                 Widget* wid, int direction,
                                 bool emitFocusEvents)
{
  FBSurface& s = boss->dialog().surface();
  int size = int(arr.size()), pos = -1;
  Widget* tmp;
  for(int i = 0; i < size; ++i)
  {
    tmp = arr[i];

    // Determine position of widget 'w'
    if(wid == tmp)
      pos = i;

    // Get area around widget
    // Note: we must use getXXX() methods and not access the variables
    // directly, since in some cases (notably those widgets with embedded
    // ScrollBars) the two quantities may be different
    int x = tmp->getAbsX() - 1,  y = tmp->getAbsY() - 1,
        w = tmp->getWidth() + 2, h = tmp->getHeight() + 2;

    // First clear area surrounding all widgets
    if(tmp->_hasFocus)
    {
      if(emitFocusEvents)
        tmp->lostFocus();
      else
        tmp->_hasFocus = false;

      s.frameRect(x, y, w, h, kDlgColor);

      tmp->setDirty();
      s.setDirty();
    }
  }

  // Figure out which which should be active
  if(pos == -1)
    return nullptr;
  else
  {
    int oldPos = pos;
    do
    {
      switch(direction)
      {
        case -1:  // previous widget
          pos--;
          if(pos < 0)
            pos = size - 1;
          break;

        case +1:  // next widget
          pos++;
          if(pos >= size)
            pos = 0;
          break;

        default:
          // pos already set
          break;
      }
      // break if all widgets should be disabled
      if(oldPos == pos)
        break;
    } while(!arr[pos]->isEnabled());
  }

  // Now highlight the active widget
  tmp = arr[pos];

  // Get area around widget
  // Note: we must use getXXX() methods and not access the variables
  // directly, since in some cases (notably those widgets with embedded
  // ScrollBars) the two quantities may be different
  int x = tmp->getAbsX() - 1,  y = tmp->getAbsY() - 1,
      w = tmp->getWidth() + 2, h = tmp->getHeight() + 2;

  if(emitFocusEvents)
    tmp->receivedFocus();
  else
    tmp->_hasFocus = true;

  s.frameRect(x, y, w, h, kWidFrameColor, FrameStyle::Dashed);

  tmp->setDirty();
  s.setDirty();

  return tmp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setDirtyInChain(Widget* start)
{
  while(start)
  {
    start->setDirty();
    start = start->_next;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                                   int x, int y, int w, int h,
                                   const string& text, TextAlign align,
                                   uInt32 shadowColor)
  : Widget(boss, font, x, y, w, h),
    _align(align)
{
  _flags = WIDGET_ENABLED;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;
  _shadowcolor = shadowColor;

  _label = text;
  _editable = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                                   int x, int y,
                                   const string& text, TextAlign align,
                                   uInt32 shadowColor)
  : StaticTextWidget(boss, font, x, y, font.getStringWidth(text), font.getLineHeight(), text, align, shadowColor)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setValue(int value)
{
  char buf[256];
  std::snprintf(buf, 255, "%d", value);
  _label = buf;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setLabel(const string& label)
{
  _label = label;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  s.drawString(_font, _label, _x, _y, _w,
               isEnabled() ? _textcolor : uInt32(kColor), _align, 0, true, _shadowcolor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           const string& label, int cmd)
  : StaticTextWidget(boss, font, x, y, w, h, label, TextAlign::Center),
    CommandSender(boss),
    _cmd(cmd),
    _useBitmap(false)
{
  _flags = WIDGET_ENABLED | WIDGET_BORDER | WIDGET_CLEARBG;
  _bgcolor = kBtnColor;
  _bgcolorhi = kBtnColorHi;
  _textcolor = kBtnTextColor;
  _textcolorhi = kBtnTextColorHi;

  _editable = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int dw,
                           const string& label, int cmd)
  : ButtonWidget(boss, font, x, y, font.getStringWidth(label) + dw, font.getLineHeight() + 4, label, cmd)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y,
                           const string& label, int cmd)
  : ButtonWidget(boss, font, x, y, 20, label, cmd)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           uInt32* bitmap, int bmw, int bmh,
                           int cmd)
  : ButtonWidget(boss, font, x, y, w, h, "", cmd)
{
  _bitmap = bitmap;
  _bmh = bmh;
  _bmw = bmw;
  _useBitmap = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseEntered()
{
  setFlags(WIDGET_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseLeft()
{
  clearFlags(WIDGET_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ButtonWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UISelect:
      // Simulate mouse event
      handleMouseUp(0, 0, MouseButton::LEFT, 0);
      return true;
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    clearFlags(WIDGET_HILITED);
    sendCommand(_cmd, 0, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  if (!_useBitmap)
    s.drawString(_font, _label, _x, _y + (_h - _fontHeight)/2 + 1, _w,
                 !isEnabled() ? hilite ? uInt32(kColor) : uInt32(kBGColorLo) :
                 hilite ? _textcolorhi : _textcolor, _align);
  else
    s.drawBitmap(_bitmap, _x + (_w - _bmw) / 2, _y + (_h - _bmh) / 2,
                 !isEnabled() ? hilite ? uInt32(kColor) : uInt32(kBGColorLo) :
                 hilite ? _textcolorhi : _textcolor,
                 _bmw, _bmh);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* 8x8 checkbox bitmap */
#ifndef FLAT_UI
static uInt32 checked_img_active[8] =
{
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111
};

static uInt32 checked_img_inactive[8] =
{
	0b11111111,
	0b11111111,
	0b11100111,
	0b11000011,
	0b11000011,
	0b11100111,
	0b11111111,
	0b11111111
};

static uInt32 checked_img_circle[8] =
{
	0b00011000,
	0b01111110,
	0b01111110,
	0b11111111,
	0b11111111,
	0b01111110,
	0b01111110,
	0b00011000
};
#else
static uInt32 checked_img_active[10] =
{
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b1111111111
};

static uInt32 checked_img_inactive[10] =
{
  0b1111111111,
  0b1111111111,
  0b1111001111,
  0b1110000111,
  0b1100000011,
  0b1100000011,
  0b1110000111,
  0b1111001111,
  0b1111111111,
  0b1111111111
};

static uInt32 checked_img_circle[10] =
{
  0b0001111000,
  0b0111111110,
  0b0111111110,
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b1111111111,
  0b0111111110,
  0b0111111110,
  0b0001111000
};
#endif // !FLAT_UI
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckboxWidget::CheckboxWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, const string& label,
                               int cmd)
  : ButtonWidget(boss, font, x, y, 16, 16, label, cmd),
    _state(false),
    _holdFocus(true),
    _drawBox(true),
    _changed(false),
    _fillColor(kColor),
    _boxY(0),
    _textY(0)
{
  _flags = WIDGET_ENABLED;
  _bgcolor = _bgcolorhi = kWidColor;

  _editable = true;

  if(label == "")
    _w = 14;
  else
    _w = font.getStringWidth(label) + 20;
  _h = font.getFontHeight() < 14 ? 14 : font.getFontHeight();


  // Depending on font size, either the font or box will need to be
  // centered vertically
  if(_h > 14)  // center box
    _boxY = (_h - 14) / 2;
  else         // center text
    _textY = (14 - _font.getFontHeight()) / 2;

  setFill(CheckboxWidget::Normal);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseEntered()
{
  setFlags(WIDGET_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseLeft()
{
  clearFlags(WIDGET_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && _editable && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    toggleState();

    // We only send a command when the widget has been changed interactively
    sendCommand(_cmd, _state, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setEditable(bool editable)
{
  _editable = editable;
  if(_editable)
  {
    _bgcolor = kWidColor;
  }
  else
  {
    _bgcolor = kBGColorHi;
    setFill(CheckboxWidget::Inactive);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setFill(FillType type)
{
  switch(type)
  {
    case CheckboxWidget::Normal:
      _img = checked_img_active;
      _drawBox = true;
      break;
    case CheckboxWidget::Inactive:
      _img = checked_img_inactive;
      _drawBox = true;
      break;
    case CheckboxWidget::Circle:
      _img = checked_img_circle;
      _drawBox = false;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setState(bool state, bool changed)
{
  if(_state != state)
  {
    _state = state;
    setDirty();
  }
  _changed = changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

#ifndef FLAT_UI
  // Draw the box
  if(_drawBox)
    s.box(_x, _y + _boxY, 14, 14, kColor, kShadowColor);
  // Do we draw a square or cross?
  s.fillRect(_x + 2, _y + _boxY + 2, 10, 10, _changed ? uInt32(kDbgChangedColor)
             : isEnabled() ? _bgcolor : uInt32(kColor));
  if(_state)
    s.drawBitmap(_img, _x + 3, _y + _boxY + 3, isEnabled() ? kCheckColor : kShadowColor);
#else
  if(_drawBox)
    s.frameRect(_x, _y + _boxY, 14, 14, hilite ? kScrollColorHi : kShadowColor);
  // Do we draw a square or cross?
  s.fillRect(_x + 1, _y + _boxY + 1, 12, 12, _changed ? kDbgChangedColor
             : isEnabled() ? _bgcolor : kColor);
  if(_state)
    s.drawBitmap(_img, _x + 2, _y + _boxY + 2, isEnabled() ? hilite ? kScrollColorHi : kCheckColor
                 : kShadowColor, 10);
#endif

  // Finally draw the label
  s.drawString(_font, _label, _x + 20, _y + _textY, _w,
               isEnabled() ? kTextColor : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           const string& label, int labelWidth, int cmd)
  : ButtonWidget(boss, font, x, y, w, h, label, cmd),
    _value(0),
    _stepValue(1),
    _valueMin(0),
    _valueMax(100),
    _isDragging(false),
    _labelWidth(labelWidth)
{
  _flags = WIDGET_ENABLED | WIDGET_TRACK_MOUSE;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font.getStringWidth(_label);

  _w = w + _labelWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValue(int value)
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;

  if(value != _value)
  {
    _value = value;
    setDirty();
    sendCommand(_cmd, _value, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setMinValue(int value)
{
  _valueMin = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setMaxValue(int value)
{
  _valueMax = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setStepValue(int value)
{
  _stepValue = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseMoved(int x, int y)
{
  // TODO: when the mouse is dragged outside the widget, the slider should
  // snap back to the old value.
  if(isEnabled() && _isDragging && x >= int(_labelWidth))
    setValue(posToValue(x - _labelWidth));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && b == MouseButton::LEFT)
  {
    _isDragging = true;
    handleMouseMoved(x, y);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && _isDragging)
    sendCommand(_cmd, _value, _id);

  _isDragging = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseWheel(int x, int y, int direction)
{
  if(isEnabled())
  {
    if(direction < 0)
      handleEvent(Event::UIUp);
    else if(direction > 0)
      handleEvent(Event::UIDown);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SliderWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UIDown:
    case Event::UILeft:
    case Event::UIPgDown:
      setValue(_value - _stepValue);
      break;

    case Event::UIUp:
    case Event::UIRight:
    case Event::UIPgUp:
      setValue(_value + _stepValue);
      break;

    case Event::UIHome:
      setValue(_valueMin);
      break;

    case Event::UIEnd:
      setValue(_valueMax);
      break;

    default:
      return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

#ifndef FLAT_UI
  // Draw the label, if any
  if(_labelWidth > 0)
    s.drawString(_font, _label, _x, _y + 2, _labelWidth,
                 isEnabled() ? kTextColor : kColor, TextAlign::Right);

  // Draw the box
  s.box(_x + _labelWidth, _y, _w - _labelWidth, _h, kColor, kShadowColor);
  // Fill the box
  s.fillRect(_x + _labelWidth + 2, _y + 2, _w - _labelWidth - 4, _h - 4,
             !isEnabled() ? kBGColorHi : kWidColor);
  // Draw the 'bar'
  s.fillRect(_x + _labelWidth + 2, _y + 2, valueToPos(_value), _h - 4,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);
#else
  // Draw the label, if any
  if(_labelWidth > 0)
    s.drawString(_font, _label, _x, _y + 2, _labelWidth,
                 isEnabled() ? kTextColor : kColor, TextAlign::Left);

  // Draw the box
  s.frameRect(_x + _labelWidth, _y, _w - _labelWidth, _h, isEnabled() && hilite ? kSliderColorHi : kShadowColor);
  // Fill the box
  s.fillRect(_x + _labelWidth + 1, _y + 1, _w - _labelWidth - 2, _h - 2,
             !isEnabled() ? kBGColorHi : kWidColor);
  // Draw the 'bar'
  s.fillRect(_x + _labelWidth + 2, _y + 2, valueToPos(_value), _h - 4,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::valueToPos(int value)
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;
  int range = std::max(_valueMax - _valueMin, 1);  // don't divide by zero

  return ((_w - _labelWidth - 4) * (value - _valueMin) / range);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::posToValue(int pos)
{
  int value = (pos) * (_valueMax - _valueMin) / (_w - _labelWidth - 4) + _valueMin;

  // Scale the position to the correct interval (according to step value)
  return value - (value % _stepValue);
}
