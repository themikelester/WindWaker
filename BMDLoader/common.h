#ifndef BMD_COMMON_H
#define BMD_COMMON_H BMD_COMMON_H

#include <gcio.h>
#include <string>
#include <vector>
#include <cstdio>
#include <iosfwd>
#include <cstdarg> //va_list
#include <assert.h>

// debugging macros so we can pin down message origin at a glance
#define WHERESTR  "[file %s, line %d]: "
#define WHEREARG  __FILE__, __LINE__
#define DEBUGPRINT(...)       fprintf(stderr, __VA_ARGS__)
#define _LOG DEBUGPRINT("LOG" WHERESTR, WHEREARG)
#define _WARN DEBUGPRINT("WARN" WHERESTR, WHEREARG)

#define log(...)  DEBUGPRINT("LOG: " __VA_ARGS__)
#define warn(...) DEBUGPRINT("WARN: " __VA_ARGS__)

void setTextColor3f(float r, float g, float b);
void drawText(const char* s, ...);
void setStartupText(const std::string& text);

#define setStartupText(txt) 

std::string getString(int pos, Chunk* f);
void readStringtable(int pos, Chunk* f, std::vector<std::string>& dest);

//does more or less the same as readStringtable(), but writes
//the read data to an ostream instead of writing it into a vector
void writeStringtable(std::ostream& out, Chunk* f, int offset);

/**
  Splits a filename into path and basename components.
  "c:\bla/blubb\bar.ext" is split into "c:\bla/blubb\"
  and "bar.ext" for example
*/
void splitPath(const std::string& filename, std::string& folder,
               std::string& basename);

/**
  Splits a basename into name and extension.
  "bar.ext" is split into "bar" and "ext" for example.
*/
void splitName(const std::string& basename, std::string& name,
               std::string& extension);

/**
  Returns true if file with name fileName exists.
  Note that the file fileName could be deleted before you
  open it even if this function returns true.
*/
bool doesFileExist(const std::string& fileName);


#endif //BMD_COMMON_H
