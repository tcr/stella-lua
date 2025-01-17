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

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include <map>

class Console;
class OSystem;
class MouseControl;
class DialogContainer;
class EventMappingWidget;

#include "Event.hxx"
#include "EventHandlerConstants.hxx"
#include "Control.hxx"
#include "StellaKeys.hxx"
#include "Variant.hxx"
#include "bspf.hxx"

/**
  This class takes care of event remapping and dispatching for the
  Stella core, as well as keeping track of the current 'mode'.

  The frontend will send translated events here, and the handler will
  check to see what the current 'mode' is.

  If in emulation mode, events received from the frontend are remapped and
  sent to the emulation core.  If in menu mode, the events are sent
  unchanged to the menu class, where (among other things) changing key
  mapping can take place.

  @author  Stephen Anthony
*/
class EventHandler
{
  public:
    /**
      Create a new event handler object
    */
    EventHandler(OSystem& osystem);
    virtual ~EventHandler();

    /**
      Returns the event object associated with this handler class.

      @return The event object
    */
    const Event& event() const { return myEvent; }

    /**
      Initialize state of this eventhandler.
    */
    void initialize();

    /**
      Maps the given Stelladaptor/2600-daptor(s) to specified ports on a real 2600.

      @param saport  How to map the ports ('lr' or 'rl')
    */
    void mapStelladaptors(const string& saport);

    /**
      Swaps the ordering of Stelladaptor/2600-daptor(s) devices.
    */
    void toggleSAPortOrder();

    /**
      Toggle whether the console is in 2600 or 7800 mode.
      Note that for now, this only affects whether the 7800 pause button is
      supported; there is no further emulation of the 7800 itself.
    */
    void set7800Mode();

    /**
      Collects and dispatches any pending events.  This method should be
      called regularly (at X times per second, where X is the game framerate).

      @param time  The current time in microseconds.
    */
    void poll(uInt64 time);

    /**
      Returns the current state of the EventHandler

      @return The EventHandlerState type
    */
    EventHandlerState state() const { return myState; }

    /**
      Resets the state machine of the EventHandler to the defaults

      @param state  The current state to set
    */
    void reset(EventHandlerState state);

    /**
      This method indicates that the system should terminate.
    */
    void quit() { handleEvent(Event::Quit, 1); }

    /**
      Sets the mouse axes and buttons to act as the controller specified in
      the ROM properties, otherwise disable mouse control completely

      @param enable  Whether to use the mouse to emulate controllers
                     Currently, this will be one of the following values:
                     'always', 'analog', 'never'
    */
    void setMouseControllerMode(const string& enable);

    /**
      Set the number of seconds between taking a snapshot in
      continuous snapshot mode.  Setting an interval of 0 disables
      continuous snapshots.

      @param interval  Interval in seconds between snapshots
    */
    void setContinuousSnapshots(uInt32 interval);

    void enterMenuMode(EventHandlerState state);
    void leaveMenuMode();
    bool enterDebugMode();
    void leaveDebugMode();
    void enterTimeMachineMenuMode(uInt32 numWinds, bool unwind);
    void takeSnapshot(uInt32 number = 0);

    /**
      Send an event directly to the event handler.
      These events cannot be remapped.

      @param type  The event
      @param value The value for the event
    */
    void handleEvent(Event::Type type, Int32 value);

    /**
      Handle events that must be processed each time a new console is
      created.  Typically, these are events set by commandline arguments.
    */
    void handleConsoleStartupEvents();

    bool frying() const { return myFryingFlag; }

    StringList getActionList(EventMode mode) const;
    VariantList getComboList(EventMode mode) const;

    /** Used to access the list of events assigned to a specific combo event. */
    StringList getComboListForEvent(Event::Type event) const;
    void setComboListForEvent(Event::Type event, const StringList& events);

    Event::Type eventForKey(StellaKey key, EventMode mode) const
      { return myKeyTable[key][mode]; }
    Event::Type eventForJoyAxis(int stick, int axis, int value, EventMode mode) const {
      const StellaJoystick* joy = myJoyHandler->joy(stick);
      return joy ? joy->axisTable[axis][(value > 0)][mode] : Event::NoType;
    }
    Event::Type eventForJoyButton(int stick, int button, EventMode mode) const {
      const StellaJoystick* joy = myJoyHandler->joy(stick);
      return joy ? joy->btnTable[button][mode] : Event::NoType;
    }
    Event::Type eventForJoyHat(int stick, int hat, JoyHat value, EventMode mode) const {
      const StellaJoystick* joy = myJoyHandler->joy(stick);
      return joy ? joy->hatTable[hat][int(value)][mode] : Event::NoType;
    }

    Event::Type eventAtIndex(int idx, EventMode mode) const;
    string actionAtIndex(int idx, EventMode mode) const;
    string keyAtIndex(int idx, EventMode mode) const;

    /**
      Bind a key to an event/action and regenerate the mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param key    The key to bind to this event
    */
    bool addKeyMapping(Event::Type event, EventMode mode, StellaKey key);

    /**
      Bind a joystick axis direction to an event/action and regenerate
      the mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  The value on the given axis
      @param updateMenus  Whether to update the action mappings (normally
                          we want to do this, unless there are a batch of
                          'adds', in which case it's delayed until the end
    */
    bool addJoyAxisMapping(Event::Type event, EventMode mode,
                           int stick, int axis, int value,
                           bool updateMenus = true);

    /**
      Bind a joystick button to an event/action and regenerate the
      mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param button The joystick button
      @param updateMenus  Whether to update the action mappings (normally
                          we want to do this, unless there are a batch of
                          'adds', in which case it's delayed until the end
    */
    bool addJoyButtonMapping(Event::Type event, EventMode mode, int stick, int button,
                             bool updateMenus = true);

    /**
      Bind a joystick hat direction to an event/action and regenerate
      the mapping array(s).

      @param event  The event we are remapping
      @param mode   The mode where this event is active
      @param stick  The joystick number
      @param hat    The joystick hat
      @param value  The value on the given hat
      @param updateMenus  Whether to update the action mappings (normally
                          we want to do this, unless there are a batch of
                          'adds', in which case it's delayed until the end
    */
    bool addJoyHatMapping(Event::Type event, EventMode mode,
                          int stick, int hat, JoyHat value,
                          bool updateMenus = true);

    /**
      Erase the specified mapping.

      @param event  The event for which we erase all mappings
      @param mode   The mode where this event is active
    */
    void eraseMapping(Event::Type event, EventMode mode);

    /**
      Resets the event mappings to default values.

      @param event  The event which to (re)set (Event::NoType resets all)
      @param mode   The mode for which the defaults are set
    */
    void setDefaultMapping(Event::Type event, EventMode mode);

    /**
      Sets the combo event mappings to those in the 'combomap' setting
    */
    void setComboMap();

    /**
      Joystick emulates 'impossible' directions (ie, left & right
      at the same time).

      @param allow  Whether or not to allow impossible directions
    */
    void allowAllDirections(bool allow) { myAllowAllDirectionsFlag = allow; }

    /**
      Determines whether the given controller must use the mouse (aka,
      whether the controller generates analog output).

      @param jack  The controller to query
    */
    bool controllerIsAnalog(Controller::Jack jack) const;

    /**
      Return a list of all joysticks currently in the internal database
      (first part of variant) and its internal ID (second part of variant).
    */
    VariantList joystickDatabase() const;

    /**
      Remove the joystick identified by 'name' from the joystick database,
      only if it is not currently active.
    */
    void removeJoystickFromDatabase(const string& name);

    /**
      Enable/disable text events (distinct from single-key events).
    */
    virtual void enableTextEvents(bool enable) = 0;

  protected:
    // Global OSystem object
    OSystem& myOSystem;

    /**
      Methods which are called by derived classes to handle specific types
      of input.
    */
    void handleTextEvent(char text);
    void handleKeyEvent(StellaKey key, StellaMod mod, bool state);
    void handleMouseMotionEvent(int x, int y, int xrel, int yrel);
    void handleMouseButtonEvent(MouseButton b, bool pressed, int x, int y);
    void handleJoyEvent(int stick, int button, uInt8 state);
    void handleJoyAxisEvent(int stick, int axis, int value);
    void handleJoyHatEvent(int stick, int hat, int value);

    /**
      Returns the human-readable name for a StellaKey.
    */
    virtual const char* const nameForKey(StellaKey key) const
      { return EmptyString.c_str(); }

    /**
      Collects and dispatches any pending events.
    */
    virtual void pollEvent() = 0;

    // Other events that can be received from the underlying event handler
    enum class SystemEvent {
      WINDOW_SHOWN,
      WINDOW_HIDDEN,
      WINDOW_EXPOSED,
      WINDOW_MOVED,
      WINDOW_RESIZED,
      WINDOW_MINIMIZED,
      WINDOW_MAXIMIZED,
      WINDOW_RESTORED,
      WINDOW_ENTER,
      WINDOW_LEAVE,
      WINDOW_FOCUS_GAINED,
      WINDOW_FOCUS_LOST
    };
    void handleSystemEvent(SystemEvent e, int data1 = 0, int data2 = 0);

    // An abstraction of a joystick in Stella.
    // A StellaJoystick holds its own event mapping information, space for
    // which is dynamically allocated based on the actual number of buttons,
    // axes, etc that the device contains.
    // Specific backend class(es) will inherit from this class, and implement
    // functionality specific to the device.
    class StellaJoystick
    {
      friend class EventHandler;

      public:
        StellaJoystick();
        virtual ~StellaJoystick();

        string getMap() const;
        bool setMap(const string& map);
        void eraseMap(EventMode mode);
        void eraseEvent(Event::Type event, EventMode mode);
        string about() const;

      protected:
        void initialize(int index, const string& desc,
                        int axes, int buttons, int hats, int balls);

      private:
        enum JoyType {
          JT_NONE               = 0,
          JT_REGULAR            = 1,
          JT_STELLADAPTOR_LEFT  = 2,
          JT_STELLADAPTOR_RIGHT = 3,
          JT_2600DAPTOR_LEFT    = 4,
          JT_2600DAPTOR_RIGHT   = 5
        };

        JoyType type;
        int ID;
        string name;
        int numAxes, numButtons, numHats;
        Event::Type (*axisTable)[2][kNumModes];
        Event::Type (*btnTable)[kNumModes];
        Event::Type (*hatTable)[4][kNumModes];
        int* axisLastValue;

      private:
        void getValues(const string& list, IntArray& map) const;

        friend ostream& operator<<(ostream& os, const StellaJoystick& s) {
          os << "  ID: " << s.ID << ", name: " << s.name << ", numaxis: " << s.numAxes
             << ", numbtns: " << s.numButtons << ", numhats: " << s.numHats;
          return os;
        }
    };

    class JoystickHandler
    {
      private:
        struct StickInfo
        {
          StickInfo(const string& map = EmptyString, StellaJoystick* stick = nullptr)
            : mapping(map), joy(stick) {}

          string mapping;
          StellaJoystick* joy;

          friend ostream& operator<<(ostream& os, const StickInfo& si) {
            os << "  joy: " << si.joy << endl << "  map: " << si.mapping;
            return os;
          }
        };

      public:
        using StickDatabase = std::map<string,StickInfo>;
        using StickList = std::map<int, StellaJoystick*>;

        JoystickHandler(OSystem& system);
        ~JoystickHandler();

        bool add(StellaJoystick* stick);
        bool remove(int id);
        bool remove(const string& name);
        void mapStelladaptors(const string& saport);
        void setDefaultMapping(Event::Type type, EventMode mode);
        void eraseMapping(Event::Type event, EventMode mode);
        void saveMapping();

        const StellaJoystick* joy(int id) const {
          const auto& i = mySticks.find(id);
          return i != mySticks.cend() ? i->second : nullptr;
        }
        const StickDatabase& database() const { return myDatabase; }
        const StickList& sticks() const { return mySticks; }

      private:
        OSystem& myOSystem;

        // Contains all joysticks that Stella knows about, indexed by name
        StickDatabase myDatabase;

        // Contains only joysticks that are currently available, indexed by id
        StickList mySticks;

        void setStickDefaultMapping(int stick, Event::Type type, EventMode mode);
        void printDatabase() const;
    };

    /**
      Add the given joystick to the list of sticks available to the handler.
    */
    void addJoystick(StellaJoystick* stick);

    /**
      Remove joystick at the current index.
    */
    void removeJoystick(int index);

  private:
    enum {
      kComboSize          = 16,
      kEventsPerCombo     = 8,
      kEmulActionListSize = 80 + kComboSize,
      kMenuActionListSize = 14
    };

    /**
      Detects and changes the eventhandler state

      @param type  The event
      @return      True if the state changed, else false
    */
    bool eventStateChange(Event::Type type);

    /**
      The following methods take care of assigning action mappings.
    */
    void setActionMappings(EventMode mode);
    void setKeyNames();
    void setKeymap();
    void setDefaultKeymap(Event::Type, EventMode mode);
    void setDefaultJoymap(Event::Type, EventMode mode);
    void saveKeyMapping();
    void saveJoyMapping();
    void saveComboMapping();

    /**
      Tests if a given event should use continuous/analog values.

      @param event  The event to test for analog processing
      @return       True if analog, else false
    */
    bool eventIsAnalog(Event::Type event) const;

    void setEventState(EventHandlerState state);

  private:
    // Structure used for action menu items
    struct ActionList {
      Event::Type event;
      string action;
      string key;
      bool allow_combo;
    };

    // Global Event object
    Event myEvent;

    // Indicates current overlay object
    DialogContainer* myOverlay;

    // MouseControl object, which takes care of switching the mouse between
    // all possible controller modes
    unique_ptr<MouseControl> myMouseControl;

    // Array of key events, indexed by StellaKey
    Event::Type myKeyTable[KBDK_LAST][kNumModes];

    // The event(s) assigned to each combination event
    Event::Type myComboTable[kComboSize][kEventsPerCombo];

    // Indicates the current state of the system (ie, which mode is current)
    EventHandlerState myState;

    // Indicates whether the joystick emulates 'impossible' directions
    bool myAllowAllDirectionsFlag;

    // Indicates whether or not we're in frying mode
    bool myFryingFlag;

    // Indicates whether the key-combos tied to the Control key are
    // being used or not (since Ctrl by default is the fire button,
    // pressing it with a movement key could inadvertantly activate
    // a Ctrl combo when it isn't wanted)
    bool myUseCtrlKeyFlag;

    // Sometimes an extraneous mouse motion event occurs after a video
    // state change; we detect when this happens and discard the event
    bool mySkipMouseMotion;

    // Whether the currently enabled console is emulating certain aspects
    // of the 7800 (for now, only the switches are notified)
    bool myIs7800;

    // Sometimes key combos with the Alt key become 'stuck' after the
    // window changes state, and we want to ignore that event
    // For example, press Alt-Tab and then upon re-entering the window,
    // the app receives 'tab'; obviously the 'tab' shouldn't be happening
    // So we keep track of the cases that matter (for now, Alt-Tab)
    // and swallow the event afterwards
    // Basically, the initial event sets the variable to 1, and upon
    // returning to the app (ie, receiving EVENT_WINDOW_FOCUS_GAINED),
    // the count is updated to 2, but only if it was already updated to 1
    // TODO - This may be a bug in SDL, and might be removed in the future
    //        It only seems to be an issue in Linux
    uInt8 myAltKeyCounter;

    // Used for continuous snapshot mode
    uInt32 myContSnapshotInterval;
    uInt32 myContSnapshotCounter;

    // Holds static strings for the remap menu (emulation and menu events)
    static ActionList ourEmulActionList[kEmulActionListSize];
    static ActionList ourMenuActionList[kMenuActionListSize];

    // Static lookup tables for Stelladaptor/2600-daptor axis/button support
    static const Event::Type SA_Axis[2][2];
    static const Event::Type SA_Button[2][4];
    static const Event::Type SA_Key[2][12];

    // Handler for all joystick addition/removal/mapping
    unique_ptr<JoystickHandler> myJoyHandler;

    // Following constructors and assignment operators not supported
    EventHandler() = delete;
    EventHandler(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;
};

#endif
