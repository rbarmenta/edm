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

#ifndef __symbol_h
#define __symbol_h 1

#include "act_grf.h"
#include "undo.h"
#include "entry_form.h"

// the following defines btnActionListType & btnActionListPtr
#include "btnActionListType.h"

#include "cadef.h"

#define ASC_MAJOR_VERSION 1
#define ASC_MINOR_VERSION 6
#define ASC_RELEASE 0

#define SYMBOL_K_NUM_STATES 64
#define SYMBOL_K_MAX_PVS 5

#define OR_ORIG 0
#define OR_CW 1
#define OR_CCW 2
#define OR_V 3
#define OR_H 4

#ifdef __symbol_cc

#include "symbol.str"

static char *dragName[] = {
  activeSymbolClass_str1,
};

static char *dragNameTruthTable[] = {
  activeSymbolClass_str2,
  activeSymbolClass_str3,
  activeSymbolClass_str4,
  activeSymbolClass_str5,
  activeSymbolClass_str6,
};

static void symUnconnectedTimeout (
  XtPointer client,
  XtIntervalId *id );

static void symbol_monitor_control_connect_state (
  struct connection_handler_args arg );

static void symbol_monitor_color_connect_state (
  struct connection_handler_args arg );

static void symbol_controlUpdate (
  struct event_handler_args ast_args );

static void symbol_colorUpdate (
  struct event_handler_args ast_args );

static void symbolSetItem (
  Widget w,
  XtPointer client,
  XtPointer call );

static void asc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

static void asc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

static void asc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

static void asc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

static void asc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

#endif

typedef struct objPlusIndexTag {
  void *objPtr;
  int index;
  unsigned int setMask;
  unsigned int clrMask;
} objPlusIndexType, *objPlusIndexPtr;

class activeSymbolClass : public activeGraphicClass {

private:

friend class undoSymbolOpClass;

objPlusIndexType argRec[SYMBOL_K_MAX_PVS];

void *voidHead[SYMBOL_K_NUM_STATES]; // cast to activeGraphicListPtr
                                     // array at runtime

int numPvs;

chid controlPvId[SYMBOL_K_MAX_PVS];
evid controlEventId[SYMBOL_K_MAX_PVS];

chid colorPvId;
evid colorEventId;

unsigned int notControlPvConnected;
int init, active, activeMode, opComplete, controlExists, colorExists,
 colorPvConnected;

int iValue;
double controlVals[SYMBOL_K_MAX_PVS], controlV, curControlV, curColorV;
double stateMinValue[SYMBOL_K_NUM_STATES];
double stateMaxValue[SYMBOL_K_NUM_STATES];
char symbolFileName[127+1];
expStringClass controlPvExpStr[SYMBOL_K_MAX_PVS];
expStringClass colorPvExpStr;

btnActionListPtr btnDownActionHead;
btnActionListPtr btnUpActionHead;
btnActionListPtr btnMotionActionHead;

int useOriginalSize, useOriginalColors;
int numStates;
int index, prevIndex;

entryListBase *elsvMin;
entryListBase *elsvMax;
double *minPtr[SYMBOL_K_NUM_STATES], *maxPtr[SYMBOL_K_NUM_STATES];

int bufX;
int bufY;
int bufW;
int bufH;

double bufStateMinValue[SYMBOL_K_NUM_STATES];
double bufStateMaxValue[SYMBOL_K_NUM_STATES];
char bufSymbolFileName[127+1],
 bufControlPvName[SYMBOL_K_MAX_PVS][activeGraphicClass::MAX_PV_NAME+1],
 bufColorPvName[activeGraphicClass::MAX_PV_NAME+1];
int bufNumStates, bufUseOriginalSize, bufUseOriginalColors;

int fgColor, bgColor, bufFgColor, bufBgColor;
colorButtonClass fgCb, bgCb;

int binaryTruthTable, bufBinaryTruthTable;

entryListBase *pvNamesObj;

int needErase, needDraw, needConnectInit, needConnect[SYMBOL_K_MAX_PVS],
 needRefresh, needColorInit, needColorRefresh, needToDrawUnconnected,
 needToEraseUnconnected;
int unconnectedTimer;

int orientation, bufOrientation, prevOr;

public:

undoClass undoObj;

friend void symUnconnectedTimeout (
  XtPointer client,
  XtIntervalId *id );

friend void symbol_monitor_control_connect_state (
  struct connection_handler_args arg );

friend void symbol_monitor_color_connect_state (
  struct connection_handler_args arg );

friend void symbol_controlUpdate (
  struct event_handler_args ast_args );

friend void symbol_colorUpdate (
  struct event_handler_args ast_args );

friend void symbolSetItem (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void asc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void asc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void asc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void asc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void asc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

activeSymbolClass ( void );

activeSymbolClass
 ( const activeSymbolClass *source );

~activeSymbolClass ( void );

int genericEdit ( void );

int edit ( void );

int editCreate ( void );

int readSymbolFile ( void );

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

int draw ( void );

int erase ( void );

int drawActive ( void );

int eraseActive ( void );

int activate (
  int pass,
  void *ptr );

int deactivate ( int pass );

int moveSelectBox (
  int _x,
  int _y );

int moveSelectBoxAbs (
  int _x,
  int _y );

int moveSelectBoxMidpointAbs (
  int _x,
  int _y );

int checkResizeSelectBox (
  int _x,
  int _y,
  int _w,
  int _h );

int resizeSelectBox (
  int _x,
  int _y,
  int _w,
  int _h );

int checkResizeSelectBoxAbs (
  int _x,
  int _y,
  int _w,
  int _h );

int resizeSelectBoxAbs (
  int _x,
  int _y,
  int _w,
  int _h );

int resizeSelectBoxAbsFromUndo (
  int _x,
  int _y,
  int _w,
  int _h );

int move (
  int x,
  int y );

int moveAbs (
  int x,
  int y );

int moveMidpointAbs (
  int x,
  int y );

int resize (
  int _x,
  int _y,
  int _w,
  int _h );

int resizeAbs (
  int _x,
  int _y,
  int _w,
  int _h );

int resizeAbsFromUndo (
  int _x,
  int _y,
  int _w,
  int _h );

int getButtonActionRequest (
  int *up,
  int *down,
  int *drag );

void btnDown (
  int x,
  int y,
  int buttonState,
  int buttonNumber );

void btnUp (
  int x,
  int y,
  int buttonState,
  int buttonNumber );

void updateGroup ( void );

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

int setProperty (
  char *prop,
  int *value );

char *firstDragName ( void );

char *nextDragName ( void );

char *dragValue (
  int i );

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

int rotate (
  int xOrigin,
  int yOrigin,
  char direction ); // '+'=clockwise, '-'=counter clockwise

int flip (
  int xOrigin,
  int yOrigin,
  char direction );

int rotateInternal (
  int xOrigin,
  int yOrigin,
  char direction ); // '+'=clockwise, '-'=counter clockwise

int flipInternal (
  int xOrigin,
  int yOrigin,
  char direction );

void flushUndo ( void );

int addUndoCreateNode ( undoClass *_undoObj );

int addUndoMoveNode ( undoClass *_undoObj );

int addUndoResizeNode ( undoClass *_undoObj );

int addUndoCopyNode ( undoClass *_undoObj );

int addUndoCutNode ( undoClass *_undoObj );

int addUndoPasteNode ( undoClass *_undoObj );

int addUndoReorderNode ( undoClass *_undoObj );

int addUndoEditNode ( undoClass *_undoObj );

int addUndoGroupNode ( undoClass *_undoObj );

int addUndoRotateNode ( undoClass *_undoObj );

int addUndoFlipNode ( undoClass *_undoObj );

int undoCreate (
  undoOpClass *opPtr );

int undoMove (
  undoOpClass *opPtr,
  int x,
  int y );

int undoResize (
  undoOpClass *opPtr,
  int x,
  int y,
  int w,
  int h );

int undoCopy (
  undoOpClass *opPtr );

int undoCut (
  undoOpClass *opPtr );

int undoPaste (
  undoOpClass *opPtr );

int undoReorder (
  undoOpClass *opPtr );

int undoEdit (
  undoOpClass *opPtr );

int undoGroup (
  undoOpClass *opPtr );

int undoRotate (
  undoOpClass *opPtr,
  int x,
  int y,
  int w,
  int h );

int undoFlip (
  undoOpClass *opPtr,
  int x,
  int y,
  int w,
  int h );

void updateColors (
  double colorValue );

};

#endif
