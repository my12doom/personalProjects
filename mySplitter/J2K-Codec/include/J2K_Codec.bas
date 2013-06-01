Attribute VB_Name = "J2K_Codec"
' J2K-Codec ver 2.0

Public Enum J2K_ERRORS
    J2K_SUCCESS = 0
    J2K_NOT_ENOUGH_MEMORY
    J2K_CORRUPTED_DATA
    J2K_CANT_READ_DATA
    J2K_INVALID_ARGUMENT
    J2K_CANCELED
    J2K_CANT_OPEN_FILE
    J2K_OPTION_UNRECOGNIZED
    J2K_NO_SUCH_TILE
    J2K_NO_SUCH_RESOLUTION
    J2K_BPP_TOO_SMALL
    J2K_NOT_PART1_FORMAT
    J2K_IMAGE_IS_TOO_LARGE
    J2K_TOO_MANY_RES_LEVEL
    J2K_TOO_LARGE_CODEBLOCKS
    J2K_LAZINESS_NOT_SUPPORTED
    J2K_VCAUSAL_NOT_SUPPORTED
    J2K_TOO_MANY_COMPONENTS
    J2K_BAD_COMPONENT_PRECISION
    J2K_ONLY_UNSIG_COMPONENTS
    J2K_DOWNSAMPLED_COMPONENTS
    J2K_ROI_NOT_SUPPORTED
    J2K_PROGR_CHANGE_NOT_SUP
    J2K_64BIT_BOXES_NOT_SUP
    J2K_INTERNAL_ERROR

    J2K_MAX_ERROR_LEN = 60
End Enum

Public Const J2K_VERSION As Long = &H22


'//////////////////////////////////////////////////////////////////////////////////////////
'// Main structure, representing a J2K image
'//
'// [out] means that the field is returned from J2K-Codec
'// [in]  means that the field is set and passed to J2K-Codec
'//////////////////////////////////////////////////////////////////////////////////////////

Public Type J2K_Image
    Width As Long          ' [out] Width and height of the image in pixels (by default).
    Height As Long         '       If "resolution" is set or tiles were selected - this will be set to the correct size
    Components As Long     ' [out] The number of components in the image
    Resolutions As Long    ' [out] Number of resolution levels for selected tiles (by default - the whole image is selected)
    hTiles As Long         ' [out] The number of tiles in horizontal direction
    vTiles As Long         ' [out] The number of tiles in vertical direction
    Precision As Long      ' [out] Component precision in bits. All components must have the same precision.
    FileType As Long       ' [out] The image file type (0 = Lossy J2K, 1 = Lossy JP2, 2 = Lossless J2K, 3 = Lossless JP2)
    ColorInfo As Long      ' [out] Color information (lowest 2 bits - MCT type, highest 29 bits - color info) - see below

    version As Long        ' [in]  J2K-Codec version your program was developed for. Use J2K_VERSION to initialize this field
    resolution As Long     ' [in]  Resolution level. If it is >= Resolutions it will be set to the maximum available level

    buffer As Long         ' [in]  Destination buffer pointer
    buffer_bpp As Long     ' [in]  Destination buffer bytes-per-pixel value
    buffer_pitch As Long   ' [in]  Destination buffer pitch
    
    reserved As Long       ' Reserved

    j2k As Long            ' [out,in] Pointer to the internal class for this image
End Type


'//////////////////////////////////////////////////////////////////////////////////////////
'// Color information
'//  Lowest 2 bits   - Multi-Component Transform type (None, Reversible, Irreversible)
'//  Highest 29 bits - Color space (sRGB, Grayscale, YCC, ICC, RICC)
'//////////////////////////////////////////////////////////////////////////////////////////

Public Enum J2K_COLOR_INFO
    CINFO_MCT_NONE = 0  'MCT not applied
    CINFO_MCT_REV = 1   'Reversible
    CINFO_MCT_IRREV = 2 'Irreversible

    CINFO_INDEXED = 4   'Palette present

    CINFO_BI = 0
    CINFO_YCC1 = 1
    CINFO_YCC2 = 3
    CINFO_YCC3 = 4
    CINFO_PHOTO_YCC = 9
    CINFO_CMY = 11
    CINFO_CMYK = 12
    CINFO_YCCK = 13
    CINFO_LAB = 14
    CINFO_SRGB = 16
    CINFO_GREY = 17
    CINFO_BI2 = 18
    CINFO_JAB = 19
    CINFO_ESRGB = 20
    CINFO_ROMMRGB = 21
    CINFO_SRGB_YCC = 22
    CINFO_YPP1125 = 23
    CINFO_YPP1250 = 24
    CINFO_ICC = 16384
    CINFO_RICC = 32768
End Enum

Declare Function J2K_GetVersion Lib "j2k-codec" () As Long

Declare Sub J2K_GetErrorStr Lib "j2k-codec" Alias "J2K_GetErrorStrA" (ByVal ErrCode As Long, ByVal errStr As String)

Declare Function J2K_GetErrStr(ByVal ErrCode As Long) As String
	Dim errStr As String * J2K_MAX_ERROR_LEN

	J2K_GetErrorStr ErrCode, errStr

	J2K_GetErrStr = errStr
End Function

Declare Sub J2K_Unlock Lib "j2k-codec" Alias "J2K_UnlockA" (ByVal Key As String)

Declare Function J2K_StartLogging Lib "j2k-codec" () As Long
Declare Sub J2K_StopLogging Lib "j2k-codec" ()

Declare Function J2K_OpenFile Lib "j2k-codec" Alias "J2K_OpenFileA" (ByVal FileName As String, Image As J2K_Image) As Long
Declare Function J2K_OpenMemory Lib "j2k-codec" (ByRef buffer As Byte, ByVal Size As Long, Image As J2K_Image) As Long

Declare Function J2K_GetInfo Lib "j2k-codec" (Image As J2K_Image) As Long

Declare Function J2K_SelectTiles Lib "j2k-codec" (Image As J2K_Image, ByVal StartTile As Long, ByVal EndTile As Long, ByVal Action As Long) As Long

Declare Function J2K_Decode Lib "j2k-codec" Alias "J2K_DecodeA" (Image As J2K_Image, Optional ByVal Options As String = "") As Long

Declare Sub J2K_Close Lib "j2k-codec" (Image As J2K_Image)
