#ifndef RM_H
#define RM_H


#define MAX_RMS 50

typedef struct rm{
 	int id, port, is_primary,socket;
 	char string_id[5];
 } rm;


#endif