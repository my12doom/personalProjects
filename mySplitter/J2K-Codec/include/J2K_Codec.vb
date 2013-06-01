Imports System.Text, System.Runtime.InteropServices
Module J2K_Codec

    Public Const Version As Integer = &H22

    Public Enum J2K_Error
        Success = 0
        NotEnoughMemory
        CorruptedData
        CantReadData
        InvalidArgument
        Canceled
        CantOpenFile
        OptionUnrecognized
        NoSuchTile
        NoSuchResolution
        BppTooSmall
        NotPart1Format
        ImageIsTooLarge
        TooManyResLevel
        TooLargeCodeblocks
        LazinessNotSupported
        VCausalNotSupported
        TooManyComponents
        BadComponentPrecision
        OnlyUnsigComponents
        DownsampledComponents
        RoiNotSupported
        ProgrChangeNotSup
        No64BitSupport
        InternalError

		MaxErrors

		MaxErrorLen = 60
    End Enum

    Public Enum J2K_Metadata
        CommentStr
        CommentBin
        Geotiff
        Xml
        Url
        Pal
        Colr
        Icc
        Unknown
    End Enum

    Public CodingSchemes() As String = {"Lossy J2K", "Lossy JP2", "Lossless J2K", "Lossless JP2"}

    Public Structure J2K_Image
        Dim Width As Integer            ' [out] Width and height of the image in pixels (by default)
        Dim Height As Integer           '       If "resolution" is set or tiles were selected - this will be set to the correct size

        Dim Components As Integer       ' [out] The number of components in the image
        Dim Resolutions As Integer      ' [out] Number of resolution levels for selected tiles (by default - the whole image is selected)
        Dim hTiles, vTiles As Integer   ' [out] The number of tiles in horizontal and vertical directions
        Dim Precision As Integer        ' [out] Component precision in bits. All components must have the same precision.
        Dim FileType As Integer         ' [out] The image file type (0 = Lossy J2K, 1 = Lossy JP2, 2 = Lossless J2K, 3 = Lossless JP2)
        Dim ColorInfo As Integer        ' [out] Color information (lowest 2 bits - MCT type, 3rd bit - indexed or not, highest 28 bits - color info) - see below

        Dim version As Integer          ' [in]  J2K-Codec version your program was developed for. Use J2K_Codec.Version to initialize this field
        Dim resolution As Integer       ' [in]  Resolution level. If it is >= Resolutions it will be set to the maximum available level

        Dim buffer As IntPtr            ' [in]  Destination buffer
        Dim buffer_bpp As Integer       ' [in]  Destination buffer bytes-per-pixel value
        Dim buffer_pitch As Integer     ' [in]  Destination buffer pitch (distance in bytes between two vertical pixels)

        Dim reserved As Integer         ' Reserved

        Dim j2k As IntPtr               ' [out,in] Pointer to the internal class for this image
    End Structure

    ' Returns codec version and build number in the 0x205678 form, where 
    ' 20 is a version (2.0) and 5678 is a build number.
    Declare Function GetVersion Lib "j2k-codec" Alias "J2K_GetVersion" () As Integer

    ' Returns the textual description of an error
    Declare Sub GetErrorStr Lib "j2k-codec" Alias "J2K_GetErrorStrA" (ByVal ErrCode As Integer, ByVal errStr As String)

    Declare Function GetErrStr(ByVal ErrCode As Integer) As String
        Dim charstr(J2K_Error.MaxErrorLen) As Char
        Dim errStr As String = CStr(charstr)

        GetErrorStr(ErrCode, errStr)

		GetErrStr = errStr
	End Function

    ' This function unlocks the full functionality of J2K-Codec. After you
    ' have purchased your personal registration key, you need to pass it 
    ' to this function.
    Declare Sub Unlock Lib "j2k-codec" Alias "J2K_UnlockA" (ByVal Key As String)

	' These functions control the debug messages logging.
	'
	' J2K_StartLogging() returns J2K_SUCCESS if there was no error.
	' NOTES: 
	'  1. Log-file name is 'j2k-codec.log'
	'  2. The performance will degrade significantly if the logging is on.

    Declare Function StartLogging Lib "j2k-codec" Alias "J2K_StartLogging" () As Integer
    Declare Sub  StopLogging Lib "j2k-codec" Alias "J2K_StopLogging" ()

    ' Open an image from a file.
    Declare Function OpenFile Lib "j2k-codec" Alias "J2K_OpenFileA" (ByVal FileName As String, ByRef image As J2K_Image) As Integer

    ' Open the image from a buffer with 'size' length
    Declare Function OpenMemory Lib "j2k-codec" Alias "J2K_OpenMemory" (ByRef Buffer As Byte, ByVal Size As Integer, ByRef image As J2K_Image) As Integer

    ' Update information from the image
    Declare Function GetInfo Lib "j2k-codec" Alias "J2K_GetInfo" (ByRef image As J2K_Image) As Integer

    ' Get meta data, embedded into the JP2 image. See help file for parameter explanation
    Declare Function GetMetadata Lib "j2k-codec" Alias "J2K_GetMetadata" (ByRef image As J2K_Image, ByVal No As Integer, ByRef Type As Integer, ByRef Buffer As IntPtr, ByRef Size As Integer) As Integer

    ' Selects or unselects a tile or a tile range, depending on 'select'.
    ' If select is true then the tiles are selected for decoding, 
    ' otherwise they are unselected. The range is defined by start and end
    ' tile numbers (inclusive) in the raster order. If end_tile==-1 then
    ' the max tile number will be used instead.
    Declare Function SelectTiles Lib "j2k-codec" Alias "J2K_SelectTiles" (ByRef image As J2K_Image, ByVal StartTile As Integer, ByVal EndTile As Integer, ByVal Action As Integer) As Integer

    ' Decodes the image, previously created with Open().
    Declare Function Decode Lib "j2k-codec" Alias "J2K_DecodeA" (ByRef image As J2K_Image, Optional ByVal Options As String = "") As Integer

    ' Destroys the image, previously created by Open(). All images must 
    ' be closed using this function to avoid memory leaks.
    Declare Sub Close Lib "j2k-codec" Alias "J2K_Close" (ByRef image As J2K_Image)
End Module
