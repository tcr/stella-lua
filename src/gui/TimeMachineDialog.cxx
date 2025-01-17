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
//============================================================================

#include "Dialog.hxx"
#include "Font.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"
#include "TimeLineWidget.hxx"

#include "Console.hxx"
#include "TIA.hxx"
#include "System.hxx"

#include "TimeMachineDialog.hxx"
#include "Base.hxx"
using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeMachineDialog::TimeMachineDialog(OSystem& osystem, DialogContainer& parent,
                                     int width)
  : Dialog(osystem, parent)
{
  const int BUTTON_W = 16, BUTTON_H = 14;

  static uInt32 PLAY[BUTTON_H] =
  {
    0b0110000000000000,
    0b0111100000000000,
    0b0111111000000000,
    0b0111111110000000,
    0b0111111111100000,
    0b0111111111111000,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111000,
    0b0111111111100000,
    0b0111111110000000,
    0b0111111000000000,
    0b0111100000000000,
    0b0110000000000000
  };
  static uInt32 REWIND_ALL[BUTTON_H] =
  {
    0,
    0b0110000110000110,
    0b0110001110001110,
    0b0110011110011110,
    0b0110111110111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0110111110111110,
    0b0110011110011110,
    0b0110001110001110,
    0b0110000110000110,
    0
  };
  static uInt32 REWIND_1[BUTTON_H] =
  {
    0,
    0b0000001100011100,
    0b0000011100011100,
    0b0000111100011100,
    0b0001111100011100,
    0b0011111100011100,
    0b0111111100011100,
    0b0111111100011100,
    0b0011111100011100,
    0b0001111100011100,
    0b0000111100011100,
    0b0000011100011100,
    0b0000001100011100,
    0
  };
  static uInt32 UNWIND_1[BUTTON_H] =
  {
    0,
    0b0011100011000000,
    0b0011100011100000,
    0b0011100011110000,
    0b0011100011111000,
    0b0011100011111100,
    0b0011100011111110,
    0b0011100011111110,
    0b0011100011111100,
    0b0011100011111000,
    0b0011100011110000,
    0b0011100011100000,
    0b0011100011000000,
    0
  };
  static uInt32 UNWIND_ALL[BUTTON_H] =
  {
    0,
    0b0110000110000110,
    0b0111000111000110,
    0b0111100111100110,
    0b0111110111110110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111111111111110,
    0b0111110111110110,
    0b0111100111100110,
    0b0111000111000110,
    0b0110000110000110,
    0
  };

  const GUI::Font& font = instance().frameBuffer().font();
  const int H_BORDER = 6, BUTTON_GAP = 4, V_BORDER = 4;
  const int buttonWidth = BUTTON_W + 8,
            buttonHeight = BUTTON_H + 10,
            rowHeight = font.getLineHeight();

  int xpos, ypos;

  // Set real dimensions
  _w = width;  // Parent determines our width (based on window size)
  _h = V_BORDER * 2 + rowHeight + buttonHeight + 2;

  this->clearFlags(WIDGET_CLEARBG); // does only work combined with blending (0..100)!
  this->clearFlags(WIDGET_BORDER);

  xpos = H_BORDER;
  ypos = V_BORDER;

  // Add index info
  myCurrentIdxWidget = new StaticTextWidget(this, font, xpos, ypos, "    ", TextAlign::Left, kBGColor);
  myCurrentIdxWidget->setTextColor(kColorInfo);
  myLastIdxWidget = new StaticTextWidget(this, font, _w - H_BORDER - font.getStringWidth("8888"), ypos,
                                         "    ", TextAlign::Right, kBGColor);
  myLastIdxWidget->setTextColor(kColorInfo);

  // Add timeline
  const uInt32 tl_h = myCurrentIdxWidget->getHeight() / 2,
               tl_x = xpos + myCurrentIdxWidget->getWidth() + 8,
               tl_y = ypos + (myCurrentIdxWidget->getHeight() - tl_h) / 2 - 1,
               tl_w = myLastIdxWidget->getAbsX() - tl_x - 8;
  myTimeline = new TimeLineWidget(this, font, tl_x, tl_y, tl_w, tl_h, "", 0, kTimeline);
  myTimeline->setMinValue(0);
  ypos += rowHeight;

  // Add time info
  myCurrentTimeWidget = new StaticTextWidget(this, font, xpos, ypos + 3, "04:32 59", TextAlign::Left, kBGColor);
  myCurrentTimeWidget->setTextColor(kColorInfo);
  myLastTimeWidget = new StaticTextWidget(this, font, _w - H_BORDER - font.getStringWidth("XX:XX XX"), ypos + 3,
                                          "12:25 59", TextAlign::Right, kBGColor);
  myLastTimeWidget->setTextColor(kColorInfo);
  xpos = myCurrentTimeWidget->getRight() + BUTTON_GAP * 4;

  // Add buttons
  myRewindAllWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, REWIND_ALL,
                                       BUTTON_W, BUTTON_H, kRewindAll);
  xpos += buttonWidth + BUTTON_GAP;

  myRewind1Widget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, REWIND_1,
                                     BUTTON_W, BUTTON_H, kRewind1);
  xpos += buttonWidth + BUTTON_GAP*2;

  myPlayWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, PLAY,
                                  BUTTON_W, BUTTON_H, kPlay);
  xpos += buttonWidth + BUTTON_GAP*2;

  myUnwind1Widget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, UNWIND_1,
                                     BUTTON_W, BUTTON_H, kUnwind1);
  xpos += buttonWidth + BUTTON_GAP;

  myUnwindAllWidget = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight, UNWIND_ALL,
                                       BUTTON_W, BUTTON_H, kUnwindAll);
  xpos = myUnwindAllWidget->getRight() + BUTTON_GAP * 3;

  // Add message
  myMessageWidget = new StaticTextWidget(this, font, xpos, ypos + 3, "                                             ",
                                         TextAlign::Left, kBGColor);
  myMessageWidget->setTextColor(kColorInfo);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::center()
{
  // Place on the bottom of the screen, centered horizontally
  const GUI::Size& screen = instance().frameBuffer().screenSize();
  const GUI::Rect& dst = surface().dstRect();
  surface().setDstPos((screen.w - dst.width()) >> 1, screen.h - dst.height() - 10);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::loadConfig()
{
  RewindManager& r = instance().state().rewindManager();
  IntArray cycles = r.cyclesList();

  // Set range and intervals for timeline
  uInt32 maxValue = cycles.size() > 1 ? uInt32(cycles.size() - 1) : 0;
  myTimeline->setMaxValue(maxValue);
  myTimeline->setStepValues(cycles);

  // Enable blending (only once is necessary)
  if(!surface().attributes().blending)
  {
    surface().attributes().blending = true;
    surface().attributes().blendalpha = 92;
    surface().applyAttributes();
  }

  handleWinds();
  myMessageWidget->setLabel("");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleKeyDown(StellaKey key, StellaMod mod)
{
  // The following 'Alt' shortcuts duplicate the shortcuts in EventHandler
  // It is best to keep them the same, so changes in EventHandler mean we
  // need to update the logic here too
  if(StellaModTest::isAlt(mod))
  {
    switch(key)
    {
      case KBDK_LEFT:  // Alt-left(-shift) rewinds 1(10) states
        handleCommand(nullptr, StellaModTest::isShift(mod) ? kRewind10 : kRewind1, 0, 0);
        break;

      case KBDK_RIGHT:  // Alt-right(-shift) unwinds 1(10) states
        handleCommand(nullptr, StellaModTest::isShift(mod) ? kUnwind10 : kUnwind1, 0, 0);
        break;

      case KBDK_DOWN:  // Alt-down rewinds to start of list
        handleCommand(nullptr, kRewindAll, 0, 0);
        break;

      case KBDK_UP:  // Alt-up rewinds to end of list
        handleCommand(nullptr, kUnwindAll, 0, 0);
        break;

      default:
        Dialog::handleKeyDown(key, mod);
    }
  }
  else if(key == KBDK_SPACE || key == KBDK_ESCAPE)
    handleCommand(nullptr, kPlay, 0, 0);
  else
    Dialog::handleKeyDown(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleCommand(CommandSender* sender, int cmd,
                                      int data, int id)
{
  switch(cmd)
  {
    case kTimeline:
    {
      Int32 winds = myTimeline->getValue() -
          instance().state().rewindManager().getCurrentIdx() + 1;
      handleWinds(winds);
      break;
    }

    case kPlay:
      instance().eventHandler().leaveMenuMode();
      break;

    case kRewind1:
      handleWinds(-1);
      break;

    case kRewind10:
      handleWinds(-10);
      break;

    case kRewindAll:
      handleWinds(-1000);
      break;

    case kUnwind1:
      handleWinds(1);
      break;

    case kUnwind10:
      handleWinds(10);
      break;

    case kUnwindAll:
      handleWinds(1000);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TimeMachineDialog::getTimeString(uInt64 cycles)
{
  const Int32 scanlines = std::max(instance().console().tia().scanlinesLastFrame(), 240u);
  const bool isNTSC = scanlines <= 287;
  const Int32 NTSC_FREQ = 1193182; // ~76*262*60
  const Int32 PAL_FREQ  = 1182298; // ~76*312*50
  const Int32 freq = isNTSC ? NTSC_FREQ : PAL_FREQ; // = cycles/second

  uInt32 minutes = uInt32(cycles / (freq * 60));
  cycles -= minutes * (freq * 60);
  uInt32 seconds = uInt32(cycles / freq);
  cycles -= seconds * freq;
  uInt32 frames = uInt32(cycles / (scanlines * 76));

  ostringstream time;
  time << Common::Base::toString(minutes, Common::Base::F_10_2) << ":";
  time << Common::Base::toString(seconds, Common::Base::F_10_2) << ".";
  time << Common::Base::toString(frames, Common::Base::F_10_2);

  return time.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleWinds(Int32 numWinds)
{
  RewindManager& r = instance().state().rewindManager();

  if(numWinds)
  {
    uInt64 startCycles = instance().console().tia().cycles();
    if(numWinds < 0)      r.rewindStates(-numWinds);
    else if(numWinds > 0) r.unwindStates(numWinds);

    uInt64 elapsed = instance().console().tia().cycles() - startCycles;
    if(elapsed > 0)
    {
      string message = r.getUnitString(elapsed);

      // TODO: add message text from addState()
      myMessageWidget->setLabel((numWinds < 0 ? "(-" : "(+") + message + ")");
    }
  }

  // Update time
  myCurrentTimeWidget->setLabel(getTimeString(r.getCurrentCycles() - r.getFirstCycles()));
  myLastTimeWidget->setLabel(getTimeString(r.getLastCycles() - r.getFirstCycles()));
  myTimeline->setValue(r.getCurrentIdx()-1);
  // Update index
  myCurrentIdxWidget->setValue(r.getCurrentIdx());
  myLastIdxWidget->setValue(r.getLastIdx());
  // Enable/disable buttons
  myRewindAllWidget->setEnabled(!r.atFirst());
  myRewind1Widget->setEnabled(!r.atFirst());
  myUnwindAllWidget->setEnabled(!r.atLast());
  myUnwind1Widget->setEnabled(!r.atLast());
}
