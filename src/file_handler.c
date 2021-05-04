#include "profile.c"
#include "rm.c"

void save_profiles(profile profile_list[MAX_CLIENTS], int id)
{
	FILE *profiles, *followers;
	int i, j;
	i = 0;

	char file_profiles[50];
	char file_followers[50];
	sprintf(file_profiles, "%d", id);
	sprintf(file_followers, "%d", id);
    
    strcat(file_profiles,"_profiles.txt");
    strcat(file_followers,"_followers.txt");



	profiles = fopen(file_profiles, "w");
	followers = fopen(file_followers, "w");

	while(profile_list[i].name!="")
	{
		if(strlen(profile_list[i].name)!=0){
			fprintf(profiles, "%i %s %i ", (int)strlen(profile_list[i].name), profile_list[i].name, profile_list[i].num_followers);
			for (j = 0; j<profile_list[i].num_followers; j++)
			{
				fprintf(followers, "%i %s ", (int)strlen(profile_list[i].followers[j]->name), profile_list[i].followers[j]->name);
			
			}
		
		}
		i++;
	}

	fclose(profiles);
	fclose(followers);
	
}

void read_profiles(profile* profile_list, int id)
{
	FILE *profiles, *followers;
	int num_profiles, i, j, length, follow_id;
	num_profiles = 0;
	char* aux;

	char file_profiles[50];
	char file_followers[50];
	sprintf(file_profiles, "%d", id);
	sprintf(file_followers, "%d", id);
    
    strcat(file_profiles,"_profiles.txt");
    strcat(file_followers,"_followers.txt");

	if((profiles = fopen(file_profiles, "r")) &&  (followers = fopen(file_followers, "r"))){
	
		
		while(!feof(profiles))
		{
			fscanf(profiles, "%i ", &length);
			profile_list[num_profiles].name = malloc((length+1)*sizeof(char));
			fscanf(profiles, "%s %i ", profile_list[num_profiles].name, &(profile_list[num_profiles].num_followers));
			profile_list[num_profiles].name[length]='\0';
			num_profiles++;
		}


		for (i = 0; i<num_profiles; i++)
		{
			for (j = 0; j<profile_list[i].num_followers; j++)
			{
				fscanf(followers, "%i ", &length);
				aux = malloc((length+1)*sizeof(char));
				fscanf(followers, "%s ", aux);
				aux[length]='\0';
				follow_id = get_profile_id(profile_list,aux);
				profile_list[i].followers[j] = &(profile_list[follow_id]);
				free(aux);
			}
		}

		fclose(profiles);
		fclose(followers);
	}
}

void read_config_file(char *config_file_name, int id, rm *this_rm, rm* primary_rm, rm *rm_list, int *rm_list_size){
	int flagThis=0,flagPrimary=0;
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int count = 0;
    int currentPort,currentIsPrimary;
    char currentId[5];
    FILE *config = fopen(config_file_name, "r");
    if (config == NULL){
    	printf("Failed to open config file\n");
        exit(1);
    }
    int countOthers = 0;
    while ((read = getline(&line, &len, config)) != -1) {


       	char * divided;
        divided = strtok (line," \n");
        count = 0;
  		while (divided != NULL)
  		{
    		
    		atoi(divided);
    		if(count==0)
    			strcpy(currentId,divided);
    		
    		if(count==1)
    			currentPort = atoi(divided);
    		
    		if(count==2)
    			currentIsPrimary = atoi(divided);

    		

    		divided = strtok (NULL, " \n");
    		if(count > 2){
    			printf("error reading config file");
    			exit(1);
    		}
    		count++;
  		}

  		if(currentIsPrimary){
  			primary_rm->is_primary = currentIsPrimary;
  			primary_rm->port = currentPort;
  			strcpy(primary_rm->string_id,currentId);
        primary_rm->id = atoi(currentId);
  			flagPrimary = 1;
  		}

  		if(atoi(currentId) == id){
  			this_rm->is_primary = currentIsPrimary;
  			this_rm->port = currentPort;
  			strcpy(this_rm->string_id,currentId);
        this_rm->id = atoi(currentId);
  			flagThis = 1;
  		}

  		if(atoi(currentId) != id && !currentIsPrimary){
  			rm_list[countOthers].is_primary = currentIsPrimary;
  			rm_list[countOthers].port = currentPort;
  			strcpy(rm_list[countOthers].string_id,currentId);
        rm_list[countOthers].id = atoi(currentId);
        rm_list[countOthers].socket =-1;
  			countOthers++;
  		}	
   


    }

    fclose(config);
    if (line)
        free(line);

    if(!flagThis){
    	printf("This RM id is not on the config.\n");
    	exit(1);
    }

    if(!flagPrimary){
    	printf("This config has no primary RM.\n");
    	exit(1);
    }

    *rm_list_size = countOthers;
    

}
