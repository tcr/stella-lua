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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef TIA_HXX
#define TIA_HXX

class Console;
class Settings;
class Sound;

#include "bspf.hxx"
#include "Device.hxx"
#include "System.hxx"
#include "TIATables.hxx"

/**
  This class is a device that emulates the Television Interface Adaptor 
  found in the Atari 2600 and 7800 consoles.  The Television Interface 
  Adaptor is an integrated circuit designed to interface between an 
  eight bit microprocessor and a television video modulator. It converts 
  eight bit parallel data into serial outputs for the color, luminosity, 
  and composite sync required by a video modulator.  

  This class outputs the serial data into a frame buffer which can then
  be displayed on screen.

  @author  Bradford W. Mott
  @version $Id$
*/

class TIA : public Device
{
  #define PIXEL_CLOCKS			3
  #define SCANLINE_CYCLES		76
  #define SCANLINE_CLOCKS		(PIXEL_CLOCKS * SCANLINE_CYCLES)
  #define SCANLINE_PIXEL		160
  #define HBLANK_CLOCKS			(SCANLINE_CLOCKS - SCANLINE_PIXEL)
  #define HBLANK_PIXEL			8

  public:
    friend class TIADebug;
    friend class RiotDebug;

    /**
      Create a new TIA for the specified console

      @param console  The console the TIA is associated with
      @param sound    The sound object the TIA is associated with
      @param settings The settings object for this TIA device
    */
    TIA(Console& console, Sound& sound, Settings& settings);
 
    /**
      Destructor
    */
    virtual ~TIA();

  public:
    /**
      Reset device to its power-on state
    */
    void reset();

    /**
      Reset frame to current YStart/Height properties
    */
    void frameReset();

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary
      to override this method for devices that remember cycle counts.
    */
    void systemCyclesReset();

    /**
      Install TIA in the specified system.  Invoked by the system
      when the TIA is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system);

    /**
      Install TIA in the specified system and device.  Invoked by
      the system when the TIA is attached to it.  All devices
      which invoke this method take responsibility for chaining
      requests back to *this* device.

      @param system The system the device should install itself in
      @param device The device responsible for this address space
    */
    void install(System& system, Device& device);

    /**
      Save the current state of this device to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const;

    /**
      Load the current state of this device from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in);

    /**
      The following are very similar to save() and load(), except they
      do a 'deeper' save of the display data itself.

      Normally, the internal framebuffer doesn't need to be saved to
      a state file, since the file already contains all the information
      needed to re-create it, starting from scanline 0.  In effect, when a
      state is loaded, the framebuffer is empty, and the next call to
      update() generates valid framebuffer data.

      However, state files saved from the debugger need more information,
      such as the exact state of the internal framebuffer itself *before*
      we call update(), including if the display was in partial frame mode.

      Essentially, a normal state save has 'frame resolution', whereas
      the debugger state save has 'cycle resolution', and hence needs
      more information.  The methods below save/load this extra info,
      and eliminate having to save approx. 50K to normal state files.
    */
    bool saveDisplay(Serializer& out) const;
    bool loadDisplay(Serializer& in);

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const { return "TIA"; }

    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address

      @return  True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value);

    /**
      This method should be called at an interval corresponding to the 
      desired frame rate to update the TIA.  Invoking this method will update
      the graphics buffer and generate the corresponding audio samples.
    */
    void update();

    /**
      Answers the current frame buffer

      @return Pointer to the current frame buffer
    */
    uInt8* currentFrameBuffer() const
      { return myCurrentFrameBuffer + myFramePointerOffset; }

    /**
      Answers the previous frame buffer

      @return Pointer to the previous frame buffer
    */
    uInt8* previousFrameBuffer() const
      { return myPreviousFrameBuffer + myFramePointerOffset; }

    /**
      Answers the width and height of the frame buffer
    */
    inline uInt32 width() const  { return 160;           }
    inline uInt32 height() const { return myFrameHeight; }
    inline uInt32 ystart() const { return myFrameYStart; }

    /**
      Changes the current Height/YStart properties.
      Note that calls to these method(s) must be eventually followed by
      ::frameReset() for the changes to take effect.
    */
    void setHeight(uInt32 height) { myFrameHeight = height; }
    void setYStart(uInt32 ystart) { myFrameYStart = ystart; }

    /**
      Enables/disables auto-frame calculation.  If enabled, the TIA
      re-adjusts the framerate at regular intervals.

      @param mode  Whether to enable or disable all auto-frame calculation
    */
    void enableAutoFrame(bool mode) { myAutoFrameEnabled = mode; }

    /**
      Enables/disables color-loss for PAL modes only.

      @param mode  Whether to enable or disable PAL color-loss mode
    */
    void enableColorLoss(bool mode)
      { myColorLossEnabled = myFramerate <= 55 ? mode : false; }

    /**
      Answers whether this TIA runs at NTSC or PAL scanrates,
      based on how many frames of out the total count are PAL frames.
    */
    bool isPAL()
      { return float(myPALFrameCounter) / myFrameCounter >= (25.0/60.0); }

    /** 
      Returns the position in the visible scanline.
    */
    inline uInt32 posThisLine() const
      { return clocksThisLine() - HBLANK_CLOCKS; }

    /**
      Answers the current color clock we've gotten to on this scanline.

      @return The current color clock
    */
    uInt32 clocksThisLine() const
      { return ((mySystem->cycles() * PIXEL_CLOCKS) - myClockWhenFrameStarted) % SCANLINE_CLOCKS; }

    /**
      Answers the scanline at which the current frame began drawing.

      @return The starting scanline
    */
    uInt32 startLine() const
      { return myStartScanline; }

    /**
      Answers the total number of scanlines the TIA generated in producing
      the current frame buffer. For partial frames, this will be the
      current scanline.

      @return The total number of scanlines generated
    */
    uInt32 scanlines() const
      { return ((mySystem->cycles() * PIXEL_CLOCKS) - myClockWhenFrameStarted) / SCANLINE_CLOCKS; }

    /**
      Answers whether the TIA is currently in 'partial frame' mode
      (we're in between a call of startFrame and endFrame).

      @return If we're in partial frame mode
    */
    bool partialFrame() const { return myPartialFrameFlag; }

    /**
      Answers the first scanline at which drawing occured in the last frame.

      @return The starting scanline
    */
    uInt32 startScanline() const { return myStartScanline; }

    /**
      Answers the current position of the virtual 'electron beam' used to
      draw the TIA image.  If not in partial frame mode, the position is
      defined to be in the lower right corner (@ width/height of the screen).
      Note that the coordinates are with respect to currentFrameBuffer(),
      taking any YStart values into account.

      @return The x/y coordinates of the scanline electron beam, and whether
              it is in the visible/viewable area of the screen
    */
    bool scanlinePos(uInt16& x, uInt16& y) const;

    /**
      Enables/disable/toggle the specified (or all) TIA bit(s).  Note that
      disabling a graphical object also disables its collisions.

      @param mode  1/0 indicates on/off, and values greater than 1 mean
                   flip the bit from its current state

      @return  Whether the bit was enabled or disabled
    */
    bool toggleBit(TIABit b, uInt8 mode = 2);
    bool toggleBits();

    /**
      Enables/disable/toggle the specified (or all) TIA bit collision(s).

      @param mode  1/0 indicates on/off, and values greater than 1 mean
                   flip the collision from its current state

      @return  Whether the collision was enabled or disabled
    */
    bool toggleCollision(TIABit b, uInt8 mode = 2);
    bool toggleCollisions();

    /**
      Toggle the display of HMOVE blanks.

      @return  Whether the HMOVE blanking was enabled or disabled
    */
    bool toggleHMOVEBlank();

    /**
      Enables/disable/toggle 'fixed debug colors' mode.

      @param mode  1/0 indicates on/off, otherwise flip from
                   its current state

      @return  Whether the mode was enabled or disabled
    */
    bool toggleFixedColors(uInt8 mode = 2);

#ifdef DEBUGGER_SUPPORT
    /**
      This method should be called to update the TIA with a new scanline.
    */
    void updateScanline();

    /**
      This method should be called to update the TIA with a new partial
      scanline by stepping one CPU instruction.
    */
    void updateScanlineByStep();

    /**
      This method should be called to update the TIA with a new partial
      scanline by tracing to target address.
    */
    void updateScanlineByTrace(int target);
#endif

  private:
    /**
      Enables/disables all TIABit bits.  Note that disabling a graphical
      object also disables its collisions.

      @param mode  Whether to enable or disable all bits
    */
    void enableBits(bool mode);

    /**
      Enables/disables all TIABit collisions.

      @param mode  Whether to enable or disable all collisions
    */
    void enableCollisions(bool mode);

    // Update the current frame buffer to the specified color clock
    void updateFrame(Int32 clock);

    // Waste cycles until the current scanline is finished
    void waitHorizontalSync();

    // Reset horizontal sync counter
    void waitHorizontalRSync();

    // Clear both internal TIA buffers to black (palette color 0)
    void clearBuffers();

    // Set up bookkeeping for the next frame
    void startFrame();

    // Update bookkeeping at end of frame
    void endFrame();

    // Convert resistance from ports to dumped value
    uInt8 dumpedInputPort(int resistance);

    // Apply motion to registers when HMOVE is currently active
    void applyActiveHMOVEMotion(int hpos, Int16& pos, Int32 motionClock);

    // Apply motion to registers when HMOVE was previously active
    void applyPreviousHMOVEMotion(int hpos, Int16& pos, uInt8 motion);

  public:
    // Console the TIA is associated with
    Console& myConsole;

    // Sound object the TIA is associated with
    Sound& mySound;

    // Settings object the TIA is associated with
    Settings& mySettings;

    // Pointer to the current frame buffer
    uInt8* myCurrentFrameBuffer;

    // Pointer to the previous frame buffer
    uInt8* myPreviousFrameBuffer;

    // Pointer to the next pixel that will be drawn in the current frame buffer
    uInt8* myFramePointer;

    // Indicates offset used by the exported frame buffer
    // (the exported frame buffer is a vertical 'sliding window' of the actual buffer)
    uInt32 myFramePointerOffset;

    // Indicates the number of 'colour clocks' offset from the base
    // frame buffer pointer
    // (this is used when loading state files with a 'partial' frame)
    uInt32 myFramePointerClocks;

    // Indicated what scanline the frame should start being drawn at
    uInt32 myFrameYStart;

    // Indicates the height of the frame in scanlines
    uInt32 myFrameHeight;

    // Indicates offset in color clocks when display should stop
    uInt32 myStopDisplayOffset;

    // Indicates color clocks when the current frame began
    Int32 myClockWhenFrameStarted;

    // Indicates color clocks when frame should begin to be drawn
    Int32 myClockStartDisplay;

    // Indicates color clocks when frame should stop being drawn
    Int32 myClockStopDisplay;

    // Indicates color clocks when the frame was last updated
    Int32 myClockAtLastUpdate;

    // Indicates how many color clocks remain until the end of 
    // current scanline.  This value is valid during the 
    // displayed portion of the frame.
    Int32 myClocksToEndOfScanLine;

    // Indicates the total number of scanlines generated by the last frame
    uInt32 myScanlineCountForLastFrame;

    // Indicates the maximum number of scanlines to be generated for a frame
    uInt32 myMaximumNumberOfScanlines;

    // Indicates potentially the first scanline at which drawing occurs
    uInt32 myStartScanline;

    // Color clock when VSYNC ending causes a new frame to be started
    Int32 myVSYNCFinishClock; 

    uInt8 myVSYNC;        // Holds the VSYNC register value
    uInt8 myVBLANK;       // Holds the VBLANK register value

    uInt8 myPriorityEncoder[2][256];
    uInt32 myColor[8];
    uInt32 myFixedColor[8];
    uInt32* myColorPtr;

    uInt16 myCollision;     // Collision register

    // Determines whether specified collisions are enabled or disabled
    // The lower 16 bits are and'ed with the collision register to mask out
    // any collisions we don't want to be processed
    // The upper 16 bits are used to store which objects is currently
    // enabled or disabled
    // This is necessary since there are 15 collision combinations which
    // are controlled by 6 objects
    uInt32 myCollisionEnabledMask;

    // Audio values; only used by TIADebug
    uInt8 myAUDV0, myAUDV1, myAUDC0, myAUDC1, myAUDF0, myAUDF1;

    // Indicates when the dump for paddles was last set
    Int32 myDumpDisabledCycle;

    // Indicates if the dump is current enabled for the paddles
    bool myDumpEnabled;

    // Latches for INPT4 and INPT5
    uInt8 myINPT4, myINPT5;

    // Indicates if HMOVE blanks are currently or previously enabled
    bool myHMOVEBlankEnabled;
    bool myAllowHMOVEBlanks;

    // Indicates if unused TIA pins are randomly driven high or low
    // Otherwise, they take on the value previously on the databus
    bool myTIAPinsDriven;

    // Determines whether specified bits (from TIABit) are enabled or disabled
    // This is and'ed with the enabled objects each scanline to mask out any
    // objects we don't want to be processed
    uInt8 myDisabledObjects;

    // Indicates if color loss should be enabled or disabled.  Color loss
    // occurs on PAL (and maybe SECAM) systems when the previous frame
    // contains an odd number of scanlines.
    bool myColorLossEnabled;

    // Indicates whether we're done with the current frame. poke() clears this
    // when VSYNC is strobed or the max scanlines/frame limit is hit.
    bool myPartialFrameFlag;

    // Automatic framerate correction based on number of scanlines
    bool myAutoFrameEnabled;

    // Number of total frames displayed by this TIA
    uInt32 myFrameCounter;

    // Number of PAL frames displayed by this TIA
    uInt32 myPALFrameCounter;

    // The framerate currently in use by the Console
    float myFramerate;

    // Whether TIA bits/collisions are currently enabled/disabled
    bool myBitsEnabled, myCollisionsEnabled;

  private:
    // Copy constructor isn't supported by this class so make it private
    TIA(const TIA&);

    // Assignment operator isn't supported by this class so make it private
    TIA& operator = (const TIA&);


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class Frame
	{
	};

	/**
		This abstract class defines a basic TIA object with a color.
	*/
	class AbstractTIAObject 
	{
	public:
		AbstractTIAObject(const TIA& tia);

    virtual void reset();
		virtual void save(Serializer& out) const;
		virtual void load(Serializer& in);
    
		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		virtual uInt8 getState();
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		virtual void handleRegisterUpdate(uInt8 addr, uInt8 value);
		virtual string getName() const = 0;

		uInt32 getColor() const {return myColor;}

	protected:
		void handleCOLU(uInt8 value);
    void handleEnabled(uInt32 value);

  protected:  
		const TIA& myTIA;		
		uInt32 myColor;
	};

  /**
		This abstract class defines a the background object.
		It just holds the color.
	*/
	class Background : public AbstractTIAObject
	{
	public:
		Background(const TIA& tia);

		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		uInt8 getState();
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);
		string getName() const {return "BK";};
	};

	/**
		This abstract class defines a graphic TIA object which can be enabled.
	*/
	class AbstractGraphicObject : public AbstractTIAObject 
	{
	public:
		AbstractGraphicObject(const TIA& tia);

    virtual void reset();
		virtual void save(Serializer& out) const;
		virtual void load(Serializer& in);
    
		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		virtual uInt8 getState(Int32 clock) const;
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		virtual void handleRegisterUpdate(uInt8 addr, uInt8 value);

	protected:
    void handleEnabled(uInt32 value);
		virtual inline uInt8 getEnableBit() const = 0;

	protected:		
		bool isEnabled;
	};

	/**
		This class defines the playfield object.
	*/
	class Playfield : public AbstractGraphicObject
	{
	public:
		Playfield(const TIA& tia);

    void reset();
		void save(Serializer& out) const;
		void load(Serializer& in);

		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		uInt8 getState(Int32 clock) const;
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);
		string getName() const {return "PF";};

		// getter:
		uInt8 getCTRLPF() const {return myCTRLPF;}
		uInt32 getPF() const {return myPF;}
		uInt8 getPriorityAndScore() const {return myPriorityAndScore;}
		uInt8 getEnabled(uInt32 hpos) const ;

	protected:
    void handleCTRLPF(uInt8 value);
		void handlePF(uInt32 value, uInt32 mask);
		inline uInt32 getMaskValue() const {return myPF;}
		inline uInt8 getEnableBit() const {return PFBit;}
	
	private:
    uInt8 myCTRLPF;       // Playfield control register
	  uInt32 myPF;          // Playfield graphics (19-12:PF2 11-4:PF1 3-0:PF0)
    uInt8 myPriorityAndScore;
    const uInt32* myMask;
	};

	/**
		This abstract class defines a moveable graphic object.
	*/
	class AbstractMoveableGraphicObject : public AbstractGraphicObject
	{
	public :
		AbstractMoveableGraphicObject(const TIA& tia);

    void reset();
		void save(Serializer& out) const;
		void load(Serializer& in);

		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		virtual uInt8 getState(Int32 clock) const;
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		virtual void handleRegisterUpdate(uInt8 addr, uInt8 value);

		uInt8 getEnabled(uInt32 hpos) const;

		inline void handlePendingMotions();
		virtual inline void updateMask() = 0;
		
    // getter:
    uInt8 getHM() const {return myHM;}
    bool isVDEL() const {return myVDEL;}
    Int16 getPos() const {return myPos;};
    Int32 getMotionClock() const {return myMotionClock;};
    Int32 getStart() const {return myStart;};
		bool isHMmmr() const {return myHMmmr;};
		
		// setter (for debugger)
		void setPos(Int16 value) {myPos = value;};

  protected:
		void handleHM(uInt8 value);
    void handleHMOVE();
    void handleRES();
    void handleVDEL(uInt8 value);
    
		inline virtual uInt8 getMaskValue() const = 0;

    inline void applyActiveHMOVEMotion(int hpos, Int16& pos);
    inline void applyPreviousHMOVEMotion(int hpos, Int16& pos);
    virtual inline Int32 getActiveHPos(Int32 hpos) const = 0;
    virtual inline Int32 getPreviousHPos(Int32 hpos) const = 0;
    void handleRESChange(Int32 newx);

	public : // for now
		uInt8 myHM;			// horizontal motion register
	protected :
		bool myVDEL;		// Indicates if object is being vertically delayed (not used for missiles)

	  // Note that these position registers contain the color clock 
		// on which the object's serial output should begin (0 to 159)
		Int16 myPos;    // object horizontal position register 

	    // The color clocks elapsed so far for each of the graphical objects,
		// as denoted by 'MOTCK' line described in A. Towers TIA Hardware Notes
		Int32 myMotionClock;

		// Indicates 'start' signal for each of the graphical objects as
		// described in A. Towers TIA Hardware Notes
		Int32 myStart; // unused

		// Latches for 'more motion required' as described in A. Towers TIA
		// Hardware Notes
		bool myHMmmr;

	  // It's VERY important that the BL, M0, M1, P0 and P1 current
		// mask pointers are always on a uInt32 boundary.  Otherwise,
		// the TIA code will fail on a good number of CPUs.
		const uInt8* myMask;

		// Indicates color clocks when the frame was last updated
		Int32 myClockAtLastUpdate;

	public : // for now!
		// Indicates at which horizontal position the HMOVE was initiated
    Int32 myCurrentHMOVEPos;
    Int32 myPreviousHMOVEPos;
	};

	/**
		This abstract class defines a player object.
	*/
	class AbstractPlayer : public AbstractMoveableGraphicObject
	{
	public:		
		AbstractPlayer(const TIA& tia);

    void reset();
		void save(Serializer& out) const;
		void load(Serializer& in);

		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		uInt8 getState(Int32 clock) const;
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);

		inline void updateMask();

    // getter
    uInt8 getGRP() const {return myGRP;};
    uInt8 getNUSIZ() const {return myNUSIZ;};
    bool isREFP() const {return myREFP;};
    uInt8 getSuppress() const {return mySuppress;};
    uInt8 getCurrentGRP() const {return myCurrentGRP;};

	protected:
		// TODO: optization: only react if new value different from old one
		void handleGRP(uInt8 value);
		void handleDelayedGRP(uInt8 value);
    void handleHMOVE();
		void handleNUSIZ(uInt8 value);
		void handleREFP(uInt8 value);
		void handleVDEL(uInt8 value);
		inline uInt8 getMaskValue() const {return myCurrentGRP;}

	private:
		void handleCurrentGRP();
    inline Int32 getActiveHPos(Int32 hpos) const;
    inline Int32 getPreviousHPos(Int32 hpos) const;
    void handleRESChange(Int32 newx);

	protected :
		uInt8 myGRP;        // Player graphics register
		bool myREFP;		    // Indicates if player is being reflected

	public : // for now
		// Index into the player mask arrays indicating whether display
		// of the first copy should be suppressed
		uInt8 mySuppress;
	protected :
		uInt8 myDGRP;       // Player delayed graphics register

		// Graphics for Player that should be displayed.  This will be
		// reflected if the player is being reflected.
		uInt8 myCurrentGRP;		
	protected:
		uInt8 myNUSIZ;      // Number and size of player
    uInt8 myOldNUSIZ;   // previous number and size of player  
    Int32 myNUSIZClock; // clock when myNUSIZ changed
    uInt32 myNUSIZCLK;  // 

    Int32 myScanCount;     // number of CLK for current copy
    Int32 myScanCountPos;  // clock when myScanCount was calculated
	};

	/**
		This  class defines the player 0 object.
	*/
	class Player0 : public AbstractPlayer
	{
	public:
		Player0(const TIA& tia);

		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);
		string getName() const {return "P0";};

	protected:
		inline uInt8 getEnableBit() const {return P0Bit;}
	};

	/**
		This class defines the player 1 object.
	*/
	class Player1 : public AbstractPlayer
	{
	public:
		Player1(const TIA& tia);

		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);
		string getName() const {return "P1";};

	protected:
		inline uInt8 getEnableBit() const {return P1Bit;}
	};

	/**
		This abstract class defines a particle object.
	*/
	class AbstractParticle : public AbstractMoveableGraphicObject
	{
		// special missile and ball logic in here
	public:
		AbstractParticle(const TIA& tia);

    void reset();
		void save(Serializer& out) const;
		void load(Serializer& in);

		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);

	  // getters:
		bool isENABLE() const {return myENABLE;}
		// setters:
		bool setENABLE(bool value) {myENABLE = value;}

	protected:
    void handleENABLE(uInt8 value);
		uInt8 inline getMaskValue() const {return 0xff;}

  protected:
    bool myENABLE;        // Indicates if particle is enabled
	};
	
	/**
		This abstract class defines a misslile object.
	*/
	class AbstractMissile : public AbstractParticle 
	{
  public:
		AbstractMissile(const TIA& tia);

    void reset();
		void save(Serializer& out) const;
		void load(Serializer& in);

		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		virtual uInt8 getState(Int32 clock) const;
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);

		inline void updateMask();

		// getter
		uInt8 getNUSIZ() const {return myNUSIZ;};
		bool isRESMP() const {return myRESMP;}

  protected:
		void handleNUSIZ(uInt8 value);
    void handleRESMP(uInt8 value);
		void handleEnabled();
    virtual AbstractPlayer& getMyPlayer() const = 0;
    inline Int32 getActiveHPos(Int32 hpos) const;
    inline Int32 getPreviousHPos(Int32 hpos) const;

  protected:
		uInt8 myNUSIZ;       // Number and size of missle
    bool myRESMP;        // Indicates if missile is reset to player 
	};

	/**
		This class defines the missile 0 object.
	*/
	class Missile0 : public AbstractMissile 
	{
  public:
		Missile0(const TIA& tia);

		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		//virtual uInt8 getState();
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);
		string getName() const {return "M0";};

	protected:
		inline uInt8 getEnableBit() const {return M0Bit;}		
    AbstractPlayer& getMyPlayer() const {return myTIA.getPlayer0();}
	};

	/**
		This class defines the missile 1 object.
	*/
	class Missile1 : public AbstractMissile // maybe aggregate a common player/missile abstract class
	{
  public: 
		Missile1(const TIA& tia);

		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		//virtual uInt8 getState();
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);
		string getName() const {return "M1";};

	protected:
		inline uInt8 getEnableBit() const {return M1Bit;}
    AbstractPlayer& getMyPlayer() const {return myTIA.getPlayer1();}
	};

	/**
		This class defines the ball object.
	*/
	class Ball : public AbstractParticle // maybe aggregate a common player/missile abstract class
	{
  public:
		Ball(const TIA& tia);

    void reset();
		void save(Serializer& out) const;
		void load(Serializer& in);

		// Triggers an update of the object until the current clock, returns true if the current pixel is enabled (TODO: color, priorities).
		uInt8 getState(Int32 clock) const;
		// Informs the object that a TIA register has been updated. The object decides if and how to handle it.
		void handleRegisterUpdate(uInt8 addr, uInt8 value);
		string getName() const {return "BL";};

		inline void updateMask();

		// getter:
		uInt8 getCTRLPF() const {return myCTRLPF;}

	protected :			
		inline uInt8 getEnableBit() const {return BLBit;}
    void handleCTRLPF(uInt8 value);
    void handleGRP1(uInt8 value);
    void handleCurrentEnabled();
    inline Int32 getActiveHPos(Int32 hpos) const;
    inline Int32 getPreviousHPos(Int32 hpos) const;

  protected :
    uInt8 myCTRLPF;     // Playfield control register
		bool myDENABLE;     // Indicates if the vertically delayed ball is enabled
    bool myCurrentEnabled;
	};


  private :
		Player0 myPlayer0;
		Player1 myPlayer1;
		Missile0 myMissile0;
		Missile1 myMissile1;
		Ball myBall;
    Playfield myPlayfield;
		Background myBackground;

  public:
    Player0 getPlayer0() const {return myPlayer0;}
    Player1 getPlayer1() const {return myPlayer1;}
    Missile0 getMissile0() const {myMissile0;}
    Missile1 getMissile1() const {myMissile1;}
    Ball getBall() const {return myBall;}
    Playfield getPlayfield() const {return myPlayfield;}
		Background getBackground() const {return myBackground;}
};

#endif
