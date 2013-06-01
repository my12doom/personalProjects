<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmJ2K
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        If disposing AndAlso components IsNot Nothing Then
            components.Dispose()
        End If
        MyBase.Dispose(disposing)
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(frmJ2K))
        Me.PictureBox1 = New System.Windows.Forms.PictureBox
        Me.StatusStrip1 = New System.Windows.Forms.StatusStrip
        Me.InfoLabel = New System.Windows.Forms.ToolStripStatusLabel
        Me.ToolStrip1 = New System.Windows.Forms.ToolStrip
        Me.menuFile = New System.Windows.Forms.ToolStripDropDownButton
        Me.menuFileOpen = New System.Windows.Forms.ToolStripMenuItem
        Me.menuFileExit = New System.Windows.Forms.ToolStripMenuItem
        Me.ToolStripDropDownButton2 = New System.Windows.Forms.ToolStripDropDownButton
        Me.FR = New System.Windows.Forms.ToolStripMenuItem
        Me.HR = New System.Windows.Forms.ToolStripMenuItem
        Me.QR = New System.Windows.Forms.ToolStripMenuItem
        Me.ER = New System.Windows.Forms.ToolStripMenuItem
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.StatusStrip1.SuspendLayout()
        Me.ToolStrip1.SuspendLayout()
        Me.SuspendLayout()
        '
        'PictureBox1
        '
        Me.PictureBox1.Location = New System.Drawing.Point(0, 28)
        Me.PictureBox1.Name = "PictureBox1"
        Me.PictureBox1.Size = New System.Drawing.Size(560, 373)
        Me.PictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize
        Me.PictureBox1.TabIndex = 0
        Me.PictureBox1.TabStop = False
        '
        'StatusStrip1
        '
        Me.StatusStrip1.Font = New System.Drawing.Font("Arial", 11.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.StatusStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.InfoLabel})
        Me.StatusStrip1.Location = New System.Drawing.Point(0, 410)
        Me.StatusStrip1.Name = "StatusStrip1"
        Me.StatusStrip1.Size = New System.Drawing.Size(565, 22)
        Me.StatusStrip1.SizingGrip = False
        Me.StatusStrip1.TabIndex = 2
        Me.StatusStrip1.Text = "StatusStrip1"
        '
        'InfoLabel
        '
        Me.InfoLabel.Name = "InfoLabel"
        Me.InfoLabel.Size = New System.Drawing.Size(124, 17)
        Me.InfoLabel.Text = "Image Information"
        '
        'ToolStrip1
        '
        Me.ToolStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.menuFile, Me.ToolStripDropDownButton2})
        Me.ToolStrip1.Location = New System.Drawing.Point(0, 0)
        Me.ToolStrip1.Name = "ToolStrip1"
        Me.ToolStrip1.Size = New System.Drawing.Size(565, 25)
        Me.ToolStrip1.TabIndex = 5
        Me.ToolStrip1.Text = "ToolStrip1"
        '
        'menuFile
        '
        Me.menuFile.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text
        Me.menuFile.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.menuFileOpen, Me.menuFileExit})
        Me.menuFile.Image = CType(resources.GetObject("menuFile.Image"), System.Drawing.Image)
        Me.menuFile.ImageTransparentColor = System.Drawing.Color.Magenta
        Me.menuFile.Name = "menuFile"
        Me.menuFile.Size = New System.Drawing.Size(36, 22)
        Me.menuFile.Text = "&File"
        Me.menuFile.ToolTipText = "Open a file"
        '
        'menuFileOpen
        '
        Me.menuFileOpen.Name = "menuFileOpen"
        Me.menuFileOpen.Size = New System.Drawing.Size(152, 22)
        Me.menuFileOpen.Text = "&Open"
        '
        'menuFileExit
        '
        Me.menuFileExit.Name = "menuFileExit"
        Me.menuFileExit.Size = New System.Drawing.Size(152, 22)
        Me.menuFileExit.Text = "E&xit"
        '
        'ToolStripDropDownButton2
        '
        Me.ToolStripDropDownButton2.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text
        Me.ToolStripDropDownButton2.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.FR, Me.HR, Me.QR, Me.ER})
        Me.ToolStripDropDownButton2.Image = CType(resources.GetObject("ToolStripDropDownButton2.Image"), System.Drawing.Image)
        Me.ToolStripDropDownButton2.ImageTransparentColor = System.Drawing.Color.Magenta
        Me.ToolStripDropDownButton2.Name = "ToolStripDropDownButton2"
        Me.ToolStripDropDownButton2.Size = New System.Drawing.Size(70, 22)
        Me.ToolStripDropDownButton2.Text = "&Resolution"
        Me.ToolStripDropDownButton2.ToolTipText = "Change resolution level"
        '
        'FR
        '
        Me.FR.Name = "FR"
        Me.FR.Size = New System.Drawing.Size(165, 22)
        Me.FR.Text = "Full Resolution"
        '
        'HR
        '
        Me.HR.Name = "HR"
        Me.HR.Size = New System.Drawing.Size(165, 22)
        Me.HR.Text = "Half Resolution"
        '
        'QR
        '
        Me.QR.Name = "QR"
        Me.QR.Size = New System.Drawing.Size(165, 22)
        Me.QR.Text = "Quarter Resolution"
        '
        'ER
        '
        Me.ER.Name = "ER"
        Me.ER.Size = New System.Drawing.Size(165, 22)
        Me.ER.Text = "Eighth Resolution"
        '
        'frmJ2K
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.AutoScroll = True
        Me.ClientSize = New System.Drawing.Size(565, 432)
        Me.Controls.Add(Me.StatusStrip1)
        Me.Controls.Add(Me.PictureBox1)
        Me.Controls.Add(Me.ToolStrip1)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
        Me.MaximizeBox = False
        Me.Name = "frmJ2K"
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen
        Me.Text = "J2K-Codec Tester"
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).EndInit()
        Me.StatusStrip1.ResumeLayout(False)
        Me.StatusStrip1.PerformLayout()
        Me.ToolStrip1.ResumeLayout(False)
        Me.ToolStrip1.PerformLayout()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents PictureBox1 As System.Windows.Forms.PictureBox
    Friend WithEvents StatusStrip1 As System.Windows.Forms.StatusStrip
    Friend WithEvents InfoLabel As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents ToolStrip1 As System.Windows.Forms.ToolStrip
    Friend WithEvents menuFile As System.Windows.Forms.ToolStripDropDownButton
    Friend WithEvents menuFileExit As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ToolStripDropDownButton2 As System.Windows.Forms.ToolStripDropDownButton
    Friend WithEvents FR As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents HR As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents QR As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ER As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents menuFileOpen As System.Windows.Forms.ToolStripMenuItem

End Class
