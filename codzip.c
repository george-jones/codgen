//
// Written by David Cornewell
// Zips files into a single file.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#ifdef unix
# include <unistd.h>
# include <utime.h>
# include <sys/types.h>
# include <sys/stat.h>
#else
# include <direct.h>
# include <io.h>
#endif

#include "zip.h"

#ifdef WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif


// start code borrowed from minizip.c
/* calculate the CRC32 of a file,
   because to encrypt a file, we need known the CRC32 of the file before */
int getFileCrc(const char* filenameinzip,void*buf,unsigned long size_buf,unsigned long* result_crc)
{
   unsigned long calculate_crc=0;
   int err=ZIP_OK;
   FILE * fin = fopen(filenameinzip,"rb");
   unsigned long size_read = 0;
   unsigned long total_read = 0;
   if (fin==NULL)
   {
       err = ZIP_ERRNO;
   }

    if (err == ZIP_OK)
        do
        {
            err = ZIP_OK;
            size_read = (int)fread(buf,1,size_buf,fin);
            if (size_read < size_buf)
                if (feof(fin)==0)
            {
                printf("error in reading %s\n",filenameinzip);
                err = ZIP_ERRNO;
            }

            if (size_read>0)
                calculate_crc = crc32(calculate_crc,buf,size_read);
            total_read += size_read;

        } while ((err == ZIP_OK) && (size_read>0));

    if (fin)
        fclose(fin);

    *result_crc=calculate_crc;
    printf("file %s crc %x\n",filenameinzip,calculate_crc);
    return err;
}


#ifdef WIN32
uLong filetime(f, tmzip, dt)
    char *f;                /* name of file to get info on */
    tm_zip *tmzip;             /* return value: access, modific. and creation times */
    uLong *dt;             /* dostime */
{
  int ret = 0;
  {
      FILETIME ftLocal;
      HANDLE hFind;
      WIN32_FIND_DATA  ff32;

      hFind = FindFirstFile(f,&ff32);
      if (hFind != INVALID_HANDLE_VALUE)
      {
        FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
        FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
        FindClose(hFind);
        ret = 1;
      }
  }
  return ret;
}
#else
#ifdef unix
uLong filetime(f, tmzip, dt)
    char *f;               /* name of file to get info on */
    tm_zip *tmzip;         /* return value: access, modific. and creation times */
    uLong *dt;             /* dostime */
{
  int ret=0;
  struct stat s;        /* results of stat() */
  struct tm* filedate;
  time_t tm_t=0;

  if (strcmp(f,"-")!=0)
  {
    char name[MAXFILENAME+1];
    int len = strlen(f);
    if (len > MAXFILENAME)
      len = MAXFILENAME;

    strncpy(name, f,MAXFILENAME-1);
    /* strncpy doesnt append the trailing NULL, of the string is too long. */
    name[ MAXFILENAME ] = '\0';

    if (name[len - 1] == '/')
      name[len - 1] = '\0';
    /* not all systems allow stat'ing a file with / appended */
    if (stat(name,&s)==0)
    {
      tm_t = s.st_mtime;
      ret = 1;
    }
  }
  filedate = localtime(&tm_t);

  tmzip->tm_sec  = filedate->tm_sec;
  tmzip->tm_min  = filedate->tm_min;
  tmzip->tm_hour = filedate->tm_hour;
  tmzip->tm_mday = filedate->tm_mday;
  tmzip->tm_mon  = filedate->tm_mon ;
  tmzip->tm_year = filedate->tm_year;

  return ret;
}
#else
uLong filetime(f, tmzip, dt)
    char *f;                /* name of file to get info on */
    tm_zip *tmzip;             /* return value: access, modific. and creation times */
    uLong *dt;             /* dostime */
{
    return 0;
}
#endif
#endif

// end code borrowed from minizip.c
#include "codzip.h"
int CreateZIP(char *zipfilename, FILE2ZIP *files)
{
	zipFile zf;
	FILE* fin;
	FILE2ZIP *infile;
	zip_fileinfo zi;
	char buf[1024];
	unsigned long crcFile=0, size_read=0;
	int ret=0, err;
	int opt_overwrite=1; //overwrite
//	int opt_overwrite=2; //append/update
	int opt_compress_level=5;

	//
	// Open zip file to write / append to.
	//
#ifdef USEWIN32IOAPI
	zlib_filefunc_def ffunc;
	fill_win32_filefunc(&ffunc);
	zf = zipOpen2(zipfilename,(opt_overwrite==2) ? 2 : 0,NULL,&ffunc);
#else
	zf = zipOpen(zipfilename,(opt_overwrite==2) ? 2 : 0);
#endif

	if (zf) {
		//
		// Go through files and zip them...
		//
		infile=files;
		while (infile) {
			zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
			zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
			zi.dosDate = 0;
			zi.internal_fa = 0;
			zi.external_fa = 0;
			filetime(infile->abs_path,&zi.tmz_date,&zi.dosDate);

			err = getFileCrc(infile->abs_path,buf,sizeof(buf),&crcFile);

			err = zipOpenNewFileInZip3(zf,infile->path_in_zip,&zi,
						 NULL,0,NULL,0,NULL /* comment*/,
						 (opt_compress_level != 0) ? Z_DEFLATED : 0,
						 opt_compress_level,0,
						 /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
						 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
						 NULL/*password*/,crcFile);
			if (err==ZIP_OK) {
				fin = fopen(infile->abs_path,"rb");
				if (fin) {
					while ( (size_read=fread(buf,1,sizeof(buf),fin)) > 0) {
						err = zipWriteInFileInZip (zf,buf,size_read);
						if (err<0) {
							printf("Error writing %s to zip file.\n", infile->path_in_zip);
							break;
						}
					}
					ret=1;
					fclose(fin);
				}
				zipCloseFileInZip(zf);
			} else {
				printf("error in opening %s in zipfile\n",infile->path_in_zip);
				break;
			}
			infile=infile->next;
		}

        zipClose(zf,NULL);
	} else {
		printf("error opening %s\n",zipfilename);
//		err= ZIP_ERRNO;
	}
	return ret;
}
