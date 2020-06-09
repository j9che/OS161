#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
#include <array.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */

//  Declaration
int index_of_longest_line(void);
bool make_right_turn(Direction origin, Direction destination);
bool different_destination(Direction v1, Direction v2);
bool from_same_direction(Direction v1, Direction v2);
bool going_in_opposite(Direction v1_origin, Direction v1_dest,
                Direction v2_origin, Direction v2_dest);
bool conflict(Direction origin, Direction destination);

// NEW SOLUTION
static struct lock *intersectionLock = NULL;
static struct cv *North = NULL;
static struct cv *South = NULL;
static struct cv *West = NULL;
static struct cv *East = NULL;
static struct array *originArray = NULL;
static struct array *destinationArray = NULL;

int waitNum [4] = {0, 0, 0, 0};
//static int arrayCapacity = 4;
//static int usedSpace = 0;
//static struct path *curTraffic = malloc(arrayCapacity * sizeof(struct path));
//struct array *originArray = array_create();
//struct array *destinationArray = array_create();

int index_of_longest_line() {
	int max = -1;
	int index = -1;

	for(int i = 0; i < 4; ++i) {
		if(max < waitNum[i]) {
			max = waitNum[i];
			index = i;
		}
	}

	return index;
}

/*void push(Direction origin, Direction destination) {
	struct path newPath = {origin, destination};

	if(usedSpace == arrayCapacity) {
		arrayCapacity *= 2;
		curTraffic = realloc(curTraffic, arrayCapacity * sizeof(struct path));
	}

	curTraffic[usedSpace++] = newPath;
}

void remove(Direction origin, Direction destination) {
	for(int i = 0; i < usedSpace; ++i) {
		if(curTraffic[i]->origin == origin && 
				curTraffic[i]->destination == destination) {
			curTraffic[i] = curTraffic[usedSpace-1];
			--usedSpace;
			return;
		}
	}
}*/

bool make_right_turn(Direction origin, Direction destination) {
	if(origin == south && destination == east) {
		return true;
	} else if(origin == east && destination == north) {
                return true;
        } else if(origin == north && destination == west) {
                return true;
        } else if(origin == west && destination == south) {
                return true;
        }

	return false;
}

bool different_destination(Direction v1, Direction v2) {
	if(v1 != v2) {
		return true;
	}

	return false;
}

bool from_same_direction(Direction v1, Direction v2) {
	if(v1 == v2) {
		return true;
	}

	return false;
}

bool going_in_opposite(Direction v1_origin, Direction v1_dest, 
		Direction v2_origin, Direction v2_dest) {
	if(v1_origin == v2_dest && v2_origin == v1_dest) {
		return true;
	}

	return false;
}

bool conflict(Direction origin, Direction destination) {
	//kprintf("array size %d \n", originArray->num);
	for(unsigned i = 0; i < originArray->num; ++i) {
		Direction *originIPointer = array_get(originArray, i);
		Direction *destinationIPointer = array_get(destinationArray, i);
	//	Direction originI = *(Direction *)originIPointer;
	//	Direction destinationI = *(Direction *)destinationIPointer;
	        kprintf("origin address %p \n", originIPointer);
		kprintf("true originI %d \n", *(Direction *)originIPointer);

		if(from_same_direction(origin, *originIPointer) ||
				going_in_opposite(origin, destination,
					*originIPointer, *destinationIPointer) ||
				(different_destination(destination, *destinationIPointer) &&
				 (make_right_turn(origin, destination) ||
				  make_right_turn(*originIPointer, *destinationIPointer)))) {
			continue;
		}
		//kprintf("origin %d \n", origin);
		//kprintf("destination %d \n", destination);
		//kprintf("origin address %p \n", originIPointer);
		//kprintf("destinationI %d \n", *destinationIPointer);
		//kprintf("index I is %d \n", i);
		//kprintf("true originI %d \n", *(Direction *)originIPointer);
		//kprintf("true originI %d \n", *originIPointer);
		//kprintf("destinationI %d \n", *(Direction *)destinationIPointer);
		//kprintf("originI %d \n", originI);
		//kprintf("destinationI %d \n", destinationI);
		return true;
	}
	return false;
}


/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

  // NEW SOLUTION

  intersectionLock = lock_create("intersectionLock");
  if (intersectionLock == NULL) {
	   panic("could not create intersection lock");
  }
  North = cv_create("North");
  if(North == NULL) panic("could not create cv");

  South = cv_create("South");
  if(South == NULL) panic("could not create cv");

  West = cv_create("West");
  if(West == NULL) panic("could not create cv");

  East = cv_create("East");
  if(East == NULL) panic("could not create cv");

  originArray = array_create();
  if(originArray == NULL) panic("could not create array");

  destinationArray = array_create();
  if(destinationArray == NULL) panic("could not create array");

  return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */

  //NEW SOLUTION;
  KASSERT(intersectionLock != NULL);
  KASSERT(North != NULL);
  KASSERT(South != NULL);
  KASSERT(West != NULL);
  KASSERT(East != NULL);
  //KASSERT(curTraffic !== NULL);
  KASSERT(originArray != NULL);
  KASSERT(destinationArray != NULL);

  lock_destroy(intersectionLock);
  cv_destroy(North);
  cv_destroy(South);
  cv_destroy(West);
  cv_destroy(East);
  //kfree(curTraffic);
  array_destroy(originArray);
  array_destroy(destinationArray);
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */

  //NEW SOLUTION
  lock_acquire(intersectionLock);
  while(conflict(origin, destination)) {
	  if(origin == north) {
		  waitNum[0]++;
		  cv_wait(North, intersectionLock);
	  } else if(origin == south) {
		  waitNum[2]++;
		  cv_wait(South, intersectionLock);
	  } else if(origin == west){
		  waitNum[3]++;
		  cv_wait(West, intersectionLock);
	  } else if(origin == east) {
		  waitNum[1]++;
		  cv_wait(East, intersectionLock);
	  }
  }
 // push(origin, destination);
 kprintf("address of origin %p \n", &origin);
 kprintf("origin is %d \n", origin);
 //kprintf("address of origin %p \n", &origin);
  array_add(originArray, &origin, NULL);
  array_add(destinationArray, &destination, NULL);
  lock_release(intersectionLock);
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */

  //NEW SOLUTION
  lock_acquire(intersectionLock);

  //remove(origin, destination);
  int index = 0;

  for(unsigned i = 0; i < originArray->num; ++i) {
	  Direction *originIPointer = array_get(originArray, i);
          Direction *destinationIPointer = array_get(destinationArray, i);
	  if(*originIPointer == origin && *destinationIPointer == destination) {
		  index = i;
	  }
  }
  //kprintf("removed at %d \n", index);
  array_remove(originArray, index);
  array_remove(destinationArray, index);

  int line = index_of_longest_line();
  //kprintf("longest line is %d \n", line);
  if(line == 0) {
	  cv_broadcast(North, intersectionLock);
  } else if(line == 1) {
	   cv_broadcast(East, intersectionLock);
  } else if(line == 2) {
	   cv_broadcast(South, intersectionLock);
  } else if(line == 3) {
	   cv_broadcast(West, intersectionLock);
  }

  lock_release(intersectionLock);
}
