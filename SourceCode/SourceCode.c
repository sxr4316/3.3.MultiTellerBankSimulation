/******************************************************************************
 * Multi-Threaded Bank Simulation - 3 Bank Tellers Simulated for a working day
 *
 * Description:
 *
 * A standalone program exhibiting multithreading characteristics by simultaneously
 *
 * adding customers to the bank queue, and indpendently attending customers by different
 *
 * tellers. The tellers have no bias, i.e. all tellers have equal probability of attending
 *
 * a customer. The simulation operates and provides data for 1 regular bank working day.
 *
 * Author:
 *  		Siddharth Ramkrishnan (sxr4316@rit.edu)	: 04.10.2016
 *
 *****************************************************************************/

#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <sys/netmgr.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

#define Rand(Min, Max) (Min + (rand()%(Max-Min+1)))

#define	MINSERVTIME		30
#define	MAXSERVTIME		360
#define	MINARRIVTIME	60
#define	MAXARRIVTIME	240
#define MINBREAKLEN		60
#define MAXBREAKLEN		240
#define MINBREAKSEPN	1800
#define MAXBREAKSEPN	3600


unsigned char CustomerArriving = 1;
unsigned char BankClosedStatus = 0;
unsigned char TellerBreakEn = 1;
unsigned int MaxIdleTime = 0;
unsigned int TotalIdleTime = 0;
unsigned int TotalNumCustomers = 0;
unsigned int TotalEntranceTime = 0;
unsigned int MaxEntranceTime = 0;
unsigned int MinEntranceTime = 0;
unsigned int TotalQueueTime = 0;
unsigned int TotalServiceTime = 0;
unsigned int MaxServiceTime = 0;
unsigned int MaxQueueDepth = 0;
unsigned int MaxQueueTime = 0;
unsigned int MinQueueTime = 0;
unsigned int CurrentArrivalTime = 0;
unsigned int CustomerQID = 0;
unsigned int CustomerSID = 0;
unsigned long int CurrentRealTime = 0;

static timer_t timer;

pthread_mutex_t QueueMutex;

pthread_t CustomerUpdnThread, TellerThreadT1, TellerThreadT2, TellerThreadT3;

typedef struct {
	unsigned char ID;
	unsigned int ArrivalTime;
	unsigned int StartServiceTime;
	unsigned int WaitingTime;
	unsigned int ServiceTime;
	unsigned int DepartureTime;
} Customer;

Customer CustomerQ[500];

typedef struct {
	unsigned char ID;
	unsigned int IdleTime;
	unsigned int MaxServiceTime;
	unsigned int MinServiceTime;
	unsigned int NumCustomerServed;
	unsigned int LastBreakTime;
	unsigned int NextBreakTime;
	unsigned int TotalIdleTime;
	unsigned int TotalServiceTime;
	unsigned int TotalBreakTime;
	unsigned int CurrentBreakLen;
} Teller;

Teller T1, T2, T3;

void InitializeTeller() {
	//Initialize Teller One
	T1.ID = 0;
	T1.IdleTime = 0;
	T1.MaxServiceTime = 0;
	T1.MinServiceTime = 0;
	T1.NumCustomerServed = 0;
	T1.LastBreakTime = 0;
	T1.NextBreakTime = 0;
	T1.CurrentBreakLen = 0;
	T1.TotalBreakTime = 0;

	//Initialize Teller Two
	T2.ID = 0;
	T2.IdleTime = 0;
	T2.MaxServiceTime = 0;
	T2.MinServiceTime = 0;
	T2.NumCustomerServed = 0;
	T2.NextBreakTime = 0;
	T2.CurrentBreakLen = 0;
	T2.TotalBreakTime = 0;

	//Initialize Teller Three
	T3.ID = 0;
	T3.IdleTime = 0;
	T3.MaxServiceTime = 0;
	T3.MinServiceTime = 0;
	T3.NumCustomerServed = 0;
	T3.NextBreakTime = 0;
	T3.CurrentBreakLen = 0;
	T3.TotalBreakTime = 0;
}

void CreateCustomer() {
	unsigned char ID = (++CustomerQID);
	CustomerQ[ID].ArrivalTime = CurrentRealTime;
	CustomerQ[ID].ServiceTime = Rand(MINSERVTIME,MAXSERVTIME);
	CustomerQ[ID].ID = ID;
	CustomerQ[ID].WaitingTime = 0;
	CustomerQ[ID].DepartureTime = 0;
}

void* UpdateCustomerQueue(void* arg)
{
	static unsigned int NextArrivalTimeDiff;

	static unsigned long int NewArrivalTime;

	while(BankClosedStatus==0)
	{
		if((CustomerArriving==0)&&(BankClosedStatus==0))
		{
			NextArrivalTimeDiff = Rand(MINARRIVTIME, MAXARRIVTIME);

			NewArrivalTime = CurrentRealTime + NextArrivalTimeDiff;

			TotalEntranceTime = (TotalEntranceTime + NextArrivalTimeDiff);

			CustomerArriving = 1;

		} else if ((CustomerArriving==1)&&(CurrentRealTime>=NewArrivalTime))
		{
			if (BankClosedStatus == 0)
			{
				pthread_mutex_lock(&QueueMutex);

				(void) CreateCustomer();

				if ((CustomerQID - CustomerSID) > MaxQueueDepth)

					MaxQueueDepth = (CustomerQID - CustomerSID);

				pthread_mutex_unlock(&QueueMutex);

				CustomerArriving = 0;
			}
		}
	}

	pthread_cancel(CustomerUpdnThread);

	return arg;
}

void* EvaluateBankTellerT1(void* arg)
{
	static unsigned char BusyStatus, BreakStatus, IdleStatus;

	static unsigned int IdleStartTime, BreakStartTime, ServiceStartTime;

	while((BankClosedStatus==0)||((CustomerQID-CustomerSID)>0))
	{
		if((TellerBreakEn==1)&&(BreakStatus==0)&&(CurrentRealTime > T1.NextBreakTime)&&(BusyStatus == 0))
		{
			if (IdleStatus == 1)
			{
				T1.IdleTime=(CurrentRealTime - IdleStartTime);

				T1.TotalIdleTime = T1.TotalIdleTime + T1.IdleTime;

				if (MaxIdleTime < T1.IdleTime)

					MaxIdleTime = T1.IdleTime;

			}

			IdleStatus = 0;

			BreakStartTime = CurrentRealTime;

			T1.CurrentBreakLen = Rand(MINBREAKLEN,MAXBREAKLEN);

			T1.TotalBreakTime = T1.TotalBreakTime + T1.CurrentBreakLen;

			BreakStatus = 1;
		}



		if ((BusyStatus == 0) && ((TellerBreakEn == 0) || (BreakStatus == 0)))
		{

			pthread_mutex_lock(&QueueMutex);

			if((CustomerQID - CustomerSID) > 0)
			{
				if (IdleStatus == 1)
				{
					T1.IdleTime=(CurrentRealTime - IdleStartTime);

					T1.TotalIdleTime = T1.TotalIdleTime + T1.IdleTime;

					if (MaxIdleTime < T1.IdleTime)

						MaxIdleTime = T1.IdleTime;

				}

				IdleStatus = 0;

				ServiceStartTime = CurrentRealTime;

				T1.ID = (CustomerSID++);

				CustomerQ[T1.ID].WaitingTime = ServiceStartTime - CustomerQ[T1.ID].ArrivalTime;

				T1.TotalServiceTime = T1.TotalServiceTime + CustomerQ[T1.ID].ServiceTime;

				if(T1.MaxServiceTime < CustomerQ[T1.ID].ServiceTime)
					T1.MaxServiceTime = CustomerQ[T1.ID].ServiceTime;

				T1.NumCustomerServed++;

				BusyStatus = 1;
			}

			pthread_mutex_unlock(&QueueMutex);
		}

		CustomerQ[T1.ID].StartServiceTime = ServiceStartTime;

		if((BusyStatus==0)&&((TellerBreakEn == 0)||(BreakStatus == 0)))
		{
			if (IdleStatus == 0)

				IdleStartTime = CurrentRealTime;

			IdleStatus = 1;
		} else if (BusyStatus == 1)
		{
			if((CurrentRealTime - ServiceStartTime) >= CustomerQ[T1.ID].ServiceTime)
			{
				CustomerQ[T1.ID].DepartureTime = CurrentRealTime;

				BusyStatus = 0;
			}
		} else if((TellerBreakEn == 1) && (BreakStatus == 1))
		{
			if((CurrentRealTime - BreakStartTime) >= T1.CurrentBreakLen)
			{
				BreakStatus = 0;

				T1.NextBreakTime	=	CurrentRealTime + Rand(MINBREAKSEPN,MAXBREAKSEPN);
			}
		} else
		{

		}
	}

	pthread_cancel(TellerThreadT1);

	return arg;
}

void* EvaluateBankTellerT2(void* arg)
{
	static unsigned char BusyStatus, BreakStatus, IdleStatus;

	static unsigned int IdleStartTime, BreakStartTime, ServiceStartTime;

	while((BankClosedStatus==0)||((CustomerQID-CustomerSID)>0))
	{
		if((TellerBreakEn==1)&&(BreakStatus==0)&&(CurrentRealTime > T2.NextBreakTime)&&(BusyStatus == 0))
		{
			if (IdleStatus == 1)
			{
				T2.IdleTime=(CurrentRealTime - IdleStartTime);

				T2.TotalIdleTime = T2.TotalIdleTime + T2.IdleTime;

				if (MaxIdleTime < T2.IdleTime)

					MaxIdleTime = T2.IdleTime;

			}

			IdleStatus = 0;

			BreakStartTime = CurrentRealTime;

			T2.CurrentBreakLen = Rand(MINBREAKLEN,MAXBREAKLEN);

			T2.TotalBreakTime = T2.TotalBreakTime + T2.CurrentBreakLen;

			BreakStatus = 1;
		}



		if ((BusyStatus == 0) && ((TellerBreakEn == 0) || (BreakStatus == 0)))
		{

			pthread_mutex_lock(&QueueMutex);

			if((CustomerQID - CustomerSID) > 0)
			{
				if (IdleStatus == 1)
				{
					T2.IdleTime=(CurrentRealTime - IdleStartTime);

					T2.TotalIdleTime = T2.TotalIdleTime + T2.IdleTime;

					if (MaxIdleTime < T2.IdleTime)

						MaxIdleTime = T2.IdleTime;

				}

				IdleStatus = 0;

				ServiceStartTime = CurrentRealTime;

				T2.ID = (CustomerSID++);

				CustomerQ[T2.ID].WaitingTime = ServiceStartTime - CustomerQ[T2.ID].ArrivalTime;

				T2.TotalServiceTime = T2.TotalServiceTime + CustomerQ[T2.ID].ServiceTime;

				if(T2.MaxServiceTime < CustomerQ[T2.ID].ServiceTime)
					T2.MaxServiceTime = CustomerQ[T2.ID].ServiceTime;

				T2.NumCustomerServed++;

				BusyStatus = 1;
			}

			pthread_mutex_unlock(&QueueMutex);
		}

		CustomerQ[T2.ID].StartServiceTime = ServiceStartTime;

		if((BusyStatus==0)&&((TellerBreakEn == 0)||(BreakStatus == 0)))
		{
			if (IdleStatus == 0)

				IdleStartTime = CurrentRealTime;

			IdleStatus = 1;
		} else if (BusyStatus == 1)
		{
			if((CurrentRealTime - ServiceStartTime) >= CustomerQ[T2.ID].ServiceTime)
			{
				CustomerQ[T2.ID].DepartureTime = CurrentRealTime;

				BusyStatus = 0;
			}
		} else if((TellerBreakEn == 1) && (BreakStatus == 1))
		{
			if((CurrentRealTime - BreakStartTime) >= T2.CurrentBreakLen)
			{
				T2.NextBreakTime	=	CurrentRealTime + Rand(MINBREAKSEPN,MAXBREAKSEPN);

				BreakStatus = 0;
			}
		} else
		{

		}
	}

	pthread_cancel(TellerThreadT2);

	return arg;
}

void* EvaluateBankTellerT3(void* arg)
{
	static unsigned char BusyStatus, BreakStatus, IdleStatus;

	static unsigned int IdleStartTime, BreakStartTime, ServiceStartTime;

	while((BankClosedStatus==0)||((CustomerQID-CustomerSID)>0))
	{
		if((TellerBreakEn==1)&&(BreakStatus==0)&&(CurrentRealTime > T3.NextBreakTime)&&(BusyStatus == 0))
		{
			if (IdleStatus == 1)
			{
				T3.IdleTime=(CurrentRealTime - IdleStartTime);

				T3.TotalIdleTime = T3.TotalIdleTime + T3.IdleTime;

				if (MaxIdleTime < T3.IdleTime)

					MaxIdleTime = T3.IdleTime;

			}

			IdleStatus = 0;

			BreakStartTime = CurrentRealTime;

			T3.CurrentBreakLen = Rand(MINBREAKLEN,MAXBREAKLEN);

			T3.TotalBreakTime = T3.TotalBreakTime + T3.CurrentBreakLen;

			BreakStatus = 1;
		}



		if ((BusyStatus == 0) && ((TellerBreakEn == 0) || (BreakStatus == 0)))
		{

			pthread_mutex_lock(&QueueMutex);

			if((CustomerQID - CustomerSID) > 0)
			{
				if (IdleStatus == 1)
				{
					T3.IdleTime=(CurrentRealTime - IdleStartTime);

					T3.TotalIdleTime = T3.TotalIdleTime + T3.IdleTime;

					if (MaxIdleTime < T3.IdleTime)

						MaxIdleTime = T3.IdleTime;

				}

				IdleStatus = 0;

				ServiceStartTime = CurrentRealTime;

				T3.ID = (CustomerSID++);

				CustomerQ[T3.ID].WaitingTime = ServiceStartTime - CustomerQ[T3.ID].ArrivalTime;

				T3.TotalServiceTime = T3.TotalServiceTime + CustomerQ[T3.ID].ServiceTime;

				if(T3.MaxServiceTime < CustomerQ[T3.ID].ServiceTime)
					T3.MaxServiceTime = CustomerQ[T3.ID].ServiceTime;

				T3.NumCustomerServed++;

				BusyStatus = 1;
			}

			pthread_mutex_unlock(&QueueMutex);
		}

		CustomerQ[T3.ID].StartServiceTime = ServiceStartTime;

		if((BusyStatus==0)&&((TellerBreakEn == 0)||(BreakStatus == 0)))
		{
			if (IdleStatus == 0)

				IdleStartTime = CurrentRealTime;

			IdleStatus = 1;
		} else if (BusyStatus == 1)
		{
			if((CurrentRealTime - ServiceStartTime) >= CustomerQ[T3.ID].ServiceTime)
			{
				CustomerQ[T3.ID].DepartureTime = CurrentRealTime;

				BusyStatus = 0;
			}
		} else if((TellerBreakEn == 1) && (BreakStatus == 1))
		{
			if((CurrentRealTime - BreakStartTime) >= T3.CurrentBreakLen)
			{
				T3.NextBreakTime	=	CurrentRealTime + Rand(MINBREAKSEPN,MAXBREAKSEPN);

				BreakStatus = 0;
			}
		} else
		{

		}
	}

	pthread_cancel(TellerThreadT3);

	return arg;
}

void clockTick(int a)
{
	CurrentRealTime = CurrentRealTime + 20;

	if (CurrentRealTime <= (7 * 60 * 60))
		BankClosedStatus = 0;
	else
		BankClosedStatus = 1;
}

void TimerInitialization()
{
	// structure needed for timer_create
	static struct sigevent event;
	// structure needed to attach a signal to a function
	static struct sigaction action;
	// structure that sets the start and interval time for the timer
	static struct itimerspec time_info;

	// itimerspec:
	//      it_value is used for the first tick
	//      it_interval is used for the interval between ticks
	// both contain:
	//      tv_sec used for seconds
	//      tv_nsec used for nanoseconds
	time_info.it_value.tv_nsec = 5000000;
	time_info.it_interval.tv_nsec = 5000000;

	// this is used to bind the action to the function
	// you want called ever timer tick
	action.sa_handler = &clockTick;

	// this is used to queue signals
	action.sa_flags = SA_SIGINFO;

	// this function takes a signal and binds it to your action
	// the last param is null because that is for another sigaction
	// struct that you want to modify
	sigaction(SIGUSR1, &action, NULL);

	// SIGUSR1 is an unused signal provided for you to use

	// this macro takes the signal you bound to your action,
	// and binds it to your event
	SIGEV_SIGNAL_INIT(&event, SIGUSR1);

	// now your function is bound to an action
	// which is bound to a signal
	// which is bound to your event

	// now you can create the timer using your event
	timer_create(CLOCK_REALTIME, &event, &timer);

	// and then start the timer, using the time_info you set up
	timer_settime(timer, 0, &time_info, 0);

	CurrentRealTime = 0;
}

void reportCustomerData()
{
	unsigned int i, MaxCustomerWaitTime, TotalCustomerWaitTime;

	MaxCustomerWaitTime = 0;
	TotalCustomerWaitTime = 0;

	if (TellerBreakEn == 0)
		printf("\r\n Teller Breaks are Disabled");
	else
		printf("\r\n Teller Breaks are Enabled");

	for (i = 1; i <= CustomerQID; i++)
	{
		TotalCustomerWaitTime = TotalCustomerWaitTime + CustomerQ[i].WaitingTime;

		TotalServiceTime = TotalServiceTime + CustomerQ[i].ServiceTime;

		if (MaxCustomerWaitTime < CustomerQ[i].WaitingTime)

			MaxCustomerWaitTime = CustomerQ[i].WaitingTime;
	}

	printf("\r\n Total Number of Customers Serviced : %d ", CustomerQID);

	printf("\r\n Average Customer Wait Time         : %d ", TotalCustomerWaitTime / CustomerQID);

	printf("\r\n Average Customer Service Time      : %d ", TotalServiceTime / CustomerQID);

	printf("\r\n Average Teller Idle Time  	        : %d ", (T1.IdleTime + T2.IdleTime	+ T3.IdleTime) / CustomerQID);

	printf("\r\n Maximum Customer Wait Time         : %d ", MaxCustomerWaitTime);

	printf("\r\n Maximum Teller Idle Time           : %d ", MaxIdleTime);

	printf("\r\n Maximum Depth of Customer Queue    : %d ", MaxQueueDepth);

}

void reportTellerData()
{

	printf("\n \n \n \r Teller A Operation Information ");
	printf("\n \r Number of Customers Served  : %d ", T1.NumCustomerServed);
	printf("\n \r Maximum Service Time        : %d ", T1.MaxServiceTime);
	printf("\n \r Total Service Time          : %d ", T1.TotalServiceTime);
	printf("\n \r Total Idle Time             : %d ", T1.TotalIdleTime);
	printf("\n \r Total Break Time            : %d ", T1.TotalBreakTime);


	printf("\n \n \n \r Teller B Operation Information ");
	printf("\n \r Number of Customers Served  : %d ", T2.NumCustomerServed);
	printf("\n \r Maximum Service Time        : %d ", T2.MaxServiceTime);
	printf("\n \r Total Service Time          : %d ", T2.TotalServiceTime);
	printf("\n \r Total Idle Time             : %d ", T2.TotalIdleTime);
	printf("\n \r Total Break Time            : %d ", T2.TotalBreakTime);


	printf("\n \n \n \r Teller B Operation Information ");
	printf("\n \r Number of Customers Served  : %d ", T3.NumCustomerServed);
	printf("\n \r Maximum Service Time        : %d ", T3.MaxServiceTime);
	printf("\n \r Total Service Time          : %d ", T3.TotalServiceTime);
	printf("\n \r Total Idle Time             : %d ", T3.TotalIdleTime);
	printf("\n \r Total Break Time            : %d ", T3.TotalBreakTime);

}

int main()
{
	int thread, policy;

	pthread_attr_t threadAttributes;

	struct sched_param parameters;

	(void) pthread_attr_init(&threadAttributes);

	(void) pthread_getschedparam(pthread_self(), &policy, &parameters);

	parameters.sched_priority--;

	pthread_attr_setschedparam(&threadAttributes, &parameters);

	thread = pthread_create(&CustomerUpdnThread, &threadAttributes,
			UpdateCustomerQueue, NULL);

	thread = pthread_create(&TellerThreadT1, &threadAttributes,
			EvaluateBankTellerT1, NULL);

	thread = pthread_create(&TellerThreadT2, &threadAttributes,
			EvaluateBankTellerT2, NULL);

	thread = pthread_create(&TellerThreadT3, &threadAttributes,
			EvaluateBankTellerT3, NULL);

	InitializeTeller();

	TimerInitialization();

	printf("\n \r  Bank is now Open for Business . Current Time : 7 : 00 AM  \n \r");

	while ((BankClosedStatus == 0) || ((CustomerQID - CustomerSID) > 0))
	{
			if(BankClosedStatus == 1)
				pthread_cancel(CustomerUpdnThread);

			printf("\r %lu : %lu  %d %d %d ",7+( CurrentRealTime / 3600 ), ((CurrentRealTime/60)%60),BankClosedStatus,CustomerQID,CustomerSID);
	}

	pthread_cancel(TellerThreadT1);

	pthread_cancel(TellerThreadT2);

	pthread_cancel(TellerThreadT3);

	printf("\r\n Bank is Closed. Viewing Operational Information \n\r");

	reportCustomerData();

	reportTellerData();

	return 0;

}
