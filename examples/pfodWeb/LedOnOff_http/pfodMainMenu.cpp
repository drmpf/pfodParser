/*   
   pfodMainMenu.cpp
 * (c)2025 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

#include "pfodMainMenu.h"
#include <pfodParser.h>
#include <pfodDebugPtr.h>
#include <pfodDwgs.h>
#include <pfodDrawing.h>
#include "pfodMainDrawing.h"  // defines mainDwg and initDrawings()

// or just used debugPtr = getDebugPtr()

// #define DEBUG
static Print* debugPtr = NULL;  // local to this file
static bool initialized = false;

static pfodCloseConnectionPtr closeConnectionFnPtr = NULL;

void sendMainMenu(pfodParser& parser);
void sendMainMenuUpdate(pfodParser& parser);

extern pfodDrawing& mainDwg;
static void closeConnection(Stream* io); 
// for ...pfodWebServer, calls to closeConnection will just do nothing as the Stream io will not match any open tcp connection stream

void init_pfodMainMenu(pfodCloseConnectionPtr _closeConnectionFnPtr) {
  if (initialized) {
    return;
  }
  (void)debugPtr;  // suppress not used warning
#ifdef DEBUG
  debugPtr = getDebugPtr();
#endif
  initialized = true;
  closeConnectionFnPtr = _closeConnectionFnPtr;
  initDrawings();
}


void handle_pfodMainMenu(pfodParser& parser) {
  if (!initialized) {
    if (debugPtr) {
      debugPtr->println(" Need to call init_pfodMainMenu() from setup().");
    }
  }
  uint8_t cmd = parser.parse();  // parse incoming data from connection
  // parser returns non-zero when a pfod command is fully parsed and not consumed by an inserted dwg
  if (cmd != 0) {
    if ('.' == cmd) {
      // pfodApp has connected and sent {.} , it is asking for the main menu
      if (parser.isRefresh()) {
        sendMainMenuUpdate(parser);  // menu is cached just send update
      } else {
        sendMainMenu(parser);  // send back the menu designed
      }

    } else if (parser.cmdEquals('A')) {  // click in A menu item that holds the main dwg
      // add touchZone handling here that is not handled in dwg's processDwgCmds
      if (parser.isRefresh()) {
        sendMainMenuUpdate(parser);  // menu is cached just send update
      } else {
        sendMainMenu(parser);  // send back the menu designed
      }

    } else if ('!' == cmd) {
      closeConnection(parser.getPfodAppStream());
    } else {
      // unknown command including @ and ! which are ignored here
      parser.print("{}");  // always send back a pfod msg otherwise pfodApp will disconnect.
    }
  }
}

static void closeConnection(Stream* io) {
  if (closeConnectionFnPtr) {
    closeConnectionFnPtr(io);
  }
}

void sendMainMenu(pfodParser& parser) {
  parser.print(F("{,"));            // start a Menu screen pfod message
  parser.print("<bg w>~");          // no prompt
  parser.sendRefreshAndVersion(0);  // version and refresh ignored by pfodWeb for now
  // send menu items
  parser.print("|+A~");
  parser.print(mainDwg);  // the unique load cmd
  parser.print(F("}"));   // close pfod message
}

void sendMainMenuUpdate(pfodParser& parser) {
  parser.print(F("{;"));  // start an Update Menu pfod message
  parser.print(F("~"));   // no change to colours and size
  // send menu items
  parser.print(F("|+A"));  // reload A calls for dwg update
  parser.print(F("}"));    // close pfod message
  // ============ end of menu ===========
}
