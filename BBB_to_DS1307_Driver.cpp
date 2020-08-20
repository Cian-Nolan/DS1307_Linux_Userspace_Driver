#include<iostream>
#include<fcntl.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<linux/i2c.h>
#include<linux/i2c-dev.h>
#include<sstream>
#include <string>
#include <errno.h>
using namespace std;
#define BUFFER_SIZE 19

#define HEX(x) setw(2) << setfill('0') << hex << (int)(x)

int bcdToDec(char b)
{
	return (b/16)*10 + (b%16);
}

/*
The rtc class wraps the functionality of the real time clock. The class constructor takes 
two integer arguments, representing (1) the bus ID and (2) the device ID as discovered from 
the i2cdetect command.
*/
class rtc
{
private:
	unsigned int bus;
	unsigned int device;
	int file;
	const char* name;
	virtual int RTCReadClock();
	char buf[BUFFER_SIZE];
	int bufInt[BUFFER_SIZE];
	char bufAlarm[BUFFER_SIZE];
	int bufAlarmInt[BUFFER_SIZE];

public:
	rtc(unsigned int bus, unsigned int device);
	virtual ~rtc();
	virtual int RTCopen();
	virtual void RTCclose();
	virtual int currentTime();
	virtual int RTCGetHour();
	virtual int RTCGetMinute();
	virtual int RTCGetSecond();
	virtual int RTCGetDay();
	virtual int RTCGetMonthDay();
	virtual int RTCGetMonth();
	virtual int RTCGetYear();
	virtual int RTCStopClock();
	virtual int RTCStartClock();
	virtual int RTCSetTime(int hour, int minute, int second);
	virtual int RTCSetDate(int day, int month, int year, int weekday);
	virtual int RTCSetAlarm(int hour, int minute, int second, int day, int month, int year);
	virtual bool RTCAlarmActive();
	virtual int RTCSetFrequencySQW(int frequency);
	virtual bool RTCEnableSQW(bool enable);
	virtual bool RTCEnableOUT(bool enable);

};


rtc::rtc(unsigned int bus, unsigned int device)
{
	this->file=-1;
	this->bus = bus;
	this->device = device;
	this->RTCopen();
	this->name = "";
}
//DESTRUCTOR
rtc::~rtc()
{
	RTCclose();
	cout <<	"program terminated " << endl;
}



/*
RTCSetTime() takes three integer arguments (hour, minute, second). These values are passed 
into a buffer, which is subsequently written to the device registers
*/
int rtc::RTCSetTime(int hour, int minute, int second)
{
	RTCReadClock();

	unsigned char buffer[4];
	buffer[0] = 0x00;
	buffer[1] = (unsigned char)second;
	buffer[2] = (unsigned char)minute;
	buffer[3] = (unsigned char)hour;
	if(::write(this->file, buffer, 4)!=4)
	{
		perror("I2C: Failed write to the device\n");
		return 1;
	}

	cout << "exe!" << endl;

	cout<< "1 " << hour << endl;
	cout<< "2 " << buffer[2] << endl;
	cout<< "3 " << buffer[3] << endl;

	return 0;
}

int rtc::RTCSetDate(int day, int month, int year, int weekday)
{
	RTCReadClock();
	unsigned char buffer[5];
	buffer[0] = 0x03;
	buffer[1] = (unsigned char)weekday;
	buffer[2] = (unsigned char)day;
	buffer[3] = (unsigned char)month;
	buffer[4] = (unsigned char)year;

	cout << "buf 1 is" << (int)month << endl;
	if(::write(this->file, buffer, 5)!=5)
	{
		perror("I2C: Failed write to the device\n");
		return 1;
	}

	cout << "exe!" << endl;
	return 0;
}


/*
The RTCAlarmActive() function compares the values in 0x10 - 0x15 to the current time values in 
registers 0x00 - 0x06. The alarm becomes active when alarm parameters of second, minute, hour, day,
month and year are equal to the current value for second, minute, hour, day month and year. When the 
alarm conditions are satisfied, fopen() opens the appropriate GPIO file, then "1" is printed to the 
file which switches on the LED. When the alarm once again becomes inactive, "0" is written to the same 
file to switch off the LED.
*/
bool rtc::RTCAlarmActive()
{
	usleep(1000000);
	RTCReadClock();
	cout << "current time is: " << bufInt[0] << endl;
	char writeBuffer[1] = {0x10};
	if(write(this->file, writeBuffer, 1)!=1)
	{
		perror("Failed to reset the read address\n");
		return 1;
	}
	if(read(this->file, bufAlarm, BUFFER_SIZE)!=BUFFER_SIZE)
	{
		perror("Failed to read in the buffer\n");
		return 1;
	}

	for (int k = 0;  k < BUFFER_SIZE;  k++)
	{
		bufAlarmInt[k] = bcdToDec(bufAlarm[k]);
	}

    // Credit: Exploring Beaglebone/ch05/makeLED/makeLED.c, Derek Molloy
	FILE* fp;
	char filename[100] = "/../sys/class/gpio/gpio60/value";
	bool timeOK;
	bool dateOK;

	if ((bufAlarmInt[0] == bufInt[0]) && (bufAlarmInt[1] == bufInt[1]) && (bufAlarmInt[2] == bufInt[2]))
	{
		timeOK = true;
	}else
	{
		timeOK = false;
	}

	if ((bufAlarmInt[3] == bufInt[4]) && (bufAlarmInt[4] == bufInt[5]) && (bufAlarmInt[5] == bufInt[6]))
	{
		dateOK = true;
	}else
	{
		dateOK = false;
	}

	if ( (timeOK==true) && (dateOK==true))
	{
		cout << "ALARM TRIGGERED" << endl;
		fp = fopen(filename, "w+");
		fprintf(fp,"%s","1");
		fclose(fp);
		return 1;

	}else
	{
		fp = fopen(filename, "w+");
		fprintf(fp,"%s","0");
		fclose(fp);
		return 0;
	}
}

/*
The alarm is set by the RTCSetAlarm() function. The function takes six integers arguments 
(alarm hour, alarm minute, alarm second, alarm day, alarm month, alarm year). These values 
are passed into an array which is subsequently written to registers 0x10 - 0x15.
*/

int rtc::RTCSetAlarm(int hour, int minute, int second, int day, int month, int year)
{
	RTCReadClock();
	unsigned char buffer[7];
	buffer[0] = 0x10;
	buffer[1] = (unsigned char)second;
	buffer[2] = (unsigned char)minute;
	buffer[3] = (unsigned char)hour;
	buffer[4] = (unsigned char)day;
	buffer[5] = (unsigned char)month;
	buffer[6] = (unsigned char)year;
	if(::write(this->file, buffer, 7)!=7)
	{
		perror("I2C: Failed write to the device\n");
		return 1;
	}

	cout << "alarm set!" << endl;
	return 0;
}

/*
Registers 0 to BUFFER_SIZE are read into the bufClock[] char array. The bufClock[] array is then 
converted to an array of integers, bufClockInt[], with the use of the bcdToDec() function.
*/
int rtc::RTCReadClock()
{

	char writeBuffer[1] = {0x00};
	if(write(this->file, writeBuffer, 1)!=1)
	{
		perror("Failed to reset the read address\n");
		return 1;
	}

	//char buf[BUFFER_SIZE];
	if(read(this->file, buf, BUFFER_SIZE)!=BUFFER_SIZE)
	{
		perror("Failed to read in the buffer\n");
		return 1;
	}
	for (int k = 0;  k < BUFFER_SIZE;  k++)
	{
		bufInt[k] = bcdToDec(buf[k]);
	}
}

//CLOSE CONNECTION TO FILE
void rtc::RTCclose()
{
	close(this->file);
	cout <<	"connection closed" << endl;
}

/*
This acts to open a connection to the specified i2c device. 
A switch statement is used to pass the appropriate file path to the name variable.
*/
int rtc::RTCopen()
{
	cout << "Starting the DS3231 application on BUS: " << this->bus << ", DEVICE: " << this->device << endl;

	switch (this->bus)
	{
	case 0:
		this->name = "/dev/i2c-0";
		break;
	case 1:
		this->name = "/dev/i2c-1";
		break;
	case 2:
		this->name = "/dev/i2c-2";
		break;
	default:
		cout << "INVALID BUS ERROR" <<  endl;
	}

	if((this->file=open(name, O_RDWR)) < 0){
		perror("failed to open the bus\n");
		return 1;
		printf("file descriptor is: %d\n" , this->file);
	}
	if(ioctl(this->file, I2C_SLAVE, this->device) < 0){
		perror("Failed to connect to the sensor\n");
		return 1;
	}
}

/*
Each of the registers within bufClockInt[] can then be printed to the console and returned by means 
of a number of accessor functions. The accessor function calls the 
RTCReadClock() function which updates the bufClock[] array. The appropriate array element is read from 
the array, the value is printed to the console, and then the value is returned.
*/
int rtc::RTCGetYear()
{
	RTCReadClock();
	cout << "Current Day Year: " << bufInt[6] << endl;
	return bufInt[6];
}

int rtc::RTCGetMonth()
{
	RTCReadClock();
	cout << "Current Day of Month: " << bufInt[5] << endl;
	return bufInt[5];
}

int rtc::RTCGetMonthDay()
{
	RTCReadClock();
	cout << "Current Day of Month: " << bufInt[4] << endl;
	return bufInt[4];
}
int rtc::RTCGetDay()
{
	RTCReadClock();
	cout << "Current Day: " << bufInt[3] << endl;
	return bufInt[3];
}

int rtc::RTCGetHour()
{
	RTCReadClock();
	cout << "Current Hour: " << bufInt[2] << endl;
	return bufInt[2];
}

int rtc::RTCGetMinute()
{
	RTCReadClock();
	cout << "Current Minute: " << bufInt[1] << endl;
	return bufInt[1];
}
int rtc::RTCGetSecond()
{
	RTCReadClock();
	cout << "Current Second: " << bufInt[0] << endl;
	return bufInt[0];
}

//RETURN CURRENT TIME
int rtc::currentTime()
{
	RTCReadClock();

	printf("The RTC time is %02d:%02d:%02d\n", bufInt[2],bufInt[1],bufInt[0]);

	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		cout << "#" << i << ": "<< bufInt[i] << endl;
	}
}

/*
The clock halt bit is used to stop the oscillator when timekeeping is not required by the system, which 
acts to reduce energy consumption. This feature can be controlled via bit 7 of register 0x00. The control 
to stop/start the clock was added to the rtc class as a novel functionality. If the start/stop function is called, and the 
clock is already in that required state, an error message is printed to the console.
*/
int rtc::RTCStopClock()
{
	RTCReadClock();

	char readByte = buf[0];
	char controlByte = 	0b10000000;
	char resultByte;
	if ( (int)readByte < 128 )
	{
		resultByte  = (readByte ^ controlByte);
		unsigned char buffer[2];
		buffer[0] = 0x00;
		buffer[1] = resultByte;
		if(::write(this->file, buffer, 2)!=2)
		{
			perror("I2C: Failed write to the device\n");
			return 1;
		}
	}else
	{
		cout << "RTC already Stopped. Cannot execute RTCStopClock() unless device is running" << endl;
	}
	return 0;
}

int rtc::RTCStartClock()
{
	RTCReadClock();
	char readByte = buf[0];
	char controlByte = 	0b10000000;
	char resultByte;
	if ( (int)readByte >= 128 )
	{
		resultByte  = (readByte ^ controlByte);
		unsigned char buffer[2];
		buffer[0] = 0x00;
		buffer[1] = resultByte;
		if(::write(this->file, buffer, 2)!=2)
		{
			perror("I2C: Failed write to the device\n");
			return 1;
		}

	}else
	{
		cout << "RTC already running. Cannot execute RTCStartClock() unless device is stopped" << endl;
	}
	return 0;

}

/*
The general process used for manipulating individual bits in the register was to (1) read the current register 
value, (2) mask the register so that only the bit(s) of interest are eliminated, (3) combine the masked register 
with the 'control' register, which has the specific bits that require modifying. The resultant register is called 
the 'result' register. The 'result' register is then written to the register 0x07.
*/
int rtc::RTCSetFrequencySQW(int frequency)
{
	RTCReadClock();
	char readByte = buf[7];
	char maskByte = 0b11111100;
	char controlByte;

	switch (frequency)
	{
	case 1:
		controlByte = 0b00000000;
		cout << "case 1" <<endl;
		break;
	case 4:
		controlByte = 0b00000001;
		break;
	case 8:
		controlByte = 0b00000010;
		break;
	case 32:
		controlByte = 0b00000011;
		break;
	default:
		controlByte = 0b00000000;
	}

	char resultByte;

	resultByte  =  ( controlByte | (readByte & maskByte) );

	cout << "mask byte is: " << (int)maskByte << endl;
	cout << "read byte is: " << (int)readByte << endl;
	cout << "control byte is: " << (int)controlByte << endl;
	cout << "result byte is: " << (int)resultByte << endl;

	unsigned char buffer[2];
	buffer[0] = 0x07;
	buffer[1] = resultByte;
	if(::write(this->file, buffer, 2)!=2)
	{
		perror("I2C: Failed write to the device\n");
		return 1;
	}

	return 0;

}

bool rtc::RTCEnableSQW(bool enable)
{
	RTCReadClock();
	char readByte = buf[7];
	char maskByte = 0b11101111;
	char controlByte;
	char resultByte;

	if (enable==true)
	{
		controlByte = 0b00010000;
		resultByte  =  ( controlByte | (readByte & maskByte) );
	}else
	{
		controlByte = 0b00000000;
		resultByte  =  ( controlByte | (readByte & maskByte) );
	}


	cout << "mask byte is: " << (int)maskByte << endl;
	cout << "read byte is: " << (int)readByte << endl;
	cout << "control byte is: " << (int)controlByte << endl;
	cout << "result byte is: " << (int)resultByte << endl;

	unsigned char buffer[2];
	buffer[0] = 0x07;
	buffer[1] = resultByte;
	if(::write(this->file, buffer, 2)!=2)
	{
		perror("I2C: Failed write to the device\n");
		return 1;
	}

	return 0;
}


bool rtc::RTCEnableOUT(bool enable)
{
	RTCReadClock();
	char readByte = buf[7];
	char maskByte = 0b01111111;
	char controlByte;
	char resultByte;

	if (enable==true)
	{
		controlByte = 0b10000000;
		resultByte  =  ( controlByte | (readByte & maskByte) );
	}else
	{
		controlByte = 0b00000000;
		resultByte  =  ( controlByte | (readByte & maskByte) );
	}


	cout << "mask byte is: " << (int)maskByte << endl;
	cout << "read byte is: " << (int)readByte << endl;
	cout << "control byte is: " << (int)controlByte << endl;
	cout << "result byte is: " << (int)resultByte << endl;

	unsigned char buffer[2];
	buffer[0] = 0x07;
	buffer[1] = resultByte;
	if(::write(this->file, buffer, 2)!=2)
	{
		perror("I2C: Failed write to the device\n");
		return 1;
	}

	return 0;
}


int main()
{

	rtc clock(2, 0x68);

	clock.currentTime();

	clock.RTCGetHour();

	clock.RTCGetMinute();

	clock.RTCGetSecond();

	clock.RTCGetDay();

	clock.RTCGetMonthDay();

	clock.RTCGetMonth();

	clock.RTCGetYear();

	//clock.RTCStartClock();

	clock.RTCSetTime(0x10,0x15,0x40);

	clock.RTCSetDate(0x11,0x12,0x13,0x07); //(dd,mm,yy,wd)

	clock.RTCSetAlarm(0x10,0x15,0x45,0x11,0x12,0x13); // (hh,mm,ss,dd,mm,yy)

	clock.RTCSetFrequencySQW(32);

	clock.RTCEnableSQW(true);

	clock.RTCEnableOUT(false);

}