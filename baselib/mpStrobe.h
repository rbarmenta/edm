//  edm - extensible display manager

//  Copyright (C) 1999 John W. Sinclair

//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __mpStrobe_h
#define __mpStrobe_h 1

#include "act_grf.h"
#include "entry_form.h"
#include "keypad.h"
#include "pv_factory.h"
#include "cvtFast.h"
#include "sys_types.h"
#include "thread.h"

#define MPSC_MAJOR_VERSION 4
#define MPSC_MINOR_VERSION 0
#define MPSC_RELEASE 0

#define MPSC_IDLE 1
#define MPSC_PINGING 2

#define MPSC_K_TOGGLE 0
#define MPSC_K_CYCLE 1
#define MPSC_K_TRIG 2
#define MPSC_K_RANDOM 3

#ifdef __mpStrobe_cc

#include "mpStrobe.str"

static void doBlink (
  void *ptr
);

static void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id );

static void mpsc_monitor_vis_connect_state (
  ProcessVariable *pv,
  void *userarg );

static void mpsc_visUpdate (
  ProcessVariable *pv,
  void *userarg );

static void mpsc_monitor_color_connect_state (
  ProcessVariable *pv,
  void *userarg );

static void mpsc_colorUpdate (
  ProcessVariable *pv,
  void *userarg );

static void mpsc_controlUpdate (
  ProcessVariable *pv,
  void *userarg );

static void mpsc_destUpdate (
  ProcessVariable *pv,
  void *userarg );

static void mpsc_ping (
  XtPointer client,
  XtIntervalId *id );

static char *dragName[] = {
  activeMpStrobeClass_str8,
  activeMpStrobeClass_str9,
  activeMpStrobeClass_str33,
  activeMpStrobeClass_str29
};

static void mpsc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mpsc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mpsc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mpsc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mpsc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

static void mpsc_monitor_control_connect_state (
  ProcessVariable *pv,
  void *userarg );

static void mpsc_monitor_dest_connect_state (
  ProcessVariable *pv,
  void *userarg );

#endif

class activeMpStrobeClass : public activeGraphicClass {

private:

friend void doBlink (
  void *ptr
);

friend void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id );

friend void mpsc_monitor_vis_connect_state (
  ProcessVariable *pv,
  void *userarg );

friend void mpsc_visUpdate (
  ProcessVariable *pv,
  void *userarg );

friend void mpsc_monitor_color_connect_state (
  ProcessVariable *pv,
  void *userarg );

friend void mpsc_colorUpdate (
  ProcessVariable *pv,
  void *userarg );

friend void mpsc_controlUpdate (
  ProcessVariable *pv,
  void *userarg );

friend void mpsc_destUpdate (
  ProcessVariable *pv,
  void *userarg );

friend void mpsc_ping (
  XtPointer client,
  XtIntervalId *id );

friend void mpsc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mpsc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mpsc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mpsc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mpsc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void mpsc_monitor_control_connect_state (
  ProcessVariable *pv,
  void *userarg );

friend void mpsc_monitor_dest_connect_state (
  ProcessVariable *pv,
  void *userarg );

int opComplete;

int minW;
int minH;

typedef struct editBufTag {
// edit buffer
  int bufX;
  int bufY;
  int bufW;
  int bufH;
  int bufFgColor;
  int bufBgColor;
  int bufOffColor;
  int bufTopShadowColor;
  int bufBotShadowColor;
  int bufAutoPing;
  int buf3D;
  int bufInvisible;
  int bufDisableBtn;
  char bufCycleTypeStr[31+1];
  double bufFirstVal;
  double bufSecondVal;
  double bufPingRate;
  int bufVisInverted;
  colorButtonClass fgCb;
  colorButtonClass bgCb;
  colorButtonClass offCb;
  colorButtonClass topShadowCb;
  colorButtonClass botShadowCb;
  char bufOnLabel[39+1];
  char bufOffLabel[39+1];
  char bufControlPvName[PV_Factory::MAX_PV_NAME+1];
  char bufDestPvName[PV_Factory::MAX_PV_NAME+1];
  char bufVisPvName[PV_Factory::MAX_PV_NAME+1];
  char bufMinVisString[39+1];
  char bufMaxVisString[39+1];
  char bufColorPvName[PV_Factory::MAX_PV_NAME+1];
} editBufType, *editBufPtr;

editBufPtr eBuf;

entryListBase *invisPvEntry, *visInvEntry, *minVisEntry, *maxVisEntry;

int controlType, destType, destSize;

pvColorClass fgColor, bgColor, offColor;
int topShadowColor;
int botShadowColor;
expStringClass onLabel;
expStringClass offLabel;
int autoPing, _3D, invisible, disableBtn, cycleType;
unsigned int randSeed;
double firstVal, secondVal;

fontMenuClass fm;
char fontTag[63+1];
XFontStruct *fs;
int fontAscent, fontDescent, fontHeight;

static const int controlPvConnection = 1;
static const int destPvConnection = 2;
static const int visPvConnection = 3;
static const int colorPvConnection = 4;

pvConnectionClass connection;

ProcessVariable *controlPvId, *destPvId;

expStringClass controlPvExpString;

expStringClass destPvExpString;

double pingRate;

int controlExists, destExists, buttonPressed;

int controlPvConnected, destPvConnected, active, activeMode, init;

int pingTimerActive, pingTimerValue;
XtIntervalId pingTimer;
double controlV, curControlV, destV, curDestV;

int needConnectInit, needDestConnectInit, needCtlInfoInit, needRefresh,
 needErase, needDraw, needToDrawUnconnected, needToEraseUnconnected, needDestUpdate;
XtIntervalId unconnectedTimer;
int initialConnection, initialDestValueConnection,
 initialVisConnection, initialColorConnection;

ProcessVariable *visPvId;
expStringClass visPvExpString;
int visExists;
double visValue, curVisValue, minVis, maxVis;
char minVisString[39+1];
char maxVisString[39+1];
int prevVisibility, visibility, visInverted;
int needVisConnectInit, needVisInit, needVisUpdate;

ProcessVariable *colorPvId;
expStringClass colorPvExpString;
int colorExists;
double colorValue, curColorValue;
int needColorConnectInit, needColorInit, needColorUpdate;

int rootX, rootY;

int state;

public:

activeMpStrobeClass ( void );

activeMpStrobeClass
 ( const activeMpStrobeClass *source );

~activeMpStrobeClass ( void );

char *objName ( void ) {

  return name;

}

int createInteractive (
  activeWindowClass *aw_obj,
  int x,
  int y,
  int w,
  int h );

int save (
  FILE *f );

int createFromFile (
  FILE *fptr,
  char *name,
  activeWindowClass *actWin );

int genericEdit ( void );

int edit ( void );

int editCreate ( void );

int draw ( void );

int erase ( void );

int drawActive ( void );

int eraseActive ( void );

int activate ( int pass, void *ptr );

int deactivate ( int pass );

void updateDimensions ( void );

void btnUp (
  XButtonEvent *be,
  int x,
  int y,
  int buttonState,
  int buttonNumber,
  int *action );

void btnDown (
  XButtonEvent *be,
  int x,
  int y,
  int buttonState,
  int buttonNumber,
  int *action );

void pointerIn (
  int _x,
  int _y,
  int buttonState );

int getButtonActionRequest (
  int *up,
  int *down,
  int *drag,
  int *focus );

int expandTemplate (
  int numMacros,
  char *macros[],
  char *expansions[] );

int expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] );

int expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] );

int containsMacros ( void );

void executeDeferred ( void );

char *firstDragName ( void );

char *nextDragName ( void );

char *dragValue (
  int i );

void changeDisplayParams (
  unsigned int flag,
  char *fontTag,
  int alignment,
  char *ctlFontTag,
  int ctlAlignment,
  char *btnFontTag,
  int btnAlignment,
  int textFgColor,
  int fg1Color,
  int fg2Color,
  int offsetColor,
  int bgColor,
  int topShadowColor,
  int botShadowColor );

void changePvNames (
  int flag,
  int numCtlPvs,
  char *ctlPvs[],
  int numReadbackPvs,
  char *readbackPvs[],
  int numNullPvs,
  char *nullPvs[],
  int numVisPvs,
  char *visPvs[],
  int numAlarmPvs,
  char *alarmPvs[] );

void getPvs (
  int max,
  ProcessVariable *pvs[],
  int *n );

char *crawlerGetFirstPv ( void );

char *crawlerGetNextPv ( void );

};

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeMpStrobeClassPtr ( void );
void *clone_activeMpStrobeClassPtr ( void * );

#ifdef __cplusplus
}
#endif

#endif