
// Common file to link J2K-Codec automatically

#include <stdio.h>
#include <stdlib.h>

#include "..\..\include\j2k-codec.cpp"

#ifndef _MSC_VER // Non-MS compiler, use MSVC 6.0 libs
	#define _MSC_VER 1200
#endif

#ifdef J2K_STATIC // Static linking (add-on)

	#ifdef J2K_16BITS
		#define PATH_16BIT "16-bit\\\\" // 16-bit version
	#else
		#define PATH_16BIT ""			// PRO version (8-bit)
	#endif

	#if(_MSC_VER == 1200) // MSVC 6.0

		#pragma message("Static MSVC 6.0 libraries")

		#if _MT
			#if _DLL
				#ifdef _DEBUG
					#pragma comment(lib, "..\\..\\..\\lib\\MSVC.60\\j2k-static-mtd-dbg.lib") 
				#else
					#pragma comment(lib, "..\\..\\..\\lib\\MSVC.60\\j2k-static-mtd.lib") 
				#endif
			#else
				#ifdef _DEBUG
					#pragma comment(lib, "..\\..\\..\\lib\\MSVC.60\\j2k-static-mt-dbg.lib") 
				#else
					#pragma comment(lib, "..\\..\\..\\lib\\MSVC.60\\j2k-static-mt.lib") 
				#endif
			#endif
		#else
			#ifdef _DEBUG
				#pragma comment(lib, "..\\..\\..\\lib\\MSVC.60\\"PATH_16BIT"j2k-static-st-dbg.lib") 
			#else
				#pragma comment(lib, "..\\..\\..\\lib\\MSVC.60\\"PATH_16BIT"j2k-static-st.lib") 
			#endif
		#endif

	#elif(_MSC_VER > 1200) // .NET

		#pragma message("Static VS2005 libraries")

		#ifdef _DEBUG
			#pragma comment(lib, "..\\..\\..\\lib\\MSVC.80\\j2k-static-mt-dbg.lib") 
		#else
			#pragma comment(lib, "..\\..\\..\\lib\\MSVC.80\\j2k-static-mt.lib") 
		#endif

	#else

		#error Need at least MSVC v6.0

	#endif

#else // Dynamic linking

	#pragma message("Using DLL")

	#pragma comment(lib, "..\\..\\..\\lib\\j2k-dynamic.lib") // Dynamic linking

#endif
