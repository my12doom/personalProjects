VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Form1"
   ClientHeight    =   7050
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   7440
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   7050
   ScaleWidth      =   7440
   StartUpPosition =   3  'Windows Default
   Begin VB.PictureBox Picture1 
      Appearance      =   0  'Flat
      AutoRedraw      =   -1  'True
      BackColor       =   &H80000005&
      ForeColor       =   &H80000008&
      Height          =   6855
      Left            =   120
      ScaleHeight     =   6825
      ScaleWidth      =   7185
      TabIndex        =   0
      Top             =   120
      Width           =   7215
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


Private Sub DecodeImage(ByRef FileName1 As String, ByRef FileName2 As String)

    Dim hBitmap As Long
    Dim err1 As Long, err2 As Long

    ' Open image files
    Dim img1 As J2K_Image, img2 As J2K_Image
    
    img1.version = J2K_VERSION
    img2.version = J2K_VERSION
    
    err1 = J2K_OpenFile(FileName1, img1)
    err2 = J2K_OpenFile(FileName2, img2)
    
    If err1 <> J2K_SUCCESS Or err2 <> J2K_SUCCESS Then
       MsgBox "Failed to open one of the files!", vbOKOnly, "J2K-Codec Error"
        J2K_Close img1
        J2K_Close img2
       Exit Sub
    End If
        
    If img1.Width <> img2.Width Or img1.Height <> img2.Height Then
        MsgBox "Both images must have the same dimensions!", vbOKOnly, "Error!"
        J2K_Close img1
        J2K_Close img2
        Exit Sub
    End If
    
    ' Calculate New Height to combine both files
    Dim NewHeight As Long
    NewHeight = img1.Height * 2
                    
    ' Display this information in window's title
    Me.Caption = "J2K Image: " + CStr(img1.Width) + " x " + CStr(NewHeight) + " x " + CStr(img1.Components)
    
    ' Let's create a bitmap that we will use as a storage for
    ' the decoded image and which can be drawn fast
    
    Dim bmp As BITMAPINFO
        
    bmp.Size = Len(bmp) - 4
    bmp.Width = img1.Width
    bmp.Height = NewHeight
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
        J2K_Close img1
        J2K_Close img2
        Exit Sub
    End If
            
    img1.buffer = pBits
    img1.buffer_bpp = 4
    img1.buffer_pitch = img1.Width * 4 * 2
            
    err1 = J2K_Decode(img1, "bmp=1,video=0")
    If err1 = J2K_SUCCESS Then
                         
        img2.buffer = pBits + img1.Width * 4
        img2.buffer_bpp = 4
        img2.buffer_pitch = img1.Width * 4 * 2
                         
        err2 = J2K_Decode(img2, "bmp=1,video=0")
        If err2 = J2K_SUCCESS Then
        
            ' Let's resize PictureBox and the Form to embrace the image
            Picture1.ScaleMode = vbPixels
            Picture1.Width = (img1.Width + 2) * 15
            Picture1.Height = (NewHeight + 2) * 15
        
            Me.Width = (img1.Width + 20) * 15
            Me.Height = (NewHeight + 50) * 15
                    
            ' Finally draw the bitmap, using compatible context and BitBlt
            Dim hBitmapDC As Long
        
            hBitmapDC = CreateCompatibleDC(Picture1.hDC)
            If hBitmapDC = 0 Then
                MsgBox "Failed to create compatible DC!", vbOKOnly, "System Error"
            Else
                SelectObject hBitmapDC, hBitmap
            
                ' Here we could use the StretchBlt() Windows API function instead,
                ' to scale the image to the size of the PictureBox
                BitBlt Picture1.hDC, 0, 0, img1.Width, NewHeight, hBitmapDC, 0, 0, SRCCOPY
                Picture1.Refresh
            
                DeleteDC hBitmapDC
            End If
        Else
            J2K_GetErrorStr Err, errStr
            MsgBox "Failed to decode file 2!" + Chr(13) + "Error Code: " + CStr(Err) + Chr(13) + "Error Text: " + errStr, vbOKOnly, "J2K-Codec Error"
        End If
       
     Else
        J2K_GetErrorStr Err, errStr
        MsgBox "Failed to decode file 1!" + Chr(13) + "Error Code: " + CStr(Err) + Chr(13) + "Error Text: " + errStr, vbOKOnly, "J2K-Codec Error"
     End If
        
    DeleteObject hBitmap
    J2K_Close img1
    J2K_Close img2

End Sub

Private Sub Form_Load()
         
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
             
    DecodeImage "odd.j2k", "even.j2k"
    
End Sub
