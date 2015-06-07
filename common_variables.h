/*
 * common_variables.h
 *
 *  Created on: 05/06/2015
 *      Author: andressaandriao
 */

#ifndef COMMON_VARIABLES_H_
#define COMMON_VARIABLES_H_

typedef struct pcdata {
		char hostip[16];
		char hostname[50];
}hostdata;

extern hostdata hostslist;

#endif /* COMMON_VARIABLES_H_ */
