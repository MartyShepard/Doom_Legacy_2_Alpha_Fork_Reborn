#ifndef filesrch_h
#define filesrch_h 1


#define MAXSEARCHDEPTH 20

filestatus_t I_Filesearch(char *filename, char *startpath, unsigned char *wantedmd5sum,
bool completepath, int maxsearchdepth);

#endif // __FILESRCH_H__
