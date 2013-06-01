VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Form1"
   ClientHeight    =   7050
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   11790
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   7050
   ScaleWidth      =   11790
   StartUpPosition =   3  'Windows Default
   Begin VB.PictureBox Picture1 
      Appearance      =   0  'Flat
      AutoRedraw      =   -1  'True
      BackColor       =   &H80000005&
      ForeColor       =   &H80000008&
      Height          =   6855
      Left            =   4560
      ScaleHeight     =   6825
      ScaleWidth      =   7185
      TabIndex        =   3
      Top             =   120
      Width           =   7215
   End
   Begin VB.DriveListBox Drive1 
      Height          =   315
      Left            =   0
      TabIndex        =   2
      Top             =   120
      Width           =   2415
   End
   Begin VB.DirListBox Dir1 
      Height          =   4365
      Left            =   0
      TabIndex        =   1
      Top             =   480
      Width           =   2415
   End
   Begin VB.FileListBox File1 
      Height          =   4770
      Left            =   2400
      Pattern         =   "*.j2k;*jp2"
      TabIndex        =   0
      Top             =   120
      Width           =   2055
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Const SRCCOPY = &HCC0020
Private Declare Function BitBlt Lib "gdi32" (ByVal hDestDC As Long, ByVal x As Long, ByVal y As Long, ByVal nWidth As Long, ByVal nHeight As Long, ByVal hSrcDC As Long, ByVal xSrc As Long, ByVal ySrc As Long, ByVal dwRop As Long) As Long
Private Declare Function StretchBlt Lib "gdi32" (ByVal hDestDC As Long, ByVal x As Long, ByVal y As Long, ByVal nWidth As Long, ByVal nHeight As Long, ByVal hSrcDC As Long, ByVal xSrc As Long, ByVal ySrc As Long, ByVal srcWidth As Long, ByVal srcHeight As Long, ByVal dwRop As Long) As Long
        
Private Type BITMAPINFO
   Size As Long
   Width As Long
   Height As Long
   Planes As Integer
   BitCount As Integer
   Compression As Long
   SizeImage As Long
   XPelsPerMeter As Long
   YPelsPerMeter As Long
   ClrUsed As Long
   ClrImportant As Long
   Colors As Long
End Type

Private Declare Function CreateDIBSection Lib "gdi32" (ByVal hDC As Long, binfo As BITMAPINFO, ByVal iUsage As Long, ByRef ptr As Long, ByVal hSection As Long, ByVal dwOffset As Long) As Long
Private Declare Function SelectObject Lib "gdi32" (ByVal hDC As Long, ByVal hObject As Long) As Long
Private Declare Function CreateCompatibleDC Lib "gdi32" (ByVal hDC As Long) As Long
Private Declare Function DeleteDC Lib "gdi32" (ByVal hDC As Long) As Long
Private Declare Function DeleteObject Lib "gdi32" (ByVal handle As Long) As Long

Private Sub Dir1_Change()
    File1.Path = Dir1.Path
End Sub

Private Sub Drive1_Change()
    Dir1.Path = Drive1.Drive
End Sub

Private Sub File1_Click()
    DecodeImage File1.Path + "\" + File1.FileName
End Sub

Private Sub DecodeImage(ByRef FileName As String)

    FileName = LCase(FileName)

    If InStrB(FileName, ".j2k") = 0 And InStrB(FileName, ".jp2") = 0 Then
        Picture1.Cls
        Exit Sub
    End If
        
    Dim err As Long
       
    Dim hBitmap As Long

    ' Open image file
    Dim img As J2K_Image
    img.version = J2K_VERSION
    
    err = J2K_OpenFile(FileName, img)

    ' The commented code below shows how to load JPEG2000 files from memory

    'Dim f As Integer
    'Dim dat() As Byte
    'Dim lng As Long
        
    'f = FreeFile
    'Open FileName For Binary As f
    'lng = LOF(f)
    'ReDim Preserve dat(lng)
    'Get #f, , dat
    'Close #f
       
    'err = J2K_OpenMemory(dat(0), lng, img)
        
    If err <> J2K_SUCCESS Then
       MsgBox "Failed to open file!" + Chr(13) + "Error Code: " + CStr(err) + Chr(13) + "Error Text: " + J2K_GetErrStr(err), vbOKOnly, "J2K-Codec Error"
       Exit Sub
    End If
        
                
    ' Display this information in window's title
    Me.Caption = "J2K Image: " + CStr(img.Width) + " x " + CStr(img.Height) + " x " + CStr(img.Components)
    
    ' Let's create a bitmap that we will use as a storage for
    ' the decoded image and which can be drawn fast
    
    Dim bmp As BITMAPINFO
        
    bmp.Size = Len(bmp) - 4
    bmp.Width = img.Width
    bmp.Height = img.Height
    bmp.Planes = 1
    bmp.BitCount = 32    ' Always 4 bytes per pixel
    bmp.Compression = 0
    bmp.SizeImage = 0
    bmp.XPelsPerMeter = 0
    bmp.YPelsPerMeter = 0
    bmp.ClrUsed = 0
    bmp.ClrImportant = 0
        
    Dim pBits As Long ' Pointer to bitmap's memory
    pBits = 0
                        
    hBitmap = CreateDIBSection(0, bmp, 0, pBits, 0, 0)
    If hBitmap = 0 Then
        MsgBox "Failed to create bitmap!", vbOKOnly, "System Error"
        J2K_Close img
        Exit Sub
    End If
            
    img.buffer = pBits
    img.buffer_bpp = 4
                
    err = J2K_Decode(img, "bmp=1")
    If err = J2K_SUCCESS Then
                         
        ' Let's resize PictureBox and the Form to embrace the image
        Picture1.ScaleMode = vbPixels
        Picture1.Width = (img.Width + 2) * 15
        Picture1.Height = (img.Height + 2) * 15
        
        If img.Height < 320 Then
            Me.Width = (310 + img.Width + 5) * 15
            Me.Height = (320 + 37) * 15
        Else
            Me.Width = (310 + img.Width + 5) * 15
            Me.Height = (img.Height + 37 + 10) * 15
        End If
        
        ' Finally draw the bitmap, using compatible context and BitBlt
        Dim hBitmapDC As Long
        
        hBitmapDC = CreateCompatibleDC(Picture1.hDC)
        If hBitmapDC = 0 Then
            MsgBox "Failed to create compatible DC!", vbOKOnly, "System Error"
        Else
            SelectObject hBitmapDC, hBitmap
            
            ' Here we could use the StretchBlt() Windows API function instead,
            ' to scale the image to the size of the PictureBox
            BitBlt Picture1.hDC, 0, 0, Width, Height, hBitmapDC, 0, 0, SRCCOPY
            Picture1.Refresh
            
            DeleteDC hBitmapDC
        End If
        
        ' This is an easy and slow way of displaying the decoded image
        'Dim Buffer() As Long
        'Dim color As Long
        'Dim x As Long, y As Long
        'Dim r As Long, g As Long, b As Long
        'Dim Offset As Long
        
        ' Here we would need to ReDim the Buffer to have proper size (Width*Height*4)
        ' Then call J2K_Decode for Buffer(0)
           
        'For y = 0 To Height - 1
        '    Offset = y * Width
        '    For x = 0 To Width - 1
            
                ' This must be done if Buffer is byte array
                'b = Buffer(Offset + x)
                'g = Buffer(Offset + x + 1)
                'r = Buffer(Offset + x + 2)
                'Color = r + g * 256 + b * 65536
                'Picture1.PSet (x, y), Color
                
                ' Use this if Buffer is Long (Don't forget "co=RGB" option)
                'Picture1.PSet (x, y), Buffer(Offset + X) And &HFFFFFF
                              
        '     Next X
        'Next Y
     Else
        J2K_GetErrorStr err, errStr
        MsgBox "Failed to decode file!" + Chr(13) + "Error Code: " + CStr(err) + Chr(13) + "Error Text: " + errStr, vbOKOnly, "J2K-Codec Error"
     End If
        
    DeleteObject hBitmap
    J2K_Close img

End Sub

Private Sub Form_Load()
        
    Dir1.Path = App.Path
   
    ' Let's check J2K-Codec version and whether VB will find j2k-codec.dll...
    Dim ver As Long
    Dim verMajor, verMinor, verBuild As String

    ver = J2K_GetVersion()
        
    verMajor = Hex(ver \ &H100000)
    verMinor = Hex((ver \ &H10000) And 15)
    verBuild = CStr(ver And 65535)
        
    Me.Caption = "J2K-Codec ver " + verMajor + "." + verMinor
    Me.Caption = Me.Caption + " (build " + verBuild + ")"
    
    Picture1.AutoRedraw = True
            
    ' Let's unlock J2K-Codec. Here will go your personal key,
    ' which you receive after you have purchased J2K-Codec.
    ' For now, this will do nothing.
        
    J2K_Unlock "Your Registration Key"
        
End Sub
