
int GetBitMapData(LPTSTR filepath, DWORD*pdata, int width, int height);
unsigned char clip_0_255(int in);
DWORD RGB2YUV(DWORD rgb);
DWORD YUV2RGB(DWORD yuv);
void mix(unsigned char *pDst, int width, int height, int stride);
void gen(unsigned char *pDst, int width, int height, int stride);
