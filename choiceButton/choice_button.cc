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

#define __choice_button_cc 1

#include "choice_button.h"
#include "app_pkg.h"
#include "act_win.h"

#include "thread.h"

static void doBlink (
  void *ptr
) {

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ptr;

  if ( !acbo->activeMode ) {
    if ( acbo->isSelected() ) acbo->drawSelectBoxCorners(); // erase via xor
    acbo->smartDrawAll();
    if ( acbo->isSelected() ) acbo->drawSelectBoxCorners();
  }
  else {
    acbo->bufInvalidate();
    acbo->needDraw = 1;
    acbo->actWin->addDefExeNode( acbo->aglPtr );
  }

}

static void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) client;

  if ( !acbo->connection.pvsConnected() ) {
    acbo->needToDrawUnconnected = 1;
    acbo->needDraw = 1;
    acbo->actWin->addDefExeNode( acbo->aglPtr );
  }

  acbo->unconnectedTimer = 0;

}

#ifdef __epics__

static void acb_monitor_control_connect_state (
  struct connection_handler_args arg )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    acbo->connection.setPvConnected( (void *) acbo->controlPvConnection );
    acbo->needConnectInit = 1;

    if ( acbo->connection.pvsConnected() ) {
      acbo->fgColor.setConnected();
    }

  }
  else {

    acbo->connection.setPvDisconnected( (void *) acbo->controlPvConnection );
    acbo->fgColor.setDisconnected();
    acbo->controlValid = 0;
    acbo->needDraw = 1;
    acbo->active = 0;

  }

  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_infoUpdate (
  struct event_handler_args ast_args )
{

  if ( ast_args.status == ECA_DISCONN ) {
    return;
  }

int i;
activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ast_args.usr;
struct dbr_gr_enum enumRec;

  enumRec = *( (struct dbr_gr_enum *) ast_args.dbr );

  acbo->numStates = enumRec.no_str;

  for ( i=0; i<acbo->numStates; i++ ) {

    if ( acbo->stateString[i] == NULL ) {
      acbo->stateString[i] = new char[MAX_ENUM_STRING_SIZE+1];
    }

    strncpy( acbo->stateString[i], enumRec.strs[i], MAX_ENUM_STRING_SIZE );

  }

  acbo->curValue = enumRec.value;

  acbo->needInfoInit = 1;
  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_controlUpdate (
  struct event_handler_args ast_args )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ast_args.usr;

  acbo->curValue = *( (short *) ast_args.dbr );

  acbo->controlValid = 1;
  acbo->needRefresh = 1;
  acbo->needDraw = 1;
  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_alarmUpdate (
  struct event_handler_args ast_args )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ast_args.usr;
struct dbr_sts_enum statusRec;

  if ( !acbo->readExists ) {

    statusRec = *( (struct dbr_sts_enum *) ast_args.dbr );

    acbo->fgColor.setStatus( statusRec.status, statusRec.severity );
    acbo->bgColor.setStatus( statusRec.status, statusRec.severity );

    acbo->needDraw = 1;
    acbo->actWin->appCtx->proc->lock();
    acbo->actWin->addDefExeNode( acbo->aglPtr );
    acbo->actWin->appCtx->proc->unlock();

  }

}

static void acb_monitor_read_connect_state (
  struct connection_handler_args arg )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    acbo->connection.setPvConnected( (void *) acbo->readPvConnection );
    acbo->needReadConnectInit = 1;

    if ( acbo->connection.pvsConnected() ) {
      acbo->fgColor.setConnected();
    }

  }
  else {

    acbo->connection.setPvDisconnected( (void *) acbo->readPvConnection );
    acbo->fgColor.setDisconnected();
    acbo->readValid = 0;
    acbo->needDraw = 1;
    acbo->active = 0;

  }

  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_readInfoUpdate (
  struct event_handler_args ast_args )
{

  if ( ast_args.status == ECA_DISCONN ) {
    return;
  }

int i;
activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ast_args.usr;
struct dbr_gr_enum enumRec;

  enumRec = *( (struct dbr_gr_enum *) ast_args.dbr );

  if ( !acbo->controlExists ) {

  acbo->numStates = enumRec.no_str;

    for ( i=0; i<acbo->numStates; i++ ) {

      if ( acbo->stateString[i] == NULL ) {
        acbo->stateString[i] = new char[MAX_ENUM_STRING_SIZE+1];
      }

      strncpy( acbo->stateString[i], enumRec.strs[i],
       MAX_ENUM_STRING_SIZE );

    }

  }

  acbo->curReadValue = enumRec.value;

  acbo->needReadInfoInit = 1;
  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_readUpdate (
  struct event_handler_args ast_args )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ast_args.usr;

  acbo->curReadValue = *( (short *) ast_args.dbr );

  acbo->readValid = 1;
  acbo->needRefresh = 1;
  acbo->needDraw = 1;
  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_readAlarmUpdate (
  struct event_handler_args ast_args )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ast_args.usr;
struct dbr_sts_enum statusRec;

  statusRec = *( (struct dbr_sts_enum *) ast_args.dbr );

  acbo->fgColor.setStatus( statusRec.status, statusRec.severity );
  acbo->bgColor.setStatus( statusRec.status, statusRec.severity );

  acbo->needDraw = 1;
  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_monitor_vis_connect_state (
  struct connection_handler_args arg )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    acbo->needVisConnectInit = 1;

  }
  else {

    acbo->connection.setPvDisconnected( (void *) acbo->visPvConnection );
    acbo->fgColor.setDisconnected();
    acbo->active = 0;
    acbo->needDraw = 1;

  }

  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_visInfoUpdate (
  struct event_handler_args arg )
{

  if ( arg.status == ECA_DISCONN ) {
    return;
  }

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ca_puser(arg.chid);

struct dbr_gr_double controlRec = *( (dbr_gr_double *) arg.dbr );

  acbo->curVisValue = controlRec.value;

  acbo->actWin->appCtx->proc->lock();
  acbo->needVisInit = 1;
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_visUpdate (
  struct event_handler_args arg )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ca_puser(arg.chid);

  acbo->curVisValue = * ( (double *) arg.dbr );

  acbo->actWin->appCtx->proc->lock();
  acbo->needVisUpdate = 1;
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_monitor_color_connect_state (
  struct connection_handler_args arg )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    acbo->needColorConnectInit = 1;

  }
  else {

    acbo->connection.setPvDisconnected( (void *) acbo->colorPvConnection );
    acbo->fgColor.setDisconnected();
    acbo->active = 0;
    acbo->needDraw = 1;

  }

  acbo->actWin->appCtx->proc->lock();
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_colorInfoUpdate (
  struct event_handler_args ast_args )
{

  if ( ast_args.status == ECA_DISCONN ) {
    return;
  }

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ast_args.usr;

struct dbr_gr_double controlRec = *( (dbr_gr_double *) ast_args.dbr );

  acbo->curColorValue = controlRec.value;

  acbo->actWin->appCtx->proc->lock();
  acbo->needColorInit = 1;
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

static void acb_colorUpdate (
  struct event_handler_args ast_args )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) ast_args.usr;

  acbo->curColorValue = * ( (double *) ast_args.dbr );

  acbo->actWin->appCtx->proc->lock();
  acbo->needColorUpdate = 1;
  acbo->actWin->addDefExeNode( acbo->aglPtr );
  acbo->actWin->appCtx->proc->unlock();

}

#endif

static void acbc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) client;

  acbo->actWin->setChanged();

  acbo->eraseSelectBoxCorners();
  acbo->erase();

  strncpy( acbo->fontTag, acbo->fm.currentFontTag(), 63 );
  acbo->actWin->fi->loadFontTag( acbo->fontTag );
  acbo->actWin->drawGc.setFontTag( acbo->fontTag, acbo->actWin->fi );
  acbo->actWin->fi->getTextFontList( acbo->fontTag, &acbo->fontList );
  acbo->fs = acbo->actWin->fi->getXFontStruct( acbo->fontTag );

  acbo->topShadowColor = acbo->bufTopShadowColor;
  acbo->botShadowColor = acbo->bufBotShadowColor;

  acbo->fgColorMode = acbo->bufFgColorMode;
  if ( acbo->fgColorMode == ACBC_K_COLORMODE_ALARM )
    acbo->fgColor.setAlarmSensitive();
  else
    acbo->fgColor.setAlarmInsensitive();
  acbo->fgColor.setColorIndex( acbo->bufFgColor, acbo->actWin->ci );

  acbo->bgColorMode = acbo->bufBgColorMode;
  if ( acbo->bgColorMode == ACBC_K_COLORMODE_ALARM )
    acbo->bgColor.setAlarmSensitive();
  else
    acbo->bgColor.setAlarmInsensitive();
  acbo->bgColor.setColorIndex( acbo->bufBgColor, acbo->actWin->ci );

  acbo->selColor.setColorIndex( acbo->bufSelColor, acbo->actWin->ci );

  acbo->inconsistentColor.setColorIndex( acbo->bufInconsistentColor,
   acbo->actWin->ci );

  acbo->visPvExpString.setRaw( acbo->bufVisPvName );
  strncpy( acbo->minVisString, acbo->bufMinVisString, 39 );
  strncpy( acbo->maxVisString, acbo->bufMaxVisString, 39 );

  if ( acbo->bufVisInverted )
    acbo->visInverted = 0;
  else
    acbo->visInverted = 1;

  acbo->colorPvExpString.setRaw( acbo->bufColorPvName );

  acbo->x = acbo->bufX;
  acbo->sboxX = acbo->bufX;

  acbo->y = acbo->bufY;
  acbo->sboxY = acbo->bufY;

  acbo->w = acbo->bufW;
  acbo->sboxW = acbo->bufW;

  acbo->h = acbo->bufH;
  acbo->sboxH = acbo->bufH;

  acbo->controlPvExpStr.setRaw( acbo->bufControlPvName );

  acbo->readPvExpStr.setRaw( acbo->bufReadPvName );

  acbo->orientation = acbo->bufOrientation;


  acbo->updateDimensions();

}

static void acbc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) client;

  acbc_edit_update ( w, client, call );
  acbo->refresh( acbo );

}

static void acbc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) client;

  acbc_edit_update ( w, client, call );
  acbo->ef.popdown();
  acbo->operationComplete();

}

static void acbc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) client;

  acbo->ef.popdown();
  acbo->operationCancel();

}

static void acbc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeChoiceButtonClass *acbo = (activeChoiceButtonClass *) client;

  acbo->ef.popdown();
  acbo->operationCancel();
  acbo->erase();
  acbo->deleteRequest = 1;
  acbo->drawAll();

}

activeChoiceButtonClass::activeChoiceButtonClass ( void ) {

int i;

  name = new char[strlen("activeChoiceButtonClass")+1];
  strcpy( name, "activeChoiceButtonClass" );

  numStates = 0;

  for ( i=0; i<MAX_ENUM_STATES; i++ ) {
    stateString[i] = NULL;
  }

  fgColorMode = ACBC_K_COLORMODE_STATIC;
  bgColorMode = ACBC_K_COLORMODE_STATIC;

  active = 0;
  activeMode = 0;

  fontList = NULL;

  connection.setMaxPvs( 4 );

  unconnectedTimer = 0;

  visibility = 0;
  prevVisibility = -1;
  visInverted = 0;
  strcpy( minVisString, "" );
  strcpy( maxVisString, "" );

  orientation = ACBC_K_ORIENTATION_VERT;

  setBlinkFunction( (void *) doBlink );

}

activeChoiceButtonClass::~activeChoiceButtonClass ( void ) {

  if ( name ) delete name;
  if ( fontList ) XmFontListFree( fontList );

  if ( unconnectedTimer ) {
    XtRemoveTimeOut( unconnectedTimer );
    unconnectedTimer = 0;
  }

  updateBlink( 0 );

}

// copy constructor
activeChoiceButtonClass::activeChoiceButtonClass
 ( const activeChoiceButtonClass *source ) {

int i;
activeGraphicClass *acbo = (activeGraphicClass *) this;

  acbo->clone( (activeGraphicClass *) source );

  name = new char[strlen("activeChoiceButtonClass")+1];
  strcpy( name, "activeChoiceButtonClass" );

  numStates = 0;

  for ( i=0; i<MAX_ENUM_STATES; i++ ) {
    stateString[i] = NULL;
  }

  strncpy( fontTag, source->fontTag, 63 );
  fs = actWin->fi->getXFontStruct( fontTag );
  actWin->fi->getTextFontList( fontTag, &fontList );

  fontAscent = source->fontAscent;
  fontDescent = source->fontDescent;
  fontHeight = source->fontHeight;

  topShadowColor = source->topShadowColor;
  botShadowColor = source->botShadowColor;
  topShadowCb = source->topShadowCb;
  botShadowCb = source->botShadowCb;

  fgColor.copy(source->fgColor);
  bgColor.copy(source->bgColor);
  selColor = source->selColor;
  inconsistentColor = source->inconsistentColor;
  fgCb = source->fgCb;
  bgCb = source->bgCb;
  selCb = source->selCb;
  inconsistentCb = source->inconsistentCb;

  fgColorMode = source->fgColorMode;
  bgColorMode = source->bgColorMode;

  controlPvExpStr.copy( source->controlPvExpStr );
  readPvExpStr.copy( source->readPvExpStr );
  visPvExpString.copy( source->visPvExpString );
  colorPvExpString.copy( source->colorPvExpString );

  active = 0;
  activeMode = 0;

  connection.setMaxPvs( 4 );

  unconnectedTimer = 0;

  visibility = 0;
  prevVisibility = -1;
  visInverted = source->visInverted;
  strncpy( minVisString, source->minVisString, 39 );
  strncpy( maxVisString, source->maxVisString, 39 );

  orientation = source->orientation;

  setBlinkFunction( (void *) doBlink );

}

int activeChoiceButtonClass::createInteractive (
  activeWindowClass *aw_obj,
  int _x,
  int _y,
  int _w,
  int _h ) {

  actWin = (activeWindowClass *) aw_obj;
  x = _x;
  y = _y;
  w = _w;
  h = _h;

  strcpy( fontTag, actWin->defaultBtnFontTag );
  actWin->fi->loadFontTag( fontTag );
  fs = actWin->fi->getXFontStruct( fontTag );
  actWin->fi->getTextFontList( fontTag, &fontList );

  updateDimensions();

  topShadowColor = actWin->defaultTopShadowColor;
  botShadowColor = actWin->defaultBotShadowColor;

  fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  bgColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  selColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  inconsistentColor.setColorIndex( actWin->defaultOffsetColor,
   actWin->ci );

  this->draw();

  this->editCreate();

  return 1;

}

int activeChoiceButtonClass::save (
  FILE *f )
{

int index;

  fprintf( f, "%-d %-d %-d\n", ACBC_MAJOR_VERSION, ACBC_MINOR_VERSION,
   ACBC_RELEASE );

  fprintf( f, "%-d\n", x );
  fprintf( f, "%-d\n", y );
  fprintf( f, "%-d\n", w );
  fprintf( f, "%-d\n", h );

  index = fgColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );

  fprintf( f, "%-d\n", fgColorMode );

  index = selColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );

  index = bgColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );

  fprintf( f, "%-d\n", bgColorMode );

  index = topShadowColor;
  actWin->ci->writeColorIndex( f, index );

  index = botShadowColor;
  actWin->ci->writeColorIndex( f, index );

  if ( controlPvExpStr.getRaw() )
    writeStringToFile( f, controlPvExpStr.getRaw() );
  else
    writeStringToFile( f, "" );

  writeStringToFile( f, fontTag );

  if ( readPvExpStr.getRaw() )
    writeStringToFile( f, readPvExpStr.getRaw() );
  else
    writeStringToFile( f, "" );

  index = inconsistentColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );

  if ( visPvExpString.getRaw() )
    writeStringToFile( f, visPvExpString.getRaw() );
  else
    writeStringToFile( f, "" );
  fprintf( f, "%-d\n", visInverted );
  writeStringToFile( f, minVisString );
  writeStringToFile( f, maxVisString );

  if ( colorPvExpString.getRaw() )
    writeStringToFile( f, colorPvExpString.getRaw() );
  else
    writeStringToFile( f, "" );

  fprintf( f, "%-d\n", orientation );

  return 1;

}

int activeChoiceButtonClass::createFromFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int index;
int major, minor, release;
char oneName[activeGraphicClass::MAX_PV_NAME+1];

  this->actWin = _actWin;

  fscanf( f, "%d %d %d\n", &major, &minor, &release ); actWin->incLine();

  if ( major > ACBC_MAJOR_VERSION ) {
    postIncompatable();
    return 0;
  }

  fscanf( f, "%d\n", &x ); actWin->incLine();
  fscanf( f, "%d\n", &y ); actWin->incLine();
  fscanf( f, "%d\n", &w ); actWin->incLine();
  fscanf( f, "%d\n", &h ); actWin->incLine();

  this->initSelectBox(); // call after getting x,y,w,h

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  fgColor.setColorIndex( index, actWin->ci );

  fscanf( f, "%d\n", &fgColorMode ); actWin->incLine();

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  selColor.setColorIndex( index, actWin->ci );

  if ( fgColorMode == ACBC_K_COLORMODE_ALARM )
    fgColor.setAlarmSensitive();
  else
    fgColor.setAlarmInsensitive();

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  bgColor.setColorIndex( index, actWin->ci );

  fscanf( f, "%d\n", &bgColorMode ); actWin->incLine();

  if ( bgColorMode == ACBC_K_COLORMODE_ALARM )
    bgColor.setAlarmSensitive();
  else
    bgColor.setAlarmInsensitive();

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  topShadowColor = index;

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  botShadowColor = index;

  readStringFromFile( oneName, activeGraphicClass::MAX_PV_NAME+1, f );
   actWin->incLine();
  controlPvExpStr.setRaw( oneName );

  readStringFromFile( fontTag, 63+1, f ); actWin->incLine();

  readStringFromFile( oneName, activeGraphicClass::MAX_PV_NAME+1, f );
   actWin->incLine();
  readPvExpStr.setRaw( oneName );

  actWin->ci->readColorIndex( f, &index );
  actWin->incLine(); actWin->incLine();
  inconsistentColor.setColorIndex( index, actWin->ci );

  actWin->fi->loadFontTag( fontTag );
  actWin->drawGc.setFontTag( fontTag, actWin->fi );

  fs = actWin->fi->getXFontStruct( fontTag );
  actWin->fi->getTextFontList( fontTag, &fontList );

  readStringFromFile( oneName, activeGraphicClass::MAX_PV_NAME+1, f );
   actWin->incLine();
  visPvExpString.setRaw( oneName );

  fscanf( f, "%d\n", &visInverted ); actWin->incLine();

  readStringFromFile( minVisString, 39+1, f ); actWin->incLine();

  readStringFromFile( maxVisString, 39+1, f ); actWin->incLine();

  readStringFromFile( oneName, activeGraphicClass::MAX_PV_NAME+1, f );
   actWin->incLine();
  colorPvExpString.setRaw( oneName );

  fscanf( f, "%d\n", &orientation );

  updateDimensions();

  return 1;

}

int activeChoiceButtonClass::genericEdit ( void ) {

char title[32], *ptr;

  ptr = actWin->obj.getNameFromClass( "activeChoiceButtonClass" );
  if ( ptr )
    strncpy( title, ptr, 31 );
  else
    strncpy( title, activeChoiceButtonClass_str2, 31 );

  Strncat( title, activeChoiceButtonClass_str3, 31 );

  bufX = x;
  bufY = y;
  bufW = w;
  bufH = h;

  strncpy( bufFontTag, fontTag, 63 );

  bufTopShadowColor = topShadowColor;
  bufBotShadowColor = botShadowColor;

  bufFgColor = fgColor.pixelIndex();
  bufFgColorMode = fgColorMode;

  bufBgColor = bgColor.pixelIndex();
  bufBgColorMode = bgColorMode;

  bufSelColor = selColor.pixelIndex();

  bufInconsistentColor = inconsistentColor.pixelIndex();

  if ( controlPvExpStr.getRaw() )
    strncpy( bufControlPvName, controlPvExpStr.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
  else
    strcpy( bufControlPvName, "" );

  if ( readPvExpStr.getRaw() )
    strncpy( bufReadPvName, readPvExpStr.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
  else
    strcpy( bufReadPvName, "" );

  if ( visPvExpString.getRaw() )
    strncpy( bufVisPvName, visPvExpString.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
  else
    strcpy( bufVisPvName, "" );

  if ( visInverted )
    bufVisInverted = 0;
  else
    bufVisInverted = 1;

  if ( colorPvExpString.getRaw() )
    strncpy( bufColorPvName, colorPvExpString.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
  else
    strcpy( bufColorPvName, "" );

  strncpy( bufMinVisString, minVisString, 39 );
  strncpy( bufMaxVisString, maxVisString, 39 );

  bufOrientation = orientation;

  ef.create( actWin->top, actWin->appCtx->ci.getColorMap(),
   &actWin->appCtx->entryFormX,
   &actWin->appCtx->entryFormY, &actWin->appCtx->entryFormW,
   &actWin->appCtx->entryFormH, &actWin->appCtx->largestH,
   title, NULL, NULL, NULL );

  ef.addTextField( activeChoiceButtonClass_str4, 35, &bufX );
  ef.addTextField( activeChoiceButtonClass_str5, 35, &bufY );
  ef.addTextField( activeChoiceButtonClass_str6, 35, &bufW );
  ef.addTextField( activeChoiceButtonClass_str7, 35, &bufH );
  ef.addTextField( activeChoiceButtonClass_str17, 35, bufControlPvName,
   activeGraphicClass::MAX_PV_NAME );
  ef.addTextField( activeChoiceButtonClass_str18, 35, bufReadPvName,
   activeGraphicClass::MAX_PV_NAME );

  ef.addOption( activeChoiceButtonClass_str35, activeChoiceButtonClass_str36,
   &bufOrientation );

  ef.addColorButton( activeChoiceButtonClass_str8, actWin->ci, &fgCb,
   &bufFgColor );
  ef.addToggle( activeChoiceButtonClass_str10, &bufFgColorMode );

  ef.addColorButton( activeChoiceButtonClass_str37, actWin->ci,
   &selCb, &bufSelColor );

  ef.addColorButton( activeChoiceButtonClass_str11, actWin->ci, &bgCb,
   &bufBgColor );

  ef.addColorButton( activeChoiceButtonClass_str29, actWin->ci,
   &inconsistentCb, &bufInconsistentColor );

  ef.addColorButton( activeChoiceButtonClass_str14, actWin->ci, &topShadowCb,
   &bufTopShadowColor );

  ef.addColorButton( activeChoiceButtonClass_str15, actWin->ci, &botShadowCb,
   &bufBotShadowColor );

  ef.addFontMenu( activeChoiceButtonClass_str16, actWin->fi, &fm, fontTag );

  XtUnmanageChild( fm.alignWidget() ); // no alignment info

  ef.addTextField( activeChoiceButtonClass_str34, 30, bufColorPvName,
   activeGraphicClass::MAX_PV_NAME );

  ef.addTextField( activeChoiceButtonClass_str30, 30, bufVisPvName,
   activeGraphicClass::MAX_PV_NAME );
  ef.addOption( " ", activeChoiceButtonClass_str31, &bufVisInverted );
  ef.addTextField( activeChoiceButtonClass_str32, 30, bufMinVisString, 39 );
  ef.addTextField( activeChoiceButtonClass_str33, 30, bufMaxVisString, 39 );

  return 1;

}

int activeChoiceButtonClass::editCreate ( void ) {

  this->genericEdit();
  ef.finished( acbc_edit_ok, acbc_edit_apply, acbc_edit_cancel_delete, this );
  actWin->currentEf = NULL;
  ef.popup();

  return 1;

}

int activeChoiceButtonClass::edit ( void ) {

  this->genericEdit();
  ef.finished( acbc_edit_ok, acbc_edit_apply, acbc_edit_cancel, this );
  actWin->currentEf = &ef;
  ef.popup();

  return 1;

}

int activeChoiceButtonClass::erase ( void ) {

  if ( deleteRequest || activeMode ) return 1;

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  return 1;

}

int activeChoiceButtonClass::eraseActive ( void ) {

  if ( !init || !activeMode ) return 1;

  if ( prevVisibility == 0 ) {
    prevVisibility = visibility;
    return 1;
  }

  prevVisibility = visibility;

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  return 1;

}

int activeChoiceButtonClass::draw ( void ) {

int tX, tY;
XRectangle xR = { x, y, w, h };
int blink = 0;
int i, buttonX, buttonY, buttonH, buttonW, margin = 2;
int buttonNumStates = 3;
int buttonSelected = buttonNumStates-1;

char *buttonLabel[3] = { "0", "1", "2" };

  if ( deleteRequest || activeMode ) return 1;

  actWin->drawGc.saveFg();

  if ( orientation == ACBC_K_ORIENTATION_HORZ ) {

    buttonH = h;
    if ( buttonH < 3 ) buttonH = 3;
    buttonW = ( w - (buttonNumStates-1) * margin ) / buttonNumStates;
    if ( buttonW < 3 ) buttonW = 3;

    // background
    actWin->drawGc.setFG( bgColor.pixelIndex(), &blink );

    actWin->drawGc.setLineStyle( LineSolid );
    actWin->drawGc.setLineWidth( 1 );

    XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.normGC(), x, y, w, h );

    actWin->drawGc.setFG( fgColor.pixelIndex(), &blink );

    buttonX = x;
    buttonY = y;

    // buttons
    for ( i=0; i<buttonNumStates; i++ ) {

      if ( buttonSelected == i ) {

        actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY, buttonX+buttonW, buttonY );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX+buttonW, buttonY, buttonX+buttonW,
          buttonY+buttonH );

        actWin->drawGc.setFG( actWin->ci->pix(topShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY, buttonX,
         buttonY+buttonH );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY+buttonH,
         buttonX+buttonW, buttonY+buttonH );

      }
      else {

        actWin->drawGc.setFG( actWin->ci->pix(topShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY, buttonX+buttonW, buttonY );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX+buttonW, buttonY, buttonX+buttonW,
          buttonY+buttonH );

        actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY, buttonX,
         buttonY+buttonH );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY+buttonH,
         buttonX+buttonW, buttonY+buttonH );

      }

      buttonX += buttonW + margin;

    }

    if ( fs ) {

      actWin->drawGc.setFG( fgColor.pixelIndex(), &blink );
      actWin->drawGc.setFontTag( fontTag, actWin->fi );

      buttonX = x;
      buttonY = y;

      // labels
      for ( i=0; i<buttonNumStates; i++ ) {

        xR.x = buttonX + 1;
        xR.y = buttonY + 1;
        xR.width = buttonW - 2;
        xR.height = buttonH - 2;

        actWin->drawGc.addNormXClipRectangle( xR );

        tX = buttonX + buttonW/2;
        tY = buttonY + buttonH/2 - fontAscent/2;

        drawText( actWin->drawWidget, &actWin->drawGc, fs, tX, tY,
         XmALIGNMENT_CENTER, buttonLabel[i] );

        actWin->drawGc.removeNormXClipRectangle();

        buttonX += buttonW + margin;

      }

    }

  }
  else if ( orientation == ACBC_K_ORIENTATION_VERT ) {

    buttonH = ( h - (buttonNumStates-1) * margin ) / buttonNumStates;
    if ( buttonH < 3 ) buttonH = 3;
    buttonW = w;
    if ( buttonW < 3 ) buttonW = 3;

    // background
    actWin->drawGc.setFG( bgColor.pixelIndex(), &blink );

    actWin->drawGc.setLineStyle( LineSolid );
    actWin->drawGc.setLineWidth( 1 );

    XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.normGC(), x, y, w, h );

    actWin->drawGc.setFG( fgColor.pixelIndex(), &blink );

    buttonX = x;
    buttonY = y;

    // buttons
    for ( i=0; i<buttonNumStates; i++ ) {

      if ( buttonSelected == i ) {

        actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY, buttonX+buttonW, buttonY );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX+buttonW, buttonY, buttonX+buttonW,
          buttonY+buttonH );

        actWin->drawGc.setFG( actWin->ci->pix(topShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY, buttonX,
         buttonY+buttonH );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY+buttonH,
         buttonX+buttonW, buttonY+buttonH );

      }
      else {

        actWin->drawGc.setFG( actWin->ci->pix(topShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY, buttonX+buttonW, buttonY );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX+buttonW, buttonY, buttonX+buttonW,
          buttonY+buttonH );

        actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY, buttonX,
         buttonY+buttonH );

        XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
         actWin->drawGc.normGC(), buttonX, buttonY+buttonH,
         buttonX+buttonW, buttonY+buttonH );

      }

      buttonY += buttonH + margin;

    }

    if ( fs ) {

      actWin->drawGc.setFG( fgColor.pixelIndex(), &blink );
      actWin->drawGc.setFontTag( fontTag, actWin->fi );

      buttonX = x;
      buttonY = y;

      // labels
      for ( i=0; i<buttonNumStates; i++ ) {

        xR.x = buttonX + 1;
        xR.y = buttonY + 1;
        xR.width = buttonW - 2;
        xR.height = buttonH - 2;

        actWin->drawGc.addNormXClipRectangle( xR );

        tX = buttonX + buttonW/2;
        tY = buttonY + buttonH/2 - fontAscent/2;

        drawText( actWin->drawWidget, &actWin->drawGc, fs, tX, tY,
         XmALIGNMENT_CENTER, buttonLabel[i] );

        actWin->drawGc.removeNormXClipRectangle();

        buttonY += buttonH + margin;

      }

    }

  }

  actWin->drawGc.restoreFg();

  updateBlink( blink );

  return 1;

}

int activeChoiceButtonClass::drawActive ( void ) {

int tX, tY;
XRectangle xR = { x, y, w, h };
int blink = 0;
int i, buttonX, buttonY, buttonH, buttonW, margin = 2;
int buttonNumStates;
int buttonSelected;
int inconsistent;

  if ( !init ) {
    if ( needToDrawUnconnected ) {
      actWin->executeGc.saveFg();
      actWin->executeGc.setFG( bgColor.getDisconnectedIndex(), &blink );
      actWin->executeGc.setLineWidth( 1 );
      actWin->executeGc.setLineStyle( LineSolid );
      XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
       actWin->executeGc.normGC(), x, y, w, h );
      actWin->executeGc.restoreFg();
      needToEraseUnconnected = 1;
      updateBlink( blink );
    }
  }
  else if ( needToEraseUnconnected ) {
    actWin->executeGc.setLineWidth( 1 );
    actWin->executeGc.setLineStyle( LineSolid );
    XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), x, y, w, h );
    needToEraseUnconnected = 0;
    eraseActive();
    smartDrawAllActive();
  }

  if ( !init || !activeMode || !visibility ) return 1;

  prevVisibility = visibility;

  buttonNumStates = numStates;

  actWin->executeGc.saveFg();
  actWin->executeGc.setLineWidth( 1 );
  actWin->executeGc.setLineStyle( LineSolid );

  inconsistent = 0;

  if ( controlExists && readExists ) {

    if ( ( value != readValue ) || !controlValid || !readValid ) {
      inconsistent = 1;
    }

    buttonSelected = readValue;

  }
  else if ( readExists ) {

    buttonSelected = readValue;

  }
  else if ( controlExists ) {

    buttonSelected = value;

  }
  else {

    inconsistent = 1;
    buttonSelected = -1;
    init = 1;

  }

  if ( orientation == ACBC_K_ORIENTATION_HORZ ) {

    buttonH = h;
    if ( buttonH < 3 ) buttonH = 3;
    buttonW = ( w - (buttonNumStates-1) * margin ) / buttonNumStates;
    if ( buttonW < 3 ) buttonW = 3;

   buttonX = x;
    buttonY = y;

    // buttons
    for ( i=0; i<buttonNumStates; i++ ) {

      if ( buttonSelected == i ) {

        if ( inconsistent ) {

          actWin->executeGc.setFG( inconsistentColor.getIndex(), &blink );

          XFillRectangle( actWin->d, XtWindow(actWin->executeWidget),
           actWin->executeGc.normGC(), buttonX, buttonY, buttonW, buttonH );

	}
	else {

          actWin->executeGc.setFG( selColor.getIndex(), &blink );

          XFillRectangle( actWin->d, XtWindow(actWin->executeWidget),
           actWin->executeGc.normGC(), buttonX, buttonY, buttonW, buttonH );

	}

        actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonX+buttonW, buttonY );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX+buttonW, buttonY, buttonX+buttonW,
          buttonY+buttonH );

        actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonX,
         buttonY+buttonH );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY+buttonH,
         buttonX+buttonW, buttonY+buttonH );

      }
      else {

        actWin->executeGc.setFG( bgColor.getIndex(), &blink );

        XFillRectangle( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonW, buttonH );

        actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonX+buttonW, buttonY );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX+buttonW, buttonY, buttonX+buttonW,
          buttonY+buttonH );

        actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonX,
         buttonY+buttonH );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY+buttonH,
         buttonX+buttonW, buttonY+buttonH );

      }

      buttonX += buttonW + margin;

    }

    if ( fs ) {

      actWin->executeGc.setFontTag( fontTag, actWin->fi );

      buttonX = x;
      buttonY = y;

      // labels
      for ( i=0; i<buttonNumStates; i++ ) {

        if ( buttonSelected == i ) {
          actWin->executeGc.setFG( fgColor.getIndex(), &blink );
	}
	else {
          actWin->executeGc.setFG( fgColor.pixelIndex(), &blink );
	}

        xR.x = buttonX + 1;
        xR.y = buttonY + 1;
        xR.width = buttonW - 2;
        xR.height = buttonH - 2;

        actWin->executeGc.addNormXClipRectangle( xR );

        tX = buttonX + buttonW/2;
        tY = buttonY + buttonH/2 - fontAscent/2;

        drawText( actWin->executeWidget, &actWin->executeGc, fs, tX, tY,
         XmALIGNMENT_CENTER, stateString[i] );

        actWin->executeGc.removeNormXClipRectangle();

        buttonX += buttonW + margin;

      }

    }

  }
  else if ( orientation == ACBC_K_ORIENTATION_VERT ) {

    buttonH = ( h - (buttonNumStates-1) * margin ) / buttonNumStates;
    if ( buttonH < 3 ) buttonH = 3;
    buttonW = w;
    if ( buttonW < 3 ) buttonW = 3;

    buttonX = x;
    buttonY = y;

    // buttons
    for ( i=0; i<buttonNumStates; i++ ) {

      if ( buttonSelected == i ) {

        if ( inconsistent ) {

          actWin->executeGc.setFG( inconsistentColor.getIndex(), &blink );

          XFillRectangle( actWin->d, XtWindow(actWin->executeWidget),
           actWin->executeGc.normGC(), buttonX, buttonY, buttonW, buttonH );

	}
	else {

          actWin->executeGc.setFG( selColor.getIndex(), &blink );

          XFillRectangle( actWin->d, XtWindow(actWin->executeWidget),
           actWin->executeGc.normGC(), buttonX, buttonY, buttonW, buttonH );

	}

        actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonX+buttonW, buttonY );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX+buttonW, buttonY, buttonX+buttonW,
          buttonY+buttonH );

        actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonX,
         buttonY+buttonH );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY+buttonH,
         buttonX+buttonW, buttonY+buttonH );

      }
      else {

        actWin->executeGc.setFG( bgColor.getIndex(), &blink );

        XFillRectangle( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonW, buttonH );

        actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonX+buttonW, buttonY );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX+buttonW, buttonY, buttonX+buttonW,
          buttonY+buttonH );

        actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY, buttonX,
         buttonY+buttonH );

        XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
         actWin->executeGc.normGC(), buttonX, buttonY+buttonH,
         buttonX+buttonW, buttonY+buttonH );

      }

      buttonY += buttonH + margin;

    }

    if ( fs ) {

      actWin->executeGc.setFontTag( fontTag, actWin->fi );

      buttonX = x;
      buttonY = y;

      // labels
      for ( i=0; i<buttonNumStates; i++ ) {

        if ( buttonSelected == i ) {
          actWin->executeGc.setFG( fgColor.getIndex(), &blink );
	}
	else {
          actWin->executeGc.setFG( fgColor.pixelIndex(), &blink );
	}

        xR.x = buttonX + 1;
        xR.y = buttonY + 1;
        xR.width = buttonW - 2;
        xR.height = buttonH - 2;

        actWin->executeGc.addNormXClipRectangle( xR );

        tX = buttonX + buttonW/2;
        tY = buttonY + buttonH/2 - fontAscent/2;

        drawText( actWin->executeWidget, &actWin->executeGc, fs, tX, tY,
         XmALIGNMENT_CENTER, stateString[i] );

        actWin->executeGc.removeNormXClipRectangle();

        buttonY += buttonH + margin;

      }

    }

  }

  actWin->executeGc.restoreFg();

  updateBlink( blink );

  return 1;

}

int activeChoiceButtonClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat, retStat = 1;;

  stat = controlPvExpStr.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = readPvExpStr.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = visPvExpString.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = colorPvExpString.expand1st( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;

  return retStat;

}

int activeChoiceButtonClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat, retStat = 1;

  stat = controlPvExpStr.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = readPvExpStr.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = visPvExpString.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;
  stat = colorPvExpString.expand2nd( numMacros, macros, expansions );
  if ( !( stat & 1 ) ) retStat = stat;

  return retStat;

}

int activeChoiceButtonClass::containsMacros ( void ) {

  if ( controlPvExpStr.containsPrimaryMacros() ) return 1;

  if ( readPvExpStr.containsPrimaryMacros() ) return 1;

  if ( visPvExpString.containsPrimaryMacros() ) return 1;

  if ( colorPvExpString.containsPrimaryMacros() ) return 1;

  return 0;

}

int activeChoiceButtonClass::activate (
  int pass,
  void *ptr
) {

int stat, opStat;

  switch ( pass ) {

  case 1:

    opComplete = 0;

    break;

  case 2:

    if ( !opComplete ) {

      aglPtr = ptr;
      needConnectInit = needInfoInit = needReadConnectInit = needReadInfoInit =
       needRefresh = needDraw = needVisConnectInit = needVisInit =
       needVisUpdate = needColorConnectInit =
       needColorInit = needColorUpdate = 0;
      needToEraseUnconnected = 0;
      needToDrawUnconnected = 0;
      unconnectedTimer = 0;
      controlValid = readValid = 0;
      controlPvId = readPvId = visPvId = colorPvId = NULL;

      controlExists = readExists = visExists = colorExists = 0;

      pvCheckExists = 0;
      connection.init();

#ifdef __epics__
      alarmEventId = controlEventId = readAlarmEventId = readEventId =
       visEventId = colorEventId = 0;
#endif

      init = 0;
      active = 0;
      activeMode = 1;
      numStates = 0;

      buttonPressed = 0;

      if ( !unconnectedTimer ) {
        unconnectedTimer = appAddTimeOut( actWin->appCtx->appContext(),
         2000, unconnectedTimeout, this );
      }

      if ( !pvCheckExists ) {

        pvCheckExists = 1;

        if ( strcmp( controlPvExpStr.getRaw(), "" ) != 0 ) {
          controlExists = 1;
          connection.addPv(); // must do this only once per pv
	}
        else {
          controlExists = 0;
	}

        if ( strcmp( readPvExpStr.getRaw(), "" ) != 0 ) {
          readExists = 1;
          connection.addPv(); // must do this only once per pv
	}
        else {
          readExists = 0;
	}

        if ( strcmp( visPvExpString.getRaw(), "" ) != 0 ) {
          visExists = 1;
          connection.addPv(); // must do this only once per pv
        }
        else {
          visExists = 0;
          visibility = 1;
        }

        if ( strcmp( colorPvExpString.getRaw(), "" ) != 0 ) {
          colorExists = 1;
          connection.addPv(); // must do this only once per pv
        }
        else {
          colorExists = 0;
        }
      }

      opStat = 1;

#ifdef __epics__

      if ( controlExists ) {
        stat = ca_search_and_connect( controlPvExpStr.getExpanded(),
         &controlPvId, acb_monitor_control_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeChoiceButtonClass_str20,
           controlPvExpStr.getExpanded() );
          opStat = 0;
        }
      }

      if ( readExists ) {
        stat = ca_search_and_connect( readPvExpStr.getExpanded(),
         &readPvId, acb_monitor_read_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeChoiceButtonClass_str21,
           readPvExpStr.getExpanded() );
          opStat = 0;
        }
      }

      if ( visExists ) {

        stat = ca_search_and_connect( visPvExpString.getExpanded(), &visPvId,
         acb_monitor_vis_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeChoiceButtonClass_str21,
           visPvExpString.getExpanded() );
          opStat = 0;
        }
      }

      if ( colorExists ) {

        stat = ca_search_and_connect( colorPvExpString.getExpanded(),
         &colorPvId, acb_monitor_color_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeChoiceButtonClass_str20,
           colorPvExpString.getExpanded() );
          opStat = 0;
        }
      }

      opComplete = opStat;

#endif

      return opStat;

    }

    break;

  case 3:
  case 4:
  case 5:
  case 6:

    break;

  }

  return 1;

}

int activeChoiceButtonClass::deactivate (
  int pass
) {

int stat, i;

  active = 0;
  activeMode = 0;

  switch ( pass ) {

  case 1:

    if ( unconnectedTimer ) {
      XtRemoveTimeOut( unconnectedTimer );
      unconnectedTimer = 0;
    }

    updateBlink( 0 );

#ifdef __epics__

    if ( controlExists ) {
      if ( controlPvId ) {
        stat = ca_clear_channel( controlPvId );
        if ( stat != ECA_NORMAL ) printf( activeChoiceButtonClass_str23 );
        controlPvId = NULL;
      }
    }

    if ( readExists ) {
      if ( readPvId ) {
        stat = ca_clear_channel( readPvId );
        if ( stat != ECA_NORMAL ) printf( activeChoiceButtonClass_str23 );
        readPvId = NULL;
      }
    }

    if ( visExists ) {
      if ( visPvId ) {
        stat = ca_clear_channel( visPvId );
        if ( stat != ECA_NORMAL ) printf( activeChoiceButtonClass_str23 );
        visPvId = NULL;
      }
    }

    if ( colorExists ) {
      if ( colorPvId ) {
        stat = ca_clear_channel( colorPvId );
        if ( stat != ECA_NORMAL ) printf( activeChoiceButtonClass_str22 );
        colorPvId = NULL;
      }
    }

#endif

    break;

  case 2:

    for ( i=0; i<numStates; i++ ) {
      if ( stateString[i] ) {
        delete stateString[i];
        stateString[i] = NULL;
      }
    }

    break;

  }

  return 1;

}

void activeChoiceButtonClass::updateDimensions ( void )
{

  if ( fs ) {
    fontAscent = fs->ascent;
    fontDescent = fs->descent;
    fontHeight = fontAscent + fontDescent;
  }
  else {
    fontAscent = 10;
    fontDescent = 5;
    fontHeight = fontAscent + fontDescent;
  }

}

void activeChoiceButtonClass::btnUp (
  int _x,
  int _y,
  int buttonState,
  int buttonNumber,
  int *action )
{

  *action = 0;

}

void activeChoiceButtonClass::btnDown (
  int _x,
  int _y,
  int buttonState,
  int buttonNumber,
  int *action )
{

short value;
int stat, i, state, buttonX, buttonY, buttonH, buttonW, margin = 2;

  *action = 0;

  if ( !init || !visibility ) return;

  if ( !controlExists ) return;

  if ( !ca_write_access( controlPvId ) ) return;

  if ( buttonNumber == 1 ) {

    state = -1;

    if ( orientation == ACBC_K_ORIENTATION_HORZ ) {

      buttonH = h;
      if ( buttonH < 3 ) buttonH = 3;
      buttonW = ( w - (numStates-1) * margin ) / numStates;
      if ( buttonW < 3 ) buttonW = 3;

      buttonX = x;
      buttonY = y;

      // buttons
      for ( i=0; i<numStates; i++ ) {

        buttonX += buttonW + margin;

        if ( buttonX > _x ) {
          state = i;
          break;
	}

      }

    }
    else if ( orientation == ACBC_K_ORIENTATION_VERT ) {

    buttonH = ( h - (numStates-1) * margin ) / numStates;
    if ( buttonH < 3 ) buttonH = 3;
    buttonW = w;
    if ( buttonW < 3 ) buttonW = 3;

      buttonX = x;
      buttonY = y;

      // buttons
      for ( i=0; i<numStates; i++ ) {

        buttonY += buttonH + margin;

        if ( buttonY > _y ) {
          state = i;
          break;
	}

      }

    }

    if ( ( state >= 0 ) && ( state < numStates ) ) {
      value = (short) state;
      stat = ca_put( DBR_ENUM, controlPvId, &value );
    }

  }

}

void activeChoiceButtonClass::pointerIn (
  int _x,
  int _y,
  int buttonState )
{

  if ( !init || !visibility ) return;

  if ( !ca_write_access( controlPvId ) ) {
    actWin->cursor.set( XtWindow(actWin->executeWidget), CURSOR_K_NO );
  }
  else {
    actWin->cursor.set( XtWindow(actWin->executeWidget), CURSOR_K_DEFAULT );
  }

  activeGraphicClass::pointerIn( _x, _y, buttonState );

}

int activeChoiceButtonClass::getButtonActionRequest (
  int *up,
  int *down,
  int *drag,
  int *focus )
{

  *drag = 0;

  if ( controlExists )
    *focus = 1;
  else
    *focus = 0;

  if ( !controlExists ) {
    *up = 0;
    *down = 0;
    return 1;
  }

  *down = 1;
  *up = 1;

  return 1;

}

void activeChoiceButtonClass::executeDeferred ( void ) {

short v, rV;
int nc, nrc, ni, nri, nr, nd, nvc, nvi, nvu, ncolc, ncoli, ncolu;
int stat, index, invisColor;

  if ( actWin->isIconified ) return;

//----------------------------------------------------------------------------

  actWin->appCtx->proc->lock();
  nc = needConnectInit; needConnectInit = 0;
  nrc = needReadConnectInit; needReadConnectInit = 0;
  ni = needInfoInit; needInfoInit = 0;
  nri = needReadInfoInit; needReadInfoInit = 0;
  nr = needRefresh; needRefresh = 0;
  nd = needDraw; needDraw = 0;
  nvc = needVisConnectInit; needVisConnectInit = 0;
  nvi = needVisInit; needVisInit = 0;
  nvu = needVisUpdate; needVisUpdate = 0;
  ncolc = needColorConnectInit; needColorConnectInit = 0;
  ncoli = needColorInit; needColorInit = 0;
  ncolu = needColorUpdate; needColorUpdate = 0;
  v = curValue;
  rV = curReadValue;
  visValue = curVisValue;
  colorValue = curColorValue;
  actWin->remDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

  if ( !activeMode ) return;

//----------------------------------------------------------------------------

#ifdef __epics__

  if ( nc ) {

    stat = ca_get_callback( DBR_GR_ENUM, controlPvId,
     acb_infoUpdate, (void *) this );

  }

  if ( nrc ) {

    stat = ca_get_callback( DBR_GR_ENUM, readPvId,
     acb_readInfoUpdate, (void *) this );

  }

  if ( ni ) {

    value = v;

    if ( !controlEventId ) {

      stat = ca_add_masked_array_event( DBR_ENUM, 1, controlPvId,
       acb_controlUpdate, (void *) this, (float) 0.0, (float) 0.0,
       (float) 0.0, &controlEventId, DBE_VALUE );
      if ( stat != ECA_NORMAL ) {
        printf( activeChoiceButtonClass_str24 );
      }

    }

    if ( !alarmEventId ) {

      stat = ca_add_masked_array_event( DBR_STS_ENUM, 1, controlPvId,
       acb_alarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
       (float) 0.0, &alarmEventId, DBE_ALARM );
      if ( stat != ECA_NORMAL ) {
        printf( activeChoiceButtonClass_str25 );
      }

    }

    if ( connection.pvsConnected() ) {
      init = 1;
      active = 1;
      drawActive();
    }

  }

  if ( nri ) {

    curReadValue = rV;

    if ( !readEventId ) {

      stat = ca_add_masked_array_event( DBR_ENUM, 1, readPvId,
       acb_readUpdate, (void *) this, (float) 0.0, (float) 0.0,
       (float) 0.0, &readEventId, DBE_VALUE );
      if ( stat != ECA_NORMAL ) {
        printf( activeChoiceButtonClass_str26 );
      }

    }

    if ( !readAlarmEventId ) {

      stat = ca_add_masked_array_event( DBR_STS_ENUM, 1, readPvId,
       acb_readAlarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
       (float) 0.0, &readAlarmEventId, DBE_ALARM );
      if ( stat != ECA_NORMAL ) {
        printf( activeChoiceButtonClass_str27 );
      }

    }

    if ( connection.pvsConnected() ) {
      init = 1;
      active = 1;
      drawActive();
    }

  }

  if ( nvc ) {

    minVis = atof( minVisString );
    maxVis = atof( maxVisString );

    connection.setPvConnected( (void *) visPvConnection );

    stat = ca_get_callback( DBR_GR_DOUBLE, visPvId,
     acb_visInfoUpdate, (void *) this );

  }

  if ( nvi ) {

    stat = ca_add_masked_array_event( DBR_DOUBLE, 1, visPvId,
     acb_visUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
     &visEventId, DBE_VALUE );
    if ( stat != ECA_NORMAL ) printf( activeChoiceButtonClass_str27 );

    if ( ( visValue >= minVis ) &&
         ( visValue < maxVis ) )
      visibility = 1 ^ visInverted;
    else
      visibility = 0 ^ visInverted;

    if ( prevVisibility != visibility ) {
      if ( !visibility ) eraseActive();
    }

    if ( connection.pvsConnected() ) {
      active = 1;
      init = 1;
      fgColor.setConnected();
      smartDrawAllActive();
    }

  }

  if ( ncolc ) {

    stat = ca_get_callback( DBR_GR_DOUBLE, colorPvId,
     acb_colorInfoUpdate, (void *) this );

  }

  if ( ncoli ) {

    stat = ca_add_masked_array_event( DBR_DOUBLE, 1, colorPvId,
     acb_colorUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
     &colorEventId, DBE_VALUE );
    if ( stat != ECA_NORMAL ) printf( activeChoiceButtonClass_str25 );

    invisColor = 0;

    index = actWin->ci->evalRule( bgColor.pixelIndex(), colorValue );
    invisColor |= actWin->ci->isInvisible( index );
    bgColor.changeIndex( index, actWin->ci );

    index = actWin->ci->evalRule( fgColor.pixelIndex(), colorValue );
    invisColor |= actWin->ci->isInvisible( index );
    fgColor.changeIndex( index, actWin->ci );

    if ( !visExists ) {

      if ( invisColor ) {
        visibility = 0;
      }
      else {
        visibility = 1;
      }

      if ( prevVisibility != visibility ) {
        if ( !visibility ) eraseActive();
      }

    }

    connection.setPvConnected( (void *) colorPvConnection );

    if ( connection.pvsConnected() ) {
      active = 1;
      init = 1;
      fgColor.setConnected();
      smartDrawAllActive();
    }

  }

#endif

//----------------------------------------------------------------------------

  if ( nr ) {
    readValue = rV;
    value = v;
    eraseActive();
    smartDrawAllActive();
  }

//----------------------------------------------------------------------------

  if ( nd ) {
    smartDrawAllActive();
  }

//----------------------------------------------------------------------------

  if ( nvu ) {

    if ( ( visValue >= minVis ) &&
         ( visValue < maxVis ) )
      visibility = 1 ^ visInverted;
    else
      visibility = 0 ^ visInverted;

    if ( prevVisibility != visibility ) {
      if ( !visibility ) eraseActive();
      stat = smartDrawAllActive();
    }

  }

//----------------------------------------------------------------------------

  if ( ncolu ) {

    invisColor = 0;

    index = actWin->ci->evalRule( bgColor.pixelIndex(), colorValue );
    invisColor |= actWin->ci->isInvisible( index );
    bgColor.changeIndex( index, actWin->ci );

    index = actWin->ci->evalRule( fgColor.pixelIndex(), colorValue );
    invisColor |= actWin->ci->isInvisible( index );
    fgColor.changeIndex( index, actWin->ci );

    if ( !visExists ) {

      if ( invisColor ) {
        visibility = 0;
      }
      else {
        visibility = 1;
      }

      if ( prevVisibility != visibility ) {
        if ( !visibility ) eraseActive();
      }

    }

    stat = smartDrawAllActive();

  }

}

char *activeChoiceButtonClass::firstDragName ( void ) {

  dragIndex = 0;
  return dragName[dragIndex];

}

char *activeChoiceButtonClass::nextDragName ( void ) {

  if ( dragIndex < (int) ( sizeof(dragName) / sizeof(char *) ) - 1 ) {
    dragIndex++;
    return dragName[dragIndex];
  }
  else {
    return NULL;
  }

}

char *activeChoiceButtonClass::dragValue (
  int i ) {

  if ( i == 0 ) {
    return controlPvExpStr.getExpanded();
  }
  else if ( i == 1 ) {
    return readPvExpStr.getExpanded();
  }
  else if ( i == 2 ) {
    return colorPvExpString.getExpanded();
  }
  else {
    return visPvExpString.getExpanded();
  }

}

void activeChoiceButtonClass::changeDisplayParams (
  unsigned int _flag,
  char *_fontTag,
  int _alignment,
  char *_ctlFontTag,
  int _ctlAlignment,
  char *_btnFontTag,
  int _btnAlignment,
  int _textFgColor,
  int _fg1Color,
  int _fg2Color,
  int _offsetColor,
  int _bgColor,
  int _topShadowColor,
  int _botShadowColor )
{

  if ( _flag & ACTGRF_TEXTFGCOLOR_MASK )
    fgColor.setColorIndex( _textFgColor, actWin->ci );

  if ( _flag & ACTGRF_BGCOLOR_MASK )
    bgColor.setColorIndex( _bgColor, actWin->ci );

  if ( _flag & ACTGRF_TOPSHADOWCOLOR_MASK )
    topShadowColor = _topShadowColor;

  if ( _flag & ACTGRF_BOTSHADOWCOLOR_MASK )
    botShadowColor = _botShadowColor;

  if ( _flag & ACTGRF_BTNFONTTAG_MASK ) {

    strcpy( fontTag, _btnFontTag );
    actWin->fi->loadFontTag( fontTag );
    fs = actWin->fi->getXFontStruct( fontTag );
    actWin->fi->getTextFontList( fontTag, &fontList );

    updateDimensions();

  }

}

void activeChoiceButtonClass::changePvNames (
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
  char *alarmPvs[] )
{

  if ( flag & ACTGRF_CTLPVS_MASK ) {
    if ( numCtlPvs ) {
      controlPvExpStr.setRaw( ctlPvs[0] );
    }
  }

  if ( flag & ACTGRF_VISPVS_MASK ) {
    if ( numVisPvs ) {
      visPvExpString.setRaw( ctlPvs[0] );
    }
  }

}

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeChoiceButtonClassPtr ( void ) {

activeChoiceButtonClass *ptr;

  ptr = new activeChoiceButtonClass;
  return (void *) ptr;

}

void *clone_activeChoiceButtonClassPtr (
  void *_srcPtr )
{

activeChoiceButtonClass *ptr, *srcPtr;

  srcPtr = (activeChoiceButtonClass *) _srcPtr;

  ptr = new activeChoiceButtonClass( srcPtr );

  return (void *) ptr;

}

#ifdef __cplusplus
}
#endif