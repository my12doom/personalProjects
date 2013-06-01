using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace J2K_CodecPrj
{
	public class J2KViewer : Form
	{
		#region Controls
        private System.Windows.Forms.PictureBox pictureBox;
		private System.Windows.Forms.Label lblFileInfo;
		private System.Windows.Forms.OpenFileDialog dlgOpen;
        private ToolStrip toolStrip1;
        private ToolStripDropDownButton toolStripDropDownButton1;
        private ToolStripMenuItem fileToolStripMenuItem;
        private ToolStripMenuItem exitToolStripMenuItem;
        private StatusStrip statusStrip1;
        private ToolStripStatusLabel InfoLabel;
        private ToolStripDropDownButton toolStripDropDownButton2;
        private ToolStripMenuItem FR;
        private ToolStripMenuItem HR;
        private ToolStripMenuItem QR;
        private ToolStripMenuItem ER;
		private System.ComponentModel.Container components = null;
		#endregion

		#region Initialization code
		public J2KViewer()
		{
			InitializeComponent();
		}

		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(J2KViewer));
            this.pictureBox = new System.Windows.Forms.PictureBox();
            this.lblFileInfo = new System.Windows.Forms.Label();
            this.dlgOpen = new System.Windows.Forms.OpenFileDialog();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.toolStripDropDownButton1 = new System.Windows.Forms.ToolStripDropDownButton();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripDropDownButton2 = new System.Windows.Forms.ToolStripDropDownButton();
            this.FR = new System.Windows.Forms.ToolStripMenuItem();
            this.HR = new System.Windows.Forms.ToolStripMenuItem();
            this.QR = new System.Windows.Forms.ToolStripMenuItem();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.InfoLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.ER = new System.Windows.Forms.ToolStripMenuItem();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).BeginInit();
            this.toolStrip1.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // pictureBox
            // 
            this.pictureBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.pictureBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.pictureBox.Location = new System.Drawing.Point(0, 27);
            this.pictureBox.Name = "pictureBox";
            this.pictureBox.Size = new System.Drawing.Size(524, 321);
            this.pictureBox.TabIndex = 1;
            this.pictureBox.TabStop = false;
            // 
            // lblFileInfo
            // 
            this.lblFileInfo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.lblFileInfo.Location = new System.Drawing.Point(76, 9);
            this.lblFileInfo.Name = "lblFileInfo";
            this.lblFileInfo.Size = new System.Drawing.Size(448, 16);
            this.lblFileInfo.TabIndex = 3;
            this.lblFileInfo.Text = "File:";
            // 
            // toolStrip1
            // 
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripDropDownButton1,
            this.toolStripDropDownButton2});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(524, 25);
            this.toolStrip1.TabIndex = 4;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // toolStripDropDownButton1
            // 
            this.toolStripDropDownButton1.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.toolStripDropDownButton1.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.exitToolStripMenuItem});
            this.toolStripDropDownButton1.Image = ((System.Drawing.Image)(resources.GetObject("toolStripDropDownButton1.Image")));
            this.toolStripDropDownButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripDropDownButton1.Name = "toolStripDropDownButton1";
            this.toolStripDropDownButton1.Size = new System.Drawing.Size(36, 22);
            this.toolStripDropDownButton1.Text = "&File";
            this.toolStripDropDownButton1.ToolTipText = "Open file";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(100, 22);
            this.fileToolStripMenuItem.Text = "&Open";
            this.fileToolStripMenuItem.Click += new System.EventHandler(this.fileToolStripMenuItem_Click);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(100, 22);
            this.exitToolStripMenuItem.Text = "E&xit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // toolStripDropDownButton2
            // 
            this.toolStripDropDownButton2.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.toolStripDropDownButton2.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FR,
            this.HR,
            this.QR,
            this.ER});
            this.toolStripDropDownButton2.Image = ((System.Drawing.Image)(resources.GetObject("toolStripDropDownButton2.Image")));
            this.toolStripDropDownButton2.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripDropDownButton2.Name = "toolStripDropDownButton2";
            this.toolStripDropDownButton2.Size = new System.Drawing.Size(70, 22);
            this.toolStripDropDownButton2.Text = "&Resolution";
            this.toolStripDropDownButton2.ToolTipText = "Change resolution level";
            // 
            // FR
            // 
            this.FR.Name = "FR";
            this.FR.Size = new System.Drawing.Size(165, 22);
            this.FR.Text = "Full Resolution";
            this.FR.Click += new System.EventHandler(this.FR_Click);
            // 
            // HR
            // 
            this.HR.Name = "HR";
            this.HR.Size = new System.Drawing.Size(165, 22);
            this.HR.Text = "Half Resolution";
            this.HR.Click += new System.EventHandler(this.HR_Click);
            // 
            // QR
            // 
            this.QR.Name = "QR";
            this.QR.Size = new System.Drawing.Size(165, 22);
            this.QR.Text = "Quarter Resolution";
            this.QR.Click += new System.EventHandler(this.QR_Click);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.InfoLabel});
            this.statusStrip1.Location = new System.Drawing.Point(0, 351);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(524, 22);
            this.statusStrip1.SizingGrip = false;
            this.statusStrip1.TabIndex = 5;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // InfoLabel
            // 
            this.InfoLabel.Name = "InfoLabel";
            this.InfoLabel.Size = new System.Drawing.Size(96, 17);
            this.InfoLabel.Text = "Image Information";
            // 
            // ER
            // 
            this.ER.Name = "ER";
            this.ER.Size = new System.Drawing.Size(165, 22);
            this.ER.Text = "Eighth Resolution";
            this.ER.Click += new System.EventHandler(this.ER_Click);
            // 
            // J2KViewer
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(524, 373);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.toolStrip1);
            this.Controls.Add(this.lblFileInfo);
            this.Controls.Add(this.pictureBox);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "J2KViewer";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "J2K Viewer";
            this.Load += new System.EventHandler(this.J2KViewer_Load);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).EndInit();
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

		}
		#endregion

		[STAThread]
		static void Main() 
		{
			Application.Run(new J2KViewer());
		}
		#endregion

		#region Bitmap
		[DllImport("gdi32.dll")]
		static extern IntPtr CreateDIBSection(IntPtr hdc, [In] ref BITMAPINFO pbmi,
			uint iUsage, ref IntPtr ppvBits, IntPtr hSection, uint dwOffset);

        [DllImport("gdi32.dll")]
        static extern void DeleteObject(IntPtr handle);

		[StructLayout(LayoutKind.Sequential)]
		struct BITMAPINFO 
		{
			public Int32 Size;
			public Int32 Width;
			public Int32 Height;
			public Int16 Planes;
			public Int16 BitCount;
			public Int32 Compression;
			public Int32 SizeImage;
			public Int32 XPelsPerMeter;
			public Int32 YPelsPerMeter;
			public Int32 ClrUsed;
			public Int32 ClrImportant;
		}
		#endregion

		private void ShowImage(string fileName, int resolution)
		{
			// Open picture file and obtain an image handle
            J2K_Image img = new J2K_Image(); img.version=J2K_Codec.Version;

             
            J2K_Error err = J2K_Codec.OpenFile(fileName, ref img);
            if(err != J2K_Error.Success)
            {
				// If error then show the error message and exit function
				MessageBox.Show(J2K_Codec.GetErrStr(err), "Error");
				J2K_Codec.Close(ref img);
				return;
			}

            // Set resolution level and update Width and Height
            img.resolution = resolution;
            J2K_Codec.GetInfo(ref img);

            // Show to the user the file name and size
			InfoLabel.Text = string.Format("{0}  |  {1} x {2} x {3}",
					new FileInfo(fileName).Name, img.Width, img.Height, img.Components);
            
			// Create a bitmap info structure
			BITMAPINFO bmp = new BITMAPINFO();
			bmp.Size = Marshal.SizeOf(bmp);
			bmp.Width = img.Width;		// Image width
			bmp.Height = img.Height;	// Image height
			bmp.Planes = 1;
			bmp.BitCount = 32;			// Always 4 bytes per pixel
			bmp.Compression = 0;		// No compression (RGB format)
			bmp.SizeImage = 0;
			bmp.XPelsPerMeter = 0;
			bmp.YPelsPerMeter = 0;
			bmp.ClrUsed = 0;
			bmp.ClrImportant = 0;

			// Create the bitmap in memory. The "bits" variable will receive a pointer
			// to the memory allocated for the bitmap.
			IntPtr bits = IntPtr.Zero;
			IntPtr hBitmap = CreateDIBSection(IntPtr.Zero, ref bmp, 0, ref bits, IntPtr.Zero, 0);
			if (hBitmap == IntPtr.Zero)
			{
				// There was an error creating the bitmap
				J2K_Codec.Close(ref img);
				MessageBox.Show("Can't create bitmap - not enough memory?");
			}

			// Decode the image into the newly created bitmap memory (pointed to by the
			// "bits" variable).
			this.Cursor = Cursors.WaitCursor;

            img.buffer = bits;
            img.buffer_bpp = 4;
            img.buffer_pitch = img.Width * img.buffer_bpp;

            err = J2K_Codec.Decode(ref img, "bmp=1");
			if (err == J2K_Error.Success)
			{
				// Obtain a managed bitmap
				Bitmap result = Bitmap.FromHbitmap(hBitmap);

				// Show it in the image box
				pictureBox.Image = result;

                // Resize the window to embrace the image
                ResizeWindow(img.Width, img.Height);
			} 
			else 
			{
				// If error then show the error message
				MessageBox.Show(J2K_Codec.GetErrStr(err), "Error");
			}

            // Delete the DIB to release memory
            DeleteObject(hBitmap);

            // Close the image
            J2K_Codec.Close(ref img);
	            	
            // Restore cursor
			this.Cursor = Cursors.Default;			
		}

		private void J2KViewer_Load(object sender, System.EventArgs e)
		{
            // J2K_Codec.StartLogging(); // if you need to debug...

			// Unlock the component. Use your personal registration key here
			J2K_Codec.Unlock("Put your key here");

			// Show the version of J2K-Codec in the window title bar
			int version = J2K_Codec.GetVersion();
			int major = version >> 20;
			int minor = (version >> 16) & 0x0F;
			int build = version & 0xFFFF;
			this.Text = string.Format("J2K Viewer (codec ver: {0}.{1}.{2})", major, minor, build);
		}

        private void ResizeWindow(int width, int height)
        {
            // Minimum width of the window is 300px
            if (width < 300) width = 300;
            this.Width = width + pictureBox.Left + 8;
            this.Height = height + pictureBox.Top + 52;
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void fileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // Select .j2k or .jp2 file
            dlgOpen.Filter = "JPEG2000 files (*.j2k,*.jp2)|*.j2k;*.jp2|All files (*.*)|*.*";
            
            // Start from application folder
            dlgOpen.InitialDirectory = AppDomain.CurrentDomain.BaseDirectory;
            DialogResult res = dlgOpen.ShowDialog(this);
            if (res == DialogResult.Cancel) return;

            // Show selected image
            ShowImage(dlgOpen.FileName, 0);
        }

        private void FR_Click(object sender, EventArgs e)
        {
            if(dlgOpen.FileName != "") ShowImage(dlgOpen.FileName, 0);
        }

        private void HR_Click(object sender, EventArgs e)
        {
            if (dlgOpen.FileName != "") ShowImage(dlgOpen.FileName, 1);
        }

        private void QR_Click(object sender, EventArgs e)
        {
            if (dlgOpen.FileName != "") ShowImage(dlgOpen.FileName, 2);
        }

        private void ER_Click(object sender, EventArgs e)
        {
            if (dlgOpen.FileName != "") ShowImage(dlgOpen.FileName, 3);
        }

               
	}
}
