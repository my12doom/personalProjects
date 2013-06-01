Imports System.Runtime.InteropServices

'Initial version was written by Steven Abbott

Public Class frmJ2K
    
    Dim MyFileName As String

    Private Sub frmJ2K_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load
        'To be replaced by your real registration key
        Dim sv() As Integer = {91, 89, 111, 117, 114, 32, 82, 101, 103, 105, 115, 116, 114, 97, 116, 105, 111, 110, 32, 75, 101, 121, 93}        'This says [Your Registration Key]
        Dim uc As String = ""
        For i As Integer = 0 To sv.Length - 1
            uc &= Chr(sv(i))
        Next
        'By coding the key as an integer array, it's harder for others to read it from the .exe
        'Obviously the data could be encrypted to make it even harder to spot
        J2K_Codec.Unlock(uc)
        FR.Checked = True
    End Sub

    Private Sub menuFileExit_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles menuFileExit.Click
        Me.Close()
    End Sub

    Private Sub menuFileOpen_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles menuFileOpen.Click
        Dim OD As New OpenFileDialog
        OD.Filter = "Image Files(*.JP2;*.J2K;*.JPG;*.BMP;*.GIF;*.TIF;*.TIFF;*.PNG;)|*.JP2;*.J2K;*.JPG;*.BMP;*.GIF;*.TIF;*.TIFF;*.PNG"
        OD.ShowReadOnly = False
        OD.InitialDirectory = System.Environment.CurrentDirectory
        If OD.ShowDialog <> Windows.Forms.DialogResult.Cancel Then
            If OD.FileName.ToUpper.EndsWith("JP2") OrElse OD.FileName.ToUpper.EndsWith("J2K") Then
                MyFileName = OD.FileName
                GetJ2K(MyFileName)
            Else
                'This is a normal JPG, BMP etc. so handle it normally!
                PictureBox1.Image = Image.FromFile(OD.FileName)
            End If
        End If
    End Sub

    Private Sub GetJ2K(ByVal FileName As String)
        Dim TheData() As Byte
        Dim Address As IntPtr
        Dim Size, err As Integer

        ' Open image file
        Dim img As J2K_Image
        img.version = J2K_Codec.Version

        err = J2K_Codec.OpenFile(FileName, img)

        If err <> J2K_Error.Success Then
            ShowError("OpenFile", err)
            Exit Sub
        End If

        ' Set resolution and update Width and Height
        If ER.Checked Then
            img.resolution = 3
        ElseIf QR.Checked Then
            img.resolution = 2
        ElseIf HR.Checked Then
            img.resolution = 1
        Else
            img.resolution = 0
        End If

        J2K_Codec.GetInfo(img)
        
        ' Display this information in window's title
        Me.Text = "J2K Image: " + FileName

        Dim ver As Integer
        ver = J2K_Codec.GetVersion()
        
        Dim verMajor, verMinor, verBuild As String
        verMajor = Hex(ver \ &H100000)
        verMinor = Hex((ver \ &H10000) And 15)
        verBuild = CStr(ver And 65535)

        InfoLabel.Text = "J2K-Codec ver " & verMajor & "." & verMinor & " (build " & verBuild & ")  |  Image: " & img.Width & "x" & img.Height & "x" & img.Components & "  |  " & CodingSchemes(img.FileType) & "  |  " & img.hTiles & "x" & img.vTiles & " tiles  |  " & img.Resolutions & " res. levels"

        'It turns out to be much easier to deal with the codec's native 4-component structure.
        'If you use a 3 component image you hit problems with Stride issues.
        Size = img.Width * img.Height * 4

        Dim bmp As Bitmap
        bmp = New Bitmap(img.Width, img.Height, Imaging.PixelFormat.Format32bppArgb)

        'This way of handling LockBits is one I've developed over the years and is quite handy
        'Doing it without LockBits is amazingly slow and/or involves lots of API stuff
        Dim bmB As New BitmapLock(bmp)
        bmB.LockBitmap(4)

        'We can't send TheData directly to J2K_Decode
        'Instead we stop any Garbage Collection of those bytes and pin the address
        'This neatly avoids us having to do a Marshal.Copy
        ReDim TheData(Size)
        Dim gcH As GCHandle
        gcH = GCHandle.Alloc(TheData, GCHandleType.Pinned)
        Address = Marshal.UnsafeAddrOfPinnedArrayElement(TheData, 0)
        'In fact, you can be even smarter than this. The bmB.ImageBytes are already locked
        'so you could have
        'Address = m_BitmapData.Scan0 
        'and not bother with the transfer of TheData into bmB.ImageBytes
        'However, I've done it this way in case the user wants to do something smart with the data
        'If you don't have the Width*Components argument then you get some very wobbly images!

        img.buffer = Address
        img.buffer_bpp = 4
        img.buffer_pitch = img.Width * img.buffer_bpp

        err = J2K_Codec.Decode(img)
        If err <> J2K_Error.Success Then
            ShowError("Decoding", err)
            Exit Sub
        End If

        'We've got the data safely in TheData so we can free up the GCHandle
        gcH.Free()

        'If the image really is ARGB then we need to prepare the PictureBox1 with an appropriate "image"
        'In this case, just a simple background colour
        If img.Components = 4 Then PictureBox1.BackColor = Color.Cyan

        'Now get the image from TheData
        'By good fortune GDI+ has the same BGRA order as J2K so we can just put the data in directly
        'If the image is a 4-component CMYK then this will fail, but so far I've not found one to test
        For i As Integer = 0 To Size - 1
            bmB.ImageBytes(i) = TheData(i)
        Next
        'Now free up the bits and give them to the PictureBox
        bmB.UnlockBitmap()
        PictureBox1.Image = bmp

        ' Resize the form
        If img.Width < 350 Then
            Me.Width = 670
        Else
            Me.Width = img.Width + 300
        End If

        If img.Height < 400 Then
            Me.Height = 450
        Else
            Me.Height = img.Height + 100
        End If

        'No more need for J2K
        J2K_Codec.Close(img)

    End Sub

    Private Sub fFR_Click(ByVal sender As Object, ByVal e As System.EventArgs) Handles FR.Click, HR.Click, QR.Click, ER.Click
        Dim MI As ToolStripMenuItem = CType(sender, ToolStripMenuItem)
        FR.Checked = False
        HR.Checked = False
        QR.Checked = False
        ER.Checked = False
        MI.Checked = True
        'Update the image if Resolution is changed
        If MyFileName <> "" Then GetJ2K(MyFileName)
    End Sub

    Private Sub ShowError(ByVal EType As String, ByVal ENumber As Integer)
        MsgBox("Sorry, there was an error in " & EType & vbNewLine & J2K_Codec.GetErrStr(ENumber) & "!", MsgBoxStyle.OkOnly, "J2K-Codec Error")
    End Sub

End Class
