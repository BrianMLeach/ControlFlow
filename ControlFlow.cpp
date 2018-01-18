#include "stdafx.h"

int fut1(int a);
int fut2(int a);
int fut3(int a);
int fut4(int a);
int fut5(int a);

/* Header file start */

/* This is the largest control flow sequence allowed. Set at 32 it uses
 * 132 bytes of RAM */
#define MAX_CONTROL_FLOW 32

/* Defines the identiers for all the functions under test in the executable */
typedef enum CF_ID
{
  CF_FUT1, CF_FUT2, CF_FUT3, CF_FUT4, CF_FUT5, CF_END
};

/* This structure is optional, it allows conversion from the identifiers to
   readable function names */
typedef struct 
{
	CF_ID Id;
	const char *Name;
} CF_NAMES;

/* Called by the test case, to set up the test */
int PP_Control_Flow_Init(const CF_ID *CFFilter, const CF_ID *CFOrder);

/* Called by the probe point for the function under test */
void PP_Control_Flow(CF_ID cf);

/* Prints a report to the console, can be modified to report in whatever manner
 * is appropriate */
void PP_Control_Flow_Report(const CF_NAMES *);

/* Header file end */

/* Back end code start */
/* Array to store the functions called. The +1 is to allow us to report the
 * first function in error if too many functions are called */
static CF_ID CF_Order_Store[MAX_CONTROL_FLOW+1];

/* Convert Ids to function names */
static const CF_NAMES CF_Names[] =
{
	{ CF_FUT1, "fut1" },
	{ CF_FUT2, "fut2" },
	{ CF_FUT3, "fut3" },
	{ CF_FUT4, "fut4" },
	{ CF_FUT5, "fut5" },
	{ CF_END, "<Internal error>" }
};

static const CF_ID CF_Null[] = { CF_END };

/* A pointer to a list of functions for which are to be tested. */
static const CF_ID *CF_Filter = CF_Null;
/* A pointer to the expected call order */
static const CF_ID *CF_Order = CF_Null;

static int CF_Store_Index = 0;
static int CF_Order_End = 0;

int PP_Control_Flow_Init(const CF_ID *CFFilter, const CF_ID *CFOrder)
{
	int i, j, fRet = 1;
	for(i = 0 ; CFOrder[i] != CF_END ; i++)
	{
		for(j = 0 ; (CFFilter[j] != CF_END) && (CFFilter[j] != CFOrder[i]) ; j++)
		    ;
		if(CFFilter[j] == CF_END)
			fRet = 0; /* There is an control flow function not in the filter */
	}

	if(fRet)
	{
		if(i > MAX_CONTROL_FLOW)
			fRet = 0; /* CFOrder too large */
		else
		{
			CF_Filter = CFFilter;
			CF_Order = CFOrder;
			CF_Store_Index = 0;
			CF_Order_End = i;
			fRet = 1;
		}
	}
	return fRet;
}

/* This function is not thread safe. It will not crash, but may produce
   erroneous results. See comment at CF_Store_Index */
void PP_Control_Flow(CF_ID cf)
{
    int CF_I;
	const CF_ID *pCF;

	/* Is it an active function under test */
	for(pCF = CF_Filter ; (*pCF != CF_END) && (*pCF != cf) ; pCF++)
		;

	if(*pCF == cf)
	{
		CF_I = CF_Store_Index++; /* This increment should be interlocked */
		if(CF_I <= CF_Order_End) /* Stores one extra for report */
			CF_Order_Store[CF_I] = cf;
	}
}

void PP_Control_Flow_Report(const CF_NAMES *CFNames)
{
    int i, e, fFlowOK;
	const CF_NAMES *pNames;
	const char *szUnexpected, *szExpected;

	CF_Filter = CF_Null; /* Stops PP_Control_Flow from doing anything. */

	/* First find out if the control flow accumulated matches */

	e = (CF_Store_Index < CF_Order_End) ? CF_Store_Index : CF_Order_End;

	for(i = 0 ; i < e ; i++)
	{
		if(CF_Order_Store[i] != CF_Order[i])
			break;
	}
	fFlowOK = (i >= e);

	if(fFlowOK && (CF_Store_Index == CF_Order_End))
		printf("Control flow was successful\n");
	else
	{
		if(CF_Store_Index < CF_Order_End)
			printf("Expected %d function calls, found %d calls\n",
				   CF_Order_End, CF_Store_Index);
		else if(CF_Store_Index > CF_Order_End)
		{
			for(pNames = CFNames ; (pNames->Id != CF_Order_Store[CF_Order_End]) &&
			   (pNames->Id != CF_END) ; pNames++)
				;
			szUnexpected = pNames->Name;
			printf("Expected %d function calls, found %d calls, first incorrect was %s\n",
				   CF_Order_End, CF_Store_Index, szUnexpected);
		}

		if(!fFlowOK)
		{
			for(pNames = CFNames ; (pNames->Id != CF_Order_Store[i]) &&
			   (pNames->Id != CF_END) ; pNames++)
				;
			szUnexpected = pNames->Name;
			for(pNames = CFNames ; (pNames->Id != CF_Order[i]) &&
			   (pNames->Id != CF_END) ; pNames++)
				;
			szExpected = pNames->Name;
			printf("Control flow incorrect at index %d, expected %s, found %s\n",
				   i+1 , szExpected, szUnexpected);
		}
	}
}

/* Back end code end */

/* Test case code start */

/* Structure created for test case. Only the functions in this array are
   recorded. It must terminate with CF_END */
/* For this test CF_FUT5 is not under test */
static const CF_ID CF_Filter_Test1[] =
	{ CF_FUT1, CF_FUT2, CF_FUT3, CF_FUT4, CF_END };

/* Structure created for test case, giving call order. It must terminate with
   CF_END at the end */
static const CF_ID CF_Order_Test1[] =
	{ CF_FUT1, CF_FUT2, CF_FUT3, CF_FUT4, CF_FUT4, CF_FUT3, CF_FUT4, CF_END};

/* A Microsoft visual C main routine, used to verify the control flow code.
 * It would normally be a test case with four steps.
 * 1. Initialise with PP_Control_Flow_Init.
 * 2. Call function which starts the control flow.
 * 3. Wait a specified time (needed if control flow calls are asynchronous)
 * 4. Output the result with PP_Control_Flow_Result.
 */

int _tmain(int argc, _TCHAR* argv[])
{
	if(!PP_Control_Flow_Init(CF_Filter_Test1, CF_Order_Test1))
		printf("Control flow too large or function not in filter\n");
	else
	{
		fut1(1);
		PP_Control_Flow_Report(CF_Names);
	}
	return 0;
}

/* Test case code end */


/* An example set of functions used to verify the above */

int fut1(int a)
{
	PP_Control_Flow(CF_FUT1); /* This line would be a probe point */
	a = fut2(a);
	a = fut3(a);
	return a+1;
}

int fut2(int a)
{
	PP_Control_Flow(CF_FUT2); /* This line would be a probe point */
	a = fut3(a);
	a = fut4(a);
	return a+1;
}

int fut3(int a)
{
	PP_Control_Flow(CF_FUT3); /* This line would be a probe point */
	a = fut4(a);
	a = fut5(a);
	return a+1;
}

int fut4(int a)
{
	PP_Control_Flow(CF_FUT4); /* This line would be a probe point */
	return a+1;
}

int fut5(int a)
{
	PP_Control_Flow(CF_FUT5); /* This line would be a probe point */
	return a+2;
}
