#include "Pedo_variables.h"
#include "Pedo.h"
#include "sink_private.h"
#include "sink_debug.h"


/*
 Change algorithm as follows:
 Buffer is a FIFO, At interrupt, data is pushed into element 0
 After each interrupt, filtering is done and data is shifted one element left.
 Filter element is put into another filter if required.
 Only the following shall be stored:
 Last peak value and valley value and then a counter to count the time elapsed
 if peak value and the new change of direction value is larger than threshold and
 the time interval is large enough, we consider it as a step.
*/
void   StoreData(INT8S * sensorData);
INT16U GetStep(void);
void   SetStep( INT16U step );
void   InitCntStep(void * ptr, INT16U amp_thres, INT8U interval_thres);
void CntStep(void);


void StoreData(INT8S * sensorData) {  
	buffer[0]=sensorData[0]*sensorData[0]+sensorData[1]*sensorData[1]+sensorData[2]*sensorData[2];   
}

INT16U GetStep(void) {
  return gSteps;
}

void SetStep( INT16U step ) {
  gSteps = step;
}

/*********************************

**********************************/
void InitCntStep(void * ptr, INT16U amp_thres, INT8U interval_thres){  
	INT8U  i;

   gPedoPtr = (pedoData *) ptr;
	gStepsElapsed = 0;
	gLastValue = 0;
	gEdgeType=INVALID;
	gLastType=INVALID;
	gLastPeakValue = 0;  
	gLastValleyValue = 0; 
	gAmpThres = amp_thres;
	gIntervalThres = interval_thres;
	
	for (i=0; i<BUFFER_SIZE; i++) buffer[i] = 0;  
}

/********************************
	Filter function
**********************************/
/*Input: dataPtr, Pointer to a buffer for averaging
        length, length of data
 Output: outPtr, averaging value, all data shall shift one to left.
*/
static void Filter(INT16U  * dataPtr, INT8U length, INT16U * outPtr)
{	
	INT8U  i;
	INT32U temp;
	
	temp = 0;
	for (i=length-1;;i--) {
	  temp += dataPtr[i];
	  if (i!=0) {
	    dataPtr[i] = dataPtr[i-1];
	  } else break;
	}
	*outPtr = (INT16U) (temp / length);
}

/**********************************************
Function name:count steps
description: processing the wave signal
This shall be called when a sample is received.
***********************************************/
void CntStep(void)
{ 
	INT16U presentValue;

	/* Filter Data        */
	gStepsElapsed++;


    
    /********************************
        Filter function
    **********************************/
    /*Input: dataPtr, Pointer to a buffer for averaging
            length, length of data
     Output: outPtr, averaging value, all data shall shift one to left.
    */

#ifdef TWO_FILTER
	Filter(buffer, FILTER_POINT, &buffer[FILTER_POINT]);
	Filter(&buffer[FILTER_POINT], FILTER_POINT, &presentValue);
	if ( (gStepsElapsed < 2*FILTER_POINT) && (gEdgeType==INVALID) ) return;
#else
	Filter(buffer, FILTER_POINT, &presentValue);
	if ( (gStepsElapsed < FILTER_POINT) && (gEdgeType==INVALID) ) return;
#endif  

	if (gEdgeType==INVALID) {   
		/*if it is the first time, just judge the edge types and stores the min. & max.*/
		if (gLastValue ==0) {	    
			gLastValue = presentValue;
		} else if (presentValue < gLastValue) {	    
			gEdgeType=FALLING;      /* Falling*/
		} else {
			gEdgeType=RISING;     /* Rising*/
		}
		gLastValleyValue = presentValue; 
		gLastPeakValue = presentValue; 
		return;
	}	
	if (gEdgeType==RISING) {
		/* rising*/
		if (gLastValue>presentValue){    
			/* if the present value is smaller than last one*/
			/* a local max. was found*/
			gEdgeType=FALLING;
			if (gLastType!=ISPEAK) {
				/* When we searching for next peak*/
				if ((gStepsElapsed>gIntervalThres) && ((gLastValue-gLastValleyValue)>=gAmpThres) ) {
					/* a valid peak is a local maximum with large threshold and large timing elapsed*/
					gStepsElapsed = 0;
					gLastPeakValue = gLastValue;
					gLastType = ISPEAK;
					gSteps++;
					/*gpio_trig_ap_int();*/ /* give int*/
				}
			} else {
				/* When we searching for next valley but we find a peak*/
				/* record this as a new peak but not increase steps*/
				if (presentValue >= gLastPeakValue) {
					gStepsElapsed = 0;
					gLastPeakValue = gLastValue;
				}
			}
		} /* detected a local max.*/
	}  /* rising sample*/
	else {  /*falling edge*/
		if (gLastValue<presentValue) {
			/* if present value is larger than last one .*/
			/* a local min. was found.*/
			gEdgeType=RISING;  
			if (gLastType!=ISVALLEY) {
				if ((gStepsElapsed>gIntervalThres) && ((gLastPeakValue-gLastValue)>=gAmpThres) ) {
					/* a valid valley value is a local minimum with large threshold and large timing elapsed.*/
					gStepsElapsed = 0;
					gLastValleyValue = gLastValue;
					gLastType = ISVALLEY;
				}
			} else {
				/* when searching for next peak but found another valley.*/
				/* record this as a new valley .*/
				if (presentValue <= gLastValleyValue) {
					gStepsElapsed = 0;
					gLastValleyValue = gLastValue;
				}
			}
		}  /* detected a local min.	.*/			
	}  /* falling edge		.*/
	gLastValue = presentValue;            
}


/* Energy Calculation.*/
#define TABLE_LEN  8  /* # of different class*/
#define REST_CNT 100  /* 2s*/

#define AMP_THRES  800    /*AmpThres= Acceleration Threshold value*4000, for example, if Acceleration Threshold is 0.2g, then AmpThres=800	*/
#define INTERVAL_THRES 8  /*IntervalThres= (1/maxim frequency)/20ms, for example, if max fre is 4Hz, then IntervalThres is 8*/


static INT8U gPedoData[55];
static const INT8U STEP_INT[] = {
  32, 28, 23, 21, 18, 16, 11, 8 
};

/* ENERGY_INT is per 20ms sample * 4000*/
static const INT16U ENERGY_INT[] = {
  2, 3, 4, 6, 8, 13, 20, 27 
};


void pedometer_init(void)
{
    /*gPedoData = mallocPanic( 55 );*/
    InitCntStep(gPedoData, AMP_THRES, INTERVAL_THRES);
    SetStep(0);
}
void pedometer(void* data)
{
    INT16U oldsteps = 0;
    INT16U steps;
    INT8U  count = 0;
    INT8U  rest = FALSE;
    INT16U energyVal = 0;
    INT8U  i;

    /* store data*/
    StoreData((INT8S *)data);

    count++; 
    /* when there is no count for a long time, it consider to be rest.*/
    /* can ignore wrap around as rest is set.*/
    if (count > REST_CNT) 
    {      
        rest = TRUE;
    }
    CntStep();
    steps = GetStep();
    if (oldsteps != steps) 
    {      
        /* display steps:.*/
        /*print_str("steps: %d\n", steps);.*/
        
        if (rest) 
            rest = FALSE;
        else 
        {
            /* Add energy according to table*/
            for (i=0; i<TABLE_LEN; i++) 
            {
                if (count > STEP_INT[i]) 
                {
                    energyVal += (count*ENERGY_INT[i])/4;
                    break;
                }
            }
        }
        count = 0;
        /* show energy*/
        /*print_str("energy: %d\n", energyVal/1000);*/
    }

    oldsteps = steps;    
}
int pedometer_get_step(void)
{
    return (int)gSteps;
}

