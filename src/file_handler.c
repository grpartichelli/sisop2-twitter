#include "profile.c"

int save_profiles(profile profile_list[MAX_CLIENTS])
{
	FILE *profiles, *followers;
	int i, j;
	i = 0;

	profiles = fopen("profiles.txt", "w");
	followers = fopen("followers.txt", "w");

	while(profile_list[i].name!="")
	{
		if(strlen(profile_list[i].name)!=0){
			printf("%i %s %i ", (int)strlen(profile_list[i].name), profile_list[i].name, profile_list[i].num_followers);
			fprintf(profiles, "%i %s %i ", (int)strlen(profile_list[i].name), profile_list[i].name, profile_list[i].num_followers);
			for (j = 0; j<profile_list[i].num_followers; j++)
			{
				fprintf(followers, "%i %s ", (int)strlen(profile_list[i].followers[j]->name), profile_list[i].followers[j]->name);
				printf("%i %s ", (int)strlen(profile_list[i].followers[j]->name), profile_list[i].followers[j]->name);
			}
			printf("\n");
		}
		i++;
	}

	fclose(profiles);
	fclose(followers);
	
}

int read_profiles(profile* profile_list)
{
	FILE *profiles, *followers;
	int num_profiles, i, j, length, follow_id;
	num_profiles = 0;
	char* aux;

	profiles = fopen("profiles.txt", "r");
	followers = fopen("followers.txt", "r");
	
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