#include "profile.c"

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