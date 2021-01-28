/*****************************************************************************/
/*                                                                           */
/* FUNCTION: Parse.c -									                     */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"

#define stricmp strcasecmp
#define strnicmp strncasecmp


static char sBuf[1024*10];

int
GetIniString(char *_AppName,
             char *_KeyName,
             char *_Default,
             char *_String,
             int  _StringSize,
             char *_FileName)
{
FILE    *fh;
char    appname[255];
char    *c_ptr, *c_ptre;
int     rc;
int     loop = 1;
int     len;

  strcpy(_String, _Default);            /* Set the default string       */

  fh = fopen(_FileName, "r");

  if (fh)
    {
    if (_AppName[0] != '[')             /* Check for ['s around name    */
      {                                 /* Add them on                  */
      sprintf(appname, "[%s]", _AppName);
      }
    else                                /* Already there, don't add them*/
      strcpy(appname, _AppName);

    len = strlen(appname);

    rc = INI_ANF;
    while (loop)                        /* Search for Application Name  */
      {
      c_ptr = fgets(sBuf, sizeof(sBuf), fh);

      if (c_ptr)
        {
        while (*c_ptr && *c_ptr <= ' ')     /* Remove leading blanks        */
          c_ptr++;
                                        /* Check AppName                */
        if (c_ptr[0] == '[' && strnicmp(appname, c_ptr, len) == 0)
          {

          rc = INI_KNF;                 /* Found AppName                */
                                        /* Set next possible error      */
          len = strlen(_KeyName);

          while (loop)
            {
            c_ptr = fgets(sBuf, sizeof(sBuf), fh);

            if (c_ptr)                  /* Read in a line               */
              {
              while (*c_ptr && *c_ptr <= ' ')     /* Remove leading blanks        */
                c_ptr++;

              if (*c_ptr == '[')
                {
                loop = 0;
                break;
                } /* endif */

              if ( strnicmp(_KeyName, c_ptr, len) == 0 && (c_ptr[len] == '=' || c_ptr[len] == ' ') )
                {

                c_ptr = strchr(c_ptr, '=');

                if (c_ptr)
                  {
                  c_ptr++;

                  c_ptre = strchr(c_ptr, '\n');

                  if (c_ptre)
                    *c_ptre = 0;
                  c_ptre = strchr(c_ptr, '\r');

                  if (c_ptre)
                    *c_ptre = 0;

                  len = strlen(c_ptr) + 1;/* Calc the length              */
                  if (len > _StringSize)
                    len = _StringSize;

                  strncpy(_String, c_ptr, len);
                  loop = 0;
                  rc = INI_SUCCESSFUL;

                  }

                }

              }
            else
              loop = 0;

            }

          }

        }
      else
        loop = 0;                       /* All done                     */


      } /* End While */


    fclose(fh);
    }
  else
    rc = INI_FNF;                       /* File Error                   */

return(rc);
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int
WriteIniString(char *_AppName, char *_KeyName, char *_String, char *_FileName)
{

FILE    *fh, *tfh;
char    appname[255];
char    locbuf[512];
char    *c_ptr;
int     rc;
int     loop = 1;
int     len, klen, slen;

  if (_AppName[0] != '[')               /* Check for ['s around name    */
    {                                   /* Add them on                  */
    sprintf(appname, "[%s]", _AppName);
    }
  else                                  /* Already there, don't add them*/
    strcpy(appname, _AppName);

  len = strlen(appname);
  klen = strlen(_KeyName);
  slen = strlen(_String);

  fh = fopen(_FileName, "r");

  if (fh)
    {
    tfh = tmpfile();                    /* Create a temp file           */

    rc = INI_ANF;
    while (loop)                        /* Search for Application Name  */
      {
      c_ptr = fgets(sBuf, sizeof(sBuf), fh);

      if (c_ptr)
        {
        while (*c_ptr == ' ')     /* Remove leading blanks        */
          c_ptr++;
                                        /* Check AppName                */
        if (c_ptr[0] == '[' && strnicmp(appname, c_ptr, len) == 0)
          {
          if (klen != 0)
            fputs(sBuf, tfh);

          rc = INI_KNF;                 /* Found AppName                */
                                        /* Set next possible error      */
          while (loop)
            {
            c_ptr = fgets(sBuf, sizeof(sBuf), fh);

            if (c_ptr)                  /* Read in a line               */
              {
              while (*c_ptr == ' ')     /* Remove leading blanks        */
                c_ptr++;

              if (*c_ptr == '[' || *c_ptr == '\r' || *c_ptr == 0 || *c_ptr == '\n')
                {
                sprintf(locbuf, "  %s=%s\n", _KeyName, _String);
                if (slen != 0)
                  fputs(locbuf, tfh);

                if (*c_ptr != '[' && slen == 0)
                   ;
                else
                  fputs(sBuf, tfh);
                loop = 0;
                rc = INI_SUCCESSFUL;
                break;
                } /* endif */

              else if (klen &&
                       *c_ptr != ';' &&
                       strnicmp(_KeyName, c_ptr, klen) == 0 &&
                       (c_ptr[klen] == '=' || c_ptr[klen] == ' ') )
                {
                sprintf(locbuf, "  %s=%s\n", _KeyName, _String);
                if (slen != 0)
                  fputs(locbuf, tfh);
                loop = 0;
                rc = INI_SUCCESSFUL;
                break;
                }
              else
                {
                if (klen != 0)
                  fputs(sBuf, tfh);
                }

              }
            else
              loop = 0;

            } /* End While */

          }
        else
          {
          fputs(sBuf, tfh);
          }

        }
      else
        loop = 0;                       /* All done                     */

      } /* End While */

    if (rc == INI_KNF && klen != 0 && slen != 0)
       fprintf(tfh, "  %s=%s\n\n", _KeyName, _String);

    if (rc == INI_ANF && klen != 0 && slen != 0) {
       fprintf(tfh, "%s\n", appname);
       fprintf(tfh, "  %s=%s\n\n", _KeyName, _String);
    } /* endif */
                                        /* Write out the rest of the file */
    c_ptr = fgets(sBuf, sizeof(sBuf), fh);
    while (c_ptr) {
       fputs(sBuf, tfh);
       c_ptr = fgets(sBuf, sizeof(sBuf), fh);
    } /* endwhile */
    fclose(fh);

    fh = fopen(_FileName, "w");
    fseek(tfh, 0, SEEK_SET);

    c_ptr = fgets(sBuf, sizeof(sBuf), tfh);
    while (c_ptr) {
       fputs(sBuf, fh);
       c_ptr = fgets(sBuf, sizeof(sBuf), tfh);
    } /* endwhile */

    fclose(fh);
	fclose(tfh);
    // not needed in LINUX rmtmp();

    }
  else
    {
    fh = fopen(_FileName, "w");
    fprintf(fh, "%s\n", appname);
    fprintf(fh, "  %s=%s\n\n", _KeyName, _String);
    fclose(fh);
    rc = INI_SUCCESSFUL;
    }

return(rc);
}
/*===========================================================================*/
/* Check if the variable is equal to "YES" or something close to it          */
/*===========================================================================*/
int
CheckYes(char *_Check)
{
   if (_Check[0] == 'Y' || _Check[0] == 'y' || _Check[0] == '1')
      return(1);
   if (strnicmp(_Check, "ON", 2) == 0)
      return(1);
return(0);
}




/*===========================================================================*/
/* Check if the variable is equal to "NO"  or something close to it          */
/*===========================================================================*/
int
CheckNo(char *_Check)
{
   if (_Check[0] == 'N' || _Check[0] == 'n' || _Check[0] == '0' || _Check[0] == 0)
      return(1);
   if (strnicmp(_Check, "OFF", 3) == 0)
      return(1);

return(0);
}


