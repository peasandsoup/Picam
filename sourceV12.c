//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890
////////////////////////////////////////////////////////////////////////////////
//  written by: Craig Best                                                    //
//                                   PICAM                                    //
//                                                                            //
//	Takes photos in timed sequence on command of semaphores.                  //
//                                                                            //
//	Semaphores are in  : /home/pi/Grenlec/semaphores.                         //
//                                                                            //
//	--------------	-----------------------------------------------	          //
//	   Semaphore			Control Operation                                 //
//	--------------	-----------------------------------------------           //
//	camera-on       allows pictures to be taken                               //
//	read-config     allows sleep interval to be changed                       //
//	cam-xxxxxx      Index semaphore which controls index of current picture   //
//	--------------	-----------------------------------------------           //
//                                                                            //
//	Places the photos in :  /home/pi/GRENLEC/usb                              //
//                                                                            //
//	!!! CURRENT File format : 2107-02-04_17:23:10.jpg                         //
//                                                                            //
//	A USB flash drive can be mounted at /home/pi/GRENELEC/usb to              //
//	provide removable storage.                                                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <sys/stat.h>   

#include <unistd.h>

#define HIGHEST_INDEX 9155

/*
 * last edit: Wed Apr  4 
 * to implement:
*/
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
                        void Get_SleepTime(char*, int*);
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
                                void main()
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
{
    FILE* DefaultConfig;  //pointer to Default-config file
    //FILE* ReadIndex;    //pointer to picture index semaphore
    int iSleepTime = 30; //default 30 seconds. Replacement value will be read from Default-config, if it exists
    char sIndexCompare[] = "/home/pi/GRENLEC/semaphores/cam-000001";   //used to find index semaphore, default to 1
    char sIndexCompareNext[] = "/home/pi/GRENLEC/semaphores/cam-000001";     //used to write new index semaphore
    int iTheIndex;   //will be used to store the value of the file index
    int iNextIndex;  //will be used to store the value of the next file index, used to write the next file index semaphore
    char* sIndexNumber;    //pointer that can be used to get the number part of sIndexCompare
    char* sIndexNumberNext;    //pointer that can be used to get the number part of sIndexCompareNext
    char sTakePicture[200];      //array to hold the command to take a picture
    char sRemovePicture[200];   //array to hold the command to remove a picture
    char sErrorCheck[200];      //array that will hold the name of the previosly taken picture, for error checking purposes

    char* sFileDefaultConfig = "/home/pi/GRENLEC/Default-config";
    char* sFileReadConfig = "/home/pi/GRENLEC/semaphores/read-config";
    char* sFileCameraOn = "/home/pi/GRENLEC/semaphores/camera-on";
    char* sUSBPath = "/home/pi/GRENLEC/usb/";

    time_t rawtime;
    struct tm *timeinfo;
    char sTime[30];
    
    sIndexNumber = sIndexCompare+32;   //sIndexNumber now points to the part of sIndexCompare that contains numbers
    sIndexNumberNext = sIndexCompareNext+32;     //sIndexNumberNext now points to the part of sIndexCompareNext that contains numbers

    //redirect all stderr output to /dev/null 
    //freopen("/dev/null", "w", stderr);

    //read Default-config file, set sample interval using the read value
    Get_SleepTime(sFileDefaultConfig, &iSleepTime);

    while(1)
    {
        //IF semaphore camera-on exists
        if(fopen(sFileCameraOn, "r") != 0)
        {
            printf("camera-on found\n");
            for(iTheIndex = 1; iTheIndex <= HIGHEST_INDEX+1; iTheIndex++)        //search for the Index semaphore file
            {
                sprintf(sIndexNumber, "%.6i", iTheIndex);    //replace the index number in the string sIndexNumber with the current index iteration 
                if(fopen(sIndexCompare, "r") != 0)     //if sIndexCompare matches an existing file
                {
                    printf("Index semaphore found: %s\n", sIndexCompare);
                    break;      //if the sIndexCompare matches a filename (index semaphore), break from the loop
                }

            }
            if(iTheIndex > HIGHEST_INDEX)        //if the loop ended without finding the index semaphore 
            {

                iTheIndex = 1;
                sprintf(sIndexNumber, "%.6i", iTheIndex);
                iNextIndex = 2;
            }
            else if(iTheIndex < HIGHEST_INDEX)   //if the index semaphore was found
            {
                iNextIndex = iTheIndex + 1;
            }
            else  //iTheIndex == HIGHEST_INDEX
                iNextIndex = 1;
            sprintf(sIndexNumberNext, "%.6i", iNextIndex);    //replace the index number in the string sIndexNumberNext with the index number + 1
            
            //read system time
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime (sTime, 40, "%F_%X", timeinfo);

            //delete index semaphore
            remove(sIndexCompare);

            //assemble new index semaphore
            fopen(sIndexCompareNext, "wt");

            //remove old picture 
            strcpy(sRemovePicture, "rm ");
            strcat(sRemovePicture, sUSBPath);
            strcat(sRemovePicture, sIndexNumber);
            strcat(sRemovePicture, "_*.jpg");       //any picture whose index matches sIndexNumber
            system(sRemovePicture);

            //Take picture
            strcpy(sTakePicture, "fswebcam ");
            strcat(sTakePicture, sUSBPath);
            strcat(sTakePicture, sIndexNumber);
            strcat(sTakePicture, "_");
            strcat(sTakePicture, sTime);
            strcat(sTakePicture, ".jpg");
            system(sTakePicture);
            sleep(1);

            //check that picture was taken succesfully
            strcpy(sErrorCheck, sUSBPath);
            strcat(sErrorCheck, sIndexNumber);
            strcat(sErrorCheck, "_");
            strcat(sErrorCheck, sTime);
            strcat(sErrorCheck, ".jpg");
            for(int i = 0; i < 2; i++)
            {
                if(fopen(sErrorCheck, "r") != 0)
                {
                    printf("Picture taken succesfully.\n");
                    break;
                }
                else
                {
                    printf("Picture not found. Retaking picture.\n");
                    system(sTakePicture);
                    sleep(1);
                }

            }



            if(fopen(sFileReadConfig, "r") != 0)        //if read-config exists
            {
                printf("read-config found\n");
                //Open Default-config to be read
                DefaultConfig = fopen(sFileDefaultConfig, "r");
                //read Default-config file, set sample interval using the read value
                Get_SleepTime(sFileDefaultConfig, &iSleepTime); 
                //close Default-config file
                fclose(DefaultConfig);

            }
            else
                printf("read-config not found\n");
        }
        else
            printf("camera-on not found\n");
        //sleep
        printf("sleep time: %d\n\n", iSleepTime);
        sleep(iSleepTime);

    }
    return;
}

/*
This function takes the filename of Default-config and the current sleeptime as arguments and 
searches Default-config for the line, formatted according to the design document, which sets sleeptime.
If that line is found, sleeptime is updated.
 */
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
             void Get_SleepTime(char* sFileName, int* iSleepLength)
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
{
    FILE* FileVariable;
    size_t n = 200; //size_t means int
    char* sSingleLine = (char*)malloc(n);
    int iLineCount = 0;
    int iSleepDummy = 0;    //dummy variable to be assigned to sleep variable when all conditions are met   
    int iWordCount;   //The number of words in sSingleLine
    char sWord1[50], sWord2[50], sGarbage[50];
    char sCompare1[] = "camera";
    char sCompare2[] = "interval";

    FileVariable = fopen(sFileName, "r");
    
    for(iLineCount = 0; iLineCount < 10; iLineCount++)  //read 10 lines and then quit even if EOF is not encountered
    {
        if(FileVariable == NULL)    //if Default-config doesn't exist
        {
            break;  //do this to avoid a segmentation fault when getline is called and passed a NULL
        }
        getline(&sSingleLine, &n, FileVariable);        //scan a line from the file into sSingleLine
        iWordCount = sscanf(sSingleLine, "%s %s %i %s", sWord1, sWord2, &iSleepDummy, sGarbage);
        if(iWordCount == 3 && strcmp(sWord1, sCompare1) == 0 && strcmp(sWord2, sCompare2) == 0 && iSleepDummy >= 0)
        {
            *iSleepLength = iSleepDummy;
            break;
        }
    }

    free(sSingleLine);  //prevent a memory leak
    //return iSleepLength; 
}
