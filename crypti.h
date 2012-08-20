#ifndef __CRYPTI_H_
#define __CRYPTI_H_

#define DEBUG

//WIP debug macro
#ifdef DEBUG
#define D(s) do { \
s;} while(0)
#else
#define D(s) 
#endif

#endif

