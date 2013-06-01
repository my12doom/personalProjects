using System;
using System.Runtime.InteropServices;
using System.Text;

// Application namespace - change this to match your application if needed
namespace J2K_CodecPrj
{

    public enum J2K_Error
	{
		Success = 0,
		NotEnoughMemory,
		CorruptedData,
		CantReadData,
		InvalidArgument,
		Canceled,
		CantOpenFile,
		OptionUnrecognized,
		NoSuchTile,
		NoSuchResolution,
		BppTooSmall,
		NotPart1Format,
		ImageIsTooLarge,
		TooManyResLevel,
		TooLargeCodeblocks,
		LazinessNotSupported,
		VCausalNotSupported,
		TooManyComponents,
		BadComponentPrecision,
		OnlyUnsigComponents,
		DownsampledComponents,
		RoiNotSupported,
		ProgrChangeNotSup,
		No64BitSupport,
		InternalError,

		MaxErrors,

		MaxErrorLen = 60
	}

	public enum J2K_Metadata
	{
		CommentStr,
		CommentBin,
		Geotiff,
		Xml,
		Url,
		Pal,
		Colr,
		Icc,
		Unknown
	}

    [StructLayout(LayoutKind.Sequential, Pack = 4)]    
	public struct J2K_Image 
	{
		public int Width; 			// [out] Width and height of the image in pixels (by default)
		public int Height;  		//       If "resolution" is set or tiles were selected - this will be set to the correct size

		public int Components;		// [out] The number of components in the image
		public int Resolutions;		// [out] Number of resolution levels for selected tiles (by default - the whole image is selected)
		public int hTiles, vTiles;	// [out] The number of tiles in horizontal and vertical directions
		public int Precision;		// [out] Component precision in bits. All components must have the same precision.
		public int FileType;		// [out] The image file type (0 = Lossy J2K, 1 = Lossy JP2, 2 = Lossless J2K, 3 = Lossless JP2)
		public int ColorInfo;		// [out] Color information (lowest 2 bits - MCT type, 3rd bit - indexed or not, highest 28 bits - color info) - see below

		public int version;			// [in]  J2K-Codec version your program was developed for. Use J2K_Codec.Version to initialize this field
		public int resolution;		// [in]  Resolution level. If it is >= Resolutions it will be set to the maximum available level

		public IntPtr buffer;		// [in]  Destination buffer
		public int buffer_bpp;		// [in]  Destination buffer bytes-per-pixel value
		public int buffer_pitch;	// [in]  Destination buffer pitch (distance in bytes between two vertical pixels)

 		public int reserved;		// Reserved

		public IntPtr j2k;			// [out,in] Pointer to the internal class for this image
	}

    public abstract class J2K_Codec
	{
        public const int Version = 0x22; // compatibility version
            
        // Returns codec version and build number in the 0x205678 form, where 
		// 20 is a version (2.0) and 5678 is a build number.
		[DllImport("j2k-codec", EntryPoint="J2K_GetVersion")]
		public static extern int GetVersion();
		
		[DllImport("j2k-codec", EntryPoint="J2K_GetErrorStrA")]
		private static extern void GetErrorString(J2K_Error errCode, StringBuilder errStr);

		// Returns the textual description of an error
		public static string GetErrStr(J2K_Error errCode)
		{
            StringBuilder sb = new StringBuilder((int)J2K_Error.MaxErrorLen);
			GetErrorString(errCode, sb);
			return sb.ToString();
		}

		// This function unlocks the full functionality of J2K-Codec. After you
		// have purchased your personal registration key, you need to pass it 
		// to this function.
		[DllImport("j2k-codec", EntryPoint="J2K_UnlockA")]
		public static extern void Unlock(string key);


		// These functions control the debug messages logging.
		//
		// J2K_StartLogging() returns J2K_SUCCESS if there was no error.
		// NOTES: 
		//  1. Log-file name is 'j2k-codec.log'
		//  2. The performance will degrade significantly if the logging is on.
		[DllImport("j2k-codec", EntryPoint="J2K_StartLogging")]
		public static extern J2K_Error StartLogging();

		// Use this function to stop logging, initiated by StartLogging().
		[DllImport("j2k-codec", EntryPoint="J2K_StopLogging")]
		public static extern void StopLogging();

		// Open an image from a file with given name.
		[DllImport("j2k-codec", EntryPoint="J2K_OpenFileA")]
		public static extern J2K_Error OpenFile(string fileName, ref J2K_Image image);

		// Open the image from a buffer with 'size' length
		[DllImport("j2k-codec", EntryPoint="J2K_OpenMemory")]
		public static extern J2K_Error OpenMemory(byte[] buffer, uint size, ref J2K_Image image);

		// Update information from the image
		[DllImport("j2k-codec", EntryPoint="J2K_GetInfo")]
		public static extern J2K_Error GetInfo(ref J2K_Image image);
                     
		// Get meta data, embedded into the JP2 image. See help file for parameter explanation
		[DllImport("j2k-codec", EntryPoint="J2K_GetMetadata")]
		public static extern J2K_Error GetMetadata(ref J2K_Image image, int no, ref J2K_Metadata type, ref IntPtr data, ref int size);


		// Selects or unselects a tile or a tile range, depending on 'select'.
		// If select is true then the tiles are selected for decoding, 
		// otherwise they are unselected. The range is defined by start and end
		// tile numbers (inclusive) in the raster order. If end_tile==-1 then
		// the max tile number will be used instead.
		[DllImport("j2k-codec", EntryPoint="J2K_SelectTiles")]
		public static extern J2K_Error SelectTiles(ref J2K_Image image, int startTile, int endTile, bool select);
	
		// Decodes the image, previously created with Open().
		[DllImport("j2k-codec", EntryPoint="J2K_DecodeA")]
		public static extern J2K_Error Decode(ref J2K_Image image, string options);

        
		// Destroys the image, previously created by Open(). All images must 
		// be closed using this function to avoid memory leaks.
		[DllImport("j2k-codec", EntryPoint="J2K_Close")]
		public static extern J2K_Error Close(ref J2K_Image image);
	}
}
