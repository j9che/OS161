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

typedef struct path {
	Direction origin;
	Direction destination;
} path;

// NEW SOLUTION
static struct lock *intersectionLock = NULL;
static struct cv *traffic_cv = NULL;
static struct array* curTraffic = NULL;

int waitNum [4] = {0, 0, 0, 0};

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
	for(unsigned i = 0; i < curTraffic->num; ++i) {
		path *pathPointer = (path*)array_get(curTraffic, i);
		Direction originI = pathPointer->origin;
		Direction destinationI = pathPointer->destination;

		if(from_same_direction(origin, originI) ||
				going_in_opposite(origin, destination,
					originI, destinationI) ||
				(different_destination(destination, destinationI) &&
				 (make_right_turn(origin, destination) ||
				  make_right_turn(originI, destinationI)))) {
			continue;
		}
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
  traffic_cv = cv_create("North");
  if(traffic_cv == NULL) panic("could not create cv");

  curTraffic = array_create();
  if(curTraffic == NULL) panic("could not create array");

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
  KASSERT(traffic_cv != NULL);
  KASSERT(curTraffic != NULL);

  lock_destroy(intersectionLock);
  cv_destroy(traffic_cv);
  array_destroy(curTraffic);
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until iwaitNumt is OK for the vehicle to enter the intersection.
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
	  cv_wait(traffic_cv, intersectionLock);
  }
  path *newPath = kmalloc(sizeof(struct path));

  KASSERT(newPath != NULL);
  newPath->origin = origin;
  newPath->destination = destination;
  array_add(curTraffic, newPath, NULL);

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

  for(unsigned i = 0; i < curTraffic->num; ++i) {
	  path *pathPointer = (path *)array_get(curTraffic, i);
          Direction originI = pathPointer->origin;
          Direction destinationI = pathPointer->destination;

	  if(originI == origin && destinationI == destination) {
		  array_remove(curTraffic, i);
		  break;
	  }
  }
  cv_broadcast(traffic_cv, intersectionLock);

  lock_release(intersectionLock);
}
