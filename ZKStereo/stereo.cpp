//  ****************************************************************************************************
//
//  For non-commercial use only.  Do not distribute without permission from Carnegie Mellon University.  
//
//  Copyright (c) 2002, Carnegie Mellon University         All rights reserved.
//
//  ****************************************************************************************************


/*******************************************************************************************************

This file contains all the necessary routines for computing
disparity estimates as decribed in 

A Cooperative Algorithm for Stereo Matching and Occlusion Detection 
C. L. Zitnick and T. Kanade 
Proc. IEEE Trans. Pattern Analysis and Machine Intelligence, vol. 22, no. 7, July 2000. 

The routines for reading in images and basic image manipulation
are not included, since these would be specific to each individual
project.  Translation to other image libraries should not be 
difficult.

Any questions should be sent to clz@cs.cmu.edu

Larry Zitnick

********************************************************************************************************/

#include <math.h>
#include <stdio.h>
// #include "image.h"
#include "stereo.h"

/********************************************************

FindDisparityMap - Top level control function.  Computes
the disparity map using ComputeL0Values and DoIterations

CImage *Img0 - reference (left) image (assume the images are rectified across rows.)
CImage *Img1 - right image
int MinDisparity - minimum disparity in pixels
int MaxDisparity - maximum disparity in pixels
int WinRadL0 - window radius for computing the L0 values in row and column direction
int WinRadRC - window radius for computing the Ln values in row and column direction
int WinRadD  - window radius for computing the Ln values in disparity direction
int NumIterations - number of iterations to do
float MaxScaler - Max scaler for computing the L0 values
BOOL USE_SAD - Use SAD or normalized correlation to compute L0 values

*********************************************************/

void CStereo::FindDisparityMap(CImage *Img0, CImage *Img1, int MinDisparity, int MaxDisparity,
							   int WinRadL0, int WinRadRC, int WinRadD, 
							   int NumIterations, float MaxScaler, BOOL USE_SAD)
{
	// Assign the values
	m_MinD = MinDisparity;
	m_MaxD = MaxDisparity;
	m_Rows = Img0->m_height;
	m_Cols = Img0->m_width;
	m_Depth = m_MaxD - m_MinD;
	m_WinD = WinRadD;
	m_WinRC = WinRadRC;

	// Create 3D arrays for storing the L0 and Ln values
	m_L0Values = new CImage(m_Cols, m_Rows, m_Depth);
	m_LnValues = new CImage(m_Cols, m_Rows, m_Depth);

	// Create 2D arrays to store the disparity values and output images
	m_DisparityValues = new CImage(m_Cols, m_Rows, 2);
	m_DisplayImage = new CImage(m_Cols, m_Rows, 2);

	// Compute the L0 values
	ComputeL0Values(Img0, Img1, WinRadL0, MaxScaler, USE_SAD);

	// First iterations done
	m_IterationNum++;

	// Do the remaining iterations
	DoIterations(NumIterations);

	// Find the final disparity map using the Ln values
	FindFinalDisparityMap();

	// Scale the disparity values for display
	ScaleAndDisplayDisparityValues();

}

/********************************************************

ComputeL0Values - Compute the L0 values directly from the 
input images.  Uses either SAD (sums of absolute differences) 
or normalized correlation.

CImage *Img0 - reference (left) image (assume the images are rectified across rows.)
CImage *Img1 - right image
int WinRadL0 - window radius for computing the L0 values in row and column direction
float MaxScaler - Max scaler for computing the L0 values
BOOL USE_SAD - Use SAD or normalized correlation to compute L0 values

*********************************************************/

void CStereo::ComputeL0Values(CImage *Img0, CImage *Img1, 
							  int WinRadL0, float MaxScaler, BOOL USE_SAD)
{
	int i, d;
	int disparityDel;
	float ref_val, cam_val, avg, val, denom;
	int r0, r1, c0, c1;
	CImage AvgLVals(Img0->m_width, Img0->m_height, 1);

	// Set the L0 values to 0
	m_L0Values->clearImage();


	if(!USE_SAD)
	{
		// Use normalized correlation

		// Create some temporary arrays for storage
		CImage imageSums(Img0->m_width, Img0->m_height, 2*Img0->m_color);
		CImage sums(Img0->m_width, Img0->m_height, 3*Img0->m_color);

		// Set array values to 0
		imageSums.clearImage();

		// For each pixel in the reference image...
		for(c0=0;c0<m_Cols;c0++)				// c0 : x
			for(r0=0;r0<m_Rows;r0++)			// r0 : y
			{
	
				// If the pixel doesn't lie on the border of the image
				if(r0 > BORDERRC && r0 < m_Rows-BORDERRC && c0 > BORDERRC && c0 < m_Cols-BORDERRC)      
				{

					// For each color
					for(i=0;i<Img0->m_color;i++)
					{
						// Get reference image's pixel value at row r0, column c0 and color i
						ref_val = Img0->getValue(r0, c0, i);

						// Get the right image's pixel value at row r0, column c0 and color i
						cam_val = Img1->getValue(r0, c0, i);

						// Add the pixel values to imageSums
						imageSums.addValue(r0, c0, i*2, ref_val);
						imageSums.addValue(r0, c0, i*2 + 1, cam_val);
					}
				}
			}

		// Average the values with radius WinRadL0
		imageSums.averageImage(WinRadL0);

		// For each disparity level
		for(d=0;d<m_Depth;d++)
		{
			// Set array values to 0
			sums.clearImage();

			// For each pixel in the reference image...
			for(c0=0;c0<m_Cols;c0++)
				for(r0=0;r0<m_Rows;r0++)
				{
		
					// Compute the disparity in pixels
					disparityDel = d + m_MinD;

					// Compute the pixel location in the right image
					r1 = r0;
					c1 = c0 + disparityDel;

					// The pixel isn't along the border...
					if(r1 > BORDERRC && r1 < m_Rows-BORDERRC && c1 > BORDERRC && c1 < m_Cols-BORDERRC &&
					   r0 > BORDERRC && r0 < m_Rows-BORDERRC && c0 > BORDERRC && c0 < m_Cols-BORDERRC)      
					{

						// For each color
						for(i=0;i<Img0->m_color;i++)
						{
							ref_val = Img0->getValue(r0, c0, i);
							cam_val = Img1->getValue(r1, c1, i);

							// Add the values to sums for computing the normalized correlation
							sums.addValue(r0, c0, i*3, ref_val*ref_val);
							sums.addValue(r0, c0, i*3 + 1, cam_val*cam_val);
							sums.addValue(r0, c0, i*3 + 2, ref_val*cam_val);
						}
					}
				}

			// Average the image with radius WinRadL0
			sums.averageImage(WinRadL0);
		
			// For each pixel in the reference image...
			for(c0=0;c0<m_Cols;c0++)
				for(r0=0;r0<m_Rows;r0++)
				{
					// Compute the disparity in pixels
					disparityDel = d + m_MinD;

					// Compute the pixel location in the right image
					r1 = r0;
					c1 = c0 + disparityDel;

					val = 0.0;

					// Compute the normalized correlation value
					for(i=0;i<Img0->m_color;i++)
					{
						sums.setValue(r0, c0, i*3, sums.getValue(r0,c0,i*3) - 
							imageSums.getValue(r0,c0,i*2)*imageSums.getValue(r0,c0,i*2));
						sums.setValue(r0, c0, i*3 + 1, sums.getValue(r0,c0,i*3 + 1) - 
							imageSums.getValue(r1,c1,i*2 + 1)*imageSums.getValue(r1,c1,i*2 + 1));
						sums.setValue(r0, c0, i*3 + 2, sums.getValue(r0,c0, i*3 + 2) - 
							imageSums.getValue(r1,c1,i*2 + 1)*imageSums.getValue(r0,c0,i*2));

						denom = sums.getValue(r0, c0, 0)*sums.getValue(r0, c0, 1);
						if(denom != 0.0)
							val += sums.getValue(r0, c0, 2)/(sqrt(denom));
					}

					// Store the resulting value in L0
					m_L0Values->setValue(r0, c0, d, val/(double) Img0->m_color);
				}
		}

		// Delete arrays
		imageSums.Delete();
		sums.Delete();
	}
      
	// Use Sums of Absolute Differences
	if(USE_SAD)
	{
		// Create temporary array
		CImage sums(Img0->m_width, Img0->m_height, 3);

		// For each disparity level
		for(d=0;d<m_Depth;d++)
		{
			// Set array values to 0
			sums.clearImage();

			// For each pixel in the reference image
			for(c0=0;c0<m_Cols;c0++)
				for(r0=0;r0<m_Rows;r0++)
				{
		
					// Compute the disparity
					disparityDel = d + m_MinD;

					// Compute the pixel location in the right image
					r1 = r0;
					c1 = c0 + disparityDel;

					// Is the pixel not along the border?
					if(r1 >= BORDERRC && r1 <= m_Rows-BORDERRC && c1 >= BORDERRC && c1 <= m_Cols-BORDERRC &&
					   r0 >= BORDERRC && r0 <= m_Rows-BORDERRC && c0 >= BORDERRC && c0 <= m_Cols-BORDERRC)      
					{

						// For each color
						for(i=0;i<Img0->m_color;i++)
						{
							// Get the reference's image pixel value
							ref_val = Img0->getValue(r0, c0, i);

							// Get the right image's pixel value
							cam_val = Img1->getValue(r1, c1, i);

							// Compute the absolute difference
							val = (float) fabs((double) (ref_val - cam_val));

							// Scale to lie between 0 and 1 and inverse the value
							val = max(0.0, 1.0 - val/255.0);

							// Add the value to sums.
							sums.addValue(r0, c0, 0, val);
						}

						// Divide by the number of colors
						sums.setValue(r0, c0, 0, sums.getValue(r0, c0, 0)/(float) Img0->m_color); 
					}

				}

			// Average the absolute difference values
			sums.averageImage(WinRadL0);
		
			// For each pixel in the reference image
			for(c0=0;c0<m_Cols;c0++)
				for(r0=0;r0<m_Rows;r0++)
				{
					// Set the L0 values.
					m_L0Values->setValue(r0, c0, d, sums.getValue(r0, c0, 0));
				}
		}

		// Set array values to 0
		AvgLVals.clearImage();

		// Find the sum of all the difference values for each row and column.
		for(c0=0;c0<m_Cols;c0++)
			for(r0=0;r0<m_Rows;r0++)
				for(d=0;d<m_Depth;d++)
					AvgLVals.addValue(r0, c0, 0, m_L0Values->getValue(r0, c0, d));

		// Divide by the number of disparity levels to find the average difference
		// value for each row and column
		for(c0=0;c0<m_Cols;c0++)
			for(r0=0;r0<m_Rows;r0++)
				AvgLVals.setValue(r0, c0, 0, AvgLVals.getValue(r0, c0, 0)/(float) m_Depth);


		// Average the average values across the rows and columns.
		AvgLVals.averageImage(WinRadL0+1);

		// delete array
		sums.Delete();
	}

	// For each pixel in the reference image
	for(r0=BORDERRC;r0<m_Rows-BORDERRC;r0++)
		for(c0=BORDERRC;c0<m_Cols-BORDERRC;c0++)
		{

			// Is we used SAD
			if(USE_SAD)
			{
				avg = AvgLVals.getValue(r0, c0, 0);

				// If the avg is greater than the maximum scaler set to it the maximum scaler.
				if(avg > MaxScaler) avg = MaxScaler;
			}

			// For each disparity level
			for(d=0;d<m_Depth;d++)
			{ 
				// Get the difference value
				val = m_L0Values->getValue(r0, c0, d);

				// Scale the value if we're using SAD.
				if(USE_SAD)
					val = (val - avg)/(1.0 - avg);

				// Threshold the values to lie between 0.08 and 1.0
				if(val < 0.08)
					val = 0.08;
				if(val > 1.0)
					val = 1.0;

				// Set the value of the final L0 values.
				m_L0Values->setValue(r0, c0, d, val);
			}
		}

	// Do something around the border to make everything behave properly
	for(c0=0;c0<30;c0++)
		for(r0=BORDERRC;r0<m_Rows-BORDERRC;r0++)
			for(d=0;d<m_Depth;d++)
		{
			m_L0Values->setValue(r0, c0, d, (double) c0*m_L0Values->getValue(r0, c0, d)/30.0);
		}

	for(c0=m_Cols-30;c0<m_Cols;c0++)
		for(r0=BORDERRC;r0<m_Rows-BORDERRC;r0++)
			for(d=0;d<m_Depth;d++)
		{
			m_L0Values->setValue(r0, c0, d, (double) (m_Cols - c0)*m_L0Values->getValue(r0, c0, d)/30.0);
		}

	// Set the inital Ln values to the L0 values.
	for(c0=0;c0<m_Cols;c0++)
		for(r0=0;r0<m_Rows;r0++)
			for(d=0;d<m_Depth;d++)
				m_LnValues->setValue(r0, c0, d, m_L0Values->getValue(r0, c0, d));

	// We're done the first interation
	m_DoneFirstIteration = TRUE;

	/* make BORDERRCs of likelihood array equal to 0 */
	for(r0=0;r0<m_Rows;r0++)
		for(c0=0;c0<m_Cols;c0++)
			for(i=0;i<m_Depth;i++)
				if(r0 < BORDERRC || r0 >= m_Rows-BORDERRC || c0 < BORDERRC || 
				   c0 >= m_Cols-BORDERRC || i < BORDERD || i >= m_Depth-BORDERD)
					m_LnValues->setValue(r0, c0, i, 0.0);

	// Delete array
	AvgLVals.Delete();

}

/*****************************************************

FindFinalDisparityMap - Find the final disparity values
after the Ln values have converged.

******************************************************/

void CStereo::FindFinalDisparityMap()
{
	int r, c, i;
	float max, maxIdx, conf;
	float l0,l1,l2,del;

	// For each pixel in the reference image
	for(r=0;r<m_Rows;r++)
	{
		for(c=0;c<m_Cols;c++)
		{
			max = -1.0;
			maxIdx = 0;

			// Find the Ln value that is maximum for this row and column
			for(i=1;i<m_Depth-1;i++)
			{
				// To try and get subpixel resolution we'll look at three
				// Ln values at once.
				l0 = m_LnValues->getValue(r,c,i-1);
				l1 = m_LnValues->getValue(r,c,i);
				l2 = m_LnValues->getValue(r,c,i+1);

				if(l1 > max && l1 >= l0 && l1  >= l2)
				{
					max = l1;

					conf = l0 + l1 + l2;

					// try and get subpixel resolution 
					if(l0 < l2)
					{
						l2 -= l0;
						l1 -= l0;

						if(l1 != 0.0)
							del = l2/l1;
						else
							del = 0.0;

						maxIdx = (float) i + del*0.5;
					}
					else
					{
						l0 -= l2;
						l1 -= l2;

						if(l1 != 0.0)
							del = l0/l1;
						else
							del = 0.0;

						maxIdx = (float) i - del*0.5;
					}

				}
			}

			// Set the disparity and confidence values
			m_DisparityValues->setValue(r, c, 0, maxIdx + m_MinD);
			m_DisparityValues->setValue(r, c, 1, conf);
		}
	}

}

/*****************************************************

DoIterations - Do each iteration in refining the Ln values.

NumIterations - number of iterations to do.

******************************************************/

void CStereo::DoIterations(int NumIterations)
{
	int i;

	for(i=0;i<NumIterations;i++)
		DoAnIteration();
}

/*****************************************************

DoAnIteration - Do a single iteration to refine the Ln 
values.

******************************************************/

void CStereo::DoAnIteration()
{
	int row,col,i,c1;
	float sum, denom;
	float *tmpfa = new float [m_Rows+m_Cols+m_Depth];   
	CImage sums(m_Cols, m_Rows, 2);

	/* initialize temporary array */
   for(row=0;row<m_Rows;row++)
      tmpfa[row] = 0.0;


	/* make BORDERRCs of likelihood array equal to 0 */
	for(row=0;row<m_Rows;row++)
		for(col=0;col<m_Cols;col++)
			for(i=0;i<m_Depth;i++)
				if(row < BORDERRC || row >= m_Rows-BORDERRC || col < BORDERRC || 
				   col >= m_Cols-BORDERRC || i < BORDERD || i >= m_Depth-BORDERD)
					m_LnValues->setValue(row, col, i, 0.0);


	/* compute the Sn values and store them in diffa */

	// Sum along the row and column dimensions
	m_LnValues->sumImage(m_WinRC);

	// Sum along the disparity dimension
	for(row=BORDERRC;row<m_Rows-BORDERRC;row++)
		for(col=BORDERRC;col<m_Cols-BORDERRC;col++)
		{
			sum = 0.0;
			for(i=BORDERD-m_WinD;i<=BORDERD+m_WinD;i++)
				sum += m_LnValues->getValue(row, col, i);
			for(i=BORDERD;i<m_Depth-BORDERD;i++)
			{
				tmpfa[i] = sum;
				sum -= m_LnValues->getValue(row, col, i-m_WinD);
				sum += m_LnValues->getValue(row, col, i+m_WinD+1);
			}			

			for(i=BORDERD;i<m_Depth-BORDERD;i++)
				m_LnValues->setValue(row, col, i, tmpfa[i]);
		}
     

	delete [] tmpfa;

	/* compute the inhibitory voxels for each voxel */
	sums.clearImage();

	/* Compute the inhibitory sums along each row and column. */

	/* Relative to the reference image */
	for(row=BORDERRC;row<m_Rows-BORDERRC;row++)
		for(col=BORDERRC;col<m_Cols-BORDERRC;col++)
			for(i=BORDERD;i<m_Depth-BORDERD;i++)
				sums.addValue(row, col, 0, m_LnValues->getValue(row, col, i));


	/* Relative to the right image */
	for(row=BORDERRC;row<m_Rows-BORDERRC;row++)
		for(col=BORDERRC;col<m_Cols-BORDERRC;col++)
			for(i=BORDERD;i<m_Depth-BORDERD;i++)
			{
				c1 = col + i + m_MinD;
				if(c1 >= BORDERRC && c1 < m_Cols-BORDERRC)
					sums.addValue(row, c1, 1, m_LnValues->getValue(row, col, i));
			}

	/* Inhibit by the sum of the two */ 
	for(row=BORDERRC;row<m_Rows-BORDERRC;row++)
		for(col=BORDERRC;col<m_Cols-BORDERRC;col++)
			for(i=BORDERD;i<m_Depth-BORDERD;i++)
			{
				denom = sums.getValue(row, col, 0); 
				
				c1 = col + i + m_MinD;
				if(c1 >= BORDERRC && c1 < m_Cols-BORDERRC)
					denom += sums.getValue(row, c1, 1);
							      
				if(denom != 0.0) 
					m_LnValues->setValue(row, col, i, m_LnValues->getValue(row, col, i)/denom);
			}

    /* Make sure values are greater than 1 before squaring */
	for(row=BORDERRC;row<m_Rows-BORDERRC;row++)
		for(col=BORDERRC;col<m_Cols-BORDERRC;col++)
			for(i=BORDERD;i<m_Depth-BORDERD;i++)
			{
				if(m_LnValues->getValue(row, col, i) < 0.0)
					m_LnValues->setValue(row, col, i, 0.0); 
			}
 
	/* Square the Sn values for faster convergence */
	for(row=BORDERRC;row<m_Rows-BORDERRC;row++)
		for(col=BORDERRC;col<m_Cols-BORDERRC;col++)
			for(i=BORDERD;i<m_Depth-BORDERD;i++)
			{
				m_LnValues->setValue(row, col, i, 
					m_LnValues->getValue(row, col, i)*m_LnValues->getValue(row, col, i)); 
			}

   /* Inhibit by the intial values of the likelihood array */
	for(row=BORDERRC;row<m_Rows-BORDERRC;row++)
		for(col=BORDERRC;col<m_Cols-BORDERRC;col++)
			for(i=BORDERD;i<m_Depth-BORDERD;i++)
			{
				m_LnValues->setValue(row, col, i, 
					m_LnValues->getValue(row, col, i)*m_L0Values->getValue(row, col, i)); 
			}
   
	/* make BORDERRCs of likelihood array equal to 0 */
	for(row=0;row<m_Rows;row++)
		for(col=0;col<m_Cols;col++)
			for(i=0;i<m_Depth;i++)
				if(row < BORDERRC || row >= m_Rows-BORDERRC || col < BORDERRC || 
				   col >= m_Cols-BORDERRC || i < BORDERD || i >= m_Depth-BORDERD)
					m_LnValues->setValue(row, col, i, 0.0);

	/* Another iteration completed */
	m_IterationNum++;

	sums.Delete();
}

/******************************************************************************************
		Some miscellaneous functions for display
*******************************************************************************************/



void CStereo::ScaleAndDisplayDisparityValues()
{
	int r, c;
	float scale = 255.0/((float) m_MaxD - (float) m_MinD);

	for(r=0;r<m_Rows;r++)
		for(c=0;c<m_Cols;c++)
			m_DisplayImage->setValue(r, c, 0, scale*(m_DisparityValues->getValue(r, c, 0) - m_MinD));
}

void CStereo::DisplayConfidenceValues()
{
	int r, c;
	float val, scale = 4000.0;

	for(r=0;r<m_Rows;r++)
		for(c=0;c<m_Cols;c++)
		{
			val = scale*(m_DisparityValues->getValue(r, c, 1));
			if(val > 255.0)
				val = 255.0;

			m_DisplayImage->setValue(r, c, 1, val);
		}
}

void CStereo::DisplayLnSlice(int row, int scale)
{
	int c,i,j;
	float val;

	if(row < m_Rows)
		for(c=0;c<m_Cols;c++)
			for(i=0;i<m_Depth;i++)
				for(j=0;j<scale;j++)
				{
					val = 4000.0*m_LnValues->getValue(row, c, i);
					if(val > 255.0) val = 255.0;

					m_DisplayImage->setValue(i*scale + j, c, 0, val);
				}

}

void CStereo::DisplayL0Slice(int row, int scale)
{
	int c,i,j;
	float val;

	if(row < m_Rows)
		for(c=0;c<m_Cols;c++)
			for(i=0;i<m_Depth;i++)
				for(j=0;j<scale;j++)
				{
					val = 255.0*m_L0Values->getValue(row, c, i);
					if(val > 255.0) val = 255.0;

					m_DisplayImage->setValue(i*scale + j, c, 0, val);
				}
}

void CStereo::PrintDisparityValues(char *name)
{
	int r,c;
	FILE *outFile;

	outFile = fopen(name, "w");

	if(outFile == NULL)
		return;

	fprintf(outFile, "%d %d\n", m_DisparityValues->m_width, m_DisparityValues->m_height);

	for(r=0;r<m_Rows;r++)
		for(c=0;c<m_Cols;c++)
			fprintf(outFile, "%f %f\n", m_DisparityValues->getValue(r, c, 0), 
				m_DisparityValues->getValue(r, c, 1));

	fclose(outFile);
}



/**************************************************************************************
		A couple image routines to help
***************************************************************************************/

/************************************************

averageImage - Average the image with the specified 
radius.  Uses sumImage.

radius - radius for averaging.  Square window used.

*************************************************/

void CImage::averageImage(int radius)
{
	int row,col,i;
	float denom;

	denom = (float) radius*2.0 + 1.0;
	denom *= denom;

	sumImage(radius);

	for(i=0;i<m_color;i++)
		for(row=0;row<m_height;row++)
			for(col=radius;col<m_width-radius;col++)
			{
				setValue(row, col, i, getValue(row, col, i)/denom);
			}

} // averageImage


/************************************************

sumImage - Sum the image within a box with the
specified radius.  Uses an efficient method for
summation that is linear with respect to the 
window size.

radius - radius for summing.  Square window used.

*************************************************/

void CImage::sumImage(int radius)
{
	float *tmpa = new float [m_width + m_height];
	int row,col,i;
	float sum,denom;
	float *pt0, *pt1;
	int offset;

	denom = (float) radius*2.0 + 1.0;
	denom *= denom;

	for(i=0;i<m_color;i++)
	{
		/* sum along columns */
		offset = m_width*m_color;
		for(col=radius;col<m_width-radius;col++)			// col: x
		{
			sum = 0.0;
			for(row=0;row<=2*radius;row++)   
				sum += getValue(row, col, i);
			
			pt0 = getPointer(0, col, i);
			pt1 = getPointer(radius*2+1, col, i);

			for(row=radius;row<m_height-radius-1;row++)		// row : y
			{
				tmpa[row] = sum;
				sum -= *pt0;
				sum += *pt1;

				pt0 += offset;
				pt1 += offset;
			}

			pt0 = getPointer(radius, col, i);

			for(row=radius;row<m_height-radius;row++, pt0 += offset)		// row: y
				*pt0 = tmpa[row];
		}

		/* sum along rows */
		offset = m_color;
		for(row=radius;row<m_height-radius;row++)					// row : y
		{
			sum = 0.0;
			for(col=radius-radius;col<=radius+radius;col++)			// col : x
				sum += getValue(row, col, i);

			pt0 = getPointer(row, 0, i);
			pt1 = getPointer(row, radius*2+1, i);

			for(col=radius;col<m_width-radius-1;col++)
			{
				tmpa[col] = sum;
				sum -= *pt0;
				sum += *pt1;

				pt0 += offset;
				pt1 += offset;
			}
			for(col=radius;col<m_width-radius;col++)
				setValue(row, col, i, tmpa[col]);
		}

	}

	delete [] tmpa;
} // sumImage

const int pad = 64;

CImage::CImage(int width, int height, int channel_count)
{
	m_width = width;
	m_height = height;
	m_color = channel_count;

	m_data = new float[m_width*m_height*channel_count];
	clearImage();
}
CImage::~CImage()
{
	Delete();
}

void CImage::addValue(int y, int x, int channel, float v)
{
	float *p = getPointer(y,x,channel);
	*p += v;
}
void CImage::setValue(int y, int x, int channel, float v)
{
	float *p = getPointer(y,x,channel);
	*p = v;
}
float CImage::getValue(int y, int x, int channel)
{
	float *p = getPointer(y,x,channel);
	return *p;
}
float *CImage::getPointer(int y, int x, int channel)
{
	return &m_data[(y*m_width + x)*m_color + channel];
}


void CImage::Delete()
{
	if (m_data != NULL)
	{
		delete [] m_data;
		m_data = NULL;
	}
}
void CImage::clearImage()
{
	memset(m_data, 0, sizeof(float) * m_width*m_height*m_color);
}