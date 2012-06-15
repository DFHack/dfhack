/*
 * MacPool.m
 *
 */

#import <Foundation/Foundation.h>
#import "MacPool.h"

NSAutoreleasePool *thePool;

int create_pool() {
	fprintf(stderr,"Creating autorelease pool\n");
	thePool = [[NSAutoreleasePool alloc] init];
	return 1;
}

int destroy_pool() {
	fprintf(stderr,"Draining and releasing autorelease pool\n");
	[thePool drain];
	[thePool release];
	return 0;
}