/********************************************
Description: variables and macros definition
Author: CHEN Mengmeng 
Date: 02/20/2006
*********************************************/
#define TWO_FILTER   //if employ second-order filtering

/* implimentation definition */
typedef  unsigned char INT8U;
typedef  unsigned short  INT16U;
typedef  signed char INT8S;
typedef  signed int  INT16S; 
typedef  unsigned long INT32U;


#define FILTER_POINT 9 
/* #define AMP_THRES  800    //AmpThres= Acceleration Threshold value*4000, for example, if Acceleration Threshold is 0.2g, then AmpThres=800	*/
/* #define INTERVAL_THRES 11  //IntervalThres= (1/maxim frequency)/25ms, for example, if max fre is 4Hz, then IntervalThres is 10*/

#ifdef TWO_FILTER
#define BUFFER_SIZE  FILTER_POINT*2+1
#else
#define BUFFER_SIZE  FILTER_POINT+1
#endif  

#define INVALID      2
#define RISING       1
#define FALLING      0

#define ISPEAK       1
#define ISVALLEY     0

typedef struct {
  INT16U amp_thres;
  INT16U steps;
  INT16U steps_elapsed;
  INT16U last_value;
  INT16U last_peakvalue;
  INT16U last_valleyvalue;
  INT16U filter[BUFFER_SIZE];
  INT8U  edge_type;
  INT8U  last_type;
  INT8U  interval_thres;
} pedoData;

pedoData * gPedoPtr;

#define gSteps                   gPedoPtr->steps
#define gEdgeType                gPedoPtr->edge_type
#define gLastType                gPedoPtr->last_type
#define gIntervalThres           gPedoPtr->interval_thres
#define gAmpThres                gPedoPtr->amp_thres
#define buffer                   gPedoPtr->filter
#define gStepsElapsed            gPedoPtr->steps_elapsed
#define gLastValue               gPedoPtr->last_value
#define gLastPeakValue           gPedoPtr->last_peakvalue
#define gLastValleyValue         gPedoPtr->last_valleyvalue

/*
INT16U gSteps;                       number of steps 
INT8U  gEdgeType;                     1 for rising edge, 0 for falling edge 
INT8U  gLastType;
INT8U  __far gIntervalThres;
INT16U __far gAmpThres;
INT16U __far buffer[BUFFER_SIZE];   circular buffer

 Increment when peak/valley located
INT16U __far gStepsElapsed;
INT16U __far gLastValue;
INT16U __far gLastPeakValue;
INT16U __far gLastValleyValue;
*/
