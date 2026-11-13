/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
/*
 * OnOff_SerialApp.cpp -- turns the output pin (target_Pin.cpp) on and off from the
 * generated OnOff_Serial menu, WITHOUT editing any of the generated files.
 *
 * As described in pfodMainMenu.h and Dwg_LedXXX.h, each generated class is
 * subclassed here, an instance of the subclass is defined, and the weak
 * get_pfodMainMenu() / get_dwg_LedXXX() defaults are replaced by the
 * functions below that return these instances. The generated code then
 * uses these instances instead of its own defaults.
 *
 * - pfodMainMenuApp    sets up the pin and drives it when the 'Led is' On/Off slider changes
 * - Dwg_LedOnApp       turns the pin on when the drawing's 'Turn Led On' button is pressed
 * - Dwg_LedOffApp      turns the pin off when the drawing's 'Turn Led Off' button is pressed
 * - Dwg_LedOnOffApp    shows 'Led is On' / 'Led is Off' in the drawing
 *
 * The pin state is kept in the generated menu's onOffOut_var (0 = Off, 1 = On),
 * so the slider, the drawing text and the pin always agree.
 * This file is the same for every board; only target_Pin.cpp is board specific.
 */
#include "pfodParser.h"
#include "pfodMainMenu.h"
#include "Dwg_LedOnOff.h"
#include "Dwg_LedOn.h"
#include "Dwg_LedOff.h"
#include "target_Pin.h"

extern int onOffOut_var; // defined in the generated pfodMainMenu.cpp, 0=Off 1=On

/**
 * Set the menu's On/Off value and drive the output pin to match.
 * @param value 0 for Off, non-zero for On
 */
static void setOutput(int value) {
  onOffOut_var = (value != 0) ? 1 : 0;
  target_PinWrite(onOffOut_var != 0);
}

/**
 * The main menu, with the pin set up in init() and driven by the 'Led is' slider.
 */
class pfodMainMenuApp : public pfodMainMenu {
  public:
    /**
     * Configure the output pin, set it to the menu's initial value (Off),
     * then initialize the generated menu and its drawings.
     * Called once from setup() via init_pfodMainMenu().
     * @param _closeConnectionFnPtr passed on to pfodMainMenu::init()
     */
    void init(pfodCloseConnectionPtr _closeConnectionFnPtr) override {
      target_PinInit();
      setOutput(onOffOut_var);
      pfodMainMenu::init(_closeConnectionFnPtr);
    }
  protected:
    /**
     * Called by the generated handle() after the user moves the 'Led is' slider.
     * handle() has already stored the new value in onOffOut_var and sends the
     * menu update after this returns.
     * @param value the new slider value, 0=Off 1=On
     */
    void onOffOut_Changed(int value) override {
      setOutput(value);
    }
};

/**
 * The 'LedOnOff' drawing, with its label showing the current pin state.
 */
class Dwg_LedOnOffApp : public Dwg_LedOnOff {
  protected:
    /**
     * Send the drawing's label as 'Led is On' or 'Led is Off' instead of the
     * generated fixed 'Led is Off'. Otherwise the same as the generated
     * Dwg_LedOnOff::sendIndexedItems().
     * Sent with the full drawing, on its 2 sec refresh, and when the menu
     * update after a button press asks for the drawing to be reloaded.
     */
    void sendIndexedItems() override {
      dwgsPtr->label().idx(idx_1).color(dwgsPtr->BLACK).text(onOffOut_var ? "Led is On" : "Led is Off").bold().offset(20,11.5).center().decimals(2).send();
    }
};

/**
 * The 'LedOn' drawing, whose button turns the pin on.
 */
class Dwg_LedOnApp : public Dwg_LedOn {
  protected:
    /**
     * Called when the 'Turn Led On' button (touchZone cmd_c1) is pressed.
     * Turns the pin on and returns false, so the command passes up to the
     * main menu's handle(), which sends a menu update. That update sets the
     * slider to On and reloads the 'LedOnOff' drawing to show 'Led is On'.
     * @param row, col, touchType, editedText not used
     * @return false, the main menu sends the response
     */
    bool Dwg_LedOn_cmd_c1(int row, int col, uint8_t touchType, const byte* editedText) override {
      (void)row; (void)col; (void)touchType; (void)editedText; // suppress warnings
      setOutput(1);
      return false;
    }
};

/**
 * The 'LedOff' drawing, whose button turns the pin off.
 */
class Dwg_LedOffApp : public Dwg_LedOff {
  protected:
    /**
     * Called when the 'Turn Led Off' button (touchZone cmd_c1) is pressed.
     * Turns the pin off and returns false, so the main menu sends the update,
     * as for Dwg_LedOnApp.
     * @param row, col, touchType, editedText not used
     * @return false, the main menu sends the response
     */
    bool Dwg_LedOff_cmd_c1(int row, int col, uint8_t touchType, const byte* editedText) override {
      (void)row; (void)col; (void)touchType; (void)editedText; // suppress warnings
      setOutput(0);
      return false;
    }
};

// the instances the generated code uses, instead of its own default instances
pfodMainMenuApp mainMenuApp;
Dwg_LedOnOffApp dwg_LedOnOffApp;
Dwg_LedOnApp dwg_LedOnApp;
Dwg_LedOffApp dwg_LedOffApp;

// These replace the weak defaults in the generated .cpp files at link time.
pfodMainMenu& get_pfodMainMenu() { return mainMenuApp; }
Dwg_LedOnOff& get_dwg_LedOnOff() { return dwg_LedOnOffApp; }
Dwg_LedOn& get_dwg_LedOn() { return dwg_LedOnApp; }
Dwg_LedOff& get_dwg_LedOff() { return dwg_LedOffApp; }
