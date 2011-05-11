#include <tchar.h>
#include <d3dx9.h>
#include <stdio.h>
#include <atlbase.h>

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc<4)
	{
		printf("Invalid Arguments!.\n");
		printf("PixelShaderCompiler.exe inputfile outputfile functionname\n");
		return -1;
	}

	USES_CONVERSION;

	LPD3DXBUFFER pCode = NULL;
	LPD3DXBUFFER pErrorMsgs = NULL;
	HRESULT hr = D3DXCompileShaderFromFile(argv[1], NULL, NULL, T2A(argv[3]), "ps_2_0", 0, &pCode, &pErrorMsgs, NULL);
	if ((FAILED(hr)))
	{
		if (pErrorMsgs != NULL)
		{
			unsigned char* message = (unsigned char*)pErrorMsgs->GetBufferPointer();
			printf((LPSTR)message);
		}
		return E_FAIL;
	}
	else
	{
		FILE*f = _tfopen(argv[2], _T("wb"));
		if (f)
		{
			int size = pCode->GetBufferSize();
			fwrite(pCode->GetBufferPointer(), 1, pCode->GetBufferSize(), f);
			fflush(f);
			fclose(f);

			_tprintf(_T("PixelShaderCompiler: \"%s\" of %s Compiled to %s\n"), argv[3], argv[1], argv[2]);
		}
		else
		{
			_tprintf(_T("Failed Writing %s.\n"), argv[2]);
		}
	}

	return 0;
}