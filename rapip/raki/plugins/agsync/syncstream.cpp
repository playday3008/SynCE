#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WITH_AGSYNC

#include <AGReader.h>
#include <AGWriter.h>
#include <AGRecord.h>
#include <AGBufferWriter.h>
#include <AGUserConfig.h>
#include <AGDBConfig.h>
#include <AGDeviceInfo.h>

/* Helper for errors */
int asErrno;

int readResult(AGReader *r) {
  int res= AGReadCompactInt(r);

  if(2 == res)
    asErrno= AGReadCompactInt(r);

  return res;
}




/* Though it doesn't really matter in our case, the commands
   and responses are both technically CompactInt's rather than Int8's.
*/

int asEndSession(AGReader *r, AGWriter *w) {
  /* Command code 0 */
  AGWriteCompactInt(w, 0x00);

  /* Regardless of result (even error), return it. */
  return readResult(r);
}


AGUserConfig *asGetUserConfig(AGReader *r, AGWriter *w, AGUserConfig *uc) {
  AGWriteCompactInt(w,  0x09);
  
  /* Result code */
  if(0 != readResult(r))
    return NULL;

  /* Read size
     TODO: We could use this opportunity to read everything into a buffer,
           and then read from there. It might be faster, but I doubt
           it'll make a big difference on fast computers. So we do it
           when I have time.
  */
  /*discard= */AGReadCompactInt(r);

  /* Read the config */
  if(AG_ERROR_NONE == AGUserConfigReadData(uc, r))
    return uc;
  else
    return NULL;
}


int asPutUserConfig(AGReader *r, AGWriter *w, AGUserConfig *uc) {
  /* Prepare buffer ahead of time */
  /* The size here is just a guess, but should cover most people
     with only minimal reallocation.
  */
  AGBufferWriter *bW= AGBufferWriterNew(0x200);
  AGUserConfigWriteData(uc, (AGWriter *)bW);

  /* Write command code 0x0A */
  AGWriteCompactInt(w, 0x0A);

  /* Write size of buffer */
  AGWriteCompactInt(w, AGBufferWriterGetBufferSize(bW));

  /* Write buffer */
  AGWriteBytes(w, AGBufferWriterGetBuffer(bW),
	       AGBufferWriterGetBufferSize(bW));

  /* Deallocate our buffered writer */
  AGBufferWriterFree(bW);

  return readResult(r);
}


int asStartServer(AGReader *r, AGWriter *w, int uid) {
  /* Command code 0x07, followed by uid */
  AGWriteCompactInt(w, 0x07);
  AGWriteCompactInt(w, uid);
  return readResult(r);
}

int asEndServer(AGReader *r, AGWriter *w) {
  /* Command code 0x08 */
  AGWriteCompactInt(w, 0x08);

  return readResult(r);
}


AGDeviceInfo *asGetDeviceInfo(AGReader *r, AGWriter *w, AGDeviceInfo *devInfo){
  /* Command code 0x0B */
  AGWriteCompactInt(w, 0x0B);

  /* Read result */
  if(0 != readResult(r))
    return NULL;

  /* Stupid function doesn't return any error codes! */
  AGDeviceInfoReadData(devInfo, r);
  return devInfo;
}


int asGetNextRecord(AGReader *r, AGWriter *w, AGRecord **agrPtr) {
  int result;

  /* Command code 0x04 */
  AGWriteCompactInt(w, 0x04);

  result= readResult(r);
  if(1 == result) {
    *agrPtr= AGRecordNew(0, (AGRecordStatus) 0, 0, NULL, 0, NULL);
    AGRecordReadData(*agrPtr, r);
  } else {
    *agrPtr= NULL;
  }

  return result;
}


int asGetNextModifiedRecord(AGReader *r, AGWriter *w,AGRecord **agrPtr) {
  int result;

  /* Command code 0x05 */
  AGWriteCompactInt(w, 0x05);

  result= readResult(r);
  if(1 == result) {
    *agrPtr= AGRecordNew(0, (AGRecordStatus) 0, 0, NULL, 0, NULL);
    AGRecordReadData(*agrPtr, r);
  } else {
    *agrPtr= NULL;
  }

  return result;
}


int asOpenDatabase(AGReader *r, AGWriter *w, AGDBConfig *db) {
  /* Command code 0x03. Followup of AGDBConfig. */
  AGWriteCompactInt(w, 0x03);
  AGDBConfigWriteData(db, w);

  return readResult(r);
}


int asPerformCommand(AGReader */*r*/, AGWriter *w, int cmd,
		     unsigned char *cmdData, int len) {
  /* Command code 0x02. */
  AGWriteCompactInt(w, 0x02);
  
  /* Write command! */
  AGWriteCompactInt(w, cmd);

  /* Write length */
  AGWriteCompactInt(w, len);

  /* Write command data */
  AGWriteBytes(w, (void *)cmdData, len);

  // There is no return: return AGCLIENT_CONTINUE
  // to keep the sync process going strong.
  return 1;
}

#endif
