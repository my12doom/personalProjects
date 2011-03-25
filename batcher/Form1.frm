VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Form1"
   ClientHeight    =   5490
   ClientLeft      =   45
   ClientTop       =   375
   ClientWidth     =   6810
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   OLEDropMode     =   1  'Manual
   ScaleHeight     =   5490
   ScaleWidth      =   6810
   StartUpPosition =   3  '´°¿ÚÈ±Ê¡
   Begin VB.TextBox options 
      Height          =   375
      Left            =   840
      TabIndex        =   14
      Top             =   3720
      Width           =   4455
   End
   Begin VB.TextBox bitrate 
      Height          =   270
      Left            =   3360
      TabIndex        =   11
      Top             =   2400
      Width           =   2055
   End
   Begin VB.CommandButton Command4 
      Caption         =   "..."
      Height          =   375
      Left            =   5520
      TabIndex        =   9
      Top             =   3240
      Visible         =   0   'False
      Width           =   975
   End
   Begin VB.TextBox output 
      Height          =   375
      Left            =   840
      TabIndex        =   8
      Top             =   3240
      Width           =   4455
   End
   Begin VB.CommandButton Command3 
      Caption         =   "Batch it!"
      Height          =   735
      Left            =   1440
      TabIndex        =   7
      Top             =   4560
      Width           =   3495
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Remove"
      Height          =   375
      Left            =   5280
      TabIndex        =   6
      Top             =   840
      Width           =   1335
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Add"
      Height          =   375
      Left            =   5280
      TabIndex        =   5
      Top             =   240
      Visible         =   0   'False
      Width           =   1335
   End
   Begin VB.CheckBox Check1 
      Caption         =   "Resize"
      Height          =   375
      Left            =   240
      TabIndex        =   4
      Top             =   2760
      Width           =   975
   End
   Begin VB.TextBox resize_y 
      Height          =   375
      Left            =   3240
      TabIndex        =   3
      Top             =   2760
      Width           =   1335
   End
   Begin VB.TextBox resize_x 
      Height          =   375
      Left            =   1440
      TabIndex        =   2
      Top             =   2760
      Width           =   1455
   End
   Begin VB.ComboBox Combo1 
      Height          =   300
      Left            =   240
      TabIndex        =   1
      Top             =   2400
      Width           =   2175
   End
   Begin VB.ListBox List1 
      Height          =   2040
      Left            =   240
      OLEDropMode     =   1  'Manual
      TabIndex        =   0
      Top             =   240
      Width           =   4935
   End
   Begin VB.Label Label3 
      Caption         =   "Options:"
      Height          =   255
      Left            =   120
      TabIndex        =   13
      Top             =   3770
      Width           =   735
   End
   Begin VB.Label Label2 
      Caption         =   "Bitrate"
      Height          =   255
      Left            =   2640
      TabIndex        =   12
      Top             =   2400
      Width           =   615
   End
   Begin VB.Label Label1 
      Caption         =   "Output:"
      Height          =   375
      Left            =   120
      TabIndex        =   10
      Top             =   3285
      Width           =   735
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private Type job
    filename As String
    source As String
    out As String
    resize As Boolean
    resize_x As Long
    resize_y As Long
    bitrate As Long
    options As String
End Type

Private jobs() As job
Dim n As Long
Dim selected As Long


Private Sub add_job(ByVal filename As String)
    n = n + 1
    ReDim Preserve jobs(n)
    
    With jobs(n - 1)
        .filename = filename
        .out = get_name(filename) & ".264"
        If (get_name(filename) = "") Then .out = Left(filename, 1) & .out
        .bitrate = 11000
        
        Dim ext As String
        ext = StrConv(get_ext(filename), vbLowerCase)
        If ext = "ssif" Or ext = "mpls" Or ext = "" Then
            .source = "SsifAvs"
        Else
            .source = "DirectShowSource"
        End If
    End With
    
    list_job
    selected = n - 1
    List1.ListIndex = n - 1
End Sub

Private Sub remove_job(ByVal item As Long)
    Dim i As Long
    For i = item To n - 2
        jobs(i) = jobs(i + 1)
    Next
    
    n = n - 1
    list_job
End Sub


Private Sub list_job()
    List1.Clear
    Dim i As Long
    For i = 0 To n - 1
        List1.AddItem jobs(i).filename
    Next
    
    On Error Resume Next
    Dim tmp As Long
    tmp = selected
    List1.ListIndex = n - 1
    List1.ListIndex = tmp
End Sub


Private Sub Check1_Click()
    If n <= 0 Then Exit Sub
    If Check1.Value = 1 Then
        resize_x.Enabled = True
        resize_x.Enabled = True
        jobs(selected).resize = True
    Else
        resize_x.Enabled = False
        resize_x.Enabled = False
        jobs(selected).resize = False
    End If
    
End Sub

Private Sub Combo1_Change()
    If n <= 0 Then Exit Sub
    jobs(selected).source = Combo1.Text
End Sub

Private Sub Combo1_Click()
    If n <= 0 Then Exit Sub
    jobs(selected).source = Combo1.Text
End Sub

Private Sub Command1_Click()
    add_job ("ssif.sSif")
End Sub

Private Sub Command2_Click()
    remove_job (selected)
End Sub

Private Sub Command3_Click()
    On Error Resume Next
    Dim tmp As String
    Dim enter As String
    enter = Chr(13) + Chr(10)
    Kill apppath & "batch.bat"
    Open apppath & "batch.bat" For Binary As 1
    
    Dim i As Long
    For i = 0 To n - 1
        Kill apppath & "job" & (i + 1) & ".avs"
        Open apppath & "job" & (i + 1) & ".avs" For Binary As 2
        With jobs(i)
            If .source <> "DirectShowSource" Then
                tmp = "LoadPlugin(" & Chr(34) & .source & Chr(34) & ")" & enter
                Put 2, , tmp
            End If
            tmp = .source & "(" & Chr(34) & .filename & Chr(34) & ") " & enter
            Put 2, , tmp
            If .resize Then
                tmp = "LanczosResize(" & .resize_x & ", " & .resize_y & ")" & enter
                Put 2, , tmp
            End If
            tmp = "StereoPlayer.exe -B " & .bitrate & " job" & (i + 1) & ".avs -o " & Chr(34) & .out & Chr(34) & " " & .options & enter
            Put 1, , tmp
        End With
        
        Close 2
        
    Next
    
    Close 1
End Sub

Private Sub Form_Load()
    Combo1.AddItem "CoreAVS"
    Combo1.AddItem "SsifAVS"
    Combo1.AddItem "DirectShowSource"
End Sub

Private Sub Form_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
    Dim i As Long
    For i = 1 To Data.Files.Count
        Call add_job(Data.Files.item(i))
    Next
End Sub

Private Sub List1_Click()
    selected = List1.ListIndex
    
    With jobs(selected)
        If .resize Then
            Check1.Value = 1
            resize_x = .resize_x
            resize_y = .resize_y
            resize_x.Enabled = True
            resize_x.Enabled = True
        Else
            Check1.Value = 0
            resize_x = ""
            resize_y = ""
            resize_x.Enabled = False
            resize_x.Enabled = False
        End If
        
        output.Text = .out
        Combo1.Text = .source
        bitrate.Text = .bitrate
        options.Text = .options
    End With
End Sub

Private Sub List1_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
    Dim i As Long
    For i = 1 To Data.Files.Count
        Call add_job(Data.Files.item(i))
    Next
End Sub

Private Sub options_Change()
    If n <= 0 Then Exit Sub
    jobs(selected).options = options.Text

End Sub

Private Sub output_Change()
    If n <= 0 Then Exit Sub
    jobs(selected).out = output.Text
End Sub

Private Sub resize_x_Change()
    If n <= 0 Then Exit Sub
    On Error Resume Next
    jobs(selected).resize_x = CLng(resize_x)
End Sub

Private Sub resize_y_Change()
    If n <= 0 Then Exit Sub
    On Error Resume Next
    jobs(selected).resize_y = CLng(resize_y)
End Sub

Private Function get_name(ByVal pathname As String) As String
    Dim t() As String
    t = Split(pathname, "\")
    If (Len(t(UBound(t))) = 0) Then Exit Function
    get_name = Split(t(UBound(t)), ".")(0)
End Function

Private Function get_ext(ByVal pathname As String) As String
    Dim t() As String
    t = Split(pathname, "\")
    If (Len(t(UBound(t))) = 0) Then Exit Function
    t = Split(t(UBound(t)), ".")
    get_ext = t(UBound(t))
End Function

Private Sub Text1_Change()
    If n <= 0 Then Exit Sub
    On Error Resume Next
    jobs(selected).bitrate = CLng(bitrate)
End Sub


Private Function apppath()
    If Right(App.Path, 1) = "\" Then
        apppath = App.Path
    Else
        apppath = App.Path & "\"
    End If
End Function
