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

#include "app_pkg.h"

#include "thread.h"
#include "clipbd.h"
#include "edm.version"
#include <unistd.h>

typedef struct libRecTag {
  struct libRecTag *flink;
  char *className;
  char *fileName;
  char *typeName;
  char *text;
} libRecType, *libRecPtr;

static int g_needXtInit = 1;

static int compare_nodes (
  void *node1,
  void *node2
) {

schemeListPtr p1, p2;

  p1 = (schemeListPtr) node1;
  p2 = (schemeListPtr) node2;

  return strcmp( p1->objName, p2->objName );

}

static int compare_key (
  void *key,
  void *node
) {

schemeListPtr p;
char *oneIndex;

  p = (schemeListPtr) node;
  oneIndex = (char *) key;

  return strcmp( oneIndex, p->objName );

}

static int copy_nodes (
  void *node1,
  void *node2
) {

schemeListPtr p1, p2;

  p1 = (schemeListPtr) node1;
  p2 = (schemeListPtr) node2;

  *p1 = *p2;

  // give p1 a copy of the object name
  if ( p2->objName ) {
    p1->objName = new char[ strlen(p2->objName) + 1 ];
    strcpy( p1->objName, p2->objName );
  }

  // give p1 a copy of the file name
  if ( p2->fileName ) {
    p1->fileName = new char[ strlen(p2->fileName) + 1 ];
    strcpy( p1->fileName, p2->fileName );
  }

  return 1;

}

static void manageComponents (
  char *op,
  char *libFile )
{

typedef int (*REGFUNC)( char **, char **, char ** );
REGFUNC func;

int stat, index, comment, fileExists, fileEmpty, doAdd, alreadyExists;
char *classNamePtr, *typeNamePtr, *textPtr, *error;
void *dllHandle;
char fileName[255+1], prefix[255+1], line[255+1], buf[255+1];
int numComponents = 0;
int numToAdd = 0;
int numToRemove = 0;
char *envPtr, *tk, *more;
FILE *f;
libRecPtr head, tail, cur, prev, next;

  head = new libRecType;
  tail = head;
  tail->flink = NULL;

  envPtr = getenv(environment_str3);
  if ( envPtr ) {
    strncpy( prefix, envPtr, 255 );
    if ( prefix[strlen(prefix)-1] != '/' ) Strncat( prefix, "/", 255 );
  }
  else {
    strcpy( prefix, "/etc/edm/" );
  }

  strncpy( fileName, prefix, 255 );
  Strncat( fileName, "edmObjects", 255 );

  f = fopen( fileName, "r" );
  if ( f ) {

    printf( appContextClass_str1, fileName );

    fileExists = 1;
    fileEmpty = 0;

    // read in existing components

    more = fgets( line, 255, f );
    if ( more ) {
      tk = strtok( line, "\n" );
      numComponents = atol( tk );
      if ( numComponents <= 0 ) {
        printf( appContextClass_str2, fileName );
        fileEmpty = 1;
        fclose( f );
      }
    }
    else {
      printf( appContextClass_str2, fileName );
      fileEmpty = 1;
      fclose( f );
    }

    if ( !fileEmpty ) {

      index = 0;
      do {

        more = fgets( line, 255, f );
        if ( more ) {

          cur = new libRecType;

          strncpy( buf, line, 255 );

          comment = 0;
          tk = strtok( buf, " \t\n" );

          if ( !tk ) {
            comment = 1;
          }
          else if ( tk[0] == '#' ) {
            comment = 1;
          }

          if ( !comment ) { /* not an empty line or comment */

            tail->flink = cur;
            tail = cur;
            tail->flink = NULL;

            tk = strtok( line, " \t\n" );
            if ( !tk ) {
              printf( appContextClass_str3 );
              return;
            }
            cur->className = new char[strlen(tk)+1];
            strcpy( cur->className, tk );

            tk = strtok( NULL, " \t\n" );
            if ( !tk ) {
              printf( appContextClass_str3 );
              return;
            }
            cur->fileName = new char[strlen(tk)+1];
            strcpy( cur->fileName, tk );

            tk = strtok( NULL, " \t\n" );
            if ( !tk ) {
              printf( appContextClass_str3 );
              return;
            }
            cur->typeName = new char[strlen(tk)+1];
            strcpy( cur->typeName, tk );

            tk = strtok( NULL, "\n" );
            if ( !tk ) {
              printf( appContextClass_str3 );
              return;
            }
            cur->text = new char[strlen(tk)+1];
            strcpy( cur->text, tk );

            index++;

          }

        }

      } while ( more );

      fclose( f );

      if ( index != numComponents ) {
        printf( appContextClass_str4, fileName );
        return;
      }

    }
    else {

      fileExists = 0; // file was empty so behave as if file does not exist

    }

  }
  else {
    fileExists = 0;
  }

  if ( libFile[0] != '/' ) {
    printf( appContextClass_str5 );
    printf( appContextClass_str6 );
    return;
  }

  dllHandle = dlopen( libFile, RTLD_LAZY );
  if ((error = dlerror()) != NULL)  {
    fputs(error, stderr);
    fputs( "\n", stderr );
    return;
  }

  if ( strcmp( op, global_str8 ) == 0 ) {  // show

    printf( "\n" );

    strcpy( line, "firstRegRecord" );
    func = (REGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str8, stderr );
      return;
    }

    stat = (*func)( &classNamePtr, &typeNamePtr, &textPtr );

    strcpy( line, "nextRegRecord" );
    func = (REGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str9, stderr );
      return;
    }

    if ( !stat ) {
      strncpy( line, appContextClass_str10, 255 );
      Strncat( line, appContextClass_str11, 255 );
      printf( "%s\n\n", line );
    }

    while ( !stat ) {

      strncpy( line, classNamePtr, 255 );

      if ( strlen(line) < 37 )
        index = 42;
      else
        index = strlen(line) + 5;
      Strncat( line, "                                        ", 255 );
      strncpy( &line[index], typeNamePtr, 255 );

      if ( strlen(line) < 50 )
        index = 55;
      else
        index = strlen(line) + 5;
      Strncat( line, "                                        ", 255 );
      strncpy( &line[index], textPtr, 255 );

      printf( "%s\n", line );

      stat = (*func)( &classNamePtr, &typeNamePtr, &textPtr );

    }

    printf( "\n\n" );

  }
  else if ( strcmp( op, global_str6 ) == 0 ) {  // add

    if ( !fileExists ) {
      printf( appContextClass_str13, fileName );
    }

    printf( "\n" );

    strcpy( line, "firstRegRecord" );
    func = (REGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str14, stderr );
      return;
    }

    stat = (*func)( &classNamePtr, &typeNamePtr, &textPtr );

    strcpy( line, "nextRegRecord" );
    func = (REGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str9, stderr );
      return;
    }

    numToAdd = 0;
    alreadyExists = 0;
    while ( !stat ) {

      doAdd = 1;

      cur = head->flink;
      while ( cur ) {
        if ( strcmp( classNamePtr, cur->className ) == 0 ) {
          printf( appContextClass_str15, classNamePtr );
          doAdd = 0;
          alreadyExists = 1;
          break;
	}
        cur = cur->flink;
      }

      if ( doAdd ) {

        numToAdd++;

        printf( appContextClass_str16,
         classNamePtr, typeNamePtr, textPtr );

        cur = new libRecType;
        tail->flink = cur;
        tail = cur;
        tail->flink = NULL;

        cur->className = new char[strlen(classNamePtr)+1];
        strcpy( cur->className, classNamePtr );
        cur->fileName = new char[strlen(libFile)+1];
        strcpy( cur->fileName, libFile );
        cur->typeName = new char[strlen(typeNamePtr)+1];
        strcpy( cur->typeName, typeNamePtr );
        cur->text = new char[strlen(textPtr)+1];
        strcpy( cur->text, textPtr );

        numComponents++;

      }

      stat = (*func)( &classNamePtr, &typeNamePtr, &textPtr );

    }

    if ( numToAdd == 0 ) {
      printf( "\n" );
      if ( alreadyExists ) {
        printf( appContextClass_str122, fileName );
      }
      else {
        printf(
         appContextClass_str17, fileName );
      }
      return;
    }

    printf( "\n" );

    strncpy( line, fileName, 255 );
    Strncat( line, "~", 255 );

    if ( fileExists ) {

      stat = unlink( line );

      printf( appContextClass_str18, line );
      stat = rename( fileName, line );
      if ( stat ) {
        perror( appContextClass_str19 );
        return;
      }

    }

    f = fopen( fileName, "w" );
    if ( f ) {

      fprintf( f, "%-d\n", numComponents );
      cur = head->flink;
      while ( cur ) {
        fprintf( f, "%s %s %s %s\n", cur->className, cur->fileName,
         cur->typeName, cur->text );
        cur = cur->flink;
      }

    }
    else {
      perror( fileName );
      return;
    }

    if ( numToAdd == 1 )
      printf( appContextClass_str20 );
    else if ( numToAdd > 1 )
      printf( appContextClass_str21 );

  }
  else if ( strcmp( op, global_str7 ) == 0 ) {  // remove

    if ( !fileExists ) {
      printf( appContextClass_str123 );
      return;
    }

    printf( "\n" );

    strcpy( line, "firstRegRecord" );
    func = (REGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str14, stderr );
      return;
    }

    stat = (*func)( &classNamePtr, &typeNamePtr, &textPtr );

    strcpy( line, "nextRegRecord" );
    func = (REGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str9, stderr );
      return;
    }

    numToRemove = 0;
    while ( !stat ) {

      cur = head->flink;
      prev = head;
      while ( cur ) {
        next = cur->flink;
        if ( strcmp( classNamePtr, cur->className ) == 0 ) {
          printf( appContextClass_str124, classNamePtr );
          numToRemove++;
          prev->flink = next;
          delete cur;
          break;
	}
        else {
          prev = cur;
	}
        cur = next;
      }

      stat = (*func)( &classNamePtr, &typeNamePtr, &textPtr );

    }

    if ( numToRemove == 0 ) {
      printf( appContextClass_str125, fileName );
      return;
    }

    // count remaining components
    numComponents = 0;
    cur = head->flink;
    while ( cur ) {
      numComponents++;
      cur = cur->flink;
    }

    printf( "\n" );

    strncpy( line, fileName, 255 );
    Strncat( line, "~", 255 );

    if ( fileExists ) {

      stat = unlink( line );

      printf( appContextClass_str18, line );
      stat = rename( fileName, line );
      if ( stat ) {
        perror( appContextClass_str19 );
        return;
      }

    }

    f = fopen( fileName, "w" );
    if ( f ) {

      fprintf( f, "%-d\n", numComponents );
      cur = head->flink;
      while ( cur ) {
        fprintf( f, "%s %s %s %s\n", cur->className, cur->fileName,
         cur->typeName, cur->text );
        cur = cur->flink;
      }

    }
    else {
      perror( fileName );
      return;
    }

    if ( numToRemove == 1 )
      printf( appContextClass_str126 );
    else if ( numToRemove > 1 )
      printf( appContextClass_str127 );

    if ( numComponents == 0 ) {
      printf( appContextClass_str128 );
    }

  }

}

static void managePvComponents (
  char *op,
  char *libFile )
{

typedef int (*PVREGFUNC)( char **, char ** );
PVREGFUNC func;

int stat, index, comment, fileExists, fileEmpty, doAdd, alreadyExists;
char *classNamePtr, *textPtr, *error;
void *dllHandle;
char fileName[255+1], prefix[255+1], line[255+1], buf[255+1];
int numComponents = 0;
int numToAdd = 0;
int numToRemove = 0;
char *envPtr, *tk, *more;
FILE *f;
libRecPtr head, tail, cur, prev, next;

  head = new libRecType;
  tail = head;
  tail->flink = NULL;

  envPtr = getenv(environment_str3);
  if ( envPtr ) {
    strncpy( prefix, envPtr, 255 );
    if ( prefix[strlen(prefix)-1] != '/' ) Strncat( prefix, "/", 255 );
  }
  else {
    strcpy( prefix, "/etc/edm/" );
  }

  strncpy( fileName, prefix, 255 );
  Strncat( fileName, "edmPvObjects", 255 );

  f = fopen( fileName, "r" );
  if ( f ) {

    printf( appContextClass_str1, fileName );

    fileExists = 1;
    fileEmpty = 0;

    // read in existing components

    more = fgets( line, 255, f );
    if ( more ) {
      tk = strtok( line, "\n" );
      numComponents = atol( tk );
      if ( numComponents <= 0 ) {
        printf( appContextClass_str2, fileName );
        fileEmpty = 1;
        fclose( f );
      }
    }
    else {
      printf( appContextClass_str2, fileName );
      fileEmpty = 1;
      fclose( f );
    }

    if ( !fileEmpty ) {

      index = 0;
      do {

        more = fgets( line, 255, f );
        if ( more ) {

          cur = new libRecType;

          strncpy( buf, line, 255 );

          comment = 0;
          tk = strtok( buf, " \t\n" );

          if ( !tk ) {
            comment = 1;
          }
          else if ( tk[0] == '#' ) {
            comment = 1;
          }

          if ( !comment ) { /* not an empty line or comment */

            tail->flink = cur;
            tail = cur;
            tail->flink = NULL;

            tk = strtok( line, " \t\n" );
            if ( !tk ) {
              printf( appContextClass_str3 );
              return;
            }
            cur->className = new char[strlen(tk)+1];
            strcpy( cur->className, tk );

            tk = strtok( NULL, " \t\n" );
            if ( !tk ) {
              printf( appContextClass_str3 );
              return;
            }
            cur->fileName = new char[strlen(tk)+1];
            strcpy( cur->fileName, tk );

            tk = strtok( NULL, "\n" );
            if ( !tk ) {
              printf( appContextClass_str3 );
              return;
            }
            cur->text = new char[strlen(tk)+1];
            strcpy( cur->text, tk );

            index++;

          }

        }

      } while ( more );

      fclose( f );

      if ( index != numComponents ) {
        printf( appContextClass_str4, fileName );
        return;
      }

    }
    else {

      fileExists = 0; // file was empty so behave as if file does not exist

    }

  }
  else {
    fileExists = 0;
  }

  if ( libFile[0] != '/' ) {
    printf( appContextClass_str5 );
    printf( appContextClass_str6 );
    return;
  }

  dllHandle = dlopen( libFile, RTLD_LAZY );
  if ((error = dlerror()) != NULL)  {
    fputs(error, stderr);
    fputs( "\n", stderr );
    return;
  }

  if ( strcmp( op, global_str65 ) == 0 ) {  // showpv

    printf( "\n" );

    strcpy( line, "firstPvRegRecord" );
    func = (PVREGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str8, stderr );
      return;
    }

    stat = (*func)( &classNamePtr, &textPtr );

    strcpy( line, "nextPvRegRecord" );
    func = (PVREGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str9, stderr );
      return;
    }

    if ( !stat ) {
      strncpy( line, appContextClass_str110, 255 );
      Strncat( line, appContextClass_str111, 255 );
      printf( "%s\n\n", line );
    }

    while ( !stat ) {

      strncpy( line, classNamePtr, 255 );

      if ( strlen(line) < 45 )
        index = 45;
      else
        index = strlen(line) + 5;
      Strncat( line, "                                        ", 255 );
      strncpy( &line[index], textPtr, 255 );

      printf( "%s\n", line );

      // get next
      stat = (*func)( &classNamePtr, &textPtr );

    }

    printf( "\n\n" );

  }
  else if ( strcmp( op, global_str63 ) == 0 ) {  // addpv

    if ( !fileExists ) {
      printf( appContextClass_str13, fileName );
    }

    printf( "\n" );

    strcpy( line, "firstPvRegRecord" );
    func = (PVREGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str14, stderr );
      return;
    }

    stat = (*func)( &classNamePtr, &textPtr );

    strcpy( line, "nextPvRegRecord" );
    func = (PVREGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str9, stderr );
      return;
    }

    numToAdd = 0;
    alreadyExists = 0;
    while ( !stat ) {

      doAdd = 1;

      cur = head->flink;
      while ( cur ) {
        if ( strcmp( classNamePtr, cur->className ) == 0 ) {
          printf( appContextClass_str15, classNamePtr );
          doAdd = 0;
          alreadyExists = 1;
	}
        cur = cur->flink;
      }

      if ( doAdd ) {

        numToAdd++;

        printf( appContextClass_str109, classNamePtr, textPtr );

        cur = new libRecType;
        tail->flink = cur;
        tail = cur;
        tail->flink = NULL;

        cur->className = new char[strlen(classNamePtr)+1];
        strcpy( cur->className, classNamePtr );
        cur->fileName = new char[strlen(libFile)+1];
        strcpy( cur->fileName, libFile );
        cur->text = new char[strlen(textPtr)+1];
        strcpy( cur->text, textPtr );

        numComponents++;

      }

      stat = (*func)( &classNamePtr, &textPtr );

    }

    if ( numToAdd == 0 ) {
      printf( "\n" );
      if ( alreadyExists ) {
        printf( appContextClass_str122, fileName );
      }
      else {
        printf(
         appContextClass_str17, fileName );
      }
      return;
    }

    printf( "\n" );

    strncpy( line, fileName, 255 );
    Strncat( line, "~", 255 );

    if ( fileExists ) {

      stat = unlink( line );

      printf( appContextClass_str18, line );
      stat = rename( fileName, line );
      if ( stat ) {
        perror( appContextClass_str19 );
        return;
      }

    }

    f = fopen( fileName, "w" );
    if ( f ) {

      fprintf( f, "%-d\n", numComponents );
      cur = head->flink;
      while ( cur ) {
        fprintf( f, "%s %s %s\n", cur->className, cur->fileName,
         cur->text );
        cur = cur->flink;
      }

    }
    else {
      perror( fileName );
      return;
    }

    if ( numToAdd == 1 )
      printf( appContextClass_str20 );
    else if ( numToAdd > 1 )
      printf( appContextClass_str21 );

  }
  else if ( strcmp( op, global_str64 ) == 0 ) {  // removepv

    if ( !fileExists ) {
      printf( appContextClass_str123 );
      return;
    }

    printf( "\n" );

    strcpy( line, "firstPvRegRecord" );
    func = (PVREGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str14, stderr );
      return;
    }

    stat = (*func)( &classNamePtr, &textPtr );

    strcpy( line, "nextPvRegRecord" );
    func = (PVREGFUNC) dlsym( dllHandle, line );
    if ((error = dlerror()) != NULL)  {
      fputs( appContextClass_str9, stderr );
      return;
    }

    numToRemove = 0;
    while ( !stat ) {

      cur = head->flink;
      prev = head;
      while ( cur ) {
        next = cur->flink;
        if ( strcmp( classNamePtr, cur->className ) == 0 ) {
          printf( appContextClass_str124, classNamePtr );
          numToRemove++;
          prev->flink = next;
          delete cur;
          break;
	}
        else {
          prev = cur;
	}
        cur = next;
      }

      stat = (*func)( &classNamePtr, &textPtr );

    }

    if ( numToRemove == 0 ) {
      printf( appContextClass_str125, fileName );
      return;
    }

    // count remaining components
    numComponents = 0;
    cur = head->flink;
    while ( cur ) {
      numComponents++;
      cur = cur->flink;
    }

    printf( "\n" );

    strncpy( line, fileName, 255 );
    Strncat( line, "~", 255 );

    if ( fileExists ) {

      stat = unlink( line );

      printf( appContextClass_str18, line );
      stat = rename( fileName, line );
      if ( stat ) {
        perror( appContextClass_str19 );
        return;
      }

    }

    f = fopen( fileName, "w" );
    if ( f ) {

      fprintf( f, "%-d\n", numComponents );
      cur = head->flink;
      while ( cur ) {
        fprintf( f, "%s %s %s\n", cur->className, cur->fileName,
         cur->text );
        cur = cur->flink;
      }

    }
    else {
      perror( fileName );
      return;
    }

    if ( numToRemove == 1 )
      printf( appContextClass_str126 );
    else if ( numToRemove > 1 )
      printf( appContextClass_str127 );

    if ( numComponents == 0 ) {
      printf( appContextClass_str128 );
    }

  }

}

#ifdef __epics__

void ctlPvUpdate (
  struct event_handler_args ast_args )
{

appContextClass *apco = (appContextClass *) ast_args.usr;
char *fName, str[41], name[127+1];
int stat;
activeWindowListPtr cur;
SYS_PROC_ID_TYPE procId;

  fName = (char *) ast_args.dbr;

  if ( strcmp( fName, "" ) == 0 ) {
    return;
  }

  if ( apco->shutdownFlag ) {
    return;
  }

  if ( strcmp( fName, "* SHUTDOWN *" ) == 0 ) {
    apco->shutdownFlag = 1;
    sys_get_proc_id( &procId );
    sprintf( str, "%-d", (int) procId.id );
    stat = ca_put( DBR_STRING, apco->ctlPvId, &str );
    return;
  }

  getFileName( name, fName, 127 );

  cur = apco->head->flink;
  while ( cur != apco->head ) {
    if ( strcmp( name, cur->node.displayName ) == 0 ) {
      // deiconify
      XMapWindow( cur->node.d, XtWindow(cur->node.topWidgetId()) );
      // raise
      XRaiseWindow( cur->node.d, XtWindow(cur->node.topWidgetId()) );
      strcpy( str, "" );
      stat = ca_put( DBR_STRING, apco->ctlPvId, &str );
      return;  // display is already open; don't open another instance
    }
    cur = cur->flink;
  }

  cur = new activeWindowListType;
  cur->requestDelete = 0;
  cur->requestActivate = 0;
  cur->requestActivateClear = 0;
  cur->requestReactivate = 0;
  cur->requestOpen = 0;
  cur->requestPosition = 0;
  cur->requestCascade = 0;
  cur->requestImport = 0;
  cur->requestRefresh = 0;
  cur->requestActiveRedraw = 0;
  cur->requestIconize = 0;

  cur->node.create( apco, NULL, 0, 0, 0, 0, apco->numMacros, apco->macros,
   apco->expansions );
  cur->node.realize();
  cur->node.setGraphicEnvironment( &apco->ci, &apco->fi );

  cur->blink = apco->head->blink;
  apco->head->blink->flink = cur;
  apco->head->blink = cur;
  cur->flink = apco->head;

  cur->node.storeFileName( fName );

  cur->requestOpen = 1;
  (apco->requestFlag)++;

  cur->requestActivate = 1;
  (apco->requestFlag)++;

  strcpy( str, "" );
  stat = ca_put( DBR_STRING, apco->ctlPvId, &str );

}

#else

#ifdef GENERIC_PV

void ctlPvUpdate (
  pvClass *classPtr,
  void *clientData,
  void *args
) {

appContextClass *apco = (appContextClass *) clientData;
char *fName, str[41], name[127+1];
int stat;
activeWindowListPtr cur;

//apco->proc->lock();

  fName = (char *) apco->ctlPvId->getValue( args );

  if ( strcmp( fName, "" ) == 0 ) {
    //    apco->proc->unlock();
    return;
  }

  if ( apco->shutdownFlag ) {
    //    apco->proc->unlock();
    return;
  }

  if ( strcmp( fName, "* SHUTDOWN *" ) == 0 ) {
    apco->shutdownFlag = 1;
    sys_get_proc_id( &procId );
    sprintf( str, "%-d", (int) procId.id );
    stat = ca_put( DBR_STRING, apco->ctlPvId, &str );
    //    apco->proc->unlock();
    return;
  }

  getFileName( name, fName, 127 );

  cur = apco->head->flink;
  while ( cur != apco->head ) {
    if ( strcmp( name, cur->node.displayName ) == 0 ) {
      // deiconify
      XMapWindow( cur->node.d, XtWindow(cur->node.topWidgetId()) );
      // raise
      XRaiseWindow( cur->node.d, XtWindow(cur->node.topWidgetId()) );
      strcpy( str, "" );
      stat = apco->ctlPvId->put( apco->ctlPvId->pvrString(), str );
      //      apco->proc->unlock();
      return;  // display is already open; don't open another instance
    }
    cur = cur->flink;
  }

  cur = new activeWindowListType;
  cur->requestDelete = 0;
  cur->requestActivate = 0;
  cur->requestActivateClear = 0;
  cur->requestReactivate = 0;
  cur->requestOpen = 0;
  cur->requestPosition = 0;
  cur->requestCascade = 0;
  cur->requestImport = 0;
  cur->requestRefresh = 0;
  cur->requestActiveRedraw = 0;
  cur->requestIconize = 0;

  cur->node.create( apco, NULL, 0, 0, 0, 0, apco->numMacros, apco->macros,
   apco->expansions );
  cur->node.realize();
  cur->node.setGraphicEnvironment( &apco->ci, &apco->fi );

  cur->blink = apco->head->blink;
  apco->head->blink->flink = cur;
  apco->head->blink = cur;
  cur->flink = apco->head;

  cur->node.storeFileName( fName );

  cur->requestOpen = 1;
  (apco->requestFlag)++;

  cur->requestActivate = 1;
  (apco->requestFlag)++;

  strcpy( str, "" );
  stat = apco->ctlPvId->put( apco->ctlPvId->pvrString(), str );

  //  apco->proc->unlock();

}

#endif

#endif

void setPath_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

callbackBlockPtr cbPtr = (callbackBlockPtr) client;
char *item = (char *) cbPtr->ptr;
appContextClass *apco = (appContextClass *) cbPtr->apco;

  //printf( "setPath_cb, item = [%s]\n", item );

  strncpy( apco->curPath, item, 127 );

}

void app_fileSelectOk_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

XmFileSelectionBoxCallbackStruct *cbs =
 (XmFileSelectionBoxCallbackStruct *) call;
appContextClass *apco = (appContextClass *) client;
activeWindowListPtr cur;
char *fName;

  if ( !XmStringGetLtoR( cbs->value, XmFONTLIST_DEFAULT_TAG, &fName ) ) {
    goto done;
  }

  if ( !*fName ) {
    XtFree( fName );
    goto done;
  }

  cur = new activeWindowListType;

  cur->requestDelete = 0;
  cur->requestActivate = 0;
  cur->requestActivateClear = 0;
  cur->requestReactivate = 0;
  cur->requestOpen = 0;
  cur->requestPosition = 0;
  cur->requestCascade = 0;
  cur->requestImport = 0;
  cur->requestRefresh = 0;
  cur->requestActiveRedraw = 0;
  cur->requestIconize = 0;

  cur->node.create( apco, NULL, 0, 0, 0, 0, apco->numMacros, apco->macros,
   apco->expansions );
  cur->node.realize();
  cur->node.setGraphicEnvironment( &apco->ci, &apco->fi );

  cur->blink = apco->head->blink;
  apco->head->blink->flink = cur;
  apco->head->blink = cur;
  cur->flink = apco->head;

  cur->node.storeFileName( fName );

  XtFree( fName );

  cur->requestOpen = 1;
  (apco->requestFlag)++;

  if ( apco->executeOnOpen ) {
    cur->requestActivate = 1;
    (apco->requestFlag)++;
  }

done:

  XtUnmanageChild( w );

}

void app_fileSelectCancel_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

  XtUnmanageChild( w );

}

void app_importSelectOk_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

XmFileSelectionBoxCallbackStruct *cbs =
 (XmFileSelectionBoxCallbackStruct *) call;
appContextClass *apco = (appContextClass *) client;
activeWindowListPtr cur;
char *fName;

  if ( !XmStringGetLtoR( cbs->value, XmFONTLIST_DEFAULT_TAG, &fName ) ) {
    goto done;
  }

  if ( !*fName ) {
    XtFree( fName );
    goto done;
  }

  cur = new activeWindowListType;

  cur->requestDelete = 0;
  cur->requestActivate = 0;
  cur->requestActivateClear = 0;
  cur->requestReactivate = 0;
  cur->requestOpen = 0;
  cur->requestPosition = 0;
  cur->requestCascade = 0;
  cur->requestImport = 0;
  cur->requestRefresh = 0;
  cur->requestActiveRedraw = 0;
  cur->requestIconize = 0;

  cur->node.create( apco, NULL, 0, 0, 0, 0, apco->numMacros, apco->macros,
   apco->expansions );
  cur->node.realize();
  cur->node.setGraphicEnvironment( &apco->ci, &apco->fi );

  cur->blink = apco->head->blink;
  apco->head->blink->flink = cur;
  apco->head->blink = cur;
  cur->flink = apco->head;

  cur->node.storeFileName( fName );

  XtFree( fName );

  cur->requestOpen = 1;
  cur->requestImport = 1;
  (apco->requestFlag)++;

done:

  XtUnmanageChild( w );

}

void app_importSelectCancel_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

  XtUnmanageChild( w );

}

void new_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
activeWindowListPtr cur, next;

  // traverse list and delete nodes so marked
  cur = apco->head->flink;
  while ( cur != apco->head ) {
    next = cur->flink;
    if ( cur->requestDelete ) {
      cur->blink->flink = cur->flink;
      cur->flink->blink = cur->blink;
      apco->removeAllDeferredExecutionQueueNode( &cur->node );
      delete cur;
    }
    cur = next;
  }

  cur = new activeWindowListType;
  cur->requestDelete = 0;
  cur->requestActivate = 0;
  cur->requestActivateClear = 0;
  cur->requestReactivate = 0;
  cur->requestOpen = 0;
  cur->requestPosition = 0;
  cur->requestCascade = 0;
  cur->requestImport = 0;
  cur->requestRefresh = 0;
  cur->requestActiveRedraw = 0;
  cur->requestIconize = 0;
  cur->node.create( apco, NULL, 100, 100, 500, 600, apco->numMacros,
   apco->macros, apco->expansions );
  cur->node.realize();
  cur->node.setGraphicEnvironment( &apco->ci, &apco->fi );
  cur->node.setDisplayScheme( &apco->displayScheme );
  XtMapWidget( XtParent( cur->node.drawWidgetId() ) );

  cur->blink = apco->head->blink;
  apco->head->blink->flink = cur;
  apco->head->blink = cur;
  cur->flink = apco->head;

}

void refreshUserLib_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

  apco->reopenUserLib();

}

void open_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
int n;
Arg args[10];
XmString xmStr;
char prefix[127+1];

  strncpy( prefix, apco->curPath, 127 );

  n = 0;

  if ( strcmp( prefix, "" ) != 0 ) {
    xmStr = XmStringCreateLocalized( prefix );
    XtSetArg( args[n], XmNdirectory, xmStr ); n++;
  }

  XtSetValues( apco->fileSelectBox, args, n );

  if ( strcmp( prefix, "" ) != 0 ) XmStringFree( xmStr );

  XtManageChild( apco->fileSelectBox );

  XSetWindowColormap( apco->display, XtWindow(XtParent(apco->fileSelectBox)),
   apco->ci.getColorMap() );

}

void open_user_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
int n;
Arg args[10];
XmString xmStr;
char prefix[127+1];

  strncpy( prefix, apco->curPath, 127 );

  n = 0;

  if ( strcmp( prefix, "" ) != 0 ) {
    xmStr = XmStringCreateLocalized( prefix );
    XtSetArg( args[n], XmNdirectory, xmStr ); n++;
  }

  XtSetValues( apco->fileSelectBox, args, n );

  if ( strcmp( prefix, "" ) != 0 ) XmStringFree( xmStr );

  XtManageChild( apco->fileSelectBox );

  XSetWindowColormap( apco->display, XtWindow(XtParent(apco->fileSelectBox)),
   apco->ci.getColorMap() );

}

void import_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
int n;
Arg args[10];
XmString xmStr;
char prefix[127+1];

  strncpy( prefix, apco->curPath, 127 );

  n = 0;

  if ( strcmp( prefix, "" ) != 0 ) {
    xmStr = XmStringCreateLocalized( prefix );
    XtSetArg( args[n], XmNdirectory, xmStr ); n++;
  }

  XtSetValues( apco->importSelectBox, args, n );

  if ( strcmp( prefix, "" ) != 0 ) XmStringFree( xmStr );

  XtManageChild( apco->importSelectBox );

  XSetWindowColormap( apco->display, XtWindow(XtParent(apco->importSelectBox)),
   apco->ci.getColorMap() );

}

void continue_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

  apco->confirm.popdown();

}

void abort_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

  apco->confirm.popdown();
  apco->exitFlag = 1;

}

void exit_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
activeWindowListPtr cur;
int changes = 0;

Window root, child;
int rootX, rootY, winX, winY;
unsigned int mask;

  cur = apco->head->flink;
  while ( cur != apco->head ) {
    if ( cur->node.changed() ) changes = 1;
    cur = cur->flink;
  }

  if ( changes ) {
    XQueryPointer( apco->display, XtWindow(apco->appTop), &root, &child,
     &rootX, &rootY, &winX, &winY, &mask );
    apco->confirm.create( apco->appTop, rootX, rootY, 2,
     appContextClass_str24, NULL, NULL );
    apco->confirm.addButton( appContextClass_str25, abort_cb, (void *) apco );
    apco->confirm.addButton( appContextClass_str26, continue_cb,
     (void *) apco );
    apco->confirm.finished();
    apco->confirm.popup();
    XSetWindowColormap( apco->display, XtWindow(apco->confirm.top()),
     apco->ci.getColorMap() );
    return;
  }

  apco->exitFlag = 1;

}

void dont_shutdown_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

  apco->confirm.popdown();

}

void do_shutdown_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

  apco->confirm.popdown();
  apco->shutdownFlag = 1;

}

void shutdown_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

activeWindowListPtr cur;
int changes = 0;

Window root, child;
int rootX, rootY, winX, winY;
unsigned int mask;

  cur = apco->head->flink;
  while ( cur != apco->head ) {
    if ( cur->node.changed() ) changes = 1;
    cur = cur->flink;
  }

  if ( changes ) {

    XQueryPointer( apco->display, XtWindow(apco->appTop), &root, &child,
     &rootX, &rootY, &winX, &winY, &mask );

    apco->confirm.create( apco->appTop, rootX, rootY, 2,
     appContextClass_str24, NULL, NULL );
    apco->confirm.addButton( appContextClass_str137, do_shutdown_cb,
     (void *) apco );
    apco->confirm.addButton( appContextClass_str26, dont_shutdown_cb,
     (void *) apco );
    apco->confirm.finished();

    apco->confirm.popup();

    XSetWindowColormap( apco->display, XtWindow(apco->confirm.top()),
     apco->ci.getColorMap() );

    return;

  }

  apco->shutdownFlag = 1;

}

void reload_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

  apco->reloadFlag = 1;

}

void view_pvList_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

  apco->pvList.popup();

}

void renderImages_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
XmString str;

  if ( apco->renderImagesFlag ) {
    apco->renderImagesFlag = 0;
    str = XmStringCreateLocalized( appContextClass_str134 );
    XtVaSetValues( apco->renderImagesB,
     XmNlabelString, str,
     NULL );
    XmStringFree( str );
  }
  else {
    apco->renderImagesFlag = 1;
    str = XmStringCreateLocalized( appContextClass_str135 );
    XtVaSetValues( apco->renderImagesB,
     XmNlabelString, str,
     NULL );
    XmStringFree( str );
  }

}

void view_xy_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
XmString str;

  if ( apco->viewXy ) {
    apco->viewXy = 0;
    str = XmStringCreateLocalized( appContextClass_str112 );
    XtVaSetValues( apco->viewXyB,
     XmNlabelString, str,
     NULL );
    XmStringFree( str );
  }
  else {
    apco->viewXy = 1;
    str = XmStringCreateLocalized( appContextClass_str113 );
    XtVaSetValues( apco->viewXyB,
     XmNlabelString, str,
     NULL );
    XmStringFree( str );
  }

}

void view_screens_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
char string[79+1];
int i, nodeCount = 0;
activeWindowListPtr cur;

  cur = apco->head->flink;
  while ( cur != apco->head ) {
    if ( blank( cur->node.displayName ) ) {
      apco->postMessage( appContextClass_str27 );
    }
    else {
      apco->postMessage( cur->node.displayName );
    }
    nodeCount++;
    cur = cur->flink;
  }

  if ( nodeCount ) {

    sprintf( string, appContextClass_str28, nodeCount );
    for( i=0; string[i]; i++ ) string[i] = '-';
    apco->postMessage( string );

    sprintf( string, appContextClass_str28, nodeCount );
    apco->postMessage( string );

    strcpy( string, " " );
    apco->postMessage( string );

  }
  else {

    strcpy( string, appContextClass_str29 );
    apco->postMessage( string );

    strcpy( string, " " );
    apco->postMessage( string );

  }

}

void view_msgBox_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;

  apco->msgBox.popup();

  XSetWindowColormap( apco->display, XtWindow(apco->msgBox.top()),
   apco->ci.getColorMap() );

}

void checkpointPid_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
char procIdName[31+1];
SYS_PROC_ID_TYPE procId;

  sys_get_proc_id( &procId );
  sprintf( procIdName, "PID = %-d", (int) procId.id );

  apco->postMessage( procIdName );

}

void help_cb (
  Widget w,
  XtPointer client,
  XtPointer call )
{

appContextClass *apco = (appContextClass *) client;
activeWindowListPtr cur;
char buf[255+1], *envPtr;
int i, numMacros;
char *sysValues[5], *ptr;

char *fName = "helpMain";

char *sysMacros[] = {
  "help"
};

  // is help file already open?
  cur = apco->head->flink;
  while ( cur != apco->head ) {
    if ( strcmp( fName, cur->node.displayName ) == 0 ) {
      // deiconify
      XMapWindow( cur->node.d, XtWindow(cur->node.topWidgetId()) );
      // raise
      XRaiseWindow( cur->node.d, XtWindow(cur->node.topWidgetId()) );
      return;  // display is already open; don't open another instance
    }
    cur = cur->flink;
  }

  envPtr = getenv( environment_str5 );
  if ( envPtr ) {

    strncpy( buf, envPtr, 255 );

    if ( buf[strlen(buf)-1] != '/' ) {
      Strncat( buf, "/", 255 );
    }

  }
  else {

    strcpy( buf, "/etc/edm/" );

  }

  // build system macros

  numMacros = 0;

  ptr = new char[strlen(buf)+1];
  strcpy( ptr, buf );
  sysValues[0] = ptr;

  numMacros++;

  // ============

  Strncat( buf, fName, 255 );
  Strncat( buf, ".edl", 255 );

  cur = new activeWindowListType;
  cur->requestDelete = 0;
  cur->requestActivate = 0;
  cur->requestActivateClear = 0;
  cur->requestReactivate = 0;
  cur->requestOpen = 0;
  cur->requestPosition = 0;
  cur->requestCascade = 0;
  cur->requestImport = 0;
  cur->requestRefresh = 0;
  cur->requestActiveRedraw = 0;
  cur->requestIconize = 0;

  cur->node.createNoEdit( apco, NULL, 0, 0, 0, 0, numMacros,
   sysMacros, sysValues );

  for ( i=0; i<numMacros; i++ ) {
    delete sysValues[i];
  }

  cur->node.realize();
  cur->node.setGraphicEnvironment( &apco->ci, &apco->fi );

  cur->blink = apco->head->blink;
  apco->head->blink->flink = cur;
  apco->head->blink = cur;
  cur->flink = apco->head;

  cur->node.storeFileName( buf );

  cur->requestOpen = 1;
  (apco->requestFlag)++;

  cur->requestActivate = 1;
  (apco->requestFlag)++;

}

appContextClass::appContextClass (
  void )
{

  executeCount = 0;
  isActive = 0;
  exitFlag = 0;
  shutdownFlag = 0;
  reloadFlag = 0;
  saveContextOnExit = 0;
  primaryServer = 0;
  executeOnOpen = 0;
  noEdit = 0;
  requestFlag = 0;
  iconified = 0;
  usingControlPV = 0;
  renderImagesFlag = 1;

  entryFormX = 0;
  entryFormY = 0;
  entryFormW = 0;
  entryFormH = 600;
  largestH = 600;

  head = new activeWindowListType;
  head->flink = head;
  head->blink = head;

  numMacros = 0;
  macroHead = new macroListType;
  macroHead->flink = macroHead;
  macroHead->blink = macroHead;

  numFiles = 0;
  fileHead = new fileListType;
  fileHead->flink = fileHead;
  fileHead->blink = fileHead;

  strcpy( ctlPV, "" );
  strcpy( userLib, "" );

  cutHead1 = new activeGraphicListType;
  cutHead1->flink = cutHead1;
  cutHead1->blink = cutHead1;

  viewXy = 0;

  getFilePaths();
  strncpy( curPath, dataFilePrefix[0], 127 );

  buildSchemeList();

  // sentinel node
  callbackBlockHead = new callbackBlockType;
  callbackBlockTail = callbackBlockHead;
  callbackBlockTail->flink = NULL;

}

appContextClass::~appContextClass (
  void )
{

int i, numOpenWindows;
activeWindowListPtr cur, next;
activeGraphicListPtr curCut, nextCut;
macroListPtr curMacro, nextMacro;
fileListPtr curFile, nextFile;
callbackBlockPtr curCbBlock, nextCbBlock;

  if ( saveContextOnExit ) {
    //fprintf( shutdownFilePtr, "appCtx {\n" );
    //fprintf( shutdownFilePtr, "  primaryServer=%-d\n", primaryServer );
    fprintf( shutdownFilePtr, "%-d\n", primaryServer );
    //if ( blank(displayName) ) {
      //fprintf( shutdownFilePtr, "  display=<NULL>\n" );
    //}
    //else {
    //  fprintf( shutdownFilePtr, "  display=%s\n", displayName );
    //}
    writeStringToFile( shutdownFilePtr, displayName );
    //fprintf( shutdownFilePtr, "  noEdit=%-d\n", noEdit );
    fprintf( shutdownFilePtr, "%-d\n", noEdit );
  }

  ci.closeColorWindow();

  // empty cut list
  curCut = cutHead1->flink;
  while ( curCut != cutHead1 ) {
    nextCut = curCut->flink;
    delete curCut->node;
    delete curCut;
    curCut = nextCut;
  }
  delete cutHead1;

  // walk macroList and delete
  if ( saveContextOnExit ) {
    //fprintf( shutdownFilePtr, "  macros {\n" );
    //fprintf( shutdownFilePtr, "    num=%-d\n", numMacros );
    fprintf( shutdownFilePtr, "%-d\n", numMacros );
  }
  curMacro = macroHead->flink;
  while ( curMacro != macroHead ) {
    nextMacro = curMacro->flink;
    if ( saveContextOnExit ) {
      //fprintf( shutdownFilePtr, "    %s=%s\n", curMacro->macro,
      // curMacro->expansion );
      fprintf( shutdownFilePtr, "%s=%s\n", curMacro->macro,
       curMacro->expansion );
    }
    if ( curMacro->macro ) delete curMacro->macro;
    if ( curMacro->expansion ) delete curMacro->expansion;
    delete curMacro;
    curMacro = nextMacro;
  }
  delete macroHead;
  //if ( saveContextOnExit ) {
  //  fprintf( shutdownFilePtr, "  }\n" );
  //}

  // walk fileList and delete
  curFile = fileHead->flink;
  while ( curFile != fileHead ) {
    nextFile = curFile->flink;
    if ( curFile->file ) delete curFile->file;
    delete curFile;
    curFile = nextFile;
  }
  delete fileHead;

  if ( saveContextOnExit ) {
    // get number of open windows
    numOpenWindows = 0;
    cur = head->flink;
    while ( cur != head ) {
      numOpenWindows++;
      cur = cur->flink;
    }
    fprintf( shutdownFilePtr, "%-d\n", numOpenWindows );
  }

  // walk activeWindowList and delete
  cur = head->flink;
  while ( cur != head ) {
    next = cur->flink;
    if ( cur->node.mode == AWC_EXECUTE ) {
      cur->node.returnToEdit( 0 );
    }
    if ( saveContextOnExit ) {
      //fprintf( shutdownFilePtr, "  actWin {\n" );
      cur->node.checkPoint( primaryServer, shutdownFilePtr );
      //fprintf( shutdownFilePtr, "  }\n" );
    }
    cur->blink->flink = cur->flink; // maintain list structure!
    cur->flink->blink = cur->blink;
    delete cur;
    cur = next;
  }
  processAllEvents( app, display );
  delete head;

  // delete widgets

  XtRemoveCallback( importSelectBox, XmNcancelCallback,
   app_importSelectCancel_cb, (void *) this );
  XtRemoveCallback( importSelectBox, XmNcancelCallback,
   app_importSelectCancel_cb, (void *) this );
  XtUnmanageChild( importSelectBox );
  XtDestroyWidget( importSelectBox );

  XtRemoveCallback( fileSelectBox, XmNcancelCallback,
   app_fileSelectCancel_cb, (void *) this );
  XtRemoveCallback( fileSelectBox, XmNcancelCallback,
   app_fileSelectCancel_cb, (void *) this );
  XtUnmanageChild( fileSelectBox );
  XtDestroyWidget( fileSelectBox );

  pvList.destroy();

  msgBox.destroy();

  XtUnmapWidget( appTop );

  XtDestroyWidget( mainWin );

  processAllEvents( app, display );

  XtDestroyWidget( appTop );

  processAllEvents( app, display );

  if ( dataFilePrefix ) {
    for ( i=0; i<numPaths; i++ ) {
      delete dataFilePrefix[i];
      dataFilePrefix[i] = NULL;
    }
    delete dataFilePrefix;
    dataFilePrefix = NULL;
  }

  destroySchemeList();

  curCbBlock = callbackBlockHead->flink;
  while ( curCbBlock ) {
    nextCbBlock = curCbBlock->flink;
    delete curCbBlock;
    curCbBlock = nextCbBlock;
  }
  delete callbackBlockHead;

  //if ( saveContextOnExit ) {
  //  fprintf( shutdownFilePtr, "}\n" );
  //}

  // these are done in main.cc
      //XtCloseDisplay( display );
      //XtDestroyApplicationContext( app );

}

void appContextClass::reloadAll ( void )
{

activeWindowListPtr cur;

  // walk activeWindowList and reload
  cur = head->flink;
  while ( cur != head ) {
    if ( !cur->requestDelete ) {
      cur->requestActivate = 0;
      cur->requestActivateClear = 0;
      cur->requestReactivate = 0;
      cur->requestOpen = 1;
      requestFlag++;
      cur->requestPosition = 1;
      cur->requestCascade = 0;
      cur->requestImport = 0;
      cur->requestRefresh = 0;
      cur->requestActiveRedraw = 0;
      cur->requestIconize = 0;
      cur->x = cur->node.x;
      cur->y = cur->node.y;
      if ( cur->node.mode == AWC_EXECUTE ) {
        cur->node.returnToEdit( 0 );
        cur->node.noRaise = 1;
        processAllEvents( app, display );
        cur->requestActivate = 1;
        cur->requestActivateClear = 1;
        requestFlag++;
      }
      cur->node.reloadSelf();
    }
    cur = cur->flink;
  }
  processAllEvents( app, display );

}

void appContextClass::refreshAll ( void )
{

activeWindowListPtr cur;

  // walk activeWindowList and request refresh
  cur = head->flink;
  while ( cur != head ) {
    if ( !cur->requestDelete ) {
      if ( cur->node.mode == AWC_EXECUTE ) {
        cur->node.refreshActive();
      }
      else {
        cur->node.refresh();
      }
    }
    cur = cur->flink;
    processAllEvents( app, display );
  }

}

void appContextClass::getFilePaths ( void ) {

int i, stat;
char *envPtr, *gotIt, buf[1270+1], save[127+1], path[127+1], *tk;

  // EDMFILES
  envPtr = getenv( environment_str2 );
  if ( envPtr ) {

    strncpy( buf, envPtr, 1270 );

    tk = strtok( buf, ":" );
    if ( tk ) {

      strncpy( colorPath, tk, 127 );
      if ( colorPath[strlen(colorPath)-1] != '/' ) {
        Strncat( colorPath, "/", 127 );
      }

    }
    else {

      strncpy( colorPath, "./", 127 );

    }

  }
  else {

    strncpy( colorPath, "./", 127 );

  }

  // EDMDATAFILES

  envPtr = getenv( environment_str1 );
  if ( envPtr ) {

    // count number of search paths
    strncpy( buf, envPtr, 1270 );

    numPaths = 0;
    tk = strtok( buf, ":" );
    while ( tk ) {

      numPaths++;

      tk = strtok( NULL, ":" );

    }

    if ( numPaths == 0 ) {

      strcpy( path, "." );

      gotIt = getcwd( save, 127 );
      if ( !gotIt ) {
        printf( appContextClass_str118, __LINE__, __FILE__ );
        exit(0);
      }

      stat = chdir( path );
      if ( stat ) {
        perror( appContextClass_str119 );
        printf( appContextClass_str120 );
      }
      getcwd( path, 127 );

      chdir( save );

      if ( path[strlen(path)-1] != '/' )
       Strncat( path, "/", 127 );

      numPaths = 1;
      dataFilePrefix = new char *[1];
      dataFilePrefix[0] = new char[strlen(path)+1];
      strcpy( dataFilePrefix[0], path );

      return;

    }

    dataFilePrefix = new char *[numPaths];

    strncpy( buf, envPtr, 1270 );
    tk = strtok( buf, ":" );
    for ( i=0; i<numPaths; i++ ) {

      strncpy( path, tk, 127 );
      if ( path[strlen(path)-1] == '/' ) path[strlen(path)-1] = 0;

      gotIt = getcwd( save, 127 );
      if ( !gotIt ) {
        printf( appContextClass_str118, __LINE__, __FILE__ );
        exit(0);
      }

      stat = chdir( path );
      if ( stat ) {
        perror( appContextClass_str119 );
        printf( appContextClass_str120 );
      }
      getcwd( path, 127 );

      chdir( save );

      if ( path[strlen(path)-1] != '/' )
       Strncat( path, "/", 127 );

      dataFilePrefix[i] = new char[strlen(path)+1];
      strcpy( dataFilePrefix[i], path );

      tk = strtok( NULL, ":" );

    }

  }
  else {

    getcwd( path, 127 );

    if ( path[strlen(path)-1] != '/' )
     Strncat( path, "/", 127 );

    numPaths = 1;

    dataFilePrefix = new char *[1];
    dataFilePrefix[0] = new char[strlen(path)+1];
    strcpy( dataFilePrefix[0], path );

  }

}


void appContextClass::expandFileName (
  int index,
  char *expandedName,
  char *inName,
  char *ext,
  int maxSize )
{

char *gotOne;

  if ( index >= numPaths ) {
    strcpy( expandedName, "" );
    return;
  }

  gotOne = strstr( inName, "/" );

  if ( gotOne ) {
    strncpy( expandedName, inName, maxSize );
  }
  else {
    strncpy( expandedName, dataFilePrefix[index], maxSize );
    Strncat( expandedName, inName, maxSize );
  }

  if ( strlen(expandedName) > strlen(ext) ) {
    if ( strcmp( &expandedName[strlen(expandedName)-strlen(ext)], ext )
     != 0 ) {
      Strncat( expandedName, ext, maxSize );
    }
  }
  else {
    Strncat( expandedName, ext, maxSize );
  }

}

#define GETTING_SET_NAME 1
#define GETTING_LIST 2
void appContextClass::buildSchemeList ( void )
{

char *envPtr, *ptr;
char prefix[127+1], fName[127+1], buf[255+1], oName[127+1], sName[63+1],
 line[255+1], *tk;
schemeListPtr cur, curSet;
FILE *f;
int stat, dup, state, i;

  numSchemeSets = 0;
  schemeListExists = 0;
  schemeList = (AVL_HANDLE) NULL;
  schemeSet = (AVL_HANDLE) NULL;
  schemeSetList = (char **) NULL;

  //printf( "build scheme list\n" );
  stat = avl_init_tree( compare_nodes, compare_key, copy_nodes,
   &(this->schemeList) );
  if ( !( stat & 1 ) ) {
    numSchemeSets = 0;
    schemeListExists = 0;
    return;
  }

  stat = avl_init_tree( compare_nodes, compare_key, copy_nodes,
   &(this->schemeSet) );
  if ( !( stat & 1 ) ) {
    numSchemeSets = 0;
    schemeListExists = 0;
    return;
  }

  // open scheme list file and build tree
  envPtr = getenv(environment_str2);
  if ( envPtr ) {
    strncpy( prefix, envPtr, 127 );
    if ( prefix[strlen(prefix)-1] != '/' ) Strncat( prefix, "/", 127 );
  }
  else {
    strcpy( prefix, "/etc/edm/" );
  }

  strncpy( fName, prefix, 127 );
  Strncat( fName, "schemes.list", 127 );

  f = fopen( fName, "r" );
  if ( !f ) {
    numSchemeSets = 0;
    schemeListExists = 0;
    return;
  }

  state = GETTING_SET_NAME;

  while ( 1 ) {

    switch ( state ) {

    case GETTING_SET_NAME:

      //printf( "GETTING_SET_NAME\n" );

      do {

        ptr = fgets ( line, 255, f );
        if ( !ptr ) {
          fclose( f );
          schemeListExists = 1;
          goto done;
        }

      } while ( blank( line ) );

      tk = strtok( line, " \t\n" );
      if ( tk ) {

        strncpy( sName, tk, 63 );

        tk = strtok( NULL, " \t\n" );
        if ( tk ) {

          if ( strcmp( tk, "{" ) != 0 ) {
            printf( "appContextClass::buildSchemeList syntax err 1\n" );
            fclose( f );
            numSchemeSets = 0;
            schemeListExists = 0;
            return;
          }

	}
	else {
          printf( "appContextClass::buildSchemeList syntax err 2\n" );
          fclose( f );
          numSchemeSets = 0;
          schemeListExists = 0;
          return;
        }

      }
      else {
        printf( "appContextClass::buildSchemeList syntax err 3\n" );
        fclose( f );
        numSchemeSets = 0;
        schemeListExists = 0;
        return;
      }

      curSet = new schemeListType;
      curSet->objName = new char[strlen(sName)+1];
      strcpy( curSet->objName, sName );
      stat = avl_insert_node( this->schemeSet, (void *) curSet, &dup );
      if ( !( stat & 1 ) ) {
        fclose( f );
        numSchemeSets = 0;
        schemeListExists = 0;
        return;
      }
      if ( dup ) {
        printf( "appContextClass::buildSchemeList dups\n" );
      }
      else {
        numSchemeSets++;
      }

      state = GETTING_LIST;

      break;

    case GETTING_LIST:

      //printf( "GETTING_LIST\n" );

      do {

        ptr = fgets ( line, 255, f );
        if ( !ptr ) {
          printf( "appContextClass::buildSchemeList syntax err 4\n" );
          fclose( f );
          numSchemeSets = 0;
          schemeListExists = 0;
          return;
        }

      } while ( blank( line ) );

      tk = strtok( line, " \t\n" );
      if ( tk ) {

        if ( strcmp( tk, "}" ) == 0 ) {
          state = GETTING_SET_NAME;
          break;
	}

        strncpy( oName, tk, 127 );

        tk = strtok( NULL, " \t\n" );
        if ( tk ) {

          strncpy( fName, tk, 127 );

          cur = new schemeListType;
          if ( !cur ) {
            fclose( f );
            numSchemeSets = 0;
            schemeListExists = 0;
            return;
          }

          strncpy( buf, sName, 255 );
          Strncat( buf, "-", 255 );
          Strncat( buf, oName, 255 );

          cur->objName = new char[strlen(buf)+1];
          strcpy( cur->objName, buf );

          cur->fileName = new char[strlen(fName)+1];
          strcpy( cur->fileName, fName );

          stat = avl_insert_node( this->schemeList, (void *) cur, &dup );
          if ( !( stat & 1 ) ) {
            fclose( f );
            numSchemeSets = 0;
            schemeListExists = 0;
            return;
          }
          if ( dup ) {
            printf( "appContextClass::buildSchemeList dups\n" );
          }

        }
        else {
          printf( "appContextClass::buildSchemeList syntax err 5\n" );
          fclose( f );
          numSchemeSets = 0;
          schemeListExists = 0;
          return;
        }

      }
      else {
        printf( "appContextClass::buildSchemeList syntax err 6\n" );
        fclose( f );
        numSchemeSets = 0;
        schemeListExists = 0;
        return;
      }

      break;

    }

  }

done:

  //printf( "numSchemeSets = %-d\n", numSchemeSets );

  stat = avl_get_first( schemeSet, (void **) &curSet );
  if ( !( stat & 1 ) ) {
    numSchemeSets = 0;
    schemeListExists = 0;
    return;
  }

  schemeSetList = new char *[numSchemeSets];
  i = 0;
  while ( curSet ) {

    //printf( "curSet->objName = [%s]\n", curSet->objName );

    schemeSetList[i] = new char[strlen(curSet->objName)+1];
    strcpy( schemeSetList[i], curSet->objName );

    stat = avl_get_next( schemeSet, (void **) &curSet );
    if ( !( stat & 1 ) ) {
      numSchemeSets = 0;
      schemeListExists = 0;
      return;
    }

    i++;

  }

}

void appContextClass::destroySchemeList ( void )
{

int stat, i;
schemeListPtr cur;

  if ( schemeSetList ) {
    for ( i=0; i<numSchemeSets; i++ ) {
      delete schemeSetList[i];
    }
    delete schemeSetList;
  }

  if ( !schemeList ) return;

  stat = avl_get_first( schemeList, (void **) &cur );
  if ( !( stat & 1 ) ) {
    return;
  }

  while ( cur ) {

    stat = avl_delete_node( schemeList, (void **) &cur );
    delete cur->objName;
    delete cur->fileName;
    delete cur;

    stat = avl_get_first( schemeList, (void **) &cur );
    if ( !( stat & 1 ) ) {
      return;
    }

  }

  //printf( "need avl delete tree\n" );
  //delete schemeList;

  if ( !schemeSet ) return;

  stat = avl_get_first( schemeSet, (void **) &cur );
  if ( !( stat & 1 ) ) {
    return;
  }

  while ( cur ) {

    stat = avl_delete_node( schemeSet, (void **) &cur );
    delete cur->objName;
    delete cur;

    stat = avl_get_first( schemeSet, (void **) &cur );
    if ( !( stat & 1 ) ) {
      return;
    }

  }

  //printf( "need avl delete tree\n" );
  //delete schemeSet;

}

int appContextClass::schemeExists (
  char *schemeSetName,
  char *objName,
  char *objType )
{

int stat;
schemeListPtr cur;
char buf[255+1];

  if ( !schemeListExists ) {
    return 0;
  }

  if ( strcmp( schemeSetName, "" ) == 0 ) {
    return 0;
  }

  strncpy( buf, schemeSetName, 255 );
  Strncat( buf, "-", 255 );
  Strncat( buf, objType, 255 );
  Strncat( buf, "-", 255 );
  Strncat( buf, objName, 255 );

  stat = avl_get_match( this->schemeList, (void *) buf,
   (void **) &cur );
  if ( !( stat & 1 ) ) {
    return 0;
  }

  if ( cur ) {
    return 1;
  }
  else {
    return 0;
  }

}

void appContextClass::getScheme (
  char *schemeSetName,
  char *objName,
  char *objType,
  char *schemeFileName,
  int maxLen )
{

int stat;
schemeListPtr cur;
char buf[255+1];

  if ( !schemeListExists ) {
    strcpy( schemeFileName, "" );
    return;
  }

  if ( strcmp( schemeSetName, "" ) == 0 ) {
    strcpy( schemeFileName, "" );
    return;
  }

  strncpy( buf, schemeSetName, 255 );
  Strncat( buf, "-", 255 );
  Strncat( buf, objType, 255 );
  Strncat( buf, "-", 255 );
  Strncat( buf, objName, 255 );

  //printf( "get scheme file for %s\n", buf );

  stat = avl_get_match( this->schemeList, (void *) buf,
   (void **) &cur );
  if ( !( stat & 1 ) ) {
    strcpy( schemeFileName, "" );
    return;
  }

  if ( cur ) {
    strncpy( schemeFileName, cur->fileName, maxLen );
  }
  else {
    strncpy( schemeFileName, "default", maxLen );
  }

}

int appContextClass::initDeferredExecutionQueue ( void )
{

int i, stat;

  stat = sys_iniq( &appDefExeFreeQueue );
  if ( !( stat & 1 ) ) {
    printf( appContextClass_str30 );
    return 2;
  }
  stat = sys_iniq( &appDefExeActiveQueue );
  if ( !( stat & 1 ) ) {
    printf( appContextClass_str30 );
    return 2;
  }
  stat = sys_iniq( &appDefExeActiveNextQueue );
  if ( !( stat & 1 ) ) {
    printf( appContextClass_str30 );
    return 2;
  }

  appDefExeFreeQueue.flink = NULL;
  appDefExeFreeQueue.blink = NULL;
  appDefExeActiveQueue.flink = NULL;
  appDefExeActiveQueue.blink = NULL;
  appDefExeActiveNextQueue.flink = NULL;
  appDefExeActiveNextQueue.blink = NULL;

  for ( i=0; i<APPDEFEXE_QUEUE_SIZE; i++ ) {

    stat = INSQTI( (void *) &appDefExeNodes[i], (void *) &appDefExeFreeQueue,
     0 );
    if ( !( stat & 1 ) ) {
      printf( appContextClass_str30 );
      return 2;
    }

  }

  return 1;

}

void appContextClass::removeAllDeferredExecutionQueueNode (
  class activeWindowClass *awo )
{

  // remove all nodes associated with awo

int q_stat_r, q_stat_i;
APPDEFEXE_NODE_PTR node;

  // first, place all next queue nodes on active queue
  do {

    q_stat_r = REMQHI( (void *) &appDefExeActiveNextQueue, (void **) &node,
     0 );

    if ( q_stat_r & 1 ) {

      q_stat_i = INSQTI( (void *) node, (void *) &appDefExeActiveQueue,
       0 );
      if ( !( q_stat_i & 1 ) ) {
        printf( appContextClass_str33 );
      }

    }
    else if ( q_stat_r != QUEWASEMP ) {
      printf( appContextClass_str32 );
    }

  } while ( q_stat_r & 1 );

  // now, remove all associated nodes from active queue
  do {

    q_stat_r = REMQHI( (void *) &appDefExeActiveQueue, (void **) &node, 0 );

    if ( q_stat_r & 1 ) {

      if ( node->awObj ) { // active window object

	if ( node->awObj == awo ) { // remove node

          q_stat_i = INSQTI( (void *) node, (void *) &appDefExeFreeQueue, 0 );
          if ( !( q_stat_i & 1 ) ) {
            printf( appContextClass_str31 );
          }

	}
	else { // don't remove, put it back

          q_stat_i = INSQTI( (void *) node, (void *) &appDefExeActiveNextQueue,
           0 );
          if ( !( q_stat_i & 1 ) ) {
            printf( appContextClass_str33 );
          }

	}

      }
      else { // active graphics object

        if ( node->obj->actWin == awo ) {  // remove node

          q_stat_i = INSQTI( (void *) node, (void *) &appDefExeFreeQueue, 0 );
          if ( !( q_stat_i & 1 ) ) {
            printf( appContextClass_str31 );
          }

	}
	else { // don't remove, put it back

          q_stat_i = INSQTI( (void *) node, (void *) &appDefExeActiveNextQueue,
           0 );
          if ( !( q_stat_i & 1 ) ) {
            printf( appContextClass_str33 );
          }

	}

      }

    }
    else if ( q_stat_r != QUEWASEMP ) {
      printf( appContextClass_str32 );
    }

  } while ( q_stat_r & 1 );

}

void appContextClass::processDeferredExecutionQueue ( void )
{

int q_stat_r, q_stat_i;
APPDEFEXE_NODE_PTR node;

  // first, place all next queue nodes on active queue
  do {

    q_stat_r = REMQHI( (void *) &appDefExeActiveNextQueue, (void **) &node,
     0 );

    if ( q_stat_r & 1 ) {

      q_stat_i = INSQTI( (void *) node, (void *) &appDefExeActiveQueue,
       0 );
      if ( !( q_stat_i & 1 ) ) {
        printf( appContextClass_str33 );
      }

    }
    else if ( q_stat_r != QUEWASEMP ) {
      printf( appContextClass_str32 );
    }

  } while ( q_stat_r & 1 );

  // process all active nodes
  do {

    q_stat_r = REMQHI( (void *) &appDefExeActiveQueue, (void **) &node, 0 );

    if ( q_stat_r & 1 ) {

      if ( node->obj ) node->obj->executeFromDeferredQueue();
      if ( node->awObj ) node->awObj->executeFromDeferredQueue();

      q_stat_i = INSQTI( (void *) node, (void *) &appDefExeFreeQueue, 0 );
      if ( !( q_stat_i & 1 ) ) {
        printf( appContextClass_str31 );
      }

    }
    else if ( q_stat_r != QUEWASEMP ) {
      printf( appContextClass_str32 );
    }

  } while ( q_stat_r & 1 );

}

void appContextClass::postDeferredExecutionQueue (
  class activeGraphicClass *ptr )
{

int q_stat_r, q_stat_i;
APPDEFEXE_NODE_PTR node;

    q_stat_r = REMQHI( (void *) &appDefExeFreeQueue, (void **) &node, 0 );

    if ( q_stat_r & 1 ) {

      node->awObj = NULL;
      node->obj = ptr;

      q_stat_i = INSQTI( (void *) node, (void *) &appDefExeActiveQueue, 0 );
      if ( !( q_stat_i & 1 ) ) {
        printf( appContextClass_str33 );
      }

    }
    else {
      printf( appContextClass_str34 );
    }

}

void appContextClass::postDeferredExecutionQueue (
  class activeWindowClass *ptr )
{

int q_stat_r, q_stat_i;
APPDEFEXE_NODE_PTR node;

    q_stat_r = REMQHI( (void *) &appDefExeFreeQueue, (void **) &node, 0 );

    if ( q_stat_r & 1 ) {

      node->awObj = ptr;
      node->obj = NULL;

      q_stat_i = INSQTI( (void *) node, (void *) &appDefExeActiveQueue, 0 );
      if ( !( q_stat_i & 1 ) ) {
        printf( appContextClass_str33 );
      }

    }
    else {
      printf( appContextClass_str34 );
    }

}

void appContextClass::postDeferredExecutionNextQueue (
  class activeGraphicClass *ptr )
{

int q_stat_r, q_stat_i;
APPDEFEXE_NODE_PTR node;

    q_stat_r = REMQHI( (void *) &appDefExeFreeQueue, (void **) &node, 0 );

    if ( q_stat_r & 1 ) {

      node->awObj = NULL;
      node->obj = ptr;

      q_stat_i = INSQTI( (void *) node, (void *) &appDefExeActiveNextQueue,
       0 );
      if ( !( q_stat_i & 1 ) ) {
        printf( appContextClass_str33 );
      }

    }
    else {
      printf( appContextClass_str34 );
    }

}

void appContextClass::postDeferredExecutionNextQueue (
  class activeWindowClass *ptr )
{

int q_stat_r, q_stat_i;
APPDEFEXE_NODE_PTR node;

    q_stat_r = REMQHI( (void *) &appDefExeFreeQueue, (void **) &node, 0 );

    if ( q_stat_r & 1 ) {

      node->awObj = ptr;
      node->obj = NULL;

      q_stat_i = INSQTI( (void *) node, (void *) &appDefExeActiveNextQueue,
       0 );
      if ( !( q_stat_i & 1 ) ) {
        printf( appContextClass_str33 );
      }

    }
    else {
      printf( appContextClass_str34 );
    }

}

void appContextClass::createMainWindow ( void ) {

XmString menuStr, str;
callbackBlockPtr curBlock;
int i;

  mainWin = XtVaCreateManagedWidget( "", xmMainWindowWidgetClass,
   appTop,
   XmNscrollBarDisplayPolicy, XmAS_NEEDED,
   XmNscrollingPolicy, XmAUTOMATIC,
   NULL );

  // create menubar
  menuBar = XmCreateMenuBar( mainWin, "", NULL, 0 );

  filePullDown = XmCreatePulldownMenu( menuBar, "", NULL, 0 );

  menuStr = XmStringCreateLocalized( appContextClass_str35 );
  fileCascade = XtVaCreateManagedWidget( "", xmCascadeButtonWidgetClass,
   menuBar,
   XmNlabelString, menuStr,
   XmNmnemonic, 'f',
   XmNsubMenuId, filePullDown,
   NULL );
  XmStringFree( menuStr );

  if ( !noEdit ) {

    str = XmStringCreateLocalized( appContextClass_str36 );
    newB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
     filePullDown,
     XmNlabelString, str,
     NULL );
    XmStringFree( str );
    XtAddCallback( newB, XmNactivateCallback, new_cb,
     (XtPointer) this );

  }

  str = XmStringCreateLocalized( appContextClass_str37 );
  newB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   filePullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( newB, XmNactivateCallback, open_cb,
   (XtPointer) this );

#if 0
  str = XmStringCreateLocalized( appContextClass_str38 );
  newB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   filePullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( newB, XmNactivateCallback, open_user_cb,
   (XtPointer) this );
#endif

  str = XmStringCreateLocalized( appContextClass_str39 );
  newB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   filePullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( newB, XmNactivateCallback, import_cb,
   (XtPointer) this );

  str = XmStringCreateLocalized( appContextClass_str40 );
  newB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   filePullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( newB, XmNactivateCallback, refreshUserLib_cb,
   (XtPointer) this );

  str = XmStringCreateLocalized( "Reload All" );
  newB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   filePullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( newB, XmNactivateCallback, reload_cb,
   (XtPointer) this );

  str = XmStringCreateLocalized( appContextClass_str41 );
  newB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   filePullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( newB, XmNactivateCallback, exit_cb,
   (XtPointer) this );

  if ( primaryServer ) {
    if ( primaryServer == 1 ) {
      str = XmStringCreateLocalized( appContextClass_str132 );
    }
    else if ( primaryServer == 2 ) {
      str = XmStringCreateLocalized( appContextClass_str133 );
    }
    newB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
     filePullDown,
     XmNlabelString, str,
     NULL );
    XmStringFree( str );
    XtAddCallback( newB, XmNactivateCallback, shutdown_cb,
     (XtPointer) this );
  }


  viewPullDown = XmCreatePulldownMenu( menuBar, "", NULL, 0 );

  menuStr = XmStringCreateLocalized( appContextClass_str42 );
  viewCascade = XtVaCreateManagedWidget( "", xmCascadeButtonWidgetClass,
   menuBar,
   XmNlabelString, menuStr,
   XmNmnemonic, 'v',
   XmNsubMenuId, viewPullDown,
   NULL );
  XmStringFree( menuStr );

  str = XmStringCreateLocalized( appContextClass_str43 );
  msgB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   viewPullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( msgB, XmNactivateCallback, view_msgBox_cb,
   (XtPointer) this );

  str = XmStringCreateLocalized( appContextClass_str44 );
  pvB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   viewPullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( pvB, XmNactivateCallback, view_pvList_cb,
   (XtPointer) this );

  str = XmStringCreateLocalized( appContextClass_str45 );
  pvB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   viewPullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( pvB, XmNactivateCallback, view_screens_cb,
   (XtPointer) this );

  str = XmStringCreateLocalized( appContextClass_str112 );
  viewXyB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   viewPullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( viewXyB, XmNactivateCallback, view_xy_cb,
   (XtPointer) this );

  str = XmStringCreateLocalized( appContextClass_str135 );
  renderImagesB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   viewPullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( renderImagesB, XmNactivateCallback, renderImages_cb,
   (XtPointer) this );

  str = XmStringCreateLocalized( appContextClass_str136 );
  checkpointPidB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   viewPullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( checkpointPidB, XmNactivateCallback, checkpointPid_cb,
   (XtPointer) this );


  pathPullDown = XmCreatePulldownMenu( menuBar, "", NULL, 0 );

  menuStr = XmStringCreateLocalized( appContextClass_str121 );
  pathCascade = XtVaCreateManagedWidget( "", xmCascadeButtonWidgetClass,
   menuBar,
   XmNlabelString, menuStr,
   XmNmnemonic, 'v',
   XmNsubMenuId, pathPullDown,
   NULL );
  XmStringFree( menuStr );

  for ( i=0; i<numPaths; i++ ) {

    curBlock = new callbackBlockType;
    curBlock->ptr = (void *) dataFilePrefix[i];
    curBlock->apco = this;

    callbackBlockTail->flink = curBlock;
    callbackBlockTail = curBlock;
    callbackBlockTail->flink = NULL;

    str = XmStringCreateLocalized( dataFilePrefix[i] );
    msgB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
     pathPullDown,
     XmNlabelString, str,
     NULL );
    XmStringFree( str );
    XtAddCallback( msgB, XmNactivateCallback, setPath_cb,
     (XtPointer) curBlock );

  }














  helpPullDown = XmCreatePulldownMenu( menuBar, "", NULL, 0 );

  menuStr = XmStringCreateLocalized( appContextClass_str114 );
  helpCascade = XtVaCreateManagedWidget( "", xmCascadeButtonWidgetClass,
   menuBar,
   XmNlabelString, menuStr,
   XmNmnemonic, 'h',
   XmNsubMenuId, helpPullDown,
   NULL );
  XmStringFree( menuStr );

  str = XmStringCreateLocalized( appContextClass_str115 );
  msgB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
   helpPullDown,
   XmNlabelString, str,
   NULL );
  XmStringFree( str );
  XtAddCallback( msgB, XmNactivateCallback, help_cb,
   (XtPointer) this );

  //str = XmStringCreateLocalized( appContextClass_str116 );
  //msgB = XtVaCreateManagedWidget( "", xmPushButtonWidgetClass,
  // helpPullDown,
  // XmNlabelString, str,
  // NULL );
  //XmStringFree( str );
  //XtAddCallback( msgB, XmNactivateCallback, help_cb,
  // (XtPointer) this );

  //mainDrawingArea = XtVaCreateManagedWidget( "", xmDrawingAreaWidgetClass,
  // mainWin,
  // NULL );

  XtVaSetValues( menuBar,
   XmNmenuHelpWidget, helpCascade,
   0 );

  XtManageChild( menuBar );

  msgDialog.createWithOffset( appTop );

}

void appContextClass::addActiveWindow (
  activeWindowListPtr node )
{

  // link in

  node->requestDelete = 0;
  node->requestActivate = 0;
  node->requestActivateClear = 0;
  node->requestReactivate = 0;
  node->requestOpen = 0;
  node->requestPosition = 0;
  node->requestCascade = 0;
  node->requestImport = 0;
  node->requestRefresh = 0;
  node->requestActiveRedraw = 0;
  node->requestIconize = 0;

  node->blink = head->blink;
  head->blink->flink = node;
  head->blink = node;
  node->flink = head;

}

int appContextClass::refreshActiveWindow (
  activeWindowClass *activeWindowNode )
{

  // simply mark for refresh and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      if ( !cur->requestRefresh ) {
        cur->requestRefresh = 1;
        requestFlag++;
      }
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::smartDrawAllActive (
  activeWindowClass *activeWindowNode )
{

  // simply mark for redraw and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      if ( !cur->requestActiveRedraw ) {
        cur->requestActiveRedraw = 1;
        requestFlag++;
      }
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::removeActiveWindow (
  activeWindowClass *activeWindowNode )
{

  // simply mark for delete and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestDelete = 1;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::openEditActiveWindow (
  activeWindowClass *activeWindowNode )
{

  // simply mark for open and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestOpen = 1;
      cur->requestPosition = 0;
      cur->requestCascade = 0;
      cur->requestImport = 0;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::openActivateActiveWindow (
  activeWindowClass *activeWindowNode )
{

  // simply mark for open and activation and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestOpen = 1;
      cur->requestPosition = 0;
      cur->requestCascade = 0;
      cur->requestImport = 0;
      cur->requestIconize = 0;
      requestFlag++;
      cur->requestActivate = 1;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::openActivateActiveWindow (
  activeWindowClass *activeWindowNode,
  int x,
  int y )
{

  // simply mark for open and activation and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestOpen = 1;
      cur->requestPosition = 1;
      cur->requestCascade = 0;
      cur->requestImport = 0;
      cur->requestIconize = 0;
      cur->x = x;
      cur->y = y;
      requestFlag++;
      cur->requestActivate = 1;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::openActivateIconifiedActiveWindow (
  activeWindowClass *activeWindowNode,
  int x,
  int y )
{

  // simply mark for open and activation and increment request flag
  // request window to be iconified

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestOpen = 1;
      cur->requestPosition = 1;
      cur->requestCascade = 0;
      cur->requestImport = 0;
      cur->requestIconize = 1;
      cur->x = x;
      cur->y = y;
      requestFlag++;
      cur->requestActivate = 1;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::openActivateCascadeActiveWindow (
  activeWindowClass *activeWindowNode )
{

  // simply mark for open and activation and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestOpen = 1;
      cur->requestPosition = 0;
      cur->requestCascade = 1;
      cur->requestImport = 0;
      cur->requestIconize = 0;
      requestFlag++;
      cur->requestActivate = 1;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::openActivateCascadeActiveWindow (
  activeWindowClass *activeWindowNode,
  int x,
  int y )
{

  // simply mark for open and activation and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestOpen = 1;
      cur->requestPosition = 1;
      cur->requestCascade = 1;
      cur->requestImport = 0;
      cur->requestIconize = 0;
      requestFlag++;
      cur->requestActivate = 1;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::activateActiveWindow (
  activeWindowClass *activeWindowNode )
{

  // simply mark for activation and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestActivate = 1;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

int appContextClass::reactivateActiveWindow (
  activeWindowClass *activeWindowNode )
{

  // simply mark for reactivation and increment request flag

activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( &(cur->node) == activeWindowNode ) {
      cur->requestReactivate = 1;
      requestFlag++;
    }
    cur = cur->flink;
  }

  return 1;

}

static void displayVersion ( void ) {

  printf( "  edm version %s Copyright (C) 1999 John W. Sinclair\n", VERSION );
  printf( "\n" );

  printf( "  This program is free software; you can redistribute it and/or modify\n" );
  printf( "  it under the terms of the GNU General Public License as published by\n" );
  printf( "  the Free Software Foundation; either version 2 of the License, or\n" );
  printf( "  (at your option) any later version.\n" );
  printf( "\n" );

printf( "  This program is distributed in the hope that it will be useful,\n" );
  printf( "  but WITHOUT ANY WARRANTY; without even the implied warranty of\n" );
  printf( "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" );
  printf( "  GNU General Public License for more details.\n" );
  printf( "\n" );
 
printf( "  You should have received a copy of the GNU General Public License\n" );
  printf( "  along with this program; if not, write to the Free Software\n" );
  printf( "  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n" );

}

static void displayParamInfo ( void ) {

  printf( "\n" );
  displayVersion();

  printf( "\n" );
   printf( global_str24 );
  printf( "\n" );

  printf( global_str25 );

#ifdef GENERIC_PV

  printf( global_str26 );

#else

  printf( global_str27 );

#endif

  printf( global_str28 );

  printf( global_str29 );

  printf( global_str30 );
  printf( global_str31 );

  printf( global_str32 );
  printf( global_str33 );

#ifdef GENERIC_PV
  printf( global_str34 );
#endif

  printf( global_str35 );
  printf( global_str36 );
  printf( global_str37 );
  printf( global_str87 );
  printf( global_str88 );

  printf( global_str77 );
  printf( global_str78 );
  printf( global_str85 );

  printf( global_str57 );
  printf( global_str58 );
  printf( global_str59 );

  printf( global_str74 );

  printf( global_str60 );
  printf( global_str61 );

  printf( global_str38 );

  printf( global_str39 );

  printf( global_str40 );

  printf( global_str41 );
  printf( global_str42 );
  printf( global_str43 );
  printf( global_str44 );
  printf( global_str45 );

  printf( global_str66 );

  printf( global_str67 );

  printf( global_str68 );
  printf( global_str69 );
  printf( global_str70 );
  printf( global_str71 );
  printf( global_str72 );

  printf( global_str46 );
  printf( global_str47 );
  printf( "\n" );
  printf( global_str75 );
  printf( "\n" );
  printf( global_str48 );
  printf( "\n" );
  printf( global_str49 );
  printf( "\n" );
  printf( global_str50 );
  printf( "\n" );
  printf( global_str51 );
  printf( global_str52 );
  printf( "\n" );
  printf( global_str53 );
  printf( "\n" );
  printf( global_str80 );
  printf( global_str81 );
  printf( global_str82 );
  printf( global_str83 );
  printf( "\n" );
  printf( global_str84 );
  printf( "\n" );
  printf( global_str54 );
  printf( "\n" );

}

#define DONE -1
#define SWITCHES 1
#define FILES 2

int appContextClass::getParams(
  int argc,
  char **argv )
{

char buf[1023+1], mac[1023+1], exp[1023+1];
int state = SWITCHES;
int i, n = 1;
char *tk;
macroListPtr curMacro;
fileListPtr curFile;

#ifdef GENERIC_PV
  strcpy( pvClassName, "" );
#endif

  strcpy( displayName, "" );
  strcpy( colormode, "" );
  local = 0;
  privColorMap = 0;

  // check first for component management commands
  if ( argc > 1 ) {

    if ( ( strcmp( argv[1], global_str6 ) == 0 ) ||
         ( strcmp( argv[1], global_str7 ) == 0 ) ||
         ( strcmp( argv[1], global_str8 ) == 0 ) ) {
      if ( argc < 3 ) {
        printf( appContextClass_str77 );
        displayParamInfo();
        exit(0);
      }
      manageComponents( argv[1], argv[2] );
      printf( "\n\n" );
      exit(0);
    }

  }

  if ( argc > 1 ) {

    if ( ( strcmp( argv[1], global_str63 ) == 0 ) ||
         ( strcmp( argv[1], global_str64 ) == 0 ) ||
         ( strcmp( argv[1], global_str65 ) == 0 ) ) {
      if ( argc < 3 ) {
        printf( appContextClass_str77 );
        displayParamInfo();
        exit(0);
      }
      managePvComponents( argv[1], argv[2] );
      printf( "\n\n" );
      exit(0);
    }

  }

  while ( n < argc ) {

    switch ( state ) {

    case SWITCHES:

      if ( argv[n][0] == '-' ) {

        if ( strcmp( argv[n], global_str11 ) == 0 ) {
          displayParamInfo();
          exit(0);
        }
        else if ( strcmp( argv[n], global_str12 ) == 0 ) {
          displayParamInfo();
          exit(0);
        }
        else if ( strcmp( argv[n], global_str13 ) == 0 ) {
          displayParamInfo();
          exit(0);
        }
        else if ( strcmp( argv[n], global_str14 ) == 0 ) {
          printf( "\n" );
          displayVersion();
          printf( "\n" );
          exit(0);
        }
        else if ( strcmp( argv[n], global_str15 ) == 0 ) {
          printf( "\n" );
          displayVersion();
          printf( "\n" );
          exit(0);
        }
        else if ( strcmp( argv[n], global_str16 ) == 0 ) {
          printf( "\n" );
          displayVersion();
          printf( "\n" );
          exit(0);
        }
        else if ( strcmp( argv[n], global_str17 ) == 0 ) {
          executeOnOpen = 1;
        }
        else if ( strcmp( argv[n], global_str9 ) == 0 ) {
          local = 1;
        }
        else if ( strcmp( argv[n], global_str10 ) == 0 ) {
        }
        else if ( strcmp( argv[n], global_str86 ) == 0 ) {
          n++; // just ignore, not used here
          if ( n >= argc ) return 2;
        }
        else if ( strcmp( argv[n], global_str18 ) == 0 ) {
          noEdit = 1;
        }
        else if ( strcmp( argv[n], global_str19 ) == 0 ) {
          n++;
          if ( n >= argc ) return 2; // missing macro arg
          strncpy( buf, argv[n], 1023 );
          tk = strtok( buf, "=," );
          while ( tk ) {
            strncpy( mac, tk, 1023 );
            tk = strtok( NULL, "=," );
            if ( tk ) {
              if ( strcmp( tk, "\"\"" ) == 0 ) {
                strcpy( exp, "" );
	      }
              else if ( strcmp( tk, "\'\'" ) == 0 ) {
                strcpy( exp, "" );
	      }
              else if ( strcmp( tk, "\\'\\'" ) == 0 ) {
                strcpy( exp, "" );
	      }
	      else {
                strncpy( exp, tk, 1023 );
	      }
            }
            else {
              return 4; // macro, but no value
            }
            numMacros++;
            curMacro = new macroListType;
            if ( !curMacro ) return 6;
            curMacro->macro = new char[strlen(mac)+1];
            if ( !curMacro->macro ) return 6;
            strcpy( curMacro->macro, mac );
            curMacro->expansion = new char[strlen(exp)+1];
            if ( !curMacro->expansion ) return 6;
            strcpy( curMacro->expansion, exp );
            macroHead->blink->flink = curMacro;
            curMacro->flink = macroHead;
            curMacro->blink = macroHead->blink;
            macroHead->blink = curMacro;
            tk = strtok( NULL, "=," );
          }

          if ( numMacros == 0 ) return 4;

        }
        else if ( strcmp( argv[n], global_str20 ) == 0 ) {
          n++;
          if ( n >= argc ) return 2;
          strncpy( ctlPV, argv[n], 127 );
        }
        else if ( strcmp( argv[n], global_str22 ) == 0 ) {
          n++;
          if ( n >= argc ) return 2;
          strncpy( userLib, argv[n], 127 );
          userLibObject.openUserLibrary( userLib );
        }
        else if ( strcmp( argv[n], global_str21 ) == 0 ) {
          n++;
          if ( n >= argc ) return 2;
          strncpy( displayName, argv[n], 127 );
        }
        else if ( strcmp( argv[n], global_str73 ) == 0 ) {
          n++; // just ignore, not used here
          if ( n >= argc ) return 2;
        }
        else if ( strcmp( argv[n], global_str76 ) == 0 ) {
          n++;
          if ( n >= argc ) return 2;
          strncpy( colormode, argv[n], 7 ); // index (default) or rgb
        }
        else if ( strcmp( argv[n], global_str79 ) == 0 ) { // private colormap
          privColorMap = 1;
        }

#ifdef GENERIC_PV
        else if ( strcmp( argv[n], global_str23 ) == 0 ) {
          n++;
          if ( n >= argc ) return 2;
          strncpy( pvClassName, argv[n], 15 );
          pvClassName[15] = 0;
        }
#endif

        else {
          return -2;
        }

        n++;

      }
      else {

        state = FILES;

      }

      break;

    case FILES:

      if ( argv[n][0] == '-' ) {

        return 8;

      }
      else {

        numFiles++;
        curFile = new fileListType;
        if ( !curFile ) return 6;
        curFile->file = new char[strlen(argv[n])+1];
        if ( !curFile->file ) return 6;
        strcpy( curFile->file, argv[n] );
        fileHead->blink->flink = curFile;
        curFile->flink = fileHead;
        curFile->blink = fileHead->blink;
        fileHead->blink = curFile;

        n++;

      }

      break;

    }

  }

  if ( numMacros > 0 ) {
    macros = new char *[numMacros];
    expansions = new char *[numMacros];
    i = 0;
    curMacro = macroHead->flink;
    while ( curMacro != macroHead ) {
      macros[i] = curMacro->macro;
      expansions[i] = curMacro->expansion;
      i++;
      curMacro = curMacro->flink;
    }
  }

  return 1;

}

int appContextClass::startApplication (
  int argc,
  char **argv,
  int _primaryServer )
{

int stat, opStat;
activeWindowListPtr cur;
char *name, *envPtr;
char prefix[127+1], fname[127+1], msg[127+1];
fileListPtr curFile;
expStringClass expStr;
Atom wm_delete_window;
XTextProperty xtext;
char title[31+1], *pTitle;
int n;
Arg args[10];
XmString xmStr1;

  primaryServer = _primaryServer;

  name = argv[0];

  stat = getParams( argc, argv );
  if ( !( stat & 1 ) ) {
    switch ( stat ) {
    case 2:
      printf( appContextClass_str93 );
      goto err_return;
    case 4:
      printf( appContextClass_str94 );
      goto err_return;
    case 6:
      printf( appContextClass_str95 );
      goto err_return;
    case 8:
      printf( appContextClass_str96 );
      goto err_return;
    case -2:
      printf( appContextClass_str97 );
      goto err_return;
    }
err_return:
    printf( global_str55 );
    exitFlag = 1;
    return 0; // error
  }

  stat = initDeferredExecutionQueue();
  if ( !( stat & 1 ) ) {
    exitFlag = 1;
    return 0; // error
  }


#ifdef __epics__

  if ( strcmp( ctlPV, "" ) != 0 ) {

    stat = ca_search( ctlPV, &ctlPvId );
    if ( stat != ECA_NORMAL ) {

      sprintf( msg, appContextClass_str99 );
      postMessage( msg );

    }
    else {

      stat = ca_pend_io( 10.0 );
      ca_pend_event( 0.00001 );
      if ( stat != ECA_NORMAL ) {

        sprintf( msg, appContextClass_str100 );
        postMessage( msg );

      }
      else {

        stat = ca_put( DBR_STRING, ctlPvId, "" );
        stat = ca_pend_io( 10.0 );
        ca_pend_event( 0.00001 );

        stat = ca_add_masked_array_event( DBR_STRING, 1, ctlPvId,
         ctlPvUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
         &ctlPvEventId, DBE_VALUE );
        if ( stat != ECA_NORMAL ) {
          sprintf( msg,
           appContextClass_str101 );
          postMessage( msg );
        }

        usingControlPV = 1;

      }

    }

  }

#else

#ifdef GENERIC_PV

  if ( strcmp( ctlPV, "" ) != 0 ) {

    ctlPvId = pvObj.createNew( pvClassName );
    if ( ctlPvId ) {

      ctlPvId->createEventId( &ctlPvEventId );

      expStr.setRaw( ctlPV );
      stat = ctlPvId->search( &expStr );
      if ( stat != PV_E_SUCCESS ) {

        sprintf( msg, appContextClass_str102 );
        postMessage( msg );

      }
      else {

        stat = ctlPvId->pendIo( 10.0 );
        if ( stat != PV_E_SUCCESS ) {

          sprintf( msg, appContextClass_str103 );
          postMessage( msg );

        }
        else {

          stat = ctlPvId->put( ctlPvId->pvrString(), (void *) "" );
          stat = ctlPvId->pendIo( 10.0 );

          stat = ctlPvId->addEvent( ctlPvId->pvrString(), 1,
	   ctlPvUpdate, (void *) this, ctlPvEventId,
	   ctlPvId->pveValue() );
          if ( stat != PV_E_SUCCESS ) {
            sprintf( msg,
             appContextClass_str104 );
            postMessage( msg );
          }

          usingControlPV = 1;

        }

      }

    }

  }

#endif

#endif

  {

  int argCount = 3;
  char *args[3] = { "edm", global_str21, displayName };

    if ( strcmp( displayName, "" ) == 0 ) argCount = 1;

#if 0
    if ( executeOnOpen ) {
      appTop = XtVaAppInitialize( &app, "edm", NULL, 0, &argCount,
       args, NULL,
       XmNiconic, True,
       NULL );
      iconified = 1;
    }
    else {
      appTop = XtVaAppInitialize( &app, "edm", NULL, 0, &argCount,
       args, NULL,
       XmNiconic, False,
       XmNmappedWhenManaged, False,
       NULL );
      iconified = 0;
    }
#endif

#if 1
    if ( g_needXtInit ) {
      g_needXtInit = 0;
      XtToolkitInitialize();
      XtSetLanguageProc( NULL, NULL, NULL );
    }

    app = XtCreateApplicationContext();

    display = XtOpenDisplay( app, NULL, NULL, "edm", NULL, 0, &argCount,
     args );

    if ( executeOnOpen ) {
      appTop = XtVaAppCreateShell( NULL, "edm", applicationShellWidgetClass,
       display,
       XmNiconic, True,
       NULL );
      iconified = 1;
    }
    else {
      appTop = XtVaAppCreateShell( NULL, "edm", applicationShellWidgetClass,
       display,
       XmNiconic, False,
       XmNmappedWhenManaged, False,
       NULL );
      iconified = 0;
    }
#endif

  }

  this->createMainWindow();

  clipbdInit( appTop );

  XtRealizeWidget( appTop );

  display = XtDisplay( appTop );

  strcpy( title, "edm " );
  Strncat( title, VERSION, 31 );
  pTitle = title;
  XStringListToTextProperty( &pTitle, 1, &xtext );
  XSetWMName( display, XtWindow(appTop), &xtext );
  XSetWMIconName( display, XtWindow(appTop), &xtext );
  XFree( xtext.value );

  processAllEvents( app, display );

  wm_delete_window = XmInternAtom( display, "WM_DELETE_WINDOW", False );
  XmAddWMProtocolCallback( appTop, wm_delete_window,
   exit_cb, (XtPointer) this );
  XtVaSetValues( appTop, XmNdeleteResponse, XmDO_NOTHING, NULL );

  largestH = XDisplayHeight( display, DefaultScreen(display) );

  msgBox.create( appTop, 0, 0, 5000, NULL, NULL );

  pvList.create( appTop, 20 );

  n = 0;
  xmStr1 = XmStringCreateLocalized( "*.edl" );
  XtSetArg( args[n], XmNpattern, xmStr1 ); n++;

  fileSelectBox = XmCreateFileSelectionDialog( appTop, "", args, n );

  XmStringFree( xmStr1 );

  XtAddCallback( fileSelectBox, XmNcancelCallback,
   app_fileSelectCancel_cb, (void *) this );
  XtAddCallback( fileSelectBox, XmNokCallback,
   app_fileSelectOk_cb, (void *) this );


  n = 0;
  xmStr1 = XmStringCreateLocalized( "*.xch" );
  XtSetArg( args[n], XmNpattern, xmStr1 ); n++;

  importSelectBox = XmCreateFileSelectionDialog( appTop, "", args, n );

  XmStringFree( xmStr1 );

  XtAddCallback( importSelectBox, XmNcancelCallback,
   app_importSelectCancel_cb, (void *) this );
  XtAddCallback( importSelectBox, XmNokCallback,
   app_importSelectOk_cb, (void *) this );

  envPtr = getenv(environment_str2);
  if ( envPtr ) {
    strncpy( prefix, envPtr, 127 );
    if ( prefix[strlen(prefix)-1] != '/' ) Strncat( prefix, "/", 127 );
  }
  else {
    strcpy( prefix, "/etc/edm/" );
  }

  strncpy( fname, prefix, 127 );
  Strncat( fname, "colors.list", 127 );

  if ( privColorMap ) {
    ci.usePrivColorMap();
  }

  opStat = ci.initFromFile( app, display, appTop, fname );

  if ( strcmp( colormode, "rgb" ) == 0 ) {
    ci.useRGB();
  }
  else if ( strcmp( colormode, "RGB" ) == 0 ) {
    ci.useRGB();
  }

  if ( ci.colorModeIsRGB() ) {
    postMessage( appContextClass_str129 );
    postMessage( appContextClass_str130 );
  }

  if ( !( opStat & 1 ) ) {
    printf( appContextClass_str106 );
    exitFlag = 1;
    return 0; // error
  }

  if ( ci.majorVersion() == 3 ) {
    postMessage( appContextClass_str131 );
  }

  XSetWindowColormap( display, XtWindow(pvList.top()), ci.getColorMap() );

  postNote( appContextClass_str117 );

  processAllEvents( app, display );

  strncpy( fname, prefix, 127 );
  Strncat( fname, "fonts.list", 127 );
  opStat = fi.initFromFile( app, display, fname );

  closeNote();

  if ( !( opStat & 1 ) ) {
    printf( appContextClass_str107 );
    exitFlag = 1;
    return 0; // error
  }

  displayScheme.setAppCtx( this );
  displayScheme.loadDefault( &ci );

  curFile = fileHead->flink;
  while ( curFile != fileHead ) {

    cur = new activeWindowListType;
    cur->requestDelete = 0;
    cur->requestActivate = 0;
    cur->requestActivateClear = 0;
    cur->requestReactivate = 0;
    cur->requestOpen = 0;
    cur->requestPosition = 0;
    cur->requestCascade = 0;
    cur->requestImport = 0;
    cur->requestRefresh = 0;
    cur->requestActiveRedraw = 0;
    cur->requestIconize = 0;

    cur->blink = head->blink;
    head->blink->flink = cur;
    head->blink = cur;
    cur->flink = head;

    cur->node.create( this, NULL, 0, 0, 0, 0, numMacros, macros, expansions );
    cur->node.realize();
    cur->node.setGraphicEnvironment( &ci, &fi );

    cur->node.storeFileName( curFile->file );

    cur->requestOpen = 1;
    requestFlag++;

    if ( executeOnOpen ) {
      cur->requestActivate = 1;
      requestFlag++;
    }

    curFile = curFile->flink;

  }

  iconTestCount = 0;

  if ( !iconified ) XtMapWidget( appTop );

  return 1;

}

int appContextClass::addActWin (
  char *name,
  int x,
  int y,
  int numMacs,
  char **syms,
  char **exps )
{

activeWindowListPtr cur;

  cur = new activeWindowListType;
  cur->requestDelete = 0;
  cur->requestActivate = 0;
  cur->requestActivateClear = 0;
  cur->requestReactivate = 0;
  cur->requestOpen = 0;
  cur->requestPosition = 0;
  cur->requestCascade = 0;
  cur->requestImport = 0;
  cur->requestRefresh = 0;
  cur->requestActiveRedraw = 0;
  cur->requestIconize = 0;

  cur->blink = head->blink;
  head->blink->flink = cur;
  head->blink = cur;
  cur->flink = head;

  cur->node.create( this, NULL, x, y, 0, 0, numMacs, syms, exps );
  cur->node.realize();
  cur->node.setGraphicEnvironment( &ci, &fi );

  cur->node.storeFileName( name );

  cur->requestOpen = 1;
  requestFlag++;

  if ( executeOnOpen ) {
    cur->requestActivate = 1;
    requestFlag++;
  }

  return 1;

}


void appContextClass::applicationLoop ( void ) {

static int msgCount = 0;
int stat, nodeCount, actionCount, iconNodeCount,
 iconActionCount, n;
activeWindowListPtr cur, next;
char msg[127+1];

  if ( epc.printEvent() ) {
    if ( epc.printCmdReady() ) {
      epc.doPrint();
    }
    if ( epc.printFinished() ) {
      postNote( appContextClass_str138 );
      msgCount = 20;
    }
  }

  if ( msgCount > 0 ) {
    if ( msgCount == 1 ) {
      closeNote();
    }
    msgCount--;
  }

  if ( reloadFlag == 2 ) {
    refreshAll();
    reloadFlag = 0;
  }
  else if ( reloadFlag == 1 ) {
    reloadAll();
    reloadFlag = 2;
  }

  if ( requestFlag ) {

    cur = head->flink;
    while ( cur != head ) {
      if ( !cur->requestDelete ) {
        if ( cur->requestRefresh ) {
          cur->requestRefresh = 0;
          if ( requestFlag > 0 ) requestFlag--;
          cur->node.refreshActive();
        }
        if ( cur->requestActiveRedraw ) {
          cur->requestActiveRedraw = 0;
          if ( requestFlag > 0 ) requestFlag--;
          cur->node.smartDrawAllActive();
        }
      }
      cur = cur->flink;
    }

    nodeCount = iconNodeCount = actionCount = iconActionCount = 0;
    // traverse list and delete nodes so marked
    cur = head->flink;
    while ( cur != head ) {
      next = cur->flink;
      nodeCount++;
      if ( cur->requestDelete ) {
        actionCount++;
        iconActionCount++;
        cur->blink->flink = cur->flink;
        cur->flink->blink = cur->blink;
        removeAllDeferredExecutionQueueNode( &cur->node );
        delete cur;
        if ( requestFlag > 0 ) requestFlag--;
      }
      cur = next;
    }

    /* if all windows have been removed then deiconify main window */
    if ( iconNodeCount == iconActionCount ) {
      if ( !usingControlPV ) deiconifyMainWindow();
    }

    cur = head->flink;
    while ( cur != head ) {
      if ( cur->requestOpen ) {
        cur->requestOpen = 0;
        if ( cur->requestImport ) {
          cur->requestImport = 0;
          stat = cur->node.import();
        }
        else {
          if ( cur->requestPosition ) {
            cur->requestPosition = 0;
            if ( cur->requestCascade ) {
              cur->requestCascade = 0;
              stat = cur->node.loadCascade( cur->x, cur->y );
            }
            else {
              stat = cur->node.load( cur->x, cur->y );
            }
          }
          else if ( cur->requestCascade ) {
            cur->requestPosition = 0;
            cur->requestCascade = 0;
            stat = cur->node.loadCascade();
          }
          else {
            stat = cur->node.load();
          }
        }
        if ( !reloadFlag ) {
          XtMapWidget( XtParent( cur->node.drawWidgetId() ) );
	}

        if ( !( stat & 1 ) ) {
          sprintf( msg, appContextClass_str108, cur->node.fileName );
          postMessage( msg );
          cur->requestDelete = 1;
          cur->requestActivate = 0;
          cur->requestReactivate = 0;
          XtUnmanageChild( cur->node.drawWidgetId() );
          XtUnmapWidget( XtParent( cur->node.drawWidgetId() ) );
          if ( requestFlag > 0 ) requestFlag--;
          if ( cur->requestActivate == 1 ) {
            cur->requestActivate = 0;
            if ( requestFlag > 0 ) requestFlag--;
          }
          if ( requestFlag == 0 ) requestFlag = 1; // because req Del = 1
        }
        else {
          if ( requestFlag > 0 ) requestFlag--;
          if ( cur->requestActivate == 1 ) cur->node.setNoRefresh();
          if ( cur->requestReactivate == 1 ) cur->node.setNoRefresh();
        }

      }
      cur = cur->flink;
    }

    cur = head->flink;
    while ( cur != head ) {
      if ( cur->requestActivate == 1 ) cur->requestActivate = 2;
      if ( cur->requestReactivate == 1 ) cur->requestReactivate = 2;
      cur = cur->flink;
    }

    processAllEvents( app, display );

    cur = head->flink;
    while ( cur != head ) {
      if ( !cur->requestDelete ) {
        if ( cur->requestActivate == 3 ) {
          if ( requestFlag > 0 ) requestFlag--;
          cur->requestActivate = 0;
          if ( cur->requestActivateClear ) {
            cur->requestActivateClear = 0;
            cur->node.clearActive();
            cur->node.refreshActive();
	  }
	}
      }
      cur = cur->flink;
    }

    nodeCount = iconNodeCount = actionCount = iconActionCount = 0;
    cur = head->flink;
    while ( cur != head ) {
      nodeCount++;
      iconNodeCount++;
      if ( cur->node.isActive() ) {
        actionCount++;
        iconActionCount++;
      }
      if ( !cur->requestDelete ) {
        if ( cur->requestActivate == 2 ) {
          cur->requestActivate = 3;
          cur->node.execute();

#ifdef __epics__
          stat = ca_pend_io( 1.0 );
          ca_pend_event( 0.00001 );
#endif
          cur->node.processObjects();

#ifdef __epics__
          stat = ca_pend_io( 1.0 );
          ca_pend_event( 0.00001 );
#endif
          processAllEvents( app, display );

          actionCount++;
          iconActionCount++;
          cur->node.setRefresh();
          cur->node.refreshActive();

          // need to iconify?
          if ( cur->requestIconize ) {
            cur->requestIconize = 0;
            XIconifyWindow( display,
             XtWindow(XtParent(cur->node.executeWidget)),
             DefaultScreen(display) );
            cur->node.isIconified = 1;
          }

        }
        if ( cur->requestReactivate == 2 ) {
          cur->requestReactivate = 0;
          cur->node.reexecute();
          actionCount++;
          if ( requestFlag > 0 ) requestFlag--;
          cur->node.setRefresh();
          cur->node.refreshActive();
        }
      }

      cur = cur->flink;

    }

    processAllEvents( app, display );

    /* if all windows have been activated then iconify main window */
    if ( ( iconNodeCount == iconActionCount ) && ( iconNodeCount != 0 ) ) {
      if ( !usingControlPV ) iconifyMainWindow();
    }
    else {
      if ( !usingControlPV ) deiconifyMainWindow();
    }

  }

  processDeferredExecutionQueue();

  n = 0;
  cur = head->flink;
  while ( cur != head ) {

    n++;

    // invoke the executeDeferred function for all activeGraphicClass
    // objects (display elements) for a given activeWindowClass object
    // (display screen)
    cur->node.processObjects();

    iconTestCount++;
    if ( iconTestCount > 10 ) { // periodically, check if iconified
      iconTestCount = 0;
      XtVaGetValues( cur->node.topWidgetId(),
       XmNiconic, &cur->node.isIconified,
       NULL );
    }

#ifdef __epics__
    stat = ca_pend_io( 1.0 );
    ca_pend_event( 0.00001 );
#endif
    processAllEvents( app, display );

    cur = cur->flink;

  }

#ifdef __epics__
  stat = ca_pend_io( 10.0 );
  ca_pend_event( 0.00001 );
#endif

  processAllEvents( app, display );

  raiseMessageWindow();

}

XtAppContext appContextClass::appContext ( void )
{

  return app;

}

Widget appContextClass::fileSelectBoxWidgetId ( void )
{

  return fileSelectBox;

}

Widget appContextClass::importSelectBoxWidgetId ( void )
{

  return importSelectBox;

}

void appContextClass::postMessage (
  char *msg )
{

  msgBox.addText( msg );

}

void appContextClass::raiseMessageWindow ( void )
{

  msgBox.raise();

}

void appContextClass::iconifyMainWindow ( void ) {

  return;

  if ( !iconified ) {
    XIconifyWindow( display, XtWindow(appTop), DefaultScreen(display) );
    iconified = 1;
  }

}

void appContextClass::deiconifyMainWindow ( void ) {

  return;

  if ( iconified ) {
    XUnmapWindow( display, XtWindow(appTop) );
    XMapWindow( display, XtWindow(appTop) );
    iconified = 0;
  }

}

int appContextClass::setProperty (
  char *winId,
  char *id,
  char *property,
  char *value )
{

int stat;
activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( strcmp( winId, cur->node.id ) == 0 ) {
      stat = cur->node.setProperty( id, property, value );
      return stat;
    }
    cur = cur->flink;
  }

  return 0;

}

int appContextClass::setProperty (
  char *winId,
  char *id,
  char *property,
  double *value )
{

int stat;
activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( strcmp( winId, cur->node.id ) == 0 ) {
      stat = cur->node.setProperty( id, property, value );
      return stat;
    }
    cur = cur->flink;
  }

  return 0;

}

int appContextClass::setProperty (
  char *winId,
  char *id,
  char *property,
  int *value )
{

int stat;
activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( strcmp( winId, cur->node.id ) == 0 ) {
      stat = cur->node.setProperty( id, property, value );
      return stat;
    }
    cur = cur->flink;
  }

  return 0;

}

int appContextClass::getProperty (
  char *winId,
  char *id,
  char *property,
  int bufSize,
  char *value )
{

int stat;
activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( strcmp( winId, cur->node.id ) == 0 ) {
      stat = cur->node.getProperty( id, property, bufSize, value );
      return stat;
    }
    cur = cur->flink;
  }

  return 0;

}

int appContextClass::getProperty (
  char *winId,
  char *id,
  char *property,
  double *value )
{

int stat;
activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( strcmp( winId, cur->node.id ) == 0 ) {
      stat = cur->node.getProperty( id, property, value );
      return stat;
    }
    cur = cur->flink;
  }

  return 0;

}

int appContextClass::getProperty (
  char *winId,
  char *id,
  char *property,
  int *value )
{

int stat;
activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    if ( strcmp( winId, cur->node.id ) == 0 ) {
      stat = cur->node.getProperty( id, property, value );
      return stat;
    }
    cur = cur->flink;
  }

  return 0;

}

void appContextClass::reopenUserLib ( void ) {

  if ( strcmp( userLib, "" ) != 0 ) {
    userLibObject.openUserLibrary( userLib );
  }

}

void appContextClass::xSynchronize (
  int onoff
) {

  XSynchronize( display, (Bool) onoff );

}

void appContextClass::exitProgram ( void ) {

  exit_cb ( NULL, this, NULL );

}

void appContextClass::findTop ( void ) {

  XUnmapWindow( display, XtWindow(appTop) );
  XMapWindow( display, XtWindow(appTop) );
  XRaiseWindow( display, XtWindow(appTop) );

}

void appContextClass::postNote ( 
  char *msg ) {

int _x, _y;

  _x = XDisplayWidth( display, DefaultScreen(display) ) / 2;
  _y = XDisplayHeight( display, DefaultScreen(display) ) / 2;

  msgDialog.popup( msg, _x, _y );

}

void appContextClass::closeNote ( void ) {

  msgDialog.popdown();

}

int appContextClass::numScreens ( void ) {

  // return number of open screens

int count = 0;
activeWindowListPtr cur;

  cur = head->flink;
  while ( cur != head ) {
    count++;
    cur = cur->flink;
  }

  return count;

}

void appContextClass::performShutdown (
  FILE *f ) {

  if ( !saveContextOnExit ) {
    shutdownFilePtr = f;
    saveContextOnExit = 1;
    exitFlag = 1;
    //abort_cb( (Widget) NULL, (XtPointer) this, (XtPointer) NULL );
  }

}

int appContextClass::getShutdownFlag ( void )
{

  return shutdownFlag;

}

int appContextClass::renderImages ( void ) {

  return renderImagesFlag;

}

void appContextClass::setRenderImages (
  int flag
) {

  renderImagesFlag = flag;

}

int appContextClass::openCheckPointScreen (
  char *screenName,
  int x,
  int y,
  int icon,
  int noEdit,
  int numCheckPointMacros,
  char *checkPointMacros
) {

activeWindowListPtr cur;
int n, stat;
char *newMacros[100+1];
char *newValues[100+1];

  //printf( "appContextClass::openCheckPointScreen\n" );

  if ( numCheckPointMacros ) {
    stat = parseSymbolsAndValues( checkPointMacros, 100,
     newMacros, newValues, &n );
  }
  else {
    n = 0;
  }

  //printf( "screenName = [%s]\n", screenName );
  //printf( "x = %-d\n", x );
  //printf( "y = %-d\n", y );
  //printf( "icon = %-d\n", icon );
  //printf( "numCheckPointMacros = %-d\n", numCheckPointMacros );
  //printf( "checkPointMacros = [%s]\n", checkPointMacros );

  //printf( "found %-d macros\n", n );
  //for ( i=0; i<n; i++ ) {
  //  printf( "sym=[%s]  val=[%s]\n", newMacros[i], newValues[i] );
  //}

  cur = new activeWindowListType;

  addActiveWindow( cur );

  if ( n > 0 ) {
    if ( noEdit ) {
      cur->node.createNoEdit( this, NULL, 0, 0, 0, 0,
       n, newMacros, newValues );
    }
    else {
      cur->node.create( this, NULL, 0, 0, 0, 0,
       n, newMacros, newValues );
    }
  }
  else {
    if ( noEdit ) {
      cur->node.createNoEdit( this, NULL, 0, 0, 0, 0,
       0, NULL, NULL );
    }
    else {
      cur->node.create( this, NULL, 0, 0, 0, 0,
       0, NULL, NULL );
    }
  }

  cur->node.realize();

  cur->node.setGraphicEnvironment( &ci, &fi );

  cur->node.storeFileName( screenName );

  cur->node.noRaise = 1;
  cur->node.isIconified = True;

  if ( icon ) {
    stat = openActivateIconifiedActiveWindow( &cur->node, x, y );
  }
  else {
    stat = openActivateActiveWindow( &cur->node, x, y );
  }

  processAllEventsWithSync( app, display );

  return 1;

}
