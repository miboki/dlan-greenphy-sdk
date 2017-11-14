#define tftpBLOCK_LENGTH            512
#define tftpMAX_RETRIES             3
#define ipconfigTFTP_TIMEOUT_MS     5000

typedef BaseType_t ( * FTFTPReceiveCallback ) ( char * /* pucData */, BaseType_t /* xDataLength */, void * /* pvCallbackHandle */ );

BaseType_t xTFTPReceiveFile(struct freertos_sockaddr *pxAddress, const char *pcFilename, FTFTPReceiveCallback fReceiveCallback, void *pvCallbackHandle );
