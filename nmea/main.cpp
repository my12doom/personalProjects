#include "nmea/nmea.h"
#include <string.h>
#include <Windows.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

void gotoxy(int x,int y)  
{
	CONSOLE_SCREEN_BUFFER_INFO    csbiInfo;                            
	HANDLE    hConsoleOut;
	hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsoleOut,&csbiInfo);
	csbiInfo.dwCursorPosition.X = x;                                    
	csbiInfo.dwCursorPosition.Y = y;                                    
	SetConsoleCursorPosition(hConsoleOut,csbiInfo.dwCursorPosition);   
}
#define portName L"\\\\.\\COM4"

/*
  returns the utc timezone offset
  (e.g. -8 hours for PST)
*/
int get_utc_offset() {

  time_t zero = 24*60*60L;
  struct tm * timeptr;
  int gmtime_hours;

  /* get the local time for Jan 2, 1900 00:00 UTC */
  timeptr = localtime( &zero );
  gmtime_hours = timeptr->tm_hour;

  /* if the local time is the "day before" the UTC, subtract 24 hours
    from the hours to get the UTC offset */
  if( timeptr->tm_mday < 2 )
    gmtime_hours -= 24;

  return gmtime_hours;

}

int main()
{
	char tmp[40960];
	int c = 0;
    nmeaINFO info;
    nmeaPARSER parser;
    nmea_zero_INFO(&info);
    nmea_parser_init(&parser);

	HANDLE handlePort_ = CreateFileW(portName,  // Specify port device: default "COM1"
		GENERIC_READ | GENERIC_WRITE,       // Specify mode that open device.
		0,                                  // the devide isn't shared.
		NULL,                               // the object gets a default security.
		OPEN_EXISTING,                      // Specify which action to take on file. 
		0,                                  // default.
		NULL);                              // default.

	DCB config_;

	// Get current configuration of serial communication port.
	if (GetCommState(handlePort_,&config_) == 0)
	{
		printf("Get configuration port has problem.");
		return FALSE;
	}
	// Assign user parameter.
	config_.BaudRate = 115200;    // Specify buad rate of communicaiton.
	config_.StopBits = ONESTOPBIT;    // Specify stopbit of communication.
	config_.Parity = 0;        // Specify parity of communication.
	config_.ByteSize = 8;    // Specify  byte of size of communication.

	// Set current configuration of serial communication port.
	if (SetCommState(handlePort_,&config_) == 0)
	{
		printf("Set configuration port has problem.");
		return FALSE;
	}

// 	// instance an object of COMMTIMEOUTS.
// 	COMMTIMEOUTS comTimeOut;
// 	// Specify time-out between charactor for receiving.
// 	comTimeOut.ReadIntervalTimeout = 3;
// 	// Specify value that is multiplied 
// 	// by the requested number of bytes to be read. 
// 	comTimeOut.ReadTotalTimeoutMultiplier = 3;
// 	// Specify value is added to the product of the 
// 	// ReadTotalTimeoutMultiplier member
// 	comTimeOut.ReadTotalTimeoutConstant = 2;
// 	// Specify value that is multiplied 
// 	// by the requested number of bytes to be sent. 
// 	comTimeOut.WriteTotalTimeoutMultiplier = 3;
// 	// Specify value is added to the product of the 
// 	// WriteTotalTimeoutMultiplier member
// 	comTimeOut.WriteTotalTimeoutConstant = 2;
// 	// set the time-out parameter into device control.
// 	SetCommTimeouts(handlePort_,&comTimeOut);
// 
	int t = GetTickCount();
	while(true)
	{
		char p;
		DWORD got=0;
		if (ReadFile(handlePort_, &p, 1, &got, NULL) && got > 0)
		{
// 			if (p == '\r')
// 				continue;
// 			else 
				if (p == '\n')
			{
				tmp[c++] = p;
				nmea_parse(&parser, tmp, c, &info);


				const char* sig_tbl[] = {"Invalid", "Fix", "Differential", "Senstive"};        /**< GPS quality indicator (0 = Invalid; 1 = Fix; 2 = Differential, 3 = Sensitive) */
				const char* fix_tbl[] = {"Not Available", "2D", "3D"};
				int     fix;        /**< Operating mode, used for navigation (1 = Fix not available; 2 = 2D; 3 = 3D) */

				double  PDOP;       /**< Position Dilution Of Precision */
				double  HDOP;       /**< Horizontal Dilution Of Precision */
				double  VDOP;       /**< Vertical Dilution Of Precision */

				double  lat;        /**< Latitude in NDEG - +/-[degree][min].[sec/60] */
				double  lon;        /**< Longitude in NDEG - +/-[degree][min].[sec/60] */
				double  elv;        /**< Antenna altitude above/below mean sea level (geoid) in meters */
				double  speed;      /**< Speed over the ground in kilometers/hour */
				double  direction;  /**< Track angle in degrees True */
				double  declination; /**< Magnetic variation degrees (Easterly var. subtracts from true course) */

				nmeaSATINFO satinfo; /**< Satellites information */

				nmeaTIME utc = info.utc;
				gotoxy(0, 0);
				printf("Time : %04d-%02d-%02d %02d:%02d:%02d\n", utc.year+1900, utc.mon+1, utc.day, utc.hour, utc.min, utc.sec);
				printf("GPS quality : %s\n", sig_tbl[info.sig]);
				printf("Operating mode : %s\n", fix_tbl[info.fix]);
				printf("DOP(Position, Horizontal, Vertical): %f, %f, %f\n", info.PDOP, info.HDOP, info.VDOP);
				printf("Posistion:\n");
				printf("  Latitude: %.10f NDEG\n", info.lat);
				printf("  Longitude: %.10f NDEG\n", info.lon);
				printf("  Altitude: %f meter\n", info.elv);
				printf("Speed: %f km/h\n", info.speed);

				struct tm tm = {utc.sec, utc.min, utc.hour, utc.day, utc.mon, utc.year};
				time_t build_time = mktime(&tm) + get_utc_offset()*3600;
				time_t computer_time = time(NULL);

				c = 0;
			}
			else
				tmp[c++] = p;
		}
		else if (got == 0)
			Sleep(1);
		else
			break;
	}

	return 0;


    nmea_parser_destroy(&parser);

    return 0;
}
