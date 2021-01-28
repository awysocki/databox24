
#ifndef DBGLOG_H
#define DBGLOG_H

void InitDBGLog( char *_Key, char * _FileName, int _Debug, int _Verbose );
void WriteDBGLog( char *_Str );
void WriteRunLog( char *_Str );

#endif
