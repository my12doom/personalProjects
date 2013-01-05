//  ****************************************************************************************************
//
//  For non-commercial use only.  Do not distribute without permission from Carnegie Mellon University.  
//
//  Copyright (c) 2002, Carnegie Mellon University     All rights reserved.   
//
//  ****************************************************************************************************

/*******************************************************************************************************

Header file for stereo.cpp

The routines for reading in images and basic image manipulation
are not included, since these would be specific to each individual
project.  Translation to other image libraries should not be 
difficult.

Any questions should be sent to clz@cs.cmu.edu

Larry Zitnick

********************************************************************************************************/
#pragma once
#include <Windows.h>


#define BORDERRC 4
#define BORDERD 3
#define NO_INFORMATION -1

class CImage
{
public:
	CImage(int width, int height, int channel_count);
	~CImage();
	
	void addValue(int y, int x, int channel, float v);
	void setValue(int y, int x, int channel, float v);
	float getValue(int x, int y, int channel);


	void Delete();
	void clearImage();
	void averageImage(int radius);
	void sumImage(int radius);
	float *getPointer(int x, int y, int channel);

	int m_width;
	int m_height;
	int m_color;
	float *m_data;
};

class CStereo
{
private:
	CImage *m_L0Values;					// Reference (left) image
	CImage *m_LnValues;					// The right image


	int m_MinD;							// Minimum disparity
	int m_MaxD;							// Maximum disparity

public:
	CImage *m_DisparityValues;			
	CImage *m_DisplayImage;
	
	int m_Rows;							// Number of rows in the images
	int m_Cols;							// Number of columns in the images
	int m_Depth;						// Number of disparity levels
	int m_WinD;							// Radius of window in the disparity direction
	int m_WinRC;						// Radius of window in the row and column directions
	int m_IterationNum;					// Number of iterations to compute
	char m_Name0[100], m_Name1[100];	// Names of the images
	char m_OutName[100];				// Name of output file

	BOOL m_DoneFirstIteration;			// Completed first iteration

	void FindDisparityMap(CImage *Img0, CImage *Img1,  
		int MinDisparity, int MaxDisparity, int WinRadL0, 
		int WinRC, int WinD, int NumIterations, float MaxScaler, BOOL USE_SAD);
	
	void ComputeL0Values(CImage *Img0, CImage *Img1, int WinRadL0, float MaxScaler, BOOL USE_SAD);

	void DoIterations(int NumIterations);
	void DoAnIteration();
	void FindFinalDisparityMap();
	void ScaleAndDisplayDisparityValues();
	void DisplayConfidenceValues();
	void DisplayL0Slice(int row, int scale);
	void DisplayLnSlice(int row, int scale);
	void PrintDisparityValues(char *name);

	void Delete()
	{
		if(m_L0Values != NULL)
		{
			m_L0Values->Delete();
			delete m_L0Values;
		}

		if(m_LnValues != NULL)
		{
			m_LnValues->Delete();
			delete m_LnValues;
		}

		if(m_DisparityValues != NULL)
		{
			m_DisparityValues->Delete();
			delete m_DisparityValues;
		}

		m_L0Values = NULL;
		m_LnValues = NULL;
		m_DisparityValues = NULL;
		m_DoneFirstIteration = FALSE;
		m_Rows = m_Depth = m_Cols = 0;
		m_IterationNum = -1;

	}

	CStereo()
	{
		m_DoneFirstIteration = FALSE;
		m_IterationNum = -1;
		m_L0Values = NULL;
		m_LnValues = NULL;
		m_DisparityValues = NULL;
		m_Rows = m_Depth = m_Cols = 0;
		m_WinD = 1;
		m_WinRC = 2;
	}
};

